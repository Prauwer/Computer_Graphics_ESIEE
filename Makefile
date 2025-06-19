# Makefile for ESIEE_Computer_Graphics

# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++11
LDFLAGS =
LIBS =
INCLUDES = -Ilibs
DEFINES =

# Source files and object files
SRCDIR = .
SOURCES = $(SRCDIR)/main.cpp $(SRCDIR)/GLShader.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = ESIEE_Computer_Graphics

# Default to macOS if OS detection fails or is not specific
OS_NAME = $(shell uname -s)

ifeq ($(OS_NAME),Darwin)
    # macOS specific settings
    DEFINES += -DGL_SILENCE_DEPRECATION
    # Try local GLFW first, then Homebrew
    LOCAL_GLFW_INCLUDE = ./glfw-3.4.bin.MACOS/include
    LOCAL_GLFW_LIB = ./glfw-3.4.bin.MACOS/lib-macos # Adjust if your lib folder is different
    
    ifeq ("$(wildcard $(LOCAL_GLFW_INCLUDE)/GLFW/glfw3.h)","")
        INCLUDES += -I/opt/homebrew/include
        LDFLAGS += -L/opt/homebrew/lib
    else
        INCLUDES += -I$(LOCAL_GLFW_INCLUDE)
        LDFLAGS += -L$(LOCAL_GLFW_LIB)
    endif
    
    LIBS += -lglfw
    LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
else ifeq ($(findstring MINGW,$(OS_NAME)),MINGW) # For MinGW/MSYS2 on Windows
    # Windows specific settings (using MinGW)
    DEFINES += -D_WIN32
    INCLUDES += -Ilibs/glfw/include -Ilibs/glew/include
    LDFLAGS += -Llibs/glfw/lib-mingw-w64 -Llibs/glew/lib/Release/x64 # Adjust paths as per your GLFW/GLEW setup
    LIBS += -lglew32 -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32
    EXECUTABLE := $(EXECUTABLE).exe # Add .exe extension for Windows
else ifeq ($(findstring CYGWIN,$(OS_NAME)),CYGWIN) # For Cygwin on Windows
    # Windows specific settings (using Cygwin)
    DEFINES += -D_WIN32
    INCLUDES += -Ilibs/glfw/include -Ilibs/glew/include 
    LDFLAGS += -Llibs/glfw/lib-mingw-w64 -Llibs/glew/lib/Release/x64 # Adjust paths
    LIBS += -lglew32 -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32
    EXECUTABLE := $(EXECUTABLE).exe # Add .exe extension for Windows
else
    # Fallback for Linux-like systems (you might need to adjust GLFW/GLEW paths)
    # DEFINES += -DGL_SILENCE_DEPRECATION # Or other Linux specific defines
    INCLUDES += -I/usr/include # Example
    LDFLAGS += -L/usr/lib    # Example
    LIBS += -lglfw -lGLEW -lGL # Example for Linux
endif

# Final flags
CXXFLAGS += $(DEFINES) $(INCLUDES)

# Rules
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS) $(LIBS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean
