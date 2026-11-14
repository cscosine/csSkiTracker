#!/usr/bin/env python3
import sys
from collections.abc import Sequence
from pathlib import Path

from csorchestrator.application.cli.cli import orchestrator_main_with_default_run
from csorchestrator.application.factory.factory import (
    OptionalOrchestratorWithReport,
    create_orchestrator_factory_all_supported_cases,
)
from csorchestrator.application.recipes.checkout_build import (
    build_repos,
    checkout_repos,
    create_and_upload_artifacts,
    create_and_upload_deployments,
)
from csorchestrator.application.recipes.create_orchestrator import (
    create_default_orchestrator,
)
from csorchestrator.application.recipes.cscmake_release import create_cscmake_release_archive
from csorchestrator.application.recipes.manifest_github import install_ubuntu_apt_packages
from csorchestrator.domain.context.context_os_architecture import OS
from csorchestrator.foundation.core.report import Report
from csorchestrator.frontend.cscmake_presets.supported_variants import (
    BuildConfig,
)
from csorchestrator.frontend.local_execution.step_utils import StepExecuteOnlyOn, StepSkipExecutionOnLocal
from csorchestrator.frontend.step.step_custom_command import StepInstallAptPackages, StepWinPSCommand
from csorchestrator.frontend.step.step_deployments import StepRunDeployment
from csorchestrator.frontend.step.step_get_precompiled_lib_github import (
    identity_mapping_function,
)

from csSkiTracker_config import install_csskitracker_build_dependencies


def create_orchestrator() -> OptionalOrchestratorWithReport:
    report = Report()

    base_target_dir = Path("workspace")
    base_install_dir = base_target_dir / Path("install")
    base_libs_dir = base_target_dir / Path("libs")
    base_build_dir = base_target_dir / Path("build")
    deploy_archives_dir = base_target_dir / Path("deploy-archives")
    build_config = BuildConfig.DEBUG_RELEASE_RELWITHDEBINFO_PARANOID
    cscmake_version = "1.0.0-rc4"

    o = create_orchestrator_factory_all_supported_cases(
        name="csSkiTracker",
        version="0.1.0",
        execution_matrix_name="orchestrator-matrix",
    )

    o = create_default_orchestrator(
        name="csSkiTracker",
        version="0.1.0",
        base_install_dir=base_install_dir,
    )

    # ----------------------------------------------------------------
    checkout_repos(
        orchestrator=o,
        base_target_dir=base_target_dir,
        repo_access_token="${{ secrets.ACTIONS_ORG_ACCESS }}",
        release_archive_list=[create_cscmake_release_archive(cscmake_version)],
    )

    report.append_report(
        install_csskitracker_build_dependencies(
            orchestrator=o,
            base_libs_dir=base_libs_dir,
            dependencies_config_path=Path("dependencies"),
            dev=True,
            download_dependencies_manifest_and_bundle=True,
            mapping_function=identity_mapping_function,
            phase_name_suffix="csSkiTracker dependencies",
        )
    )

    # the libraries of the system the Qt xcb plugin needs: the deployment (cs_deployment) resolves the plugins, and
    # fails when one is missing on the build machine
    install_ubuntu_apt_packages(
        orchestrator=o,
        packages=[
            "libxcb-cursor0",
            "libxcb-icccm4",
            "libxcb-image0",
            "libxcb-keysyms1",
            "libxcb-render-util0",
            "libxcb-shape0",
            "libxcb-xkb1",
            "libxkbcommon-x11-0",
        ],
        phase_name_suffix="Qt xcb plugin",
    )

    # ----------------------------------------------------------------
    build_repos(
        orchestrator=o,
        base_target_dir=base_target_dir,
        build_self=build_config,
    )

    create_and_upload_artifacts(
        orchestrator=o,
        base_install_dir=base_install_dir,
        repos_auto_search_list=["csSkiTracker"],
    )

    # ----------------------------------------------------------------
    # deployments (cs_deployment in csSkiTracker/skiTrackerApp/CMakeLists.txt, made by the install): archived,
    # uploaded, attached to the release; then run, extracted from their archive, with a clean environment
    create_and_upload_deployments(
        orchestrator=o,
        base_build_dir=base_build_dir,
        build_config=build_config,
        output_dir=deploy_archives_dir,
    )

    p = o.create_phase("Run Deployments")
    # Linux CI: a virtual X server for the smoke run (OpenGL through Mesa); not installed on a developer machine
    p.add_step(
        StepInstallAptPackages(
            name="install xvfb",
            description="virtual X server for the smoke run of the deployment",
            packages=["xvfb", "xauth"],
        )
        .add_extra(StepExecuteOnlyOn(os=OS.LINUX))
        .add_extra(StepSkipExecutionOnLocal())
    )
    # Windows CI: no GPU on the runners, and the OpenGL of Windows alone is 1.1 (a real machine: the one of its GPU
    # driver). The software OpenGL of Mesa (llvmpipe) as the one of the system, where Qt finds it (opengl32sw.dll,
    # QT_OPENGL=software): the Windows equivalent of Mesa on the Linux runners. Not shipped; not on a developer machine.
    p.add_step(
        StepWinPSCommand(
            name="install the software OpenGL of Mesa",
            description="Mesa llvmpipe (mesa-dist-win) as opengl32sw.dll in System32, for the smoke run",
            cmd=[
                '$ErrorActionPreference = "Stop"',
                '$version = "25.2.6"',
                '$url = "https://github.com/pal1000/mesa-dist-win/releases/download/$version/mesa3d-$version-release-msvc.7z"',
                "Invoke-WebRequest -Uri $url -OutFile mesa.7z",
                "7z e mesa.7z x64/opengl32.dll x64/libgallium_wgl.dll -omesa -y",
                'Copy-Item mesa/opengl32.dll "$env:SystemRoot/System32/opengl32sw.dll"',
                'Copy-Item mesa/libgallium_wgl.dll "$env:SystemRoot/System32/libgallium_wgl.dll"',
                'Get-Item "$env:SystemRoot/System32/opengl32sw.dll", "$env:SystemRoot/System32/libgallium_wgl.dll"',
            ],
        )
        .add_extra(StepExecuteOnlyOn(os=OS.WINDOWS))
        .add_extra(StepSkipExecutionOnLocal())
    )
    p.add_step(
        StepRunDeployment(
            name="Smoke run of the deployment app",
            description=(
                "start skiTrackerApp from its deployment on its sample dataset: its OpenGL view gets a context, "
                "exit code 0"
            ),
            deployment="app",
            # Linux: xvfb-run when there (CI), else the display of the session (a developer machine); Windows: the
            # desktop of the session, the software OpenGL of Qt (QT_OPENGL, no GPU on the CI runners)
            command={
                OS.LINUX: [
                    "sh",
                    "-c",
                    'if command -v xvfb-run >/dev/null; then exec xvfb-run -a "$0" --smoke "$1"; '
                    'else exec "$0" --smoke "$1"; fi',
                    "{folder}/bin/skiTrackerApp",
                    "{folder}/data/frame_data_bolshunov.std",
                ],
                OS.WINDOWS: ["{folder}/bin/skiTrackerApp.exe", "--smoke", "{folder}/data/frame_data_bolshunov.std"],
            },
            # QT_FORCE_STDERR_LOGGING: the messages of Qt in the log (a GUI application on Windows: not on the console);
            # GALLIUM_DRIVER: the software renderer of Mesa
            environment={"QT_OPENGL": "software", "QT_FORCE_STDERR_LOGGING": "1", "GALLIUM_DRIVER": "llvmpipe"},
            base_build_dir=base_build_dir,
            build_config=build_config,
            archives_dir=deploy_archives_dir,
            install_system_requirements=True,
        )
    )

    # single return point
    if report.has_errors():
        return OptionalOrchestratorWithReport.create_report(report)
    return OptionalOrchestratorWithReport.create_result_and_report(o, report)


def main(argv: Sequence[str] | None = None) -> int:
    script_path = str(Path(__file__).resolve())
    return orchestrator_main_with_default_run(script_path, argv)


if __name__ == "__main__":
    sys.exit(main())
