from pathlib import Path

from csorchestrator.application.recipes.manifest_github import (
    ManifestGithub,
    resolve_libs_deps_and_install_dependencies,
)
from csorchestrator.domain.orchestrator.orchestrator import Orchestrator
from csorchestrator.foundation.core.report import Report
from csorchestrator.frontend.step.step_get_precompiled_lib_github import (
    MappingFunction,
    StepGetPrecompiledLibGithub,
)

LIBRARY_LIST = ["csSkiTracker"]

# project internal dependencies
LIBRARY_DEPENDENCIES: dict[str, list[str]] = {}

# project additional dependencies for `dev`
LIBRARY_DEPENDENCIES_DEV: dict[str, list[str]] = {}


def install_csskitracker_build_dependencies(
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

    # external providers
    third_party_base_libs_provider = ManifestGithub(
        base_url=StepGetPrecompiledLibGithub.GITHUB_BASE_URL_HTTPS,
        org="cscosine",
        git_repo="third_party_base_libs",
        project_name="third_party_base_libs",
        project_version="0.1.0",
        release_tag="v0.1.0-rc3",
    )

    csfoundation_provider = ManifestGithub(
        base_url=StepGetPrecompiledLibGithub.GITHUB_BASE_URL_HTTPS,
        org="cscosine",
        git_repo="csfoundation",
        project_name="csfoundation",
        project_version="0.1.0",
        release_tag="v0.1.0-rc7",
    )

    csoptimization_provider = ManifestGithub(
        base_url=StepGetPrecompiledLibGithub.GITHUB_BASE_URL_HTTPS,
        org="cscosine",
        git_repo="csoptimization",
        project_name="csoptimization",
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

    csopencvcpu_provider = ManifestGithub(
        base_url=StepGetPrecompiledLibGithub.GITHUB_BASE_URL_HTTPS,
        org="cscosine",
        git_repo="csopencvcpu",
        project_name="csopencvcpu",
        project_version="5.0.0",
        release_tag="v5.0.1-rc2",
    )

    external_providers = [
        third_party_base_libs_provider,
        csfoundation_provider,
        csoptimization_provider,
        csqt6_provider,
        csopencvcpu_provider,
    ]

    external_dependencies = {
        # provider third_party_base_libs
        third_party_base_libs_provider.project_name: {
            "csSkiTracker": [
                "eigen3",
                "tclap",
            ],
        },
        # provider csfoundation
        csfoundation_provider.project_name: {
            "csSkiTracker": [
                "csCore",
                "csCamera",
                "csLie",
                "csVisOpenGL",
            ],
        },
        # provider csoptimization
        csoptimization_provider.project_name: {
            "csSkiTracker": [
                "csNelson",
                "csBlockMatrix",
            ],
        },
        # provider qt6
        csqt6_provider.project_name: {
            "csSkiTracker": ["qt6"],
        },
        # provider csopencvcpu
        csopencvcpu_provider.project_name: {
            "csSkiTracker": ["opencv"],
        },
    }

    external_dependencies_dev: dict[str, dict[str, list[str]]] = {}

    provider_mapping_functions_names: dict[str, str | MappingFunction] = {csqt6_provider.project_name: "qt6_mapping"}

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
