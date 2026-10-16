add_library(ouasim_core STATIC
    formatting.cpp
    formatting.h

    data/certificateinfo.h
    data/endpointinfo.h
    data/logentry.h
    data/namespaceinfo.h
    data/projectconfig.h
    data/serversnapshot.h
    data/serverstatus.h
    data/sessioninfo.h
    data/statuskind.h
    data/treenode.h
    data/typeinfo.h
    data/variableinfo.h

    mock/mockdata.cpp
    mock/mockdata.h

    models/certificatesmodel.cpp
    models/certificatesmodel.h
    models/endpointsmodel.cpp
    models/endpointsmodel.h
    models/itemroles.h
    models/itemtablemodel.h
    models/logfilterproxymodel.cpp
    models/logfilterproxymodel.h
    models/logmodel.cpp
    models/logmodel.h
    models/namespacesmodel.cpp
    models/namespacesmodel.h
    models/referencesmodel.cpp
    models/referencesmodel.h
    models/sessionsmodel.cpp
    models/sessionsmodel.h
    models/subtypesmodel.cpp
    models/subtypesmodel.h
    models/treebuilders.cpp
    models/treebuilders.h
    models/treemodel.cpp
    models/treemodel.h
    models/variablesmodel.cpp
    models/variablesmodel.h

    project/defaultproject.cpp
    project/defaultproject.h
    project/projectfile.cpp
    project/projectfile.h

    simulation/signalgenerator.cpp
    simulation/signalgenerator.h
    simulation/valuecast.cpp
    simulation/valuecast.h
)

target_include_directories(ouasim_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/models
)

target_link_libraries(ouasim_core PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
)

ouasim_link_quaserver(ouasim_core)
