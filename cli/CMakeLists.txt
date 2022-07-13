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
 
project(EmbySyncCLI) 

include( include.cmake )
include( ${CMAKE_SOURCE_DIR}/Project.cmake )
include_directories( ${CMAKE_BINARY_DIR} )

add_executable( ${PROJECT_NAME}
                ${_PROJECT_DEPENDENCIES} 
                ${_CMAKE_MODULE_FILES}
                application.rc 
          )
if ( NOT DEPLOYQT_EXECUTABLE )
    message( FATAL_ERROR "DEPLOYQT_EXECUTABLE not set" )
endif()

get_filename_component( QTDIR ${DEPLOYQT_EXECUTABLE} DIRECTORY )

set ( DEBUG_PATH 
        "%PATH%"
        "$<TARGET_FILE_DIR:SABUtils>"
        "${QTDIR}"
        )

set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER Apps 
                                    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>" 
                                    VS_DEBUGGER_COMMAND "$<TARGET_FILE:${PROJECT_NAME}>" 
                                    VS_DEBUGGER_ENVIRONMENT "PATH=${DEBUG_PATH}\nQT_PLUGIN_PATH=${DEBUG_PLUGINPATH}" 
                     )

target_link_libraries( ${PROJECT_NAME}
    PUBLIC
        ${project_pub_DEPS}
    PRIVATE 
        ${project_pri_DEPS}
)

DeployQt( ${PROJECT_NAME} . INSTALL_ONLY 1 )
DeploySystem( ${PROJECT_NAME} . INSTALL_ONLY 1 )

INSTALL( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . )
INSTALL( FILES ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${PROJECT_NAME}.pdb DESTINATION . CONFIGURATIONS Debug RelWithDebInfo )

