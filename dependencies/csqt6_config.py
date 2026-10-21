from pathlib import Path

from csorchestrator.application.recipes.manifest_github import ManifestGithub, create_steps_to_get_libs_from_manifest
from csorchestrator.domain.context.context_compiler_generator import (
    Compiler,
    ContextCompilerGenerator,
    GeneratorWithType,
)
from csorchestrator.domain.context.context_os_architecture import OS
from csorchestrator.domain.context.context_os_architecture_compiler_generator import (
    ContextOsArchitectureCompilerGenerator,
)
from csorchestrator.domain.orchestrator.orchestrator import Orchestrator
from csorchestrator.foundation.core.report import Report
from csorchestrator.frontend.step.step_get_precompiled_lib_github import MappingFunction
from csorchestrator.portable.release_manifest import ReleaseManifest


def qt6_mapping(
    context: ContextOsArchitectureCompilerGenerator,
) -> ContextOsArchitectureCompilerGenerator | None:
    if context.context_os_architecture.os == OS.LINUX:
        new_context = context
        new_context.context_compiler_generator = ContextCompilerGenerator(
            compiler_family=Compiler.GCC,
            compiler_version=ContextCompilerGenerator.COMPILER_VERSION_DEFAULT,
            build_generator=GeneratorWithType.NINJA,
        )
        return new_context
    elif context.context_os_architecture.os == OS.WINDOWS:
        new_context = context
        new_context.context_compiler_generator = ContextCompilerGenerator(
            compiler_family=Compiler.MSVC,
            compiler_version=ContextCompilerGenerator.COMPILER_VERSION_MSVC_2022_17,
            build_generator=GeneratorWithType.NINJA,
        )
        return new_context
    return None


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

    _ = dependencies_config_path
    _ = download_dependencies_manifest_and_bundle

    return create_steps_to_get_libs_from_manifest(
        orchestrator=orchestrator,
        manifest_description=manifest_description,
        manifest_loaded=manifest_loaded,
        base_libs_dir=base_libs_dir,
        lib_name_list=required_libs,
        mapping_function=mapping_function,
        phase_name=phase_name_suffix,
        dev=False,
    )
