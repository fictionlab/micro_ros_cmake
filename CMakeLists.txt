cmake_minimum_required(VERSION 3.22)
project(micro_ros_cmake)

# Define the output directory for build artifacts
set(MICRO_ROS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/micro_ros")
set(MICRO_ROS_CONFIG_FILE "${MICRO_ROS_OUTPUT_DIR}/micro_ros_cmakeConfig.cmake")

set(MICRO_ROS_TOOLCHAIN_FILE "" CACHE FILEPATH "Toolchain file for Micro-ROS")
set(MICRO_ROS_COLCON_META_FILE "" CACHE FILEPATH "Colcon meta file for Micro-ROS")
set(MICRO_ROS_EXTRA_PACKAGES_FILE "" CACHE FILEPATH "Extra packages file for Micro-ROS")
set(MICRO_ROS_VERBOSE_BUILD OFF CACHE BOOL "Verbose output for Micro-ROS build")

if (${MICRO_ROS_TOOLCHAIN_FILE} STREQUAL "")
    message(ERROR "No toolchain file specified. Please set the MICRO_ROS_TOOLCHAIN_FILE variable.")
else()
    message(STATUS "Using toolchain file: ${MICRO_ROS_TOOLCHAIN_FILE}")
    list(APPEND BUILD_SCRIPT_ARGS "-t" "${MICRO_ROS_TOOLCHAIN_FILE}")
endif()

if (${MICRO_ROS_COLCON_META_FILE} STREQUAL "")
    message(STATUS "No colcon meta file specified. Using default.")
else()
    message(STATUS "Using colcon meta file: ${MICRO_ROS_COLCON_META_FILE}")
    list(APPEND BUILD_SCRIPT_ARGS "-m" "${MICRO_ROS_COLCON_META_FILE}")
endif()

if (${MICRO_ROS_EXTRA_PACKAGES_FILE} STREQUAL "")
    message(STATUS "No extra packages file specified. Using default.")
else()
    message(STATUS "Using extra packages file: ${MICRO_ROS_EXTRA_PACKAGES_FILE}")
    list(APPEND BUILD_SCRIPT_ARGS "-e" "${MICRO_ROS_EXTRA_PACKAGES_FILE}")
endif()

if (${MICRO_ROS_VERBOSE_BUILD})
    message(STATUS "Verbose build enabled.")
    list(APPEND BUILD_SCRIPT_ARGS "-v")
else()
    message(STATUS "Verbose build disabled.")
endif()

unset(ENV{CC})
unset(ENV{CXX})

set(MICRO_ROS_BUILD_COMMAND
    python3 ${CMAKE_CURRENT_SOURCE_DIR}/build_microros.py
    --output-dir ${MICRO_ROS_OUTPUT_DIR}
    ${BUILD_SCRIPT_ARGS}
)

if(NOT EXISTS "${MICRO_ROS_CONFIG_FILE}")
    message(STATUS "Generating Micro-ROS libraries and configuration...")
    execute_process(
        COMMAND ${MICRO_ROS_BUILD_COMMAND}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE result
    )
    if(result)
        message(FATAL_ERROR "Failed to run build_microros.py script. Please check the script and its dependencies.")
    else()
        message(STATUS "Successfully generated Micro-ROS libraries and configuration.")
    endif()
else()
    message(STATUS "Micro-ROS libraries and configuration already exist. Skipping generation.")
endif()

add_custom_target(rebuild_micro_ros
    COMMAND ${MICRO_ROS_BUILD_COMMAND}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Force rebuilding Micro-ROS libraries and configuration"
)

list(APPEND CMAKE_PREFIX_PATH "${MICRO_ROS_OUTPUT_DIR}")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "Modified prefix path" FORCE)
