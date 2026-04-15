set(FOLDER_NAME Apps)

set(qtproject_SRCS
    main.cpp    
)

set(project_SRCS
    MainObj.cpp
)

set(qtproject_H
    MainObj.h
)

set(project_H
)

set(qtproject_UIS
)


set(qtproject_QRC
)

set( project_pub_DEPS
    Towel42Utils
    Core
    ${project_pub_DEPS}
)

set( project_pri_DEPS
    ${project_pri_DEPS}
)
