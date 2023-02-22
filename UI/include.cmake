set(_PROJECT_NAME UI)
set(USE_QT TRUE)
set(FOLDER_NAME Libs)

set(qtproject_SRCS
    CollectionsManager.cpp
    EditServerDlg.cpp
    MainWindow.cpp
    SettingsDlg.cpp
    DataTree.cpp
    MediaDataWidget.cpp
    MediaWindow.cpp 
    MissingEpisodes.cpp
    MissingTVDBid.cpp
    MissingMovies.cpp
    PlayStateCompare.cpp
    UserInfoCompare.cpp
    UserDataWidget.cpp
    UserWindow.cpp
    TabPageBase.cpp
    TabUIInfo.cpp
)

set(qtproject_H
    CollectionsManager.h
    EditServerDlg.h
    MainWindow.h
    SettingsDlg.h
    DataTree.h
    MediaDataWidget.h
    MediaWindow.h 
    MissingEpisodes.h
    MissingTVDBid.h
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
    CollectionsManager.ui
    EditServerDlg.ui
    MainWindow.ui
    SettingsDlg.ui
    DataTree.ui
    MediaDataWidget.ui
    MediaWindow.ui 
    MissingEpisodes.ui
    MissingTVDBid.ui
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
