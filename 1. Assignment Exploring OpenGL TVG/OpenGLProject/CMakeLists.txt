set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

cmake_minimum_required(VERSION 3.10)
project(MyOpenGLProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIBS_PATH "D:/Deseracht/Ens OpenGL/1. Assignment Exploring OpenGL TVG")

# Add source files
file(GLOB SOURCES "src/*.cpp")

# Add GLAD source
list(APPEND SOURCES "${LIBS_PATH}/glad/src/glad.c")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    "${LIBS_PATH}/glfw/include"
    "${LIBS_PATH}/glad/include"
    "${LIBS_PATH}/glm"
    "${LIBS_PATH}/tinyobjloader/include"
)

# Link libraries
target_link_directories(${PROJECT_NAME} PRIVATE
    "${LIBS_PATH}/glfw/lib-mingw-w64"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw3
    opengl32
)

# Copy shader files to build directory
file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})