cmake_minimum_required(VERSION 3.12)
project(particle_simulator CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Platform-specific compiler flags
if(MSVC)
    # Visual Studio
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /GL /DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi")
    add_compile_options(/W3)
else()
    # GCC/Clang (Linux/MinGW)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
endif()

# Find required packages
find_package(OpenGL REQUIRED)

# GLEW - handle different install methods
if(WIN32)
    # Try vcpkg first
    find_package(GLEW CONFIG)
    if(NOT GLEW_FOUND)
        # Try standard CMake find
        find_package(GLEW REQUIRED)
    endif()
else()
    find_package(GLEW REQUIRED)
endif()

# pybind11
find_package(pybind11 CONFIG)
if(NOT pybind11_FOUND)
    # Try pkg-config
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PYBIND11 pybind11)
    endif()
    
    if(NOT PYBIND11_FOUND)
        # Use Python to find pybind11
        execute_process(
            COMMAND ${PYTHON_EXECUTABLE} -m pybind11 --cmakedir
            OUTPUT_VARIABLE pybind11_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        find_package(pybind11 CONFIG REQUIRED)
    endif()
endif()

# GLM (header-only library)
find_package(glm CONFIG)
if(NOT glm_FOUND)
    # Try standard find_package
    find_package(GLM)
    if(NOT GLM_FOUND)
        # Set manually if needed
        set(GLM_INCLUDE_DIRS "" CACHE PATH "GLM include directory")
        if(NOT GLM_INCLUDE_DIRS)
            message(FATAL_ERROR "GLM not found. Please install via vcpkg or set GLM_INCLUDE_DIRS")
        endif()
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
)

# Source files
set(RENDERER_SOURCES
    src/cpp/particle_renderer.cpp
)

# Python module
pybind11_add_module(particle_renderer
    src/cpp/python_bindings.cpp
    ${RENDERER_SOURCES}
)

# Link libraries
target_link_libraries(particle_renderer PRIVATE
    ${OPENGL_LIBRARIES}
    GLEW::GLEW
)

# Windows-specific: Copy GLEW DLL to output
if(WIN32 AND NOT GLEW_USE_STATIC_LIBS)
    if(TARGET GLEW::GLEW)
        get_target_property(GLEW_DLL GLEW::GLEW IMPORTED_LOCATION_RELEASE)
        if(GLEW_DLL)
            add_custom_command(TARGET particle_renderer POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${GLEW_DLL}
                ${CMAKE_SOURCE_DIR}/src/python/
            )
        endif()
    endif()
endif()

# Installation
install(TARGETS particle_renderer
    LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/lib
)

# Copy to source directory for development
add_custom_command(TARGET particle_renderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:particle_renderer> ${CMAKE_SOURCE_DIR}/src/python/
    COMMENT "Copying particle_renderer to src/python/"
)

# Print configuration
message(STATUS "===========================================")
message(STATUS "Build Configuration")
message(STATUS "===========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "OpenGL: ${OPENGL_LIBRARIES}")
if(TARGET GLEW::GLEW)
    message(STATUS "GLEW: Found (target GLEW::GLEW)")
else()
    message(STATUS "GLEW: ${GLEW_LIBRARIES}")
endif()
message(STATUS "GLM: ${GLM_INCLUDE_DIRS}")
message(STATUS "pybind11: ${pybind11_VERSION}")
message(STATUS "===========================================")