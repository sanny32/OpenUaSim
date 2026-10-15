set(UI_FILES
    mainwindow.ui

    pages/certificatespage.ui
    pages/endpointspage.ui
    pages/logpage.ui
    pages/namespacespage.ui
    pages/objectspage.ui
    pages/sessionspage.ui
    pages/statuspage.ui
    pages/typespage.ui

    widgets/eventlogwidget.ui
    widgets/mainstatusbarwidget.ui
    widgets/statcardwidget.ui
)

add_library(ouasim_ui STATIC
    appcolors.cpp
    appcolors.h
    appicons.cpp
    appicons.h
    application.cpp
    application.h
    apptheme.cpp
    apptheme.h
    mainwindow.cpp
    mainwindow.h
    ${UI_FILES}

    pages/certificatespage.cpp
    pages/certificatespage.h
    pages/endpointspage.cpp
    pages/endpointspage.h
    pages/logpage.cpp
    pages/logpage.h
    pages/namespacespage.cpp
    pages/namespacespage.h
    pages/objectspage.cpp
    pages/objectspage.h
    pages/sessionspage.cpp
    pages/sessionspage.h
    pages/statuspage.cpp
    pages/statuspage.h
    pages/typespage.cpp
    pages/typespage.h

    style/apppalette.cpp
    style/apppalette.h
    style/appstyle.cpp
    style/appstyle.h

    widgets/eventlogwidget.cpp
    widgets/eventlogwidget.h
    widgets/itemdelegate.cpp
    widgets/itemdelegate.h
    widgets/mainstatusbarwidget.cpp
    widgets/mainstatusbarwidget.h
    widgets/statcardwidget.cpp
    widgets/statcardwidget.h
    widgets/statusindicator.cpp
    widgets/statusindicator.h
    widgets/viewsetup.cpp
    widgets/viewsetup.h
)

set_target_properties(ouasim_ui PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/pages;${CMAKE_CURRENT_SOURCE_DIR}/widgets"
)

target_include_directories(ouasim_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/pages
    ${CMAKE_CURRENT_SOURCE_DIR}/style
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)

target_link_libraries(ouasim_ui PUBLIC
    ouasim_core
    Qt${QT_VERSION_MAJOR}::Widgets
)
target_link_libraries(ouasim_ui PRIVATE Qt${QT_VERSION_MAJOR}::Svg)
