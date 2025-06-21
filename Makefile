# Makefile pour le projet ESIEE_Computer_Graphics
# Gère la compilation pour macOS (via Homebrew) et Windows (via MinGW).

# --- Configuration du Compilateur ---
CXX = g++
CXXFLAGS = -Wall -std=c++11

# --- Configuration des Fichiers ---
SRCS = main.cpp GLShader.cpp
OBJS = $(SRCS:.cpp=.o)
EXECUTABLE = ESIEE_Computer_Graphics

# --- Variables de Compilation (spécifiques à l'OS) ---
LDFLAGS = # Flags pour l'éditeur de liens
LIBS =    # Bibliothèques à lier
INCLUDES = -Ilibs # Chemins d'inclusion (pour tiny_obj_loader.h, etc.)
DEFINES =  # Macros de préprocesseur

# --- Détection de l'OS ---
OS_NAME = $(shell uname -s)

# Si l'OS est macOS (Darwin)
ifeq ($(OS_NAME),Darwin)
    DEFINES += -DGL_SILENCE_DEPRECATION
    INCLUDES += -I/opt/homebrew/include
    LDFLAGS += -L/opt/homebrew/lib
    LIBS += -lglfw
    LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

# Si l'OS est Windows (MinGW)
else ifeq ($(findstring MINGW,$(OS_NAME)),MINGW)
    DEFINES += -D_WIN32
    INCLUDES += -Ilibs/glfw/include -Ilibs/glew/include
    LDFLAGS += -Llibs/glfw/lib-mingw-w64 -Llibs/glew/lib/Release/x64
    LIBS += -lglew32 -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32
    EXECUTABLE := $(EXECUTABLE).exe

endif

# --- Règles de Compilation ---

# Cible par défaut
all: $(EXECUTABLE)

# Règle pour lier l'exécutable final
$(EXECUTABLE): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LIBS)

# Règle pour compiler les fichiers source en fichiers objet
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

# Règle pour nettoyer les fichiers générés
clean:
	rm -f $(OBJS) $(EXECUTABLE)

# Cibles non-associées à des fichiers
.PHONY: all clean
