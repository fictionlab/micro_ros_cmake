cmake_minimum_required(VERSION 3.22)
project(micro_ros_cmake)

# Define the output directory for build artifacts
set(MICRO_ROS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/micro_ros")
set(MICRO_ROS_CONFIG_FILE "${MICRO_ROS_OUTPUT_DIR}/micro_ros_cmakeConfig.cmake")

# Allow the parent project to override the toolchain file
set(MICRO_ROS_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/toolchain.cmake" CACHE FILEPATH "Toolchain file for Micro-ROS")

if(NOT EXISTS "${MICRO_ROS_CONFIG_FILE}")
    message(STATUS "Generating Micro-ROS libraries and configuration...")
    execute_process(
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/build_microros.py
                --output-dir ${MICRO_ROS_OUTPUT_DIR}
                --toolchain-file ${MICRO_ROS_TOOLCHAIN_FILE}
                -v
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

list(APPEND CMAKE_PREFIX_PATH "${MICRO_ROS_OUTPUT_DIR}")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "Modified prefix path" FORCE)
