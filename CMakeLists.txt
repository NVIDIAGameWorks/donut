#
# Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.


project(donut)
cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set_property( GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()

string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")

include(CMakeDependentOption)

option(DONUT_WITH_NVRHI "Enable NVRHI and related projects" ON)

cmake_dependent_option(DONUT_WITH_DX11 "Enable the DX11 version of Donut" ON "WIN32" OFF)
cmake_dependent_option(DONUT_WITH_DX12 "Enable the DX12 version of Donut" ON "WIN32" OFF)
option(DONUT_WITH_VULKAN "Enable the Vulkan version of Donut" ON)
option(DONUT_WITH_AFTERMATH "Enable Aftermath crash dump generation with Donut" OFF)
option(DONUT_EMBED_SHADER_PDBS "Embed shader PDBs with shader binary files" OFF)

option(DONUT_WITH_STATIC_SHADERS "Build Donut with statically linked shaders" OFF)

option(DONUT_WITH_AUDIO "Include Audio features (XAudio2)" OFF)
option(DONUT_WITH_LZ4 "Include LZ4" ON)
option(DONUT_WITH_MINIZ "Include miniz (support for zip archives)" ON)
option(DONUT_WITH_TASKFLOW "Include TaskFlow" ON)
option(DONUT_WITH_TINYEXR "Include TinyEXR" ON)
option(DONUT_WITH_UNIT_TESTS "Donut unit-tests (see CMake/CTest documentation)" OFF)

option(DONUT_WITH_STREAMLINE "Enable streamline, separate package required" OFF)
set(DONUT_STREAMLINE_FETCH_URL "" CACHE STRING "Url to streamline git repo to fetch")
set(DONUT_STREAMLINE_FETCH_TAG "" CACHE STRING "Tag of streamline git repo")
set(DONUT_STREAMLINE_FETCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/streamline" CACHE STRING "Directory to fetch streamline to, empty uses build directory default")
set(DONUT_STREAMLINE_SEARCH_PATHS "" CACHE STRING "Search paths for streamline package")


if (WIN32)
    option(DONUT_FORCE_DISCRETE_GPU "Declare symbols to make the OS run the app on discrete GPU on laptops" OFF)
endif()

add_subdirectory(thirdparty)

if (NOT TARGET ShaderMake)
    option(SHADERMAKE_FIND_FXC "" "${DONUT_WITH_DX11}")
    option(SHADERMAKE_FIND_DXC "" "${DONUT_WITH_DX12}")
    option(SHADERMAKE_FIND_DXC_SPIRV "" "${DONUT_WITH_VULKAN}")

    add_subdirectory(ShaderMake)
endif()

if (DONUT_WITH_NVRHI)
    set(NVRHI_WITH_DX11 "${DONUT_WITH_DX11}" CACHE BOOL "" FORCE)
    set(NVRHI_WITH_DX12 "${DONUT_WITH_DX12}" CACHE BOOL "" FORCE)
    set(NVRHI_WITH_VULKAN "${DONUT_WITH_VULKAN}" CACHE BOOL "" FORCE)
    set(NVRHI_WITH_AFTERMATH "${DONUT_WITH_AFTERMATH}" CACHE BOOL "" FORCE)

    add_subdirectory(nvrhi)

    set(DONUT_SHADER_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include CACHE PATH "" FORCE)
    
    add_subdirectory(shaders)
endif()

include(donut-core.cmake)
if (DONUT_WITH_NVRHI)
    include(donut-engine.cmake)
    include(donut-render.cmake)
    include(donut-app.cmake)
endif()

if (DONUT_WITH_UNIT_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif()

if (DONUT_WITH_STREAMLINE)
    # Validate that CMAKE_RUNTIME_OUTPUT_DIRECTORY is set.
    # The Streamline CMake script uses it to copy DLLs, and it will fail at compile time with obscure messages
    # if the variable is empty.
    if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        message(SEND_ERROR "Streamline integration requires that CMAKE_RUNTIME_OUTPUT_DIRECTORY is non-empty.")
    endif()
    
    # XOR logic to make sure both options are set 
    if ((DONUT_STREAMLINE_FETCH_URL AND NOT DONUT_STREAMLINE_FETCH_TAG) OR 
        (NOT DONUT_STREAMLINE_FETCH_URL AND DONUT_STREAMLINE_FETCH_TAG))
        message(SEND_ERROR "Both DONUT_STREAMLINE_FETCH_URL and DONUT_STREAMLINE_FETCH_TAG must be defined together")
    endif()

    if (DONUT_STREAMLINE_FETCH_URL AND DONUT_STREAMLINE_FETCH_TAG)
        message(STATUS "Updating Streamline from " ${DONUT_STREAMLINE_FETCH_URL} ", tag " ${DONUT_STREAMLINE_FETCH_TAG} ", into folder " ${DONUT_STREAMLINE_FETCH_DIR} " - please wait!" )
        include(FetchContent)
        FetchContent_Declare(
            streamline
            GIT_REPOSITORY ${DONUT_STREAMLINE_FETCH_URL}
            GIT_TAG ${DONUT_STREAMLINE_FETCH_TAG}
            SOURCE_DIR ${DONUT_STREAMLINE_FETCH_DIR}
        )
        FetchContent_MakeAvailable(streamline)
    else ()
        if (DONUT_STREAMLINE_SEARCH_PATHS)
            set(STREAMLINE_SEARCH_PATHS ${DONUT_STREAMLINE_SEARCH_PATHS})
        endif()
        find_package(STREAMLINE REQUIRED)

        if (STREAMLINE_FOUND)
            add_subdirectory(${STREAMLINE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/streamline)
        endif()
    endif()
endif()

