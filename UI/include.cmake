set(qtproject_SRCS
    EditServerDlg.cpp
    MainWindow.cpp
    SettingsDlg.cpp
    DataTree.cpp
    MediaUserDataWidget.cpp
    MediaWindow.cpp 
    PlayStateCompare.cpp
    UserInfoCompare.cpp
    UserDataWidget.cpp
    UserWindow.cpp
    TabPageBase.cpp
    TabUIInfo.cpp
)

set(qtproject_H
    EditServerDlg.h
    MainWindow.h
    SettingsDlg.h
    DataTree.h
    MediaUserDataWidget.h
    MediaWindow.h 
    PlayStateCompare.h
    UserInfoCompare.h
    UserWindow.h
    UserDataWidget.h
    TabPageBase.h
)

set(project_H
   TabUIInfo.h
)

set(qtproject_UIS
    EditServerDlg.ui
    MainWindow.ui
    SettingsDlg.ui
    DataTree.ui
    MediaUserDataWidget.ui
    MediaWindow.ui 
    PlayStateCompare.ui
    UserInfoCompare.ui
    UserDataWidget.ui
    UserWindow.ui
)


set(qtproject_QRC
    EmbySync.qrc
)

file(GLOB qtproject_QRC_SOURCES "resources/*")

set( project_pub_DEPS
    Qt5::Test
)
