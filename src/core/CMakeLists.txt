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

    pki/certificatestore.cpp
    pki/certificatestore.h

    project/defaultproject.cpp
    project/defaultproject.h
    project/projectdocument.cpp
    project/projectdocument.h
    project/projectfile.cpp
    project/projectfile.h
    project/projectnodes.cpp
    project/projectnodes.h

    server/addressspace.cpp
    server/addressspace.h
    server/listenersettings.cpp
    server/listenersettings.h
    server/logmapper.cpp
    server/logmapper.h
    server/servercontroller.cpp
    server/servercontroller.h
    server/sessiontracker.cpp
    server/sessiontracker.h
    server/simserver.cpp
    server/simserver.h
    server/snapshotbuilder.cpp
    server/snapshotbuilder.h
    server/typecatalog.cpp
    server/typecatalog.h

    simulation/signalgenerator.cpp
    simulation/signalgenerator.h
    simulation/simulationengine.cpp
    simulation/simulationengine.h
    simulation/valuecast.cpp
    simulation/valuecast.h
    simulation/valuesink.h
)

target_include_directories(ouasim_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/models
)

target_link_libraries(ouasim_core PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
)
target_link_libraries(ouasim_core PRIVATE Qt${QT_VERSION_MAJOR}::Network)

ouasim_link_quaserver(ouasim_core)
