set(qtproject_SRCS
    CreateCollections.cpp
    EditServerDlg.cpp
    MainWindow.cpp
    SettingsDlg.cpp
    DataTree.cpp
    MediaDataWidget.cpp
    MediaWindow.cpp 
    MissingEpisodes.cpp
    MissingMovies.cpp
    PlayStateCompare.cpp
    UserInfoCompare.cpp
    UserDataWidget.cpp
    UserWindow.cpp
    TabPageBase.cpp
    TabUIInfo.cpp
)

set(qtproject_H
    CreateCollections.h
    EditServerDlg.h
    MainWindow.h
    SettingsDlg.h
    DataTree.h
    MediaDataWidget.h
    MediaWindow.h 
    MissingEpisodes.h
    MissingMovies.h
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
    CreateCollections.ui
    EditServerDlg.ui
    MainWindow.ui
    SettingsDlg.ui
    DataTree.ui
    MediaDataWidget.ui
    MediaWindow.ui 
    MissingEpisodes.ui
    MissingMovies.ui
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
