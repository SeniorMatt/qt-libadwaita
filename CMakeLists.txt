cmake_minimum_required(VERSION 3.16)
project(qt-libadwaita VERSION 0.1.0 LANGUAGES CXX)

set(QT_MAJOR 6 CACHE STRING "Qt major version to build against (5 or 6)")
option(ADWAITA_BUILD_GALLERY "Build the adwaita-gallery widget gallery" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(Qt${QT_MAJOR} REQUIRED COMPONENTS Widgets DBus)

include(GNUInstallDirs)
# Qt looks for style plugins in <plugin path>/styles. Default: <prefix>/<libdir>/qt<N>/plugins
set(ADWAITA_PLUGIN_DIR "${CMAKE_INSTALL_LIBDIR}/qt${QT_MAJOR}/plugins/styles"
    CACHE STRING "Style plugin install dir (relative to the prefix)")
# Absolute override (KDE_INSTALL_QTPLUGINDIR style), e.g. for a Flatpak extension:
#   -DQT_PLUGIN_INSTALL_DIR=/app/lib/plugins/styles
set(QT_PLUGIN_INSTALL_DIR "" CACHE PATH "Absolute style plugin install dir; overrides ADWAITA_PLUGIN_DIR")
if(QT_PLUGIN_INSTALL_DIR)
    set(ADWAITA_PLUGIN_DEST "${QT_PLUGIN_INSTALL_DIR}")
else()
    set(ADWAITA_PLUGIN_DEST "${ADWAITA_PLUGIN_DIR}")
endif()

# Regenerate src/adwaitatokens_data.h from the active libadwaita token file. data/current names it (one line, e.g.
# "adwaita-1.10.json"); switch versions by editing that file (tools/update_libadwaita.py --apply does it).
find_package(Python3 QUIET COMPONENTS Interpreter)
set(ADWAITA_TOKENS_JSON "")
if(EXISTS "${CMAKE_SOURCE_DIR}/data/current")
    file(STRINGS "${CMAKE_SOURCE_DIR}/data/current" _adw_current LIMIT_COUNT 1)
    set(ADWAITA_TOKENS_JSON "${CMAKE_SOURCE_DIR}/data/${_adw_current}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/data/current")
endif()
if(Python3_FOUND AND ADWAITA_TOKENS_JSON AND EXISTS "${ADWAITA_TOKENS_JSON}")
    add_custom_command(
        OUTPUT "${CMAKE_SOURCE_DIR}/src/adwaitatokens_data.h"
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/gen_tokens.py"
        DEPENDS "${ADWAITA_TOKENS_JSON}" "${CMAKE_SOURCE_DIR}/data/current"
                "${CMAKE_SOURCE_DIR}/tools/gen_tokens.py" "${CMAKE_SOURCE_DIR}/tools/tokensource.py"
        COMMENT "Generating adwaitatokens_data.h from ${_adw_current}")
    set(TOKENS_DATA "${CMAKE_SOURCE_DIR}/src/adwaitatokens_data.h")
endif()

add_library(adwaita MODULE
    src/adwaitasettings.cpp src/adwaitasettings.h
    src/adwaitaanimations.cpp src/adwaitaanimations.h
    src/adwaitatokens.cpp src/adwaitatokens.h src/adwaitatokens_data.h
    src/adwaitastyle.cpp src/adwaitastyle.h
    src/adwaitastyle_buttons.cpp
    src/adwaitastyle_entries.cpp
    src/adwaitastyle_checks.cpp
    src/adwaitastyle_focus.cpp
    src/adwaitastyle_scroll.cpp
    src/adwaitastyle_range.cpp
    src/adwaitastyle_menus.cpp
    src/adwaitastyle_tabs.cpp
    src/adwaitastyle_views.cpp
    src/adwaitastyleplugin.cpp src/adwaitastyleplugin.h
    ${TOKENS_DATA})
# Plugin key is "Libadwaita" (the old adwaita-qt already owns "Adwaita"); file name qtlibadwaitastyle.so (not libadwaita.so, to avoid confusion with GNOME libadwaita).
set_target_properties(adwaita PROPERTIES PREFIX "" OUTPUT_NAME "qtlibadwaitastyle")
target_link_libraries(adwaita PRIVATE Qt${QT_MAJOR}::Widgets Qt${QT_MAJOR}::DBus)
target_include_directories(adwaita PRIVATE src)
# Optional: xcb lets the style detect a running X11 compositor (_NET_WM_CM_S<n> selection owner).
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(XCB QUIET IMPORTED_TARGET xcb)
    if(XCB_FOUND)
        target_link_libraries(adwaita PRIVATE PkgConfig::XCB)
        target_compile_definitions(adwaita PRIVATE ADWAITA_HAVE_XCB)
    endif()
endif()
install(TARGETS adwaita LIBRARY DESTINATION "${ADWAITA_PLUGIN_DEST}")

if(ADWAITA_BUILD_GALLERY)
    set(GALLERY_NAME adwaita-gallery)
    if(QT_MAJOR EQUAL 5)
        set(GALLERY_NAME adwaita-gallery-qt5)
    endif()
    add_executable(${GALLERY_NAME} gallery/main.cpp)
    target_link_libraries(${GALLERY_NAME} PRIVATE Qt${QT_MAJOR}::Widgets)
    install(TARGETS ${GALLERY_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
