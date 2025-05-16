cmake_minimum_required(VERSION 3.15)

add_definitions("-DSUPERSHADER_VERSION=${SUPERSHADER_VERSION}")

# Source files used for both library and executable
set(SUPERSHADER_SOURCES
    main.cc
    args.cc
    input.cc
    glslang.cc
    spirvcross.cc
    json.cc
    sbs-file.cc
)

# Build as library or executable based on the option
if(SUPERSHADER_LIBRARY)
    # Build as a static library
    add_library(supershader STATIC ${SUPERSHADER_SOURCES})
    # Define this so the main.cc file can be conditionally compiled
    target_compile_definitions(supershader PRIVATE SUPERSHADER_LIBRARY)
else()
    # Build as an executable
    add_executable(supershader ${SUPERSHADER_SOURCES})
endif()

target_link_libraries(supershader PRIVATE
    argparse
    glslang
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

if (WIN32 AND NOT MSVC AND NOT SUPERSHADER_LIBRARY)
    if ((CMAKE_BUILD_TYPE STREQUAL Release) OR (CMAKE_BUILD_TYPE STREQUAL MinSizeRel))
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
        #if (APPLE)
        #    add_custom_command(TARGET supershader POST_BUILD COMMAND strip -X -x ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/supershader)
        #elseif (NOT WIN32)
        #    add_custom_command(TARGET supershader POST_BUILD COMMAND strip -g -X -x ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/supershader)
        #endif()
    endif()
endif()

set_target_properties(
    supershader
    PROPERTIES
    CXX_STANDARD 17
)

# Only install the executable if not building as a library
if(NOT SUPERSHADER_LIBRARY)
    install(TARGETS supershader 
            CONFIGURATIONS Release MinSizeRel 
            RUNTIME DESTINATION bin)
else()
    # For library mode, install the library
    install(TARGETS supershader
            CONFIGURATIONS Release MinSizeRel
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin)
endif()