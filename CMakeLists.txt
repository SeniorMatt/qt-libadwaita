cmake_minimum_required(VERSION 3.16)
project(qt-libadwaita VERSION 0.1.0 LANGUAGES CXX)

option(ADWAITA_QT5 "Build against Qt 5 instead of Qt 6 (not implemented yet, door left open)" OFF)
option(ADWAITA_BUILD_GALLERY "Build the adwaita-gallery widget gallery" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

if(ADWAITA_QT5)
    set(QT_MAJOR 5)
else()
    set(QT_MAJOR 6)
endif()
find_package(QT NAMES Qt${QT_MAJOR} REQUIRED COMPONENTS Widgets DBus)
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

# Regenerate src/adwaitatokens_data.h from data/adwaita-1.10.json when the JSON exists.
find_package(Python3 QUIET COMPONENTS Interpreter)
if(Python3_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/data/adwaita-1.10.json")
    add_custom_command(
        OUTPUT "${CMAKE_SOURCE_DIR}/src/adwaitatokens_data.h"
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/gen_tokens.py"
        DEPENDS "${CMAKE_SOURCE_DIR}/data/adwaita-1.10.json" "${CMAKE_SOURCE_DIR}/tools/gen_tokens.py"
        COMMENT "Generating adwaitatokens_data.h")
    set(TOKENS_DATA "${CMAKE_SOURCE_DIR}/src/adwaitatokens_data.h")
endif()

add_library(adwaita MODULE
    src/adwaitasettings.cpp src/adwaitasettings.h
    src/adwaitatokens.cpp src/adwaitatokens.h src/adwaitatokens_data.h
    src/adwaitastyle.cpp src/adwaitastyle.h
    src/adwaitastyle_buttons.cpp
    src/adwaitastyle_entries.cpp
    src/adwaitastyle_checks.cpp
    src/adwaitastyle_focus.cpp
    src/adwaitastyleplugin.cpp src/adwaitastyleplugin.h
    ${TOKENS_DATA})
# Plugin key is "Libadwaita" (the old adwaita-qt already owns "Adwaita"); file name libadwaita.so.
set_target_properties(adwaita PROPERTIES PREFIX "" OUTPUT_NAME "libadwaita")
target_link_libraries(adwaita PRIVATE Qt${QT_MAJOR}::Widgets Qt${QT_MAJOR}::DBus)
target_include_directories(adwaita PRIVATE src)
install(TARGETS adwaita LIBRARY DESTINATION "${ADWAITA_PLUGIN_DEST}")

if(ADWAITA_BUILD_GALLERY)
    add_executable(adwaita-gallery gallery/main.cpp)
    target_link_libraries(adwaita-gallery PRIVATE Qt${QT_MAJOR}::Widgets)
    install(TARGETS adwaita-gallery RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
