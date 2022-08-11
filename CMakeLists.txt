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

cmake_minimum_required(VERSION 3.21)
if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

project(root-EmbySync) 

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/SABUtils/Modules ${CMAKE_MODULE_PATH})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)
find_package(Threads)
find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)
find_package(Deploy REQUIRED)
find_package(CheckOpenSSL REQUIRED)
find_package(AddUnitTest REQUIRED)

include( VersionInfo.cmake )

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

SET( CPACK_PACKAGE_VERSION_MAJOR ${MAJOR_VERSION} )
SET( CPACK_PACKAGE_VERSION_MINOR ${MINOR_VERSION} )
SET( CPACK_PACKAGE_VERSION_PATCH ${VERSION_FILE_PATCH_VERSION} )
SET( CPACK_PACKAGE_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${VERSION_FILE_PATCH_VERSION})
SET( CPACK_PACKAGE_NAME ${APP_NAME} )
SET( CPACK_PACKAGE_VENDOR "${VENDOR} ${EMAIL}" )
SET( CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/License.txt )
SET( CPACK_PACKAGE_HOMEPAGE_URL ${HOMEPAGE} )
set( CPACK_PACKAGE_INSTALL_REGISTRY_KEY "${VENDOR}/${APP_NAME}" )
SET( CPACK_PACKAGE_DESCRIPTION "Application to help manage your multiple Emby Servers" )

SET(CPACK_NSIS_EXECUTABLES_DIRECTORY "." )
SET(CPACK_PACKAGE_EXECUTABLES 
    "EmbySyncCLI" "EmbySync CLI"
    "EmbySyncGUI" "EmbySync"
)
SET(CPACK_CREATE_DESKTOP_LINKS
    "EmbySyncGUI"
)
    
SET(CPACK_NSIS_MODIFY_PATH OFF)
SET( CPACK_GENERATOR ZIP NSIS )
SET( CPACK_NSIS_MUI_ICON ${CMAKE_SOURCE_DIR}/UI/resources/embysync.ico )
SET( CPACK_NSIS_MUI_UNIICON ${CMAKE_SOURCE_DIR}/UI/resources/embysync.ico )
include( CPack )


