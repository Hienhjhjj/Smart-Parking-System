cmake_minimum_required(VERSION 3.5)

project(ParkingSystemTests VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    Bluetooth
    Sql
    Test
)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Core
    Widgets
    Bluetooth
    Sql
    Test
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Project source files required by tests (excluding main.cpp)
set(APP_COMMON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../database.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../userdialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../userdialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../userdialog.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../fingerprintenrolldialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../fingerprintenrolldialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../fingerprintenrolldialog.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../login.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../login.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../login.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../bleclient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../bleclient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../mainwindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../mainwindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../mainwindow.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../resources.qrc
)

enable_testing()

# 1. Test Database
add_executable(test_database
    test_database.h
    test_database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../database.h
)
target_link_libraries(test_database PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestDatabase COMMAND test_database)

# 2. Test Login
add_executable(test_login
    test_login.h
    test_login.cpp
    ${APP_COMMON_SOURCES}
)
target_link_libraries(test_login PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Bluetooth
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestLogin COMMAND test_login)

# 3. Test User Dialog
add_executable(test_user_dialog
    test_user_dialog.h
    test_user_dialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../userdialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../userdialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../userdialog.ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../fingerprintenrolldialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../fingerprintenrolldialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../fingerprintenrolldialog.ui
)
target_link_libraries(test_user_dialog PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestUserDialog COMMAND test_user_dialog)

# 4. Test MainWindow
add_executable(test_mainwindow
    test_mainwindow.h
    test_mainwindow.cpp
    ${APP_COMMON_SOURCES}
)
target_link_libraries(test_mainwindow PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Bluetooth
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestMainWindow COMMAND test_mainwindow)

# 5. Combined Test Runner
add_executable(run_all_tests
    main_test_runner.cpp
    test_database.h
    test_database.cpp
    test_login.h
    test_login.cpp
    test_user_dialog.h
    test_user_dialog.cpp
    test_mainwindow.h
    test_mainwindow.cpp
    ${APP_COMMON_SOURCES}
)
target_compile_definitions(run_all_tests PRIVATE RUN_ALL_TESTS_COMBINED)
target_link_libraries(run_all_tests PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Bluetooth
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME RunAllTests COMMAND run_all_tests)
