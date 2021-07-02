cmake_minimum_required(VERSION 3.2)
project(Win11SysCheck CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../Bin/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ../Bin/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ../Bin/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ../Bin/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ../Bin/)

add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-DIMGUI_IMPL_WIN32_DISABLE_GAMEPAD)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	add_definitions(-DTARGET_ARCH_STR="64")
	set(ARCH_STR "64")
else() 
	add_definitions(-DTARGET_ARCH_STR="86")
	set(ARCH_STR "86")
endif()

execute_process ( COMMAND git rev-list HEAD OUTPUT_VARIABLE GIT_REV_LIST_OUTPUT ERROR_VARIABLE GIT_REV_LIST_OUTPUT RESULT_VARIABLE GIT_REV_LIST_RESULT WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} )
if( NOT ${GIT_REV_LIST_RESULT} EQUAL 0 )
	message( FATAL_ERROR "get_git_revision(): can not get git revision. log followed: ${GIT_REV_LIST_OUTPUT}")
endif()
string(STRIP ${GIT_REV_LIST_OUTPUT} GIT_REV_LIST_OUTPUT)
string(REPLACE "\n" ";" GIT_REV_LIST ${GIT_REV_LIST_OUTPUT} )
list(LENGTH GIT_REV_LIST gitVersion )
add_definitions(-D_GIT_VERSION_="${gitVersion}")

if(MSVC)
	set(LIB_RT_SUFFIX "mt")
	set(LIB_RT_OPTION "/MT")

	foreach(flag_var  CMAKE_C_FLAGS  CMAKE_CXX_FLAGS)
		 foreach(config_name  ""  DEBUG  RELEASE  MINSIZEREL  RELWITHDEBINFO)
			set(var_name "${flag_var}")

			if(NOT "${config_name}" STREQUAL "")
				set(var_name "${var_name}_${config_name}")
			endif()
			
			string(REPLACE "/MD" "${LIB_RT_OPTION}" ${var_name} "${${var_name}}")
			set(${var_name} "${${var_name}}" CACHE STRING "" FORCE)
		endforeach()
	endforeach()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MT_BUILD}")
endif()

include_directories(${PROJECT_SOURCE_DIR}/extern/imgui-1.83)
include_directories(${PROJECT_SOURCE_DIR}/extern/imgui-1.83/backends)
include_directories(${PROJECT_SOURCE_DIR}/extern/spdlog-1.8.5)
include_directories(${PROJECT_SOURCE_DIR}/extern/fmt-7.1.3)
include_directories(${PROJECT_SOURCE_DIR}/extern/mINI)
include_directories(${PROJECT_SOURCE_DIR}/extern/rapidjson)
include_directories(${PROJECT_SOURCE_DIR}/extern/imgui-notify)
link_directories(${PROJECT_SOURCE_DIR}/extern/d3d9/lib_x${ARCH_STR})
link_directories(${PROJECT_SOURCE_DIR}/extern/spdlog-1.8.5)

file(GLOB HEADERS
    "${PROJECT_SOURCE_DIR}/include/*.hpp"
)
file(GLOB SOURCES
    "${PROJECT_SOURCE_DIR}/extern/imgui-1.83/backends/imgui_impl_win32.cpp"
    "${PROJECT_SOURCE_DIR}/extern/imgui-1.83/backends/imgui_impl_dx9.cpp"
    "${PROJECT_SOURCE_DIR}/extern/imgui-1.83/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

add_executable(
	${PROJECT_NAME}
	${HEADERS}
	${SOURCES}
)

target_precompile_headers(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include/pch.hpp)
target_link_libraries(${PROJECT_NAME} d3d9 d3dx9 DxErr dinput8 dwmapi dxguid PowrProf WinInet Tbs)
