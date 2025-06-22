# Makefile pour le projet ESIEE_Computer_Graphics
# Gère la compilation pour macOS (via Homebrew) et Windows (via MinGW).

# --- Configuration du Compilateur ---
CXX = g++
CXXFLAGS = -Wall -std=c++11

# --- Configuration des Fichiers ---
# Ajout des fichiers sources d'ImGui
SRCS = main.cpp GLShader.cpp \
       libs/imgui/imgui.cpp \
       libs/imgui/imgui_draw.cpp \
       libs/imgui/imgui_widgets.cpp \
       libs/imgui/imgui_tables.cpp \
       libs/imgui/imgui_impl_glfw.cpp \
       libs/imgui/imgui_impl_opengl3.cpp

OBJS = $(SRCS:.cpp=.o)
EXECUTABLE = ESIEE_Computer_Graphics

# --- Variables de Compilation (spécifiques à l'OS) ---
LDFLAGS = # Flags pour l'éditeur de liens
LIBS =    # Bibliothèques à lier
INCLUDES = -Ilibs -Ilibs/imgui # Ajout de libs/imgui pour les includes d'ImGui
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
    # --- Ajout pour ImGui (macOS) ---
    DEFINES += -DIMGUI_IMPL_OPENGL_LOADER_GL3W # macOS utilise souvent GL3W, mais GLEW peut aussi être utilisé.
                                              # Si vous utilisez GLEW sur macOS, remplacez par -DIMGUI_IMPL_OPENGL_LOADER_GLEW
                                              # ou si vous n'avez pas de chargeur explicite (ex: Core Profile directement), laissez vide et ImGui fera le nécessaire.
                                              # Pour la compatibilité cross-platform, nous allons cibler GLEW.
    # --------------------------------

# Si l'OS est Windows (MinGW)
else ifeq ($(findstring MINGW,$(OS_NAME)),MINGW)
    DEFINES += -D_WIN32
    INCLUDES += -Ilibs/glfw/include -Ilibs/glew/include -Ilibs/imgui # Ajout de libs/imgui
    LDFLAGS += -Llibs/glfw/lib-mingw-w64 -Llibs/glew/lib/Release/x64
    LIBS += -lglew32 -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32
    EXECUTABLE := $(EXECUTABLE).exe
    # --- Ajout pour ImGui (Windows) ---
    DEFINES += -DIMGUI_IMPL_OPENGL_LOADER_GLEW # Indique à ImGui d'utiliser GLEW
    # -----------------------------------

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