set(qtproject_SRCS
    MainWindow.cpp
    SettingsDlg.cpp
    MediaUserDataWidget.cpp
)

set(qtproject_H
    MainWindow.h
    SettingsDlg.h
    MediaUserDataWidget.h
)

set(project_H
)

set(qtproject_UIS
    MainWindow.ui
    SettingsDlg.ui
    MediaUserDataWidget.ui
)


set(qtproject_QRC
    EmbySync.qrc
)

file(GLOB qtproject_QRC_SOURCES "resources/*")

set( project_pub_DEPS
)
