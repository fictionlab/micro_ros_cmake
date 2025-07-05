cmake_minimum_required(VERSION 3.22)
project(micro_ros_cmake)

# Define the output directory for build artifacts
set(MICRO_ROS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/micro_ros")
set(MICRO_ROS_CONFIG_FILE "${MICRO_ROS_OUTPUT_DIR}/micro_ros_cmakeConfig.cmake")

set(MICRO_ROS_TOOLCHAIN_FILE "" CACHE FILEPATH "Toolchain file for Micro-ROS")
set(MICRO_ROS_COLCON_META_FILE "${CMAKE_CURRENT_SOURCE_DIR}/colcon.meta" CACHE FILEPATH "Colcon meta file for Micro-ROS")
set(MICRO_ROS_MCU_PACKAGES_FILE "${CMAKE_CURRENT_SOURCE_DIR}/mcu_packages.repos" CACHE FILEPATH "MCU packages file for Micro-ROS")
set(MICRO_ROS_EXTRA_PACKAGES_FILES "" CACHE STRING "Extra packages files for Micro-ROS (semicolon-separated list)")
set(MICRO_ROS_VERBOSE_BUILD OFF CACHE BOOL "Verbose output for Micro-ROS build")

set(MICRO_ROS_SOURCE_FILES)
list(APPEND MICRO_ROS_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/build_microros.py")
list(APPEND MICRO_ROS_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/dev_packages.repos")

if (MICRO_ROS_TOOLCHAIN_FILE STREQUAL "")
  message(ERROR "No toolchain file specified. Please set the MICRO_ROS_TOOLCHAIN_FILE variable.")
else()
  message(STATUS "Using toolchain file: ${MICRO_ROS_TOOLCHAIN_FILE}")
  list(APPEND BUILD_SCRIPT_ARGS "-t" "${MICRO_ROS_TOOLCHAIN_FILE}")
  list(APPEND MICRO_ROS_SOURCE_FILES "${MICRO_ROS_TOOLCHAIN_FILE}")
endif()

message(STATUS "Using colcon meta file: ${MICRO_ROS_COLCON_META_FILE}")
list(APPEND BUILD_SCRIPT_ARGS "-m" "${MICRO_ROS_COLCON_META_FILE}")
list(APPEND MICRO_ROS_SOURCE_FILES "${MICRO_ROS_COLCON_META_FILE}")

message(STATUS "Using MCU packages file: ${MICRO_ROS_MCU_PACKAGES_FILE}")
list(APPEND BUILD_SCRIPT_ARGS "--mcu-packages" "${MICRO_ROS_MCU_PACKAGES_FILE}")
list(APPEND MICRO_ROS_SOURCE_FILES "${MICRO_ROS_MCU_PACKAGES_FILE}")

if (MICRO_ROS_EXTRA_PACKAGES_FILES STREQUAL "")
  message(STATUS "No extra packages files specified.")
else()
  message(STATUS "Using extra packages files: ${MICRO_ROS_EXTRA_PACKAGES_FILES}")
  foreach(EXTRA_PACKAGES_FILE IN LISTS MICRO_ROS_EXTRA_PACKAGES_FILES)
    list(APPEND BUILD_SCRIPT_ARGS "-e" "${EXTRA_PACKAGES_FILE}")

    if(IS_DIRECTORY "${EXTRA_PACKAGES_FILE}")
      # If it's a directory, add all files in it to the source files list
      file(GLOB_RECURSE DIR_FILES "${EXTRA_PACKAGES_FILE}/*")
      foreach(DIR_FILE IN LISTS DIR_FILES)
        if(NOT IS_DIRECTORY "${DIR_FILE}")
          list(APPEND MICRO_ROS_SOURCE_FILES "${DIR_FILE}")
        endif()
      endforeach()
    else()
      # If it's a file, add it directly
      list(APPEND MICRO_ROS_SOURCE_FILES "${EXTRA_PACKAGES_FILE}")
    endif()
  endforeach()
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

# Check if any source files are newer than the config file
set(NEED_REBUILD FALSE)
if(EXISTS "${MICRO_ROS_CONFIG_FILE}")
  foreach(SOURCE_FILE IN LISTS MICRO_ROS_SOURCE_FILES)
    if(EXISTS "${SOURCE_FILE}" AND "${SOURCE_FILE}" IS_NEWER_THAN "${MICRO_ROS_CONFIG_FILE}")
      set(NEED_REBUILD TRUE)
      message(STATUS "Source file ${SOURCE_FILE} has been changed since last Micro-ROS build. Rebuild needed.")
      break()
    endif()
  endforeach()
else()
  set(NEED_REBUILD TRUE)
endif()

if(NEED_REBUILD)
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
  message(STATUS "Micro-ROS libraries and configuration are up to date. Skipping generation.")
endif()

add_custom_target(rebuild_micro_ros
  COMMAND ${MICRO_ROS_BUILD_COMMAND}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  USES_TERMINAL
  COMMENT "Force rebuilding Micro-ROS libraries and configuration"
)

list(APPEND CMAKE_PREFIX_PATH "${MICRO_ROS_OUTPUT_DIR}")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "Modified prefix path" FORCE)
