cmake_minimum_required(VERSION 3.10)
project(MySchedule)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 设置可执行文件名称
set(EXECUTABLE_NAME myschedule)

# 查找所需的库
find_package(Threads REQUIRED)

# 如果是Windows，需要链接额外的库
if (WIN32)
    # Windows下需要链接这些库以支持控制台UTF-8和系统调用
    set(PLATFORM_LIBS ${CMAKE_THREAD_LIBS_INIT})
else()
    set(PLATFORM_LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

# 收集所有源文件
set(SOURCES
    main.cpp
    account_system.cpp
    TaskManager.cpp
    Task.cpp
)

# 收集所有头文件
set(HEADERS
    account_system.h
    TaskManager.h
    Task.h
)

# 创建可执行文件
add_executable(${EXECUTABLE_NAME} ${SOURCES} ${HEADERS})

# 链接必要的库
target_link_libraries(${EXECUTABLE_NAME} 
    ${PLATFORM_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
)

# 如果是Windows，设置控制台子系统并处理UTF-8
if (WIN32)
    # 设置控制台子系统（防止出现窗口）
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
    
    # 添加Windows特定的编译选项
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _USE_MATH_DEFINES
    )
    
    # 处理UTF-8支持
    if (MSVC)
        target_compile_options(${EXECUTABLE_NAME} PRIVATE
            /execution-charset:utf-8
            /source-charset:utf-8
        )
    endif()
endif()

# 设置头文件包含路径
target_include_directories(${EXECUTABLE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 添加编译选项
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${EXECUTABLE_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O2
    )
elseif (MSVC)
    target_compile_options(${EXECUTABLE_NAME} PRIVATE
        /W4
        /O2
        /EHsc
    )
endif()

# 设置版本信息（可选）
set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    VERSION 1.0.0
    DESCRIPTION "MySchedule - Task Manager"
)

# 可选：添加安装规则
install(TARGETS ${EXECUTABLE_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 可选：添加测试支持（如果启用测试）
option(BUILD_TESTS "Build tests" OFF)
if (BUILD_TESTS)
    enable_testing()
    # 这里可以添加测试相关的配置
    message(STATUS "Tests enabled")
endif()

# 打印配置信息
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