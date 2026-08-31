cmake_minimum_required(VERSION 3.20)
project(atlas VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml Widgets Sql Concurrent)

qt_standard_project_setup()

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/indexer
)

# --- Rust indexer (atlas_indexer.dll) -------------------------------------
# Always built with cargo in release mode: search speed matters even in Debug
# app builds, and the DLL boundary keeps Rust/MSVC CRTs isolated.
set(INDEXER_DIR ${CMAKE_SOURCE_DIR}/src/indexer)
set(INDEXER_DLL ${INDEXER_DIR}/target/release/atlas_indexer.dll)
set(INDEXER_IMPLIB ${INDEXER_DIR}/target/release/atlas_indexer.dll.lib)

# Cargo runs on every build; it is incremental and a no-op takes <0.2s.
add_custom_target(atlas_indexer_build
    COMMAND cargo build --release
    WORKING_DIRECTORY ${INDEXER_DIR}
    COMMENT "Building Rust indexer (cargo build --release)"
    VERBATIM
)

qt_add_executable(atlas
    src/main.cpp
    src/app/AtlasNativeHost.hpp
    src/app/AtlasNativeHost.cpp
    src/app/PerfTrace.hpp
    src/app/PerfTrace.cpp
    src/app/SearchViewModel.hpp
    src/app/SearchViewModel.cpp
    src/app/ThemeBridge.hpp
    src/app/ConfigBridge.hpp
    src/services/apps/AppSearchService.hpp
    src/services/apps/AppSearchService.cpp
    src/services/files/WinFileSearchService.hpp
    src/services/files/WinFileSearchService.cpp
    src/services/files/FileIndexService.hpp
    src/services/files/FileIndexService.cpp
    src/services/files/ShellIcons.hpp
    src/services/files/ShellIcons.cpp
    src/services/calculator/CalculatorService.hpp
    src/services/calculator/CalculatorService.cpp
    src/services/clipboard/ClipboardService.hpp
    src/services/clipboard/ClipboardService.cpp
    src/services/snippet/SnippetService.hpp
    src/services/snippet/SnippetService.cpp
)
add_dependencies(atlas atlas_indexer_build)

qt_add_qml_module(atlas
    URI Atlas
    VERSION 1.0
    QML_FILES
        src/ui/MainPalette.qml
        src/ui/ShortcutBadge.qml
        src/ui/TextBadge.qml
        src/ui/SectionHeader.qml
        src/ui/ViciScrollBar.qml
        src/ui/SelectableDelegate.qml
        src/ui/ListItemDelegate.qml
        src/ui/CalculatorResultDelegate.qml
        src/ui/EmptyView.qml
        src/ui/HorizontalLoadingBar.qml
        src/ui/FooterButton.qml
        src/ui/Footer.qml
        src/ui/ActionItemDelegate.qml
        src/ui/ActionPanelPopover.qml
        src/ui/ResultsListView.qml
)

target_link_libraries(atlas PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::Widgets
    Qt6::Sql
    Qt6::Concurrent
)

target_link_libraries(atlas PRIVATE ${INDEXER_IMPLIB})

if (WIN32)
    target_link_libraries(atlas PRIVATE dwmapi user32 shell32 ole32 oleaut32 oledb comctl32)

    add_custom_command(TARGET atlas POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${INDEXER_DLL} $<TARGET_FILE_DIR:atlas>
        COMMENT "Copying atlas_indexer.dll"
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(WINDEPLOYQT_MODE --debug)
    else()
        set(WINDEPLOYQT_MODE --release)
    endif()
    add_custom_command(TARGET atlas POST_BUILD
        COMMAND ${Qt6_DIR}/../../../bin/windeployqt.exe ${WINDEPLOYQT_MODE} --qmldir ${CMAKE_SOURCE_DIR}/src/ui $<TARGET_FILE:atlas>
        COMMENT "Deploying Qt runtime DLLs"
    )
endif()
