from pathlib import Path

from csorchestrator.application.recipes.manifest_github import (
    ManifestGithub,
    create_steps_to_get_libs_from_manifest,
    fill_in_library_list,
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

LIBRARY_LIST = ["csCore", "csLie", "csCamera", "csVisOpenGL"]

# project internal dependencies
LIBRARY_DEPENDENCIES = {
    "csLie": ["csCore"],
    "csCamera": ["csCore"],
    "csVisOpenGL": ["csCore"],
}

# project additional dependencies for `dev`
LIBRARY_DEPENDENCIES_DEV: dict[str, list[str]] = {}


def install_requirements(
    orchestrator: Orchestrator,
    dev: bool,
    lib_list: list[str] | None = None,  # none means all
) -> None:
    """Install the Linux system requirements needed by the requested libraries.

    The OpenGL system packages are only required by ``csVisOpenGL``, so they
    are installed only when it is in ``lib_list`` (``None`` means all
    libraries).
    """

    lib_list = fill_in_library_list(lib_list, LIBRARY_LIST)

    _ = dev  # ignore dev, those packages are required both in development and as dependencies

    if "csVisOpenGL" in lib_list:
        install_ubuntu_apt_packages(
            orchestrator,
            [
                "libgl1-mesa-dev",
                "libopengl-dev",
                "mesa-common-dev",
            ],
        )


def install_csfoundation_build_dependencies(
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
    install_requirements(orchestrator=orchestrator, lib_list=required_libs, dev=dev)

    # external providers
    third_party_base_libs_provider = ManifestGithub(
        base_url=StepGetPrecompiledLibGithub.GITHUB_BASE_URL_HTTPS,
        org="cscosine",
        git_repo="third_party_base_libs",
        project_name="third_party_base_libs",
        project_version="0.1.0",
        release_tag="v0.1.0-rc3",
    )

    csqt6_provider = ManifestGithub(
        base_url=StepGetPrecompiledLibGithub.GITHUB_BASE_URL_HTTPS,
        org="cscosine",
        git_repo="csqt6",
        project_name="csqt6",
        project_version="6.11.1",
        release_tag="v6.11.1-rc3",
    )

    external_providers = [
        third_party_base_libs_provider,
    ]

    external_dependencies = {
        # provider third_party_base_libs
        third_party_base_libs_provider.project_name: {
            "csCore": [
                "eigen3",
                "fmt",
                "fmt-eigen",
                "libassert",
                "pipes",
                "NamedType",
                "tl-expected",
                "tl-optional",
                "Catch2",
            ],
            "csLie": ["eigen3"],
            "csCamera": ["eigen3"],
            "csVisOpenGL": ["eigen3"],
        },
    }

    external_dependencies_dev = {
        # provider third_party_base_libs
        third_party_base_libs_provider.project_name: {
            "csCore": [],
            "csLie": ["Catch2"],
            "csCamera": ["Catch2"],
            "csVisOpenGL": ["Catch2"],
        },
    }

    provider_mapping_functions_names = {}

    if required_libs is None or "csVisOpenGL" in required_libs:
        external_providers.append(csqt6_provider)

        external_dependencies[csqt6_provider.project_name] = {
            "csVisOpenGL": ["qt6"],
        }

        provider_mapping_functions_names = {csqt6_provider.project_name: "qt6_mapping"}

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
            provider_mapping_function=provider_mapping_functions_names,
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
    report = install_csfoundation_build_dependencies(
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
