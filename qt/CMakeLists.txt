cmake_minimum_required(VERSION 3.16)
project(MySchedule_Qt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/account_system.cpp
    src/Task.cpp
    src/TaskManager.cpp
)

set(HEADERS
    src/mainwindow.h
    src/account_system.h
    src/Task.h
    src/TaskManager.h
)

add_executable(MySchedule_Qt ${SOURCES} ${HEADERS})

target_include_directories(MySchedule_Qt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(MySchedule_Qt
    Qt6::Widgets
    Qt6::Multimedia
)
