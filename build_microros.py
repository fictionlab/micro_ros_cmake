#!/usr/bin/env python3

import sys
import os
import shutil
from pathlib import Path
import subprocess
import argparse


def main():
    parser = argparse.ArgumentParser(description="Micro-ROS Build Script")
    parser.add_argument(
        "-d", "--debug", action="store_true", help="Build in Debug mode"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Enable verbose output"
    )
    parser.add_argument(
        "-x",
        "--no-clean",
        action="store_false",
        dest="clean",
        help="Do not clean build directory",
    )
    parser.add_argument(
        "-o",
        "--output-dir",
        default=str(Path(__file__).resolve().parent / "build"),
        help="Output directory for build artifacts",
    )
    parser.add_argument(
        "-t",
        "--toolchain-file",
        default=None,
        help="Path to custom toolchain file",
    )
    parser.add_argument(
        "-m",
        "--colcon-meta",
        default=None,
        help="Path to custom colcon.meta file",
    )
    parser.add_argument(
        "-e",
        "--extra-packages",
        default=None,
        help="Path to repos file with extra packages to import",
    )
    args = parser.parse_args()

    project_dir = Path(__file__).resolve().parent
    output_dir = Path(args.output_dir).resolve()
    toolchain_file = (
        Path(args.toolchain_file).resolve() if args.toolchain_file else None
    )
    colcon_meta_file = Path(args.colcon_meta).resolve() if args.colcon_meta else None
    extra_packages_file = (
        Path(args.extra_packages).resolve() if args.extra_packages else None
    )

    build_type = "Debug" if args.debug else "Release"
    verbose_makefile = "ON" if args.verbose else "OFF"
    event_handlers = "console_cohesion+" if args.verbose else "console_stderr-"

    if args.clean:
        print("--> Cleaning build directory...")
        shutil.rmtree(output_dir, ignore_errors=True)

    print("--> Building dev_ws...")
    build_dev_ws(project_dir, output_dir, build_type, verbose_makefile, event_handlers)

    print("--> Building mcu_ws...")
    build_mcu_ws(
        project_dir,
        output_dir,
        build_type,
        verbose_makefile,
        event_handlers,
        toolchain_file,
        colcon_meta_file,
        extra_packages_file,
    )

    print("--> Generating CMake config...")
    generate_cmake_config(output_dir)


def build_dev_ws(project_dir, output_dir, build_type, verbose_makefile, event_handlers):
    dev_ws_dir = output_dir / "dev_ws"
    os.makedirs(dev_ws_dir / "src", exist_ok=True)
    os.chdir(dev_ws_dir)

    vcs_import_cmd = [
        "vcs",
        "import",
        "--input",
        str(project_dir / "dev_packages.repos"),
        "src",
    ]
    subprocess.run(vcs_import_cmd, check=True)

    colcon_build_cmd = [
        "colcon",
        "build",
        "--merge-install",
        "--event-handlers",
        event_handlers,
        "--cmake-args",
        "-DBUILD_TESTING=OFF",
        "-DCAKE_C_COMPILER=gcc",
        "-DCMAKE_CXX_COMPILER=c++",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DCMAKE_VERBOSE_MAKEFILE={verbose_makefile}",
    ]
    subprocess.run(colcon_build_cmd, check=True)


def build_mcu_ws(
    project_dir,
    output_dir,
    build_type,
    verbose_makefile,
    event_handlers,
    toolchain_file=None,
    colcon_meta_file=None,
    extra_packages_file=None,
):
    mcu_ws_dir = output_dir / "mcu_ws"
    os.makedirs(mcu_ws_dir / "src", exist_ok=True)
    os.chdir(mcu_ws_dir)

    vcs_import_cmd = [
        "vcs",
        "import",
        "--input",
        str(project_dir / "mcu_packages.repos"),
        "src",
    ]
    subprocess.run(vcs_import_cmd, check=True)

    if extra_packages_file:
        extra_src_dir = mcu_ws_dir / "src" / "extra"
        os.makedirs(extra_src_dir, exist_ok=True)
        print(
            f"--> Importing extra repos from {extra_packages_file} into {extra_src_dir} ..."
        )
        vcs_import_extra_cmd = [
            "vcs",
            "import",
            "--input",
            extra_packages_file,
            str(extra_src_dir),
        ]
        subprocess.run(vcs_import_extra_cmd, check=True)

    colcon_ignore_packages = [
        "rcl_lifecycle",
        "rcl_logging_noop",
        "rcl_logging_spdlog",
        "rcl_yaml_param_parser",
        "rclc_examples",
        "rclc_lifecycle",
        "ros2trace",
        "rosidl_cli",
        "rosidl_generator_cpp",
        "rosidl_runtime_cpp",
        "rosidl_typesupport_cpp",
        "rosidl_typesupport_introspection_c",
        "rosidl_typesupport_introspection_cpp",
        "rosidl_typesupport_introspection_tests",
        "rosidl_typesupport_microxrcedds_cpp",
        "rosidl_typesupport_microxrcedds_c_tests",
        "rosidl_typesupport_microxrcedds_test_msg",
        "sensor_msgs_py",
        "test_msgs",
        "test_rmw_implementation",
        "test_tracetools",
        "test_tracetools_launch",
        "tracetools_launch",
        "tracetools_read",
        "tracetools_test",
        "tracetools_trace",
    ]

    project_colcon_meta = project_dir / "colcon.meta"
    metas = [str(project_colcon_meta)]
    if colcon_meta_file:
        metas.append(str(colcon_meta_file))
    dev_ws_install_dir = output_dir / "dev_ws" / "install"

    os.environ["AMENT_PREFIX_PATH"] = str(dev_ws_install_dir)
    os.environ["CMAKE_PREFIX_PATH"] = str(dev_ws_install_dir)
    os.environ["PYTHONPATH"] = ":".join(
        [
            os.environ.get("PYTHONPATH", ""),
            str(
                dev_ws_install_dir
                / "lib"
                / f"python3.{sys.version_info.minor}"
                / "site-packages"
            ),
        ]
    )
    os.environ["RMW_IMPLEMENTATION"] = "rmw_microxrcedds"

    colcon_build_cmd = [
        "colcon",
        "build",
        "--merge-install",
        "--metas",
        *metas,
        "--event-handlers",
        event_handlers,
        "--cmake-args",
        "--no-warn-unused-cli",
        "-DBUILD_SHARED_LIBS=OFF",
        "-DBUILD_TESTING=OFF",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DCMAKE_VERBOSE_MAKEFILE={verbose_makefile}",
    ]
    if toolchain_file:
        colcon_build_cmd.append(f"-DCMAKE_TOOLCHAIN_FILE={toolchain_file}")
    colcon_build_cmd += [
        "--packages-ignore",
        *colcon_ignore_packages,
    ]
    subprocess.run(colcon_build_cmd, check=True)


def generate_cmake_config(output_dir):
    config_file = output_dir / "micro_ros_cmakeConfig.cmake"
    with open(config_file, "w") as f:
        f.write(
            f"""
list(APPEND CMAKE_PREFIX_PATH "{output_dir / 'dev_ws' / 'install'}" "{output_dir / 'mcu_ws' / 'install'}")
list(APPEND PYTHON_PREFIX_PATH "{output_dir / 'dev_ws' / 'install' / 'lib' / f'python3.{sys.version_info.minor}' / 'site-packages'}" "{output_dir / 'mcu_ws' / 'install' / 'lib' / f'python3.{sys.version_info.minor}' / 'site-packages'}")

list(JOIN CMAKE_PREFIX_PATH ":" AMENT_PREFIX_PATH)
list(JOIN PYTHON_PREFIX_PATH ":" PYTHONPATH)

set(ENV{{AMENT_PREFIX_PATH}} "${{AMENT_PREFIX_PATH}}")
set(ENV{{PYTHONPATH}} "${{PYTHONPATH}}")
    """
        )


if __name__ == "__main__":
    main()
