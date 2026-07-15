#!/usr/bin/env python3
import sys
from pathlib import Path
from typing import Sequence

from csorchestrator.foundation.core.report import Report
from csorchestrator.foundation.core.optional_result_with_report import (
    OptionalResultWithReport,
)
from csorchestrator.foundation.git.resolve_url import RepoUrlParts

from csorchestrator.domain.orchestrator.workflow_config import (
    WorkflowConfig,
    Cron,
    DayOfWeek,
    ReleaseCreationOnTagConfig,
)
from csorchestrator.domain.context.context_os_architecture import OS, Architecture
from csorchestrator.domain.context.context_os_architecture import (
    UBUNTU_STRING_PREFIX,
)


from csorchestrator.frontend.cscmake_presets.supported_variants import (
    BuildConfig,
)

from csorchestrator.frontend.step.step_get_repository import (
    StepGetRepositoryGitHubSelf,
    StepGetRepositoryGitHub,
    StepGetRepositoryExtraDepthOne,
    StepGetRepositoryExtraAccessToken,
)
from csorchestrator.frontend.step.step_cmake_command import StepCMakeWorkflow
from csorchestrator.frontend.step.step_get_versions_from_cmake_config_package_version import (
    StepGetVersionsFromCMakeConfigPackageVersion,
)
from csorchestrator.frontend.step.step_create_archives import StepCreateArchives
from csorchestrator.frontend.step.step_upload_artifacts import (
    StepUploadArtifacts,
    create_artifact_prefix_from_orchestrator_name_version,
)
from csorchestrator.frontend.step.step_custom_command import (
    StepBashScriptCommand,
    StepInstallAptPackages,
    StepWinPSCommand,
)
from csorchestrator.frontend.step.step_get_precompiled_lib_github import (
    StepGetPrecompiledLibGithub,
)

from csorchestrator.frontend.local_execution.step_utils import (
    StepExecuteOnlyOncePerMatrix,
    StepSkipExecutionOnLocal,
    StepExecuteOnlyOn,
)

from csorchestrator.application.factory.factory import (
    OptionalOrchestratorWithReport,
    create_orchestrator_factory_all_supported_cases,
)
from csorchestrator.application.cli.cli import orchestrator_main_with_default_run

from csorchestrator.domain.context.context_os_architecture import (
    UBUNTU_VERSIONS,
)

from csorchestrator.domain.context.context_os_architecture_compiler_generator import (
    ContextOsArchitectureCompilerGenerator,
)
from csorchestrator.domain.context.context_compiler_generator import (
    ContextCompilerGenerator,
    Compiler,
    GeneratorWithType,
)


def create_orchestrator() -> OptionalOrchestratorWithReport:
    report = Report()

    base_target_dir = Path("workspace")
    base_install_dir = base_target_dir / Path("install")
    base_libs_dir = base_target_dir / Path("libs")

    o = create_orchestrator_factory_all_supported_cases(
        name="csSkiTracker",
        version="0.1.0",
        execution_matrix_name="orchestrator-matrix",
    )

    # strip non used matrix configs
    new_list = []
    for entry in o.execution_matrix.os_architecture_compiler_generator_list:
        # remove linux non x64 architectures for now
        if (
            entry.context_os_architecture.os == OS.LINUX
            and entry.context_os_architecture.architecture != Architecture.X64
        ):
            continue
        new_list += [entry]

    o.execution_matrix.os_architecture_compiler_generator_list = new_list

    o.wf_config = WorkflowConfig(
        on_push_branches=["main", "dev"],
        on_push_tags=["'v*.*.*'"],
        on_pull_request_branches=["main"],
        on_dispatch=True,
        on_schedule=Cron.weekly(DayOfWeek.MON, hour=3),
        create_release_on_tag=ReleaseCreationOnTagConfig(name="release-from-artifacts"),
    )

    # ----------------------------------------------------------------
    p = o.create_phase("Repos Update")

    # checkout myself for github actions
    p.add_step(
        StepGetRepositoryGitHubSelf(
            name="csImageTracker git self-checkout",
            description="Checkout csImageTracker repository",
        )
    )

    p.add_step(
        StepGetRepositoryGitHub(
            name="csCMake Git clone/pull-ff",
            description="Clone or pull-ff csCMake description",
            target_directory=(base_target_dir / "csCMake").as_posix(),
            repo_url_parts=RepoUrlParts(
                repo_base_url=StepGetRepositoryGitHub.GITHUB_BASE_URL_SSH,
                repo_org="cscosine",
                repo_name="csCMake.git",
            ),
            repo_ref="dev",
        )
        .add_extra(
            StepGetRepositoryExtraDepthOne(
                on_local_checkout=False,
                on_github_action_checkout=True,
            )
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(
            StepGetRepositoryExtraAccessToken("${{ secrets.ACTIONS_ORG_ACCESS }}")
        )
    )

    # ----------------------------------------------------------------
    p = o.create_phase("Install Requirements (Linux-Ubuntu)")
    p.add_step(
        StepInstallAptPackages(
            name="install apt packages",
            description="install apt packages if not already installed in the system",
            packages=[
                "libgl1-mesa-dev",
                "libopengl-dev",
                "mesa-common-dev",
                "libomp-dev",
            ],
            dry_run=False,
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(
            StepExecuteOnlyOn(os=OS.LINUX, version_starts_with=UBUNTU_STRING_PREFIX)
        )
    )

    # ----------------------------------------------------------------
    p = o.create_phase("Install CUDA Requirements")

    p.add_step(
        StepBashScriptCommand(
            name="set non interactive installer",
            description="install apt packages if not already installed in the system",
            cmd=[
                "# Pre-accept the Microsoft Core Fonts EULA so installation can run non-interactively.",
                "sudo apt update",
                'echo "ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true" | sudo debconf-set-selections',
                "sudo DEBIAN_FRONTEND=noninteractive apt install -y ttf-mscorefonts-installer",
            ],
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(
            StepExecuteOnlyOn(os=OS.LINUX, version_starts_with=UBUNTU_STRING_PREFIX)
        )
    )

    p.add_step(
        StepBashScriptCommand(
            name="remove libunwind (Ubuntu 22.04)",
            description="remove libunwind",
            cmd=[
                "# Remove libunwind causing errors on ubuntu 22.04",
                "sudo apt remove libunwind-*",
            ],
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(
            StepExecuteOnlyOn(
                os=OS.LINUX, version_starts_with=UBUNTU_VERSIONS.UBUNTU_22_04.value
            )
        )
    )

    p.add_step(
        StepInstallAptPackages(
            name="install apt packages",
            description="install apt packages if not already installed in the system",
            packages=[
                "gstreamer1.0*",
                "libavcodec-dev",
                "libavformat-dev",
                "libdc1394-dev",
                "libgstreamer-plugins-base1.0-dev",
                "libgstreamer1.0-dev",
                "libgtk-3-dev",
                "libjpeg-dev",
                "libopenexr-dev",
                "libpng-dev",
                "libswscale-dev",
                "libtbb-dev",
                "libtbb12",
                "libtiff-dev",
                "libwebp-dev",
                "pkg-config",
                "python3-dev",
                "python3-numpy",
                "python3-pip",
                "ubuntu-restricted-extras",
            ],
            dry_run=False,
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(
            StepExecuteOnlyOn(os=OS.LINUX, version_starts_with=UBUNTU_STRING_PREFIX)
        )
    )
    p.add_step(
        StepWinPSCommand(
            name="Install numpy (Windows)",
            description="install numpy",
            # from https://developer.nvidia.com/cuda-downloads?target_os=Windows&target_arch=x86_64&target_version=10&target_type=exe_local
            cmd=[
                '$ErrorActionPreference = "Stop"',
                "",
                "python -m pip install --upgrade pip",
                "python -m pip install numpy",
            ],
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(StepExecuteOnlyOn(os=OS.WINDOWS))
    )

    p.add_step(
        StepWinPSCommand(
            name="Install CUDA (Windows)",
            description="install cuda",
            # from https://developer.nvidia.com/cuda-downloads?target_os=Windows&target_arch=x86_64&target_version=10&target_type=exe_local
            cmd=[
                '$ErrorActionPreference = "Stop"',
                "",
                '$installer = "$env:TEMP/cuda_13.3.1_windows.exe"',
                "",
                'Write-Host "Download CUDA installer..."',
                "$download = [System.Diagnostics.Stopwatch]::StartNew()",
                '$url = "https://developer.download.nvidia.com/compute/cuda/13.3.1/local_installers/cuda_13.3.1_windows.exe"',
                "curl.exe -L --fail --progress-bar -o $installer $url",
                "$download.Stop()",
                'Write-Host ("Download took {0:mm\\:ss}" -f $download.Elapsed)',
                "",
                "$install = [System.Diagnostics.Stopwatch]::StartNew()",
                '$p = Start-Process -FilePath $installer -ArgumentList "-s" -Wait -PassThru',
                "$install.Stop()",
                'Write-Host ("Installation took {0:mm\\:ss}" -f $install.Elapsed)',
                "",
                "if ($p.ExitCode -ne 0) {",
                '    throw "Installer failed with exit code $($p.ExitCode)"',
                "}",
                "",
                "Remove-Item -Path $installer -ErrorAction SilentlyContinue",
            ],
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(StepExecuteOnlyOn(os=OS.WINDOWS))
    )

    p.add_step(
        StepWinPSCommand(
            name="Verify CUDA (Windows)",
            description="verify cuda installation",
            cmd=[
                '$nvcc = "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.3/bin/nvcc.exe"',
                "",
                "if (!(Test-Path $nvcc)) {",
                '  throw "nvcc not found: $nvcc"',
                "}",
                "",
                "& $nvcc --version",
            ],
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(StepExecuteOnlyOn(os=OS.WINDOWS))
    )

    p.add_step(
        StepWinPSCommand(
            name="Install cuDNN (Windows)",
            description="install cuDNN",
            # from https://developer.nvidia.com/cudnn-downloads?target_os=Windows&target_arch=x86_64&target_version=10&target_type=exe_local
            cmd=[
                '$ErrorActionPreference = "Stop"',
                "",
                '$installer = "$env:TEMP/cudnn_9.23.2_windows_x86_64.exe"',
                "",
                'Write-Host "Downloading cuDNN installer..."',
                "$download = [System.Diagnostics.Stopwatch]::StartNew()",
                '$url = "https://developer.download.nvidia.com/compute/cudnn/9.23.2/local_installers/cudnn_9.23.2_windows_x86_64.exe"',
                "curl.exe -L --fail --progress-bar -o $installer $url",
                "$download.Stop()",
                'Write-Host ("Download took {0:mm\\:ss}" -f $download.Elapsed)',
                "",
                "$install = [System.Diagnostics.Stopwatch]::StartNew()",
                '$p = Start-Process -FilePath $installer -ArgumentList "-s" -Wait -PassThru',
                "$install.Stop()",
                'Write-Host ("Installation took {0:mm\\:ss}" -f $install.Elapsed)',
                "",
                "if ($p.ExitCode -ne 0) {",
                '    throw "Installer failed with exit code $($p.ExitCode)"',
                "}",
                "",
                "Remove-Item -Path $installer -ErrorAction SilentlyContinue",
            ],
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(StepExecuteOnlyOn(os=OS.WINDOWS))
    )

    p.add_step(
        StepWinPSCommand(
            name="Verify cuDNN (Windows)",
            description="verify cuDNN installation",
            cmd=[
                '$root = "C:/Program Files/NVIDIA/CUDNN/v9.23"',
                "",
                "# Print directory tree first (directories only view via tree)",
                'Write-Host "cuDNN directory tree ($root):"',
                'cmd /c "tree `"$root`" /a"',
                'Write-Host ""',
                "",
                '$dllPath = Join-Path $root "bin/12.9/x64"',
                '$includePath = Join-Path $root "include/12.9"',
                '$libFile = Join-Path $root "lib/12.9/x64/cudnn.lib"',
                "",
                "# Strict existence checks",
                "if (-not (Test-Path $dllPath)) {",
                '  throw "cuDNN DLL directory not found: $dllPath"',
                "}",
                "",
                "if (-not (Test-Path $includePath)) {",
                '  throw "cuDNN include directory not found: $includePath"',
                "}",
                "",
                "if (-not (Test-Path $libFile)) {",
                '  throw "cuDNN library file not found: $libFile"',
                "}",
                "",
                "# List DLLs strictly from target folder",
                '$dlls = Get-ChildItem -Path $dllPath -Filter "cudnn*.dll" -ErrorAction Stop',
                "",
                "if (-not $dlls) {",
                '  throw "No cuDNN DLLs found in $dllPath"',
                "}",
                "",
                'Write-Host ""',
                'Write-Host "Found cuDNN DLLs in ${dllPath}:"',
                "$dlls | Select-Object -ExpandProperty FullName",
                "",
                'Write-Host ""',
                'Write-Host "Include path verified: ${includePath}"',
                'Write-Host "Library verified: ${libFile}"',
            ],
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(StepExecuteOnlyOn(os=OS.WINDOWS))
    )

    p.add_step(
        StepBashScriptCommand(
            name="Install CUDA (Ubuntu 22.04)",
            description="install cuda",
            # from https://developer.nvidia.com/cuda-downloads?target_os=Linux&target_arch=x86_64&Distribution=Ubuntu&target_version=22.04&target_type=deb_local
            cmd=[
                "wget -q https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-ubuntu2204.pin",
                "sudo mv cuda-ubuntu2204.pin /etc/apt/preferences.d/cuda-repository-pin-600",
                "wget -q https://developer.download.nvidia.com/compute/cuda/13.3.1/local_installers/cuda-repo-ubuntu2204-13-3-local_13.3.1-610.43.02-1_amd64.deb",
                "sudo dpkg -i cuda-repo-ubuntu2204-13-3-local_13.3.1-610.43.02-1_amd64.deb",
                "sudo cp /var/cuda-repo-ubuntu2204-13-3-local/cuda-*-keyring.gpg /usr/share/keyrings/",
                "sudo apt-get update",
                "sudo apt-get -y install cuda-toolkit-13-3",
            ],
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(
            StepExecuteOnlyOn(
                os=OS.LINUX,
                version_starts_with=UBUNTU_VERSIONS.UBUNTU_22_04.value,
                arch=Architecture.X64,
            )
        )
    )

    p.add_step(
        StepBashScriptCommand(
            name="Install CUDA (Ubuntu 24.04)",
            description="install cuda",
            # from https://developer.nvidia.com/cuda-downloads?target_os=Linux&target_arch=x86_64&Distribution=Ubuntu&target_version=24.04&target_type=deb_local
            cmd=[
                "wget -q https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2404/x86_64/cuda-ubuntu2404.pin",
                "sudo mv cuda-ubuntu2404.pin /etc/apt/preferences.d/cuda-repository-pin-600",
                "wget -q https://developer.download.nvidia.com/compute/cuda/13.3.1/local_installers/cuda-repo-ubuntu2404-13-3-local_13.3.1-610.43.02-1_amd64.deb",
                "sudo dpkg -i cuda-repo-ubuntu2404-13-3-local_13.3.1-610.43.02-1_amd64.deb",
                "sudo cp /var/cuda-repo-ubuntu2404-13-3-local/cuda-*-keyring.gpg /usr/share/keyrings/",
                "sudo apt update",
                "sudo apt install -y cuda-toolkit-13-3",
            ],
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(
            StepExecuteOnlyOn(
                os=OS.LINUX,
                version_starts_with=UBUNTU_VERSIONS.UBUNTU_24_04.value,
                arch=Architecture.X64,
            )
        )
    )

    p.add_step(
        StepBashScriptCommand(
            name="Install cudnn (Ubuntu 22.04)",
            description="install cudnn",
            # https://developer.nvidia.com/cudnn-downloads?target_os=Linux&target_arch=x86_64&Distribution=Ubuntu&target_version=22.04&target_type=deb_local&Configuration=Full
            cmd=[
                "wget -q https://developer.download.nvidia.com/compute/cudnn/9.24.0/local_installers/cudnn-local-repo-ubuntu2204-9.24.0_1.0-1_amd64.deb",
                "sudo dpkg -i cudnn-local-repo-ubuntu2204-9.24.0_1.0-1_amd64.deb",
                "sudo cp /var/cudnn-local-repo-ubuntu2204-9.24.0/cudnn-*-keyring.gpg /usr/share/keyrings/",
                "sudo apt-get update",
                "sudo apt-get -y install libcudnn9-cuda-13 libcudnn9-dev-cuda-13",
            ],
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(
            StepExecuteOnlyOn(
                os=OS.LINUX,
                version_starts_with=UBUNTU_VERSIONS.UBUNTU_22_04.value,
                arch=Architecture.X64,
            )
        )
    )

    p.add_step(
        StepBashScriptCommand(
            name="Install cudnn (Ubuntu 24.04)",
            description="install cudnn",
            # https://developer.nvidia.com/cudnn-downloads?target_os=Linux&target_arch=x86_64&Distribution=Ubuntu&target_version=24.04&target_type=deb_local&Configuration=Full
            cmd=[
                "wget -q https://developer.download.nvidia.com/compute/cudnn/9.24.0/local_installers/cudnn-local-repo-ubuntu2404-9.24.0_1.0-1_amd64.deb",
                "sudo dpkg -i cudnn-local-repo-ubuntu2404-9.24.0_1.0-1_amd64.deb",
                "sudo cp /var/cudnn-local-repo-ubuntu2404-9.24.0/cudnn-*-keyring.gpg /usr/share/keyrings/",
                "sudo apt-get update",
                "sudo apt-get -y install libcudnn9-cuda-13 libcudnn9-dev-cuda-13",
            ],
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(
            StepExecuteOnlyOn(
                os=OS.LINUX,
                version_starts_with=UBUNTU_VERSIONS.UBUNTU_24_04.value,
                arch=Architecture.X64,
            )
        )
    )

    p.add_step(
        StepBashScriptCommand(
            name="verify a cuda stub library exists",
            description="verify cuda stub library exists",
            cmd=[
                "ls /usr/local/cuda/lib64/stubs/libcuda.so",
            ],
        )
        .add_extra(StepExecuteOnlyOncePerMatrix())
        .add_extra(
            StepExecuteOnlyOn(os=OS.LINUX, version_starts_with=UBUNTU_STRING_PREFIX)
        )
    )
    # ----------------------------------------------------------------
    p = o.create_phase("Get Precompiled Libraries")

    list_3rdPartyBaseLibs: dict[str, str] = {
        "eigen3": "3.4.0",
        "fmt": "11.2.1",
        "fmt-eigen": "1.0.0",
        "cpptrace": "0.8.3",
        "magic_enum": "0.9.7",
        "libassert": "2.1.5",
        "tclap": "1.0.0",
        "Catch2": "3.10.0",
        "pipes": "0.0.1",
        "NamedType": "1.1.0",
        "tl-expected": "1.2.0",
        "tl-optional": "1.0.0",
    }

    for lib_name, lib_version in list_3rdPartyBaseLibs.items():
        p.add_step(
            StepGetPrecompiledLibGithub(
                name=f"Get Precompiled Lib {lib_name} from 3rdPartyBaseLibs",
                description="get precompiled lib from github release",
                base_url=StepGetPrecompiledLibGithub.GITHUB_BASE_URL_HTTPS,
                org="cscosine",
                project_name="3rdPartyBaseLibs",
                project_tag="v0.1.0",
                lib_name=lib_name,
                lib_version=lib_version,
                base_libs_dir=base_libs_dir,
            )
        )

    list_csBaseLibs: dict[str, str] = {
        "csCore": "1.0.0",
        "csLie": "1.0.0",
        "csCamera": "1.0.0",
        "csVisOpenGL": "1.0.0",
    }

    for lib_name, lib_version in list_csBaseLibs.items():
        p.add_step(
            StepGetPrecompiledLibGithub(
                name=f"Get Precompiled Lib {lib_name} from csBaseLibs",
                description="get precompiled lib from github release",
                base_url=StepGetPrecompiledLibGithub.GITHUB_BASE_URL_HTTPS,
                org="cscosine",
                project_name="csBaseLibs",
                project_tag="v0.1.0",
                lib_name=lib_name,
                lib_version=lib_version,
                base_libs_dir=base_libs_dir,
            )
        )

    list_csOptimization: dict[str, str] = {
        "csBlockMatrix": "1.0.0",
        "csNelson": "1.0.0",
    }

    for lib_name, lib_version in list_csOptimization.items():
        p.add_step(
            StepGetPrecompiledLibGithub(
                name=f"Get Precompiled Lib {lib_name} from csOptimization",
                description="get precompiled lib from github release",
                base_url=StepGetPrecompiledLibGithub.GITHUB_BASE_URL_HTTPS,
                org="cscosine",
                project_name="csOptimization",
                project_tag="v0.1.0",
                lib_name=lib_name,
                lib_version=lib_version,
                base_libs_dir=base_libs_dir,
            )
        )

    def qt6_mapping(
        qt6_context: ContextOsArchitectureCompilerGenerator,
    ) -> ContextOsArchitectureCompilerGenerator | None:
        if qt6_context.context_os_architecture.os == OS.LINUX:
            if (
                qt6_context.context_os_architecture.os_version
                == UBUNTU_VERSIONS.UBUNTU_22_04.value
                or qt6_context.context_os_architecture.os_version
                == UBUNTU_VERSIONS.UBUNTU_24_04.value
            ):
                newContext = qt6_context
                newContext.context_compiler_generator = ContextCompilerGenerator(
                    compiler_family=Compiler.GCC,
                    compiler_version=ContextCompilerGenerator.COMPILER_VERSION_DEFAULT,
                    build_generator=GeneratorWithType.NINJA,
                )
                return newContext
        elif qt6_context.context_os_architecture.os == OS.WINDOWS:
            newContext = qt6_context
            newContext.context_compiler_generator = ContextCompilerGenerator(
                compiler_family=Compiler.MSVC,
                compiler_version=ContextCompilerGenerator.COMPILER_VERSION_MSVC_2022_17,
                build_generator=GeneratorWithType.NINJA,
            )
            return newContext
        return None

    p.add_step(
        StepGetPrecompiledLibGithub(
            name="Get Precompiled Lib qt6",
            description="get precompiled lib from github release",
            base_url=StepGetPrecompiledLibGithub.GITHUB_BASE_URL_HTTPS,
            org="cscosine",
            project_name="csQt6",
            project_tag="v6.11.1",
            lib_name="qt6",
            lib_version="v6.11.1",
            base_libs_dir=base_libs_dir,
            mapping_function=qt6_mapping,
        )
    )

    def opencv_mapping(
        opencv_context: ContextOsArchitectureCompilerGenerator,
    ) -> ContextOsArchitectureCompilerGenerator | None:
        if opencv_context.context_os_architecture.os == OS.LINUX:
            if (
                opencv_context.context_os_architecture.os_version
                == UBUNTU_VERSIONS.UBUNTU_22_04.value
                or opencv_context.context_os_architecture.os_version
                == UBUNTU_VERSIONS.UBUNTU_24_04.value
            ):
                newContext = opencv_context
                newContext.context_compiler_generator = ContextCompilerGenerator(
                    compiler_family=opencv_context.context_compiler_generator.compiler_family,
                    compiler_version=opencv_context.context_compiler_generator.compiler_version,
                    build_generator=GeneratorWithType.NINJA,
                )
                return newContext
        elif opencv_context.context_os_architecture.os == OS.WINDOWS:
            if (
                opencv_context.context_compiler_generator.compiler_family
                == Compiler.MSVC_CLANG
            ):
                newContext = opencv_context
                newContext.context_compiler_generator = ContextCompilerGenerator(
                    compiler_family=Compiler.MSVC,
                    compiler_version=opencv_context.context_compiler_generator.compiler_version,
                    build_generator=opencv_context.context_compiler_generator.build_generator,
                )
                return newContext
            else: 
                return opencv_context
        return None

    p.add_step(
        StepGetPrecompiledLibGithub(
            name="Get Precompiled Lib opencv",
            description="get precompiled lib from github release",
            base_url=StepGetPrecompiledLibGithub.GITHUB_BASE_URL_HTTPS,
            org="cscosine",
            project_name="csOpenCV",
            project_tag="v5.0.0",
            lib_name="opencv",
            lib_version="5.0.0",
            base_libs_dir=base_libs_dir,
            mapping_function=opencv_mapping,
        )
    )

    # ----------------------------------------------------------------
    p = o.create_phase("Configure-Build-Test-Install")
    p.add_step(
        StepCMakeWorkflow(
            name="csSkiTracker CMake Workflow",
            description="CMake workflow for csSkiTracker",
            source_dir=Path("./").as_posix(),
            config=BuildConfig.DEBUG_RELEASE_RELWITHDEBINFO_PARANOID,
        )
    )

    # ----------------------------------------------------------------
    p = o.create_phase("Create and Upload Artifacts")
    p.add_step(
        StepGetVersionsFromCMakeConfigPackageVersion(
            name="Get Versions",
            description="Get Versions for all libs",
            repos_auto_search_list=["csSkiTracker"],
            base_install_dir=base_install_dir,
            id="versions",
            output_dict_name="packages",
        )
    )

    p.add_step(
        StepCreateArchives(
            name="Create Archives",
            description="Create archives with libs and versions",
            input_id="versions",
            input_dict="packages",
            base_install_dir=base_install_dir,
        ).add_extra(StepSkipExecutionOnLocal())
    )

    p.add_step(
        StepUploadArtifacts(
            name="Upload Artifacts",
            description="Upload Artifacts with libs and versions",
            base_install_dir=base_install_dir,
            artifact_prefix=create_artifact_prefix_from_orchestrator_name_version(o),
        )
    )

    return OptionalResultWithReport.createResultAndReport(o, report)


def main(argv: Sequence[str] | None = None) -> int:
    script_path = str(Path(__file__).resolve())
    return orchestrator_main_with_default_run(script_path, argv)


if __name__ == "__main__":
    sys.exit(main())
