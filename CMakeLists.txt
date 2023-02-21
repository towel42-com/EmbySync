# The MIT License (MIT)
#
# Copyright (c) 2022 Scott Aron Bloom
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.22)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/SABUtils/Modules ${CMAKE_MODULE_PATH})
IF(WIN32)
    find_package(VerifyVisualStudio REQUIRED)
    VerifyVisualStudio( _ )
    IF ( NOT _VISUAL_STUDIO_FOUND )
        MESSAGE( FATAL_ERROR "Could not find Visual Studio compiler/linker" )
    ENDIF()
ENDIF()
project(root-EmbySync) 

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)
find_package(Threads)
find_package(Qt5 COMPONENTS Core Widgets Network Test Multimedia REQUIRED)
find_package(Deploy REQUIRED)
find_package(CheckOpenSSL REQUIRED)
find_package(AddUnitTest REQUIRED)

include( ${CMAKE_SOURCE_DIR}/VersionInfo.cmake )

CheckOpenSSL()

option( gtest_force_shared_crt "Use shared ( DLL ) run-time lib even when Google Test is built as static lib." ON ) 

set_property( GLOBAL PROPERTY USE_FOLDERS ON )

file( REAL_PATH ~/bin/EmbySync CMAKE_INSTALL_PREFIX EXPAND_TILDE)
SET( SAB_ENABLE_TESTING ON )
add_subdirectory( SABUtils )
add_subdirectory( UI )
add_subdirectory( Core )
add_subdirectory( gui )
add_subdirectory( cli )

INSTALL( FILES License.txt DESTINATION . )
INSTALL( FILES ReleaseNotes.txt DESTINATION . )

include( ${CMAKE_SOURCE_DIR}/InstallerInfo.cmake )
include( CPack )
