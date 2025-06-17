cmake_minimum_required(VERSION 3.10)
project(ESIEE_Computer_Graphics CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- GLFW --- 
# On macOS, GLFW can often be found via find_package if installed (e.g. via Homebrew)
# On Windows, you might need to provide paths or use a bundled version.
# This example assumes GLFW is findable or you'll add it.
find_package(glfw3 REQUIRED)

# --- OpenGL --- 
find_package(OpenGL REQUIRED)

# --- GLEW (for Windows/Linux if not using GLAD) ---
if(WIN32)
    find_package(GLEW REQUIRED)
    include_directories(${GLEW_INCLUDE_DIRS})
    link_libraries(${GLEW_LIBRARIES})
endif()

# --- Source Files --- 
# Adjust these paths if you adopt the suggested src/ directory structure
# For now, assuming files are in the root or specified paths

# If you move files to src/ later:
# aux_source_directory(src SRC_FILES)

set(SRC_FILES
    src/main.cpp
    src/GLShader.cpp
)

# --- Include Directories ---
include_directories(libs) # For tinyobjloader and stb_image
include_directories(src)  # For local headers like mat4.h, GLShader.h
# If you create include/ or libs/ for headers:
# include_directories(include)
# include_directories(libs/stb) # For stb_image.h

# For now, assuming stb_image.h is accessible in the include path or relative to main.cpp
# You might need to specify its directory explicitly:
# include_directories(/path/to/your/libs/stb) 

# --- Executable --- 
add_executable(${PROJECT_NAME} ${SRC_FILES})

# --- Linking --- 
target_link_libraries(${PROJECT_NAME} PRIVATE glfw ${OPENGL_LIBRARIES})

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo")
    # For Apple Silicon, ensure you are building for arm64 architecture
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for OSX" FORCE)
elseif(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${GLEW_LIBRARIES} opengl32)
endif()

# --- TinyObjLoader (will be added later) ---
# When you add tinyobjloader:
# add_subdirectory(libs/tinyobjloader) # If it has its own CMakeLists.txt
# OR
# target_sources(${PROJECT_NAME} PRIVATE libs/tinyobjloader/tiny_obj_loader.cc)
# target_include_directories(${PROJECT_NAME} PRIVATE libs/tinyobjloader)

# --- Shaders and Assets ---
# Copy asset directory and shader files to the build output directory so the executable can find them.
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
file(COPY shaders/basic.vs DESTINATION ${CMAKE_BINARY_DIR})
file(COPY shaders/basic.fs DESTINATION ${CMAKE_BINARY_DIR})

# --- Compiler Flags for macOS deprecation warning ---
if(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GL_SILENCE_DEPRECATION)
endif()


# Instructions to build:
# 1. Create a build directory: mkdir build && cd build
# 2. Configure: cmake ..
# 3. Build: make (on macOS/Linux) or open the generated .sln in Visual Studio (on Windows) and build.
