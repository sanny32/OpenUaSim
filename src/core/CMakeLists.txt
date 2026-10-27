add_library(ouasim_core STATIC
    formatting.cpp
    formatting.h
    logexport.cpp
    logexport.h
    settingsstore.cpp
    settingsstore.h

    data/certificateinfo.h
    data/endpointinfo.h
    data/logentry.h
    data/namespaceinfo.h
    data/projectconfig.h
    data/serversnapshot.h
    data/serverstatus.h
    data/sessioninfo.h
    data/subscriptioninfo.h
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
    models/itemtreemodel.h
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
    models/subscriptionsmodel.cpp
    models/subscriptionsmodel.h
    models/treebuilders.cpp
    models/treebuilders.h
    models/treemodel.cpp
    models/treemodel.h
    models/variablelisting.cpp
    models/variablelisting.h
    models/variablesmodel.cpp
    models/variablesmodel.h

    pki/certificategenerator.cpp
    pki/certificategenerator.h
    pki/certificatestore.cpp
    pki/certificatestore.h
    pki/pkiservice.cpp
    pki/pkiservice.h

    project/alarmrules.cpp
    project/alarmrules.h
    project/defaultproject.cpp
    project/defaultproject.h
    project/portablenodeid.cpp
    project/portablenodeid.h
    project/projectdocument.cpp
    project/projectdocument.h
    project/projectfile.cpp
    project/projectfile.h
    project/projectmodels.cpp
    project/projectmodels.h
    project/projectnodes.cpp
    project/projectnodes.h
    project/serversettingsrules.cpp
    project/serversettingsrules.h
    project/useraccountrules.cpp
    project/useraccountrules.h

    server/addressspace.cpp
    server/addressspace.h
    server/declarednodes.cpp
    server/declarednodes.h
    server/historystore.cpp
    server/historystore.h
    server/importednodes.cpp
    server/importednodes.h
    server/listenersettings.cpp
    server/listenersettings.h
    server/logmapper.cpp
    server/logmapper.h
    server/modelvariables.cpp
    server/modelvariables.h
    server/nodesetloader.cpp
    server/nodesetloader.h
    server/servercontroller.cpp
    server/servercontroller.h
    server/serverlaunch.cpp
    server/serverlaunch.h
    server/sessiontracker.cpp
    server/sessiontracker.h
    server/simserver.cpp
    server/simserver.h
    server/snapshotbuilder.cpp
    server/snapshotbuilder.h
    server/typecatalog.cpp
    server/typecatalog.h
    server/variableproperties.cpp
    server/variableproperties.h

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
target_precompile_headers(ouasim_core PRIVATE corepch.h)

ouasim_link_quaserver(ouasim_core)
if(OUASIM_WITH_ENCRYPTION)
    find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)
    target_link_libraries(ouasim_core PRIVATE OpenSSL::Crypto)
endif()
