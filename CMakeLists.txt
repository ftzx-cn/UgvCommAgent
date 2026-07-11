cmake_minimum_required(VERSION 3.16)
project(UgvCommAgent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.8.3/msvc2022_64")

find_package(QT NAMES Qt6 Qt5 REQUIRED)
find_package(Qt${QT_VERSION_MAJOR}
    COMPONENTS
        Core
        Gui
        Widgets
        Network
        SerialBus
        WebSockets
)
qt_standard_project_setup()

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        ./utils/cywiplineedit.cpp
        ./utils/cywiplineedit.h
        ./utils/suffixlineedit.cpp
        ./utils/suffixlineedit.h
        ./utils/PumpWidget.cpp
        ./utils/PumpWidget.h
        ./utils/TankWidget.cpp
        ./utils/TankWidget.h
        ./utils/SwitchButtonWidget.cpp
        ./utils/SwitchButtonWidget.h
        ./utils/PipeWidget.cpp
        ./utils/PipeWidget.h
        ./3rdparty/qtpropertybrowser/qtbuttonpropertybrowser.cpp
        ./3rdparty/qtpropertybrowser/qtbuttonpropertybrowser_p.h
        ./3rdparty/qtpropertybrowser/qteditorfactory.cpp
        ./3rdparty/qtpropertybrowser/qteditorfactory_p.h
        ./3rdparty/qtpropertybrowser/qtgroupboxpropertybrowser.cpp
        ./3rdparty/qtpropertybrowser/qtgroupboxpropertybrowser_p.h
        ./3rdparty/qtpropertybrowser/qtpropertybrowser.cpp
        ./3rdparty/qtpropertybrowser/qtpropertybrowser_p.h
        ./3rdparty/qtpropertybrowser/qtpropertybrowserutils.cpp
        ./3rdparty/qtpropertybrowser/qtpropertybrowserutils_p.h
        ./3rdparty/qtpropertybrowser/qtpropertymanager.cpp
        ./3rdparty/qtpropertybrowser/qtpropertymanager_p.h
        ./3rdparty/qtpropertybrowser/qttreepropertybrowser.cpp
        ./3rdparty/qtpropertybrowser/qttreepropertybrowser_p.h
        ./3rdparty/qtpropertybrowser/qtvariantproperty.cpp
        ./3rdparty/qtpropertybrowser/qtvariantproperty_p.h
        ./core/configmodel.cpp
        ./core/configmodel.h
        ./core/configsource.cpp
        ./core/configsource.h
        ./core/robustmodbusclient.cpp
        ./core/robustmodbusclient.h
        ./resources/resources.qrc
        core/websocketclient.cpp
        core/websocketclient.h
)

qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(UgvCommAgent PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE TRUE
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Network
        Qt::SerialBus
        Qt::WebSockets
)