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
    args = parser.parse_args()

    project_dir = Path(__file__).resolve().parent
    output_dir = Path(args.output_dir).resolve()

    build_type = "Debug" if args.debug else "Release"
    verbose_makefile = "ON" if args.verbose else "OFF"
    event_handlers = "console_cohesion+" if args.verbose else "console_stderr-"

    if args.clean:
        print("--> Cleaning build directory...")
        shutil.rmtree(output_dir, ignore_errors=True)

    print("--> Building dev_ws...")
    build_dev_ws(project_dir, output_dir, build_type, verbose_makefile, event_handlers)

    print("--> Building mcu_ws...")
    build_mcu_ws(project_dir, output_dir, build_type, verbose_makefile, event_handlers)

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
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DCMAKE_VERBOSE_MAKEFILE={verbose_makefile}",
    ]
    subprocess.run(colcon_build_cmd, check=True)


def build_mcu_ws(project_dir, output_dir, build_type, verbose_makefile, event_handlers):
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

    colcon_ignore_packages = [
        "lttngpy",
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
        "rosidl_typesupport_tests",
        "sensor_msgs_py",
        "test_msgs",
        "test_rmw_implementation",
        "test_ros2trace",
        "test_tracetools",
        "test_tracetools_launch",
        "tracetools_launch",
        "tracetools_read",
        "tracetools_test",
        "tracetools_trace",
    ]

    colcon_meta_path = project_dir / "colcon.meta"
    toolchain_path = project_dir / "toolchain.cmake"
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
        str(colcon_meta_path),
        "--event-handlers",
        event_handlers,
        "--cmake-args",
        "--no-warn-unused-cli",
        "-DBUILD_SHARED_LIBS=OFF",
        "-DBUILD_TESTING=OFF",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DCMAKE_VERBOSE_MAKEFILE={verbose_makefile}",
        f"-DCMAKE_TOOLCHAIN_FILE={toolchain_path}",
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

set(ENV{{"AMENT_PREFIX_PATH"}} "${{AMENT_PREFIX_PATH}}")
set(ENV{{"PYTHONPATH"}} "${{PYTHONPATH}}")
    """
        )


if __name__ == "__main__":
    main()
