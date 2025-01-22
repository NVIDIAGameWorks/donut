#
# Copyright (c) 2024, NVIDIA CORPORATION. All rights reserved.
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


find_package(PackageHandleStandardArgs)

if (NOT STREAMLINE_SEARCH_PATHS)
    set (STREAMLINE_SEARCH_PATHS
        "${CMAKE_SOURCE_DIR}/streamline"
        "${CMAKE_PROJECT_DIR}/streamline")
endif()

find_file(STREAMLINE_CMAKE_FILE CMakeLists.txt
    PATHS ${STREAMLINE_SEARCH_PATHS}
    NO_DEFAULT_PATH
)

if (STREAMLINE_CMAKE_FILE)
    get_filename_component(STREAMLINE_SOURCE_DIR "${STREAMLINE_CMAKE_FILE}" DIRECTORY)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(STREAMLINE
    REQUIRED_VARS
        STREAMLINE_SOURCE_DIR
)

