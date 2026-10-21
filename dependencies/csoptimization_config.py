from pathlib import Path

from csorchestrator.application.recipes.manifest_github import (
    ManifestGithub,
    create_steps_to_get_libs_from_manifest,
    install_ubuntu_apt_packages,
    resolve_libs_deps_and_install_dependencies,
)
from csorchestrator.domain.orchestrator.orchestrator import Orchestrator
from csorchestrator.foundation.core.report import Report
from csorchestrator.frontend.step.step_get_precompiled_lib_github import (
    MappingFunction,
    StepGetPrecompiledLibGithub,
)
from csorchestrator.portable.release_manifest import ReleaseManifest

LIBRARY_LIST = ["csBlockMatrix", "csNelson"]

# project internal dependencies
LIBRARY_DEPENDENCIES = {
    "csNelson": ["csBlockMatrix"],
}

# project additional dependencies for `dev`
LIBRARY_DEPENDENCIES_DEV: dict[str, list[str]] = {}


def install_requirements(
    orchestrator: Orchestrator,
    dev: bool,
    phase_name_suffix: str,
    lib_list: list[str] | None = None,  # none means all
) -> None:
    """Install the Linux system requirements needed by the requested libraries.

    The OpenGL system packages are only required by ``csVisOpenGL``, so they
    are installed only when it is in ``lib_list`` (``None`` means all
    libraries).
    """

    _ = lib_list  # ignore lib, omp required in any case

    _ = dev  # ignore dev, those packages are required both in development and as dependencies

    install_ubuntu_apt_packages(
        orchestrator,
        [
            "libomp-dev",
        ],
        phase_name="Install Requirements (Linux-Ubuntu)" + phase_name_suffix,
    )


def install_csoptimization_build_dependencies(
    orchestrator: Orchestrator,
    base_libs_dir: Path,
    dependencies_config_path: Path,
    dev: bool,
    download_dependencies_manifest_and_bundle: bool,
    mapping_function: MappingFunction,
    phase_name_suffix: str,
    required_libs: list[str] | None = None,
) -> Report:

    report = Report()

    # the packages we need to install
    install_requirements(
        orchestrator=orchestrator, lib_list=required_libs, dev=dev, phase_name_suffix=phase_name_suffix
    )

    # external providers
    csfoundation_provider = ManifestGithub(
        base_url=StepGetPrecompiledLibGithub.GITHUB_BASE_URL_HTTPS,
        org="cscosine",
        git_repo="csfoundation",
        project_name="csfoundation",
        project_version="0.1.0",
        release_tag="v0.1.0-rc5",
    )

    third_party_base_libs_provider = ManifestGithub(
        base_url=StepGetPrecompiledLibGithub.GITHUB_BASE_URL_HTTPS,
        org="cscosine",
        git_repo="third_party_base_libs",
        project_name="third_party_base_libs",
        project_version="0.1.0",
        release_tag="v0.1.0-rc3",
    )

    external_providers = [
        csfoundation_provider,
        third_party_base_libs_provider,
    ]

    external_dependencies = {
        # provider csfoundation
        csfoundation_provider.project_name: {
            "csBlockMatrix": ["csCore"],
            "csNelson": ["csCore"],
        },
        # provider third_party_base_libs
        third_party_base_libs_provider.project_name: {
            "csBlockMatrix": [
                "eigen3",
                "libassert",
            ],
            "csNelson": [
                "eigen3",
                "libassert",
            ],
        },
    }

    external_dependencies_dev = {
        # provider csfoundation
        csfoundation_provider.project_name: {
            "csBlockMatrix": ["csLie", "csCamera"],
            "csNelson": ["csLie", "csCamera"],
        },
        # provider third_party_base_libs
        third_party_base_libs_provider.project_name: {
            "csBlockMatrix": ["Catch2"],
            "csNelson": ["Catch2"],
        },
    }

    # and the dependent external libraries

    report.append_report(
        resolve_libs_deps_and_install_dependencies(
            orchestrator=orchestrator,
            base_libs_dir=base_libs_dir,
            dependencies_config_path=dependencies_config_path,
            dev=dev,
            download_dependencies_manifest_and_bundle=download_dependencies_manifest_and_bundle,
            mapping_function=mapping_function,
            required_libs=required_libs,
            full_library_list=LIBRARY_LIST,
            library_dependencies=LIBRARY_DEPENDENCIES,
            library_dependencies_dev=LIBRARY_DEPENDENCIES_DEV,
            external_library_dependencies=external_dependencies,
            external_library_dependencies_dev=external_dependencies_dev,
            external_provides_description=external_providers,
            provider_mapping_function={},
            phase_name_suffix=phase_name_suffix,
        )
    )

    return report


def auto_install_csorchestrator_managed_libraries(
    orchestrator: Orchestrator,
    base_libs_dir: Path,
    manifest_description: ManifestGithub,
    manifest_loaded: ReleaseManifest,
    dependencies_config_path: Path,
    mapping_function: MappingFunction,
    download_dependencies_manifest_and_bundle: bool,
    phase_name_suffix: str,
    required_libs: list[str] | None = None,
) -> Report:

    # add all libraries need to build, but exclude dev ones
    report = install_csoptimization_build_dependencies(
        orchestrator=orchestrator,
        base_libs_dir=base_libs_dir,
        dependencies_config_path=dependencies_config_path,
        dev=False,
        download_dependencies_manifest_and_bundle=download_dependencies_manifest_and_bundle,
        required_libs=required_libs,
        mapping_function=mapping_function,
        phase_name_suffix=phase_name_suffix,
    )

    report.append_report(
        create_steps_to_get_libs_from_manifest(
            orchestrator=orchestrator,
            manifest_description=manifest_description,
            manifest_loaded=manifest_loaded,
            base_libs_dir=base_libs_dir,
            lib_name_list=required_libs,
            mapping_function=mapping_function,
            phase_name=phase_name_suffix,
            dev=False,
        )
    )

    return report
