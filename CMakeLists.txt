cmake_minimum_required(VERSION 3.16)


# Set root directory, if needed
if (NOT TGL_ROOT_DIR)
    set(TGL_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
endif()

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
# Set the name of the project
set(target_name TGLDemo)
project(${target_name} C CXX)


# Use C99 and C++20
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Include helper scripts
include(${TGL_ROOT_DIR}/infra/utils.cmake)
custom_set_build_type()


# Enable Link-Time Optimization for optimized (non-Debug) configs. This
# matters specifically because a lot of tgl/'s small accessors (e.g.
# EntityInstance::GetScene()/GetSceneData()) are declared in one header but
# *defined* out-of-line in a .cpp - per-translation-unit compilation can't
# inline through that boundary or see it's safe to merge repeated calls;
# LTO is what makes those bodies visible across TUs at optimization time.
include(CheckIPOSupported)
check_ipo_supported(RESULT TGL_IPO_SUPPORTED OUTPUT TGL_IPO_ERROR)
if (NOT TGL_IPO_SUPPORTED)
    message(WARNING "LTO/IPO not supported by this toolchain: ${TGL_IPO_ERROR}")
endif()


# Fix to hide a bug in CMake. Hopefully able to be removed in the future.
set(CMAKE_POLICY_DEFAULT_CMP0012 NEW)


# Find required packages
find_package(OpenGL REQUIRED)
if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
    find_package(GLEW REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(GLFW REQUIRED glfw3)
    find_package(assimp REQUIRED)
    find_package(spdlog REQUIRED)
    find_package(Freetype REQUIRED)
endif()


# Set RPATH to avoid using LD_LIBRARY_PATH
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_INSTALL_RPATH "@executable_path")
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

# Gather the source files.
#
# `framework/` is the small OpenGL/window/resource layer this project is
# built on top of (adapted from the teaching framework of the Computer
# Graphics Department of the Polytechnic University of Bucharest).
#
# `tgl/` is the actual entity-component-system game engine, and is the
# part of this repository worth looking at.
#
# `app/` is a minimal demo that boots the engine with an empty scene.
file(GLOB_RECURSE TGL_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/framework/components/*.c*
    ${CMAKE_CURRENT_LIST_DIR}/framework/core/*.c*
    ${CMAKE_CURRENT_LIST_DIR}/framework/utils/*.c*
    #
    ${CMAKE_CURRENT_LIST_DIR}/tgl/*.c*
    #
    ${CMAKE_CURRENT_LIST_DIR}/app/*.c*
)


# Gather the header files
file(GLOB_RECURSE TGL_HEADERS_PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/framework/components/*.h*
    ${CMAKE_CURRENT_LIST_DIR}/framework/core/*.h*
    ${CMAKE_CURRENT_LIST_DIR}/framework/utils/*.h*
    ${CMAKE_CURRENT_LIST_DIR}/tgl/*.h*
    ${CMAKE_CURRENT_LIST_DIR}/app/*.h*
    #
    ${CMAKE_CURRENT_LIST_DIR}/assets/shaders/*.glsl
)


# Gather the include directories.
#
# `framework` resolves includes like "core/engine.h" and "utils/gl_utils.h";
# the root itself resolves includes like "tgl/entity.hpp" and "app/empty_scene.hpp".
set(TGL_INCLUDE_DIRS_PRIVATE
    ${TGL_ROOT_DIR}/deps/api
    ${TGL_ROOT_DIR}/framework
    ${TGL_ROOT_DIR}
)


# Add the executable
custom_add_executable(${target_name}
    ${TGL_SOURCES}
    ${TGL_HEADERS_PRIVATE}
)
if (TGL_IPO_SUPPORTED)
    set_target_properties(${target_name} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
        INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
endif()


# Detect architecture
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch")
        set(__cmake_arch arm64)
    else()
        set(__cmake_arch x86_64)
    endif()
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch")
        set(__cmake_arch armv7)
    else()
        set(__cmake_arch i686)
    endif()
endif()


# Link third-party libraries
target_link_libraries(${target_name} PRIVATE
    ${OPENGL_LIBRARIES}
)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(${target_name} PRIVATE
        ${TGL_ROOT_DIR}/deps/prebuilt/GL/${__cmake_arch}/glew32.lib
        ${TGL_ROOT_DIR}/deps/prebuilt/GLFW/${__cmake_arch}/glfw3dll.lib
        ${TGL_ROOT_DIR}/deps/prebuilt/assimp/${__cmake_arch}/assimp.lib
        ${TGL_ROOT_DIR}/deps/prebuilt/spdlog/${__cmake_arch}/spdlog.lib
        ${TGL_ROOT_DIR}/deps/prebuilt/freetype/${__cmake_arch}/freetype.lib
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(${target_name} PRIVATE
        GLEW
        glfw
        assimp
        spdlog
        freetype
    )
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Fix linking on 10.14+. See https://stackoverflow.com/questions/54068035
    target_link_directories(${target_name} PRIVATE
        /usr/local/lib
    )

    if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch")
        target_link_directories(${target_name} PRIVATE
        /opt/homebrew/lib
    )
    endif ()

    target_link_libraries(${target_name} PRIVATE
        GLEW
        glfw
        assimp
        spdlog
        freetype
    )
endif()


# Set target properties
target_include_directories(${target_name} PRIVATE ${TGL_INCLUDE_DIRS_PRIVATE})


# For Visual Studio, set the working directory and the startup project
if (MSVC)
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT ${target_name})
    set_property(TARGET ${target_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()


# Set source file properties
set_source_files_properties(${TGL_HEADERS_PRIVATE} PROPERTIES HEADER_FILE_ONLY TRUE)


# Group files under logical folders (mainly for IDEs)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES
    ${TGL_SOURCES}
    ${TGL_HEADERS_PRIVATE}
)


# Add definitions (specific to this project)
# Recommended reading: so@a/24470998/5922876, so@a/11437693/5922876
set(TGL_CXX_DEFS   LIBGFXC_EXPORTS GLM_FORCE_SILENT_WARNINGS _CRT_SECURE_NO_WARNINGS SOLVED $<$<CONFIG:Debug>:DEBUG>)
target_compile_definitions(${target_name} PRIVATE ${TGL_CXX_DEFS})


# Add compile options (specific to this project)
# Recommended reading: so@a/23995391/5922876
if (MSVC)
    set(TGL_CXX_FLAGS  /W4 /WX-)
    set(TGL_CXX_FLAGS  ${TGL_CXX_FLAGS} /wd4100 /wd4458 /wd4189)
else()
    set(TGL_CXX_FLAGS  -Wall -Wextra -pedantic -Wno-error)
    if (CMAKE_C_COMPILER_ID MATCHES "GNU")
        set(TGL_CXX_FLAGS  ${TGL_CXX_FLAGS}   -Wno-unused-parameter -Wno-unused-variable
                                                -Wno-unused-but-set-variable
                                                -Wno-missing-field-initializers -Wno-sign-compare)
    elseif (CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(TGL_CXX_FLAGS  ${TGL_CXX_FLAGS}   -Wno-unused-parameter -Wno-unused-variable
                                                -Wno-missing-field-initializers -Wno-sign-compare
                                                # Apple clang
                                                -Wno-unknown-warning-option
                                                # Windows clang
                                                -Wno-microsoft-enum-value -Wno-language-extension-token)
    endif()
endif()
target_compile_options(${target_name} PRIVATE ${TGL_CXX_FLAGS})


# Post-build events. First, we get the directory where the target was
# just built. We will then copy several files and create several symlinks
# into the target's parent directory.
get_target_property(__target_dir ${target_name} RUNTIME_OUTPUT_DIRECTORY)

# On Windows, we copy required binary files next to the target.
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${TGL_ROOT_DIR}/deps/prebuilt/GL/${__cmake_arch}/glew32.dll"
            "${TGL_ROOT_DIR}/deps/prebuilt/GLFW/${__cmake_arch}/glfw3.dll"
            "${TGL_ROOT_DIR}/deps/prebuilt/assimp/${__cmake_arch}/assimp.dll"
            "${TGL_ROOT_DIR}/deps/prebuilt/spdlog/${__cmake_arch}/spdlog.dll"
            "${TGL_ROOT_DIR}/deps/prebuilt/freetype/${__cmake_arch}/freetype.dll"

            # Destination directory
            "${__target_dir}"
    )
endif()

# This bit is necessary to allow building via Clang on Windows
if (NOT MSVC)
    set(__error_sink || rem)
endif()

# We create symbolic links of several directories next to the target.
foreach (dir IN ITEMS "assets")
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        string(REPLACE "/" "\\" __target_dir "${__target_dir}")
        string(REPLACE "/" "\\" TGL_ROOT_DIR "${TGL_ROOT_DIR}")

        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND rmdir /S /Q "${__target_dir}\\${dir}" ${__error_sink}
            COMMAND mklink /J "${__target_dir}\\${dir}" "${TGL_ROOT_DIR}\\${dir}" ${__error_sink}
        )
    else()
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND rm -rf "${__target_dir}/${dir}"
            COMMAND ln -s "${TGL_ROOT_DIR}/${dir}" "${__target_dir}/${dir}"
        )
    endif()
endforeach()


# ---------------------------------------------------------------------------
# Tests and benchmarks
#
# These are separate, small executables that only compile the tgl/ sources
# they actually need (currently just component_data.cpp) instead of reusing
# TGL_SOURCES, so they build fast and don't require a live window/OpenGL
# context. Note tgl/component_data.hpp's ForEach: it constructs a
# tgl::Archetype<T>, whose constructor validates through
# EntityInstance::GetScene() -> GameManager::Get() -> World -> a real
# window, so ForEach isn't exercised here - only Add/Remove/Get/size, which
# don't need that.
# ---------------------------------------------------------------------------
option(TGL_BUILD_TESTS "Build the tgl unit tests" ON)
option(TGL_BUILD_BENCHMARKS "Build the tgl benchmarks" ON)

if (TGL_BUILD_TESTS)
    add_executable(tgl_tests
        ${TGL_ROOT_DIR}/tgl/component_data.cpp
        ${TGL_ROOT_DIR}/tgl/entity.cpp
        ${TGL_ROOT_DIR}/tgl/scene.cpp
        ${TGL_ROOT_DIR}/tgl/scene_manager.cpp
        ${TGL_ROOT_DIR}/tests/component_data_tests.cpp
        ${TGL_ROOT_DIR}/tests/scene_entity_tests.cpp
    )
    target_include_directories(tgl_tests PRIVATE ${TGL_INCLUDE_DIRS_PRIVATE})
    target_compile_definitions(tgl_tests PRIVATE ${TGL_CXX_DEFS})
    if (TGL_IPO_SUPPORTED)
        set_target_properties(tgl_tests PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
            INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    endif()

    enable_testing()
    add_test(NAME tgl_tests COMMAND tgl_tests)
endif()

if (TGL_BUILD_BENCHMARKS)
    add_executable(tgl_bench
        ${TGL_ROOT_DIR}/tgl/component_data.cpp
        ${TGL_ROOT_DIR}/tgl/entity.cpp
        ${TGL_ROOT_DIR}/tgl/scene.cpp
        ${TGL_ROOT_DIR}/tgl/scene_manager.cpp
        ${TGL_ROOT_DIR}/bench/component_data_bench.cpp
    )
    target_include_directories(tgl_bench PRIVATE ${TGL_INCLUDE_DIRS_PRIVATE})
    target_compile_definitions(tgl_bench PRIVATE ${TGL_CXX_DEFS})
    if (TGL_IPO_SUPPORTED)
        set_target_properties(tgl_bench PROPERTIES
            INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
            INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    endif()
endif()
