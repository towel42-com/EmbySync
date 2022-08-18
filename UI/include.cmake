set(qtproject_SRCS
    EditServerDlg.cpp
    MainWindow.cpp
    SettingsDlg.cpp
    MediaTree.cpp
    MediaUserDataWidget.cpp
    MediaWindow.cpp 
)

set(qtproject_H
    EditServerDlg.h
    MainWindow.h
    SettingsDlg.h
    MediaTree.h
    MediaUserDataWidget.h
    MediaWindow.h 
)

set(project_H
)

set(qtproject_UIS
    EditServerDlg.ui
    MainWindow.ui
    SettingsDlg.ui
    MediaTree.ui
    MediaUserDataWidget.ui
    MediaWindow.ui 
)


set(qtproject_QRC
    EmbySync.qrc
)

file(GLOB qtproject_QRC_SOURCES "resources/*")

set( project_pub_DEPS
    Qt5::Test
)
