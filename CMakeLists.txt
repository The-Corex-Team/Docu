cmake_minimum_required(VERSION 3.21)

project(Docu VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets OpenGLWidgets)

qt_standard_project_setup()

qt_add_executable(Docu
    src/Style.h
    src/main.cpp
    src/gui/MainWindow.cpp
    src/gui/MainWindow.h
    src/gui/Ribbon.cpp
    src/gui/Ribbon.h
    src/gui/Editor/DocumentView.cpp
    src/gui/Editor/DocumentView.h
    src/document/Document.cpp
    src/document/Document.h
    src/document/DocumentController.cpp
    src/document/DocumentController.h
    src/document/ImageLayerFormat.h
    src/document/PageSettings.cpp
    src/document/PageSettings.h
    src/io/ImageLoader.cpp
    src/io/ImageLoader.h
    src/io/DocumentFile.cpp
    src/io/DocumentFile.h
)

target_link_libraries(Docu PRIVATE Qt6::Widgets Qt6::OpenGLWidgets)

target_include_directories(Docu PRIVATE src)

include(CTest)
if(BUILD_TESTING)
    add_executable(DocuResizeInvariantCheck
        tests/ResizeInvariantCheck.cpp
        src/gui/Editor/DocumentView.cpp
        src/gui/Editor/DocumentView.h
        src/document/Document.cpp
        src/document/Document.h
        src/document/ImageLayerFormat.h
        src/document/PageSettings.cpp
        src/document/PageSettings.h
    )
    target_link_libraries(DocuResizeInvariantCheck PRIVATE
        Qt6::Widgets
        Qt6::OpenGLWidgets
    )
    target_include_directories(DocuResizeInvariantCheck PRIVATE src)

    add_executable(DocuImageInsertionCheck
        tests/ImageInsertionCheck.cpp
        src/gui/Editor/DocumentView.cpp
        src/gui/Editor/DocumentView.h
        src/document/Document.cpp
        src/document/Document.h
        src/document/DocumentController.cpp
        src/document/DocumentController.h
        src/document/ImageLayerFormat.h
        src/document/PageSettings.cpp
        src/document/PageSettings.h
        src/io/ImageLoader.cpp
        src/io/ImageLoader.h
    )
    target_link_libraries(DocuImageInsertionCheck PRIVATE
        Qt6::Widgets
        Qt6::OpenGLWidgets
    )
    target_include_directories(DocuImageInsertionCheck PRIVATE src)

    add_test(NAME DocuResizeInvariantCheck COMMAND DocuResizeInvariantCheck)
    set_tests_properties(DocuResizeInvariantCheck PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
    add_test(NAME DocuImageInsertionCheck COMMAND DocuImageInsertionCheck)
    set_tests_properties(DocuImageInsertionCheck PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )

    add_executable(DocuDocumentFileCheck
        tests/DocumentFileCheck.cpp
        src/io/DocumentFile.cpp
        src/io/DocumentFile.h
        src/document/Document.cpp
        src/document/Document.h
        src/document/ImageLayerFormat.h
        src/document/PageSettings.cpp
        src/document/PageSettings.h
    )
    target_link_libraries(DocuDocumentFileCheck PRIVATE Qt6::Widgets)
    target_include_directories(DocuDocumentFileCheck PRIVATE src)
    add_test(NAME DocuDocumentFileCheck COMMAND DocuDocumentFileCheck)
    set_tests_properties(DocuDocumentFileCheck PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()
