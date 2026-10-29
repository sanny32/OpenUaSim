set(UI_FILES
    mainwindow.ui
    dialogs/opcuainfodialog.ui
    dialogs/aboutdialog.ui

    dialogs/alarmdialog.ui
    dialogs/methodsourcedialog.ui
    dialogs/messagedialog.ui
    dialogs/addnodedialog.ui
    dialogs/serversettingsdialog.ui
    dialogs/subscriptionsdialog.ui
    dialogs/usersdialog.ui
    dialogs/valuedialog.ui

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
    widgets/methodpropertieswidget.ui
    widgets/propertynodewidget.ui
    widgets/variablepropertieswidget.ui
)

add_library(ouasim_ui STATIC
    appcolors.cpp
    appcolors.h
    appicons.cpp
    appicons.h
    applanguage.cpp
    applanguage.h
    application.cpp
    application.h
    apptheme.cpp
    apptheme.h
    certificateactions.cpp
    certificateactions.h
    modelactions.cpp
    modelactions.h

    dialogs/appbasedialog.cpp
    dialogs/appbasedialog.h
    dialogs/opcuainfodialog.cpp
    dialogs/opcuainfodialog.h
    dialogs/aboutdialog.cpp
    dialogs/aboutdialog.h
    dialogs/renamenodedialog.cpp
    dialogs/renamenodedialog.h
    dialogs/renamenodedialog.ui
    dialogs/alarmdialog.cpp
    dialogs/alarmdialog.h
    dialogs/methodsourcedialog.cpp
    dialogs/methodsourcedialog.h
    dialogs/messagedialog.cpp
    dialogs/messagedialog.h
    dialogs/addnodedialog.cpp
    dialogs/addnodedialog.h
    dialogs/serversettingsdialog.cpp
    dialogs/serversettingsdialog.h
    dialogs/subscriptionsdialog.cpp
    dialogs/subscriptionsdialog.h
    dialogs/usersdialog.cpp
    dialogs/usersdialog.h
    dialogs/valuedialog.cpp
    dialogs/valuedialog.h

    mainwindow.cpp
    mainwindow.h
    widgetlayout.cpp
    widgetlayout.h
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

    widgets/elidedlabel.cpp
    widgets/elidedlabel.h
    widgets/elidedcombobox.cpp
    widgets/elidedcombobox.h
    widgets/texttooltip.cpp
    widgets/texttooltip.h
    widgets/elidedlineedit.cpp
    widgets/elidedlineedit.h
    widgets/eventlogwidget.cpp
    widgets/eventlogwidget.h
    widgets/itemdelegate.cpp
    widgets/itemdelegate.h
    widgets/mainstatusbarwidget.cpp
    widgets/mainstatusbarwidget.h
    widgets/methodpropertieswidget.cpp
    widgets/methodpropertieswidget.h
    widgets/propertynodewidget.cpp
    widgets/propertynodewidget.h
    widgets/statusindicator.cpp
    widgets/statusindicator.h
    widgets/viewsetup.cpp
    widgets/viewsetup.h
    widgets/valuepreviewdelegate.cpp
    widgets/valuepreviewdelegate.h
    widgets/variablepropertieswidget.cpp
    widgets/variablepropertieswidget.h
)

if(APPLE)
    target_sources(ouasim_ui PRIVATE dialogs/macwindowbuttons.h dialogs/macwindowbuttons.mm)
    set_source_files_properties(dialogs/macwindowbuttons.mm PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    target_link_libraries(ouasim_ui PRIVATE "-framework AppKit")
endif()

set_target_properties(ouasim_ui PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/dialogs;${CMAKE_CURRENT_SOURCE_DIR}/pages;${CMAKE_CURRENT_SOURCE_DIR}/widgets"
)

target_include_directories(ouasim_ui PRIVATE "${PROJECT_BINARY_DIR}/generated")

target_include_directories(ouasim_ui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/pages
    ${CMAKE_CURRENT_SOURCE_DIR}/style
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)

target_link_libraries(ouasim_ui PUBLIC
    ouasim_core
    Qt${QT_VERSION_MAJOR}::Widgets
)
target_link_libraries(ouasim_ui PRIVATE Qt${QT_VERSION_MAJOR}::Svg Qt${QT_VERSION_MAJOR}::Network)
target_precompile_headers(ouasim_ui PRIVATE uipch.h)

set(OUASIM_TS_FILES "${PROJECT_SOURCE_DIR}/translations/openuasim_ru.ts")
set(OUASIM_LUPDATE_OPTIONS -no-obsolete -locations none)
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.7)
    qt_add_translations(ouasim_ui
        TS_FILES ${OUASIM_TS_FILES}
        SOURCE_TARGETS ouasim_core ouasim_ui
        RESOURCE_PREFIX /i18n
        LUPDATE_OPTIONS ${OUASIM_LUPDATE_OPTIONS}
    )
else()
    # SOURCE_TARGETS arrived in Qt 6.7; older versions take the sources to scan directly.
    file(GLOB_RECURSE OUASIM_TRANSLATED_SOURCES CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/core/*.cpp" "${PROJECT_SOURCE_DIR}/core/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.ui")
    qt_add_translations(ouasim_ui
        TS_FILES ${OUASIM_TS_FILES}
        SOURCES ${OUASIM_TRANSLATED_SOURCES}
        RESOURCE_PREFIX /i18n
        LUPDATE_OPTIONS ${OUASIM_LUPDATE_OPTIONS}
    )
endif()
