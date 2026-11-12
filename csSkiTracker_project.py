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
)
from csorchestrator.application.recipes.create_orchestrator import (
    create_default_orchestrator,
)
from csorchestrator.application.recipes.cscmake_release import create_cscmake_release_archive
from csorchestrator.foundation.core.report import Report
from csorchestrator.frontend.cscmake_presets.supported_variants import (
    BuildConfig,
)
from csorchestrator.frontend.step.step_get_precompiled_lib_github import (
    identity_mapping_function,
)

from csSkiTracker_config import install_csskitracker_build_dependencies


def create_orchestrator() -> OptionalOrchestratorWithReport:
    report = Report()

    base_target_dir = Path("workspace")
    base_install_dir = base_target_dir / Path("install")
    base_libs_dir = base_target_dir / Path("libs")
    cscmake_version = "1.0.0-rc2"

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

    # ----------------------------------------------------------------
    build_repos(
        orchestrator=o,
        base_target_dir=base_target_dir,
        build_self=BuildConfig.DEBUG_RELEASE_RELWITHDEBINFO_PARANOID,
    )

    create_and_upload_artifacts(
        orchestrator=o,
        base_install_dir=base_install_dir,
        repos_auto_search_list=["csSkiTracker"],
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
