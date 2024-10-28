set(lib_name refskia)
set(target_name ${lib_name})

project(${lib_name})

include(../cmake/ConfigThirdparty.cmake)

add_library(${target_name} STATIC empty.cpp)

if(NOT WINDOWS)
  set_target_properties(${target_name} PROPERTIES PUBLIC "${platform_spec_path}/libskia.a")
else()
  target_link_libraries(${target_name} PUBLIC "${platform_spec_path}/skia.dll.lib")

  set(OutBinDir ${CMAKE_BINARY_DIR}/Bin)
  add_custom_command(TARGET ${target_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${platform_spec_path}/skia.dll"
        "${OutBinDir}/$<CONFIG>")
endif()

target_include_directories(${target_name} PUBLIC ${CMAKE_CURRENT_LIST_DIR})