cmake_minimum_required(VERSION 3.28)
project(wolfmv)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

# Configure GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

set(CMAKE_XCODE_GENERATE_SCHEME OFF)

# Thirdparty libraries
add_subdirectory( deps/glad )
add_subdirectory( deps/glfw )
add_subdirectory( deps/imgui )

add_executable( ${PROJECT_NAME}
        src/main.cpp
        
        src/Math.h
        src/Matrix.cpp
        src/Vector.cpp
        
        src/DrawCall.h
        
        src/MDSModel.cpp
        src/MDSModel.h
        src/MDSFile.h
        
        src/MD3Model.cpp
        src/MD3Model.h
        
        src/Renderer.cpp
        src/Renderer.h
        
        src/RenderableModel.cpp
        src/RenderableModel.h
        
        src/Camera.cpp
        src/Camera.h
        
        src/Shader.cpp
        src/Shader.h
        
        src/MainQueue.h

        deps/imgui/backends/imgui_impl_glfw.cpp
        deps/imgui/backends/imgui_impl_opengl3.cpp
        
        deps/tinyfiledialogs.c
)

target_include_directories(${PROJECT_NAME} PRIVATE deps/glm)

target_link_libraries(${PROJECT_NAME} PRIVATE glad glfw imgui)

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/assets
)
