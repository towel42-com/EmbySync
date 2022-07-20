set(qtproject_SRCS
    MainWindow.cpp
    SettingsDlg.cpp
    MediaUserDataWidget.cpp
    UserDataDlg.cpp
)

set(qtproject_H
    MainWindow.h
    SettingsDlg.h
    MediaUserDataWidget.h
    UserDataDlg.h
)

set(project_H
)

set(qtproject_UIS
    MainWindow.ui
    SettingsDlg.ui
    UserDataDlg.ui
    MediaUserDataWidget.ui
)


set(qtproject_QRC
    EmbySync.qrc
)

file(GLOB qtproject_QRC_SOURCES "resources/*")

set( project_pub_DEPS
)
