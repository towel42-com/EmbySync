#
# (c) Copyright 2004 - 2019 Blue Pearl Software Inc.
# All rights reserved.
#
# This source code belongs to Blue Pearl Software Inc.
# It is considered trade secret and confidential, and is not to be used
# by parties who have not received written authorization
# from Blue Pearl Software Inc.
#
# Only authorized users are allowed to use, copy and modify
# this software provided that the above copyright notice
# remains in all copies of this software.
#
#
# $Author: scott $ - $Revision: 56729 $ - $Date: 2019-12-24 19:28:25 -0800 (Tue, 24 Dec 2019) $
# $HeadURL: http://bpsvn/svn/trunk/Shared/CheckForLatestVersion/CMakeLists.txt $
#
#
#

project( SHARED_CheckForLatestVersion )

SET( QT_DONT_USE_QTGUI FALSE )
SET( QT_USE_QTNETWORK TRUE )
SET( ADD_QT_KEYWORDS TRUE )

include(include.cmake)
include(${SHARED_SOURCE_DIR}/Project.cmake)
SET( QT_LIBRARIES Qt5::Core;Qt5::Sql )
QT5_WRAP_UI(qtproject_UIS_H ${qtproject_UIS})
QT5_WRAP_CPP(qtproject_MOC_SRCS ${qtproject_H})
AddQtIncludes( Network )

source_group("Generated Files" FILES ${qtproject_UIS_H} ${qtproject_MOC_SRCS} ${qtproject_QRC_SRCS} )
source_group("Resource Files" FILES ${qtproject_QRC} ${qtproject_QRC_SOURCES} )
source_group("Designer Files" FILES ${qtproject_UIS} )
source_group("Header Files" FILES ${qtproject_H} )
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(
    ${PROJECT_NAME} STATIC
    ${qtproject_SRCS} 
    ${qtproject_QRC_SRCS} 
    ${qtproject_UIS_H} 
    ${qtproject_MOC_SRCS} 
    ${qtproject_H} 
    ${project_H} 
    ) 
set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER Shared )
add_header_test(${PROJECT_NAME} ${project_H} ${qtproject_H})
IF( BPS_RUN_PCLINT )
    add_pc_lint( ${PROJECT_NAME} ${qtproject_SRCS})
ENDIF()

ADD_TO_TARGET()
