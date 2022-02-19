cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(vker VERSION 0.1.0 LANGUAGES CXX)

find_package(Vulkan REQUIRED FATAL_ERROR)

add_subdirectory(external)

set(SOURCES
    src/buffer.cpp
    src/engine.cpp
    src/image.cpp
    src/main.cpp
    src/model.cpp
    src/pipeline.cpp
    src/renderer.cpp
    src/shader.cpp
    src/window.cpp
)

set (HEADERS
    src/buffer.h
    src/camera.h
    src/engine.h
    src/image.h
    src/model.h
    src/pipeline.h
    src/renderer.h
    src/shader.h
    src/types.h
    src/utils.h
    src/window.h
    src/vertex.h
)

add_executable(vker ${SOURCES} ${HEADERS})
target_compile_features(vker PRIVATE cxx_std_20)
target_include_directories(vker PRIVATE include)
target_link_libraries(vker fmt::fmt glfw glm::glm tinyobjloader Vulkan::Vulkan)