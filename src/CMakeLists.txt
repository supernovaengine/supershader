cmake_minimum_required(VERSION 3.15)

add_definitions("-DSUPERSHADER_VERSION=${SUPERSHADER_VERSION}")
				 
add_executable(supershader 
        main.cc
        args.cc
        input.cc
        glslang.cc
        spirvcross.cc
        json.cc
        sbs-file.cc
        )

target_link_libraries(supershader PRIVATE
        argparse
        glslang 
        OGLCompiler 
        OSDependent 
        SPIRV 
        SPVRemapper 
        spirv-cross-core 
        spirv-cross-cpp 
        spirv-cross-glsl 
        spirv-cross-hlsl 
        spirv-cross-reflect
        spirv-cross-util 
        spirv-cross-msl
        )  

if (MSVC)
	if (ENABLE_D3D11_COMPILER)
		add_definitions(-DD3D11_COMPILER)
		target_link_libraries(supershader PRIVATE d3dcompiler)
	endif()
elseif (NOT WIN32)
	
endif()

# static linking
if (STATIC_LINK AND WIN32 AND NOT MSVC)
	if ((CMAKE_BUILD_TYPE STREQUAL Release) OR (CMAKE_BUILD_TYPE STREQUAL MinSizeRel))
		set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
		#if (APPLE)
		#	add_custom_command(TARGET supershader POST_BUILD COMMAND strip -X -x ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/supershader)
		#elseif (NOT WIN32)
		#	add_custom_command(TARGET supershader POST_BUILD COMMAND strip -g -X -x ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/supershader)
		#endif()
	endif()
endif()

set_target_properties(
        supershader
        PROPERTIES
        CXX_STANDARD 11
)

install(TARGETS supershader 
        CONFIGURATIONS Release MinSizeRel 
        RUNTIME DESTINATION bin)