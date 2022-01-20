cmake_minimum_required(VERSION 3.15)
project(supershader)

function(remove_compile_options DEST_VAR COMPILER_FLAGS FLAGS)
        separate_arguments(FLAGS)
        foreach(FLAG ${FLAGS})
            string(REPLACE "${FLAG}" "" COMPILER_FLAGS "${COMPILER_FLAGS}")
        endforeach()
        set(${DEST_VAR} ${COMPILER_FLAGS} PARENT_SCOPE)
endfunction()

option(STATIC_LINK "Statically link with system standard libraries" ON)

option(ENABLE_SPVREMAPPER "Enables building of SPVRemapper" ON)
option(ENABLE_AMD_EXTENSIONS "Enables support of AMD-specific extensions" ON)
option(ENABLE_GLSLANG_BINARIES "Builds glslangValidator and spirv-remap" ON)
option(ENABLE_NV_EXTENSIONS "Enables support of Nvidia-specific extensions" ON)
option(ENABLE_HLSL "Enables HLSL input support" OFF)
option(ENABLE_OPT "Enables spirv-opt capability if present" ON)

option(ARGPARSE_SHARED "Build shared library" OFF)
option(ARGPARSE_STATIC "Build static library" ON)

option(USE_CCACHE "Use ccache" OFF)
if (WIN32)
    option(ENABLE_D3D11_COMPILER "Use Direct3D11 compiler (d3dcompiler.lib + d3dcompiler_47.dll)" ON)
endif()

set(SX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

if(ENABLE_AMD_EXTENSIONS)
    add_definitions(-DAMD_EXTENSIONS)
endif(ENABLE_AMD_EXTENSIONS)

if(ENABLE_NV_EXTENSIONS)
    add_definitions(-DNV_EXTENSIONS)
endif(ENABLE_NV_EXTENSIONS)

if(ENABLE_HLSL)
    add_definitions(-DENABLE_HLSL)
endif(ENABLE_HLSL)

if (MSVC)
	remove_compile_options(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "/GR")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /wd4530")
	
	if (STATIC_LINK)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	endif()
endif()

if (APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for Mac OS X")
endif()

include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/libs/argparse")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/libs/json/single_include")

add_subdirectory(libs/glslang)
add_subdirectory(libs/spirv-cross)
add_subdirectory(libs/argparse)
if(ENABLE_OPT)
    add_definitions(-DENABLE_OPT=1)
    include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/libs/glslang/External/spirv-tools/include")
endif()
add_subdirectory(src)