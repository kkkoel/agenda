cmake_minimum_required(VERSION 3.10)
project(MySchedule)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(EXECUTABLE_NAME myschedule)

set(SOURCES
    main.cpp
    account_system.cpp
    TaskManager.cpp
    Task.cpp
)

set(HEADERS
    account_system.h
    TaskManager.h
    Task.h
)

add_executable(${EXECUTABLE_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${EXECUTABLE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (WIN32)
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )

    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _USE_MATH_DEFINES
    )

    if (MSVC)
        target_compile_options(${EXECUTABLE_NAME} PRIVATE
            /execution-charset:utf-8
            /source-charset:utf-8
        )
    endif()

    if (MINGW)
        target_link_libraries(${EXECUTABLE_NAME}
            -lwinmm
            -lws2_32
        )
    endif()

elseif (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(${EXECUTABLE_NAME}
        ${CMAKE_THREAD_LIBS_INIT}
        pthread
    )

    target_compile_options(${EXECUTABLE_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -O2
    )

elseif (APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(${EXECUTABLE_NAME}
        ${CMAKE_THREAD_LIBS_INIT}
        pthread
    )

    target_compile_options(${EXECUTABLE_NAME} PRIVATE
        -Wall -Wextra -Wpedantic -O2
    )
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${EXECUTABLE_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O2
    )
endif()

if (MSVC)
    target_compile_options(${EXECUTABLE_NAME} PRIVATE
        /W4
        /O2
        /EHsc
    )
endif()

set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    VERSION 1.0.0
    DESCRIPTION "MySchedule - Task Manager"
)

install(TARGETS ${EXECUTABLE_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

option(BUILD_TESTS "Build tests" OFF)
if (BUILD_TESTS)
    enable_testing()
    message(STATUS "Tests enabled")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Executable: ${EXECUTABLE_NAME}")
if (WIN32)
    message(STATUS "Platform: Windows")
elseif (APPLE)
    message(STATUS "Platform: macOS")
elseif (UNIX)
    message(STATUS "Platform: Unix/Linux")
endif()