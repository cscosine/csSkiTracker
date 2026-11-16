from pathlib import Path

from csorchestrator.application.recipes.manifest_github import (
    ManifestGithub,
    create_steps_to_get_libs_from_manifest,
    install_ubuntu_apt_packages,
)
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
    elif context.context_os_architecture.os == OS.MACOS:
        # every macOS consumer (Ninja, Ninja Multi-Config, Xcode) uses the Apple Clang Ninja build of its macOS version
        new_context = context
        new_context.context_compiler_generator = ContextCompilerGenerator(
            compiler_family=Compiler.APPLE_CLANG,
            compiler_version=ContextCompilerGenerator.COMPILER_VERSION_DEFAULT,
            build_generator=GeneratorWithType.NINJA,
        )
        return new_context
    return None


# Linux: the libraries of the system the Qt plugins of a GUI application need at run time (the xcb platform plugin,
# libxcb-cursor0 since Qt 6.5) and not the libraries of Qt: installed with the qt6 archives, so that a consumer's
# deployment (cs_deployment_qt_gui, which resolves the plugins when it is made) and its run find them
QT6_PLUGINS_UBUNTU_PACKAGES = [
    "libxcb-cursor0",
    "libxcb-icccm4",
    "libxcb-image0",
    "libxcb-keysyms1",
    "libxcb-render-util0",
    "libxcb-shape0",
    "libxcb-xkb1",
    "libxkbcommon-x11-0",
]


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

    if required_libs is None or "qt6" in required_libs:
        install_ubuntu_apt_packages(
            orchestrator=orchestrator,
            packages=QT6_PLUGINS_UBUNTU_PACKAGES,
            phase_name_suffix=f"{phase_name_suffix} (Qt plugins)",
        )

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
