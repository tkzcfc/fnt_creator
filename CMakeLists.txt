cmake_minimum_required(VERSION 3.10)

project(fnt_creator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(OutBinDir ${CMAKE_BINARY_DIR}/Bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OutBinDir}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OutBinDir}/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OutBinDir}/$<CONFIG>")
set(CMAKE_VS_DEBUGGER_WORKING_DIRECTORY "${OutBinDir}/$<CONFIG>")

# 添加glfw库
add_subdirectory(3rd/glfw)
# refskia
add_subdirectory(3rd/refskia)
# asyncplusplus
add_subdirectory(3rd/asyncplusplus)

# 设置编译条件
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /MTd")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -std=c++11")
    if(NOT WIN32)
        set(GLAD_LIBRARIES dl)
    endif()
endif()

# 包含头文件的目录
include_directories(
    3rd/glad/include/
    3rd/glfw/include/
    3rd/glm/
    3rd/stb/
    3rd/imgui/
    3rd/imgui/backends/
)

# 收集项目文件
file(
    GLOB VENDORS_SOURCES 
    3rd/glad/src/glad.c
    3rd/imgui/backends/imgui_impl_glfw.cpp
    3rd/imgui/backends/imgui_impl_opengl3.cpp
    3rd/imgui/imgui*.cpp
)

# 添加文件分组
source_group("vendors" FILES ${VENDORS_SOURCES})


############################################# fnt_creator #############################################
set(APP_NAME fnt_creator)

file(GLOB APP_SOURCES
    apps/fnt_creator/*.h
    apps/fnt_creator/*.cpp
)

# 链接可执行文件
add_executable(${APP_NAME} ${APP_SOURCES} ${VENDORS_SOURCES})

# 链接库
target_link_libraries(${APP_NAME} glfw refskia Async++ ${GLAD_LIBRARIES})

set_target_properties(${APP_NAME} PROPERTIES
    FOLDER "apps"
)

