cmake_minimum_required(VERSION 3.5)

project(ParkingSystem VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS
    Widgets
    Bluetooth
    Sql
)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Widgets
    Bluetooth
    Sql
)

set(PROJECT_SOURCES
    main.cpp

    login.cpp
    login.h
    login.ui

    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    bleclient.cpp
    bleclient.h

    database.cpp
    database.h

    userdialog.cpp
    userdialog.h
    userdialog.ui

    userdashboard.ui
    
    fingerprintenrolldialog.cpp
    fingerprintenrolldialog.h
    fingerprintenrolldialog.ui
	
    resources.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)

    qt_add_executable(ParkingSystem
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

else()

    if(ANDROID)

        add_library(ParkingSystem SHARED
            ${PROJECT_SOURCES}
        )

    else()

        add_executable(ParkingSystem
            ${PROJECT_SOURCES
        )

    endif()

endif()

target_link_libraries(ParkingSystem PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Bluetooth
    Qt${QT_VERSION_MAJOR}::Sql
)

set_target_properties(ParkingSystem PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING
        ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ParkingSystem
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ParkingSystem)
endif()
