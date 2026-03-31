cmake_minimum_required(VERSION 3.20)
project(miter VERSION 0.2.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/editor/MdEditor.cpp
    src/editor/EditorWidget.cpp
    src/editor/LineNumberArea.cpp
    src/editor/SearchBar.cpp
    src/editor/TabManager.cpp
    src/editor/SessionManager.cpp
    src/parser/ContextStack.cpp
    src/parser/BlockParser.cpp
    src/parser/InlineParser.cpp
    src/parser/LatexParser.cpp
    src/highlight/MdHighlighter.cpp
    src/config/Theme.cpp
    src/config/Settings.cpp
    src/util/CjkUtil.cpp
    src/util/TomlParser.cpp
)

if(APPLE)
    add_executable(miter MACOSX_BUNDLE ${SOURCES})
elseif(WIN32)
    add_executable(miter WIN32 ${SOURCES})
else()
    add_executable(miter ${SOURCES})
endif()

qt_add_resources(miter "miter_themes"
    PREFIX "/themes"
    FILES
        themes/dark.toml
        themes/white.toml
)

target_include_directories(miter PRIVATE src)

target_link_libraries(miter PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)

if(MSVC)
    target_compile_options(miter PRIVATE
        /W4
        /permissive-
        /utf-8
        $<$<CONFIG:Release>:/O2>
    )
else()
    target_compile_options(miter PRIVATE
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Release>:-O2>
    )
endif()

# Tests
option(BUILD_TESTS "Build test suite" ON)
if(BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

# Install
install(TARGETS miter
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY themes DESTINATION ${CMAKE_INSTALL_DATADIR}/miter)

if(UNIX AND NOT APPLE)
    install(FILES miter.desktop DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
endif()
