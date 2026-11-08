cmake_minimum_required(VERSION 3.16)

project(wallmenu VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

add_executable(wallmenu
    src/main.cpp
    src/MainWindow.cpp
    src/ImageModel.cpp
    src/Config.cpp
    src/WallpaperSetter.cpp
)

target_include_directories(wallmenu PRIVATE src)

target_link_libraries(wallmenu PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
)

include(GNUInstallDirs)
install(TARGETS wallmenu RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Generated at install time so Exec honours `cmake --install --prefix`. The path
# is absolute because desktop sessions often lack ~/.local/bin on PATH.
install(CODE "
    set(WALLMENU_EXECUTABLE \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/wallmenu\")
    configure_file(
        \"${CMAKE_CURRENT_SOURCE_DIR}/wallmenu.desktop.in\"
        \"${CMAKE_CURRENT_BINARY_DIR}/wallmenu.desktop\"
        @ONLY
    )
")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/wallmenu.desktop"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
