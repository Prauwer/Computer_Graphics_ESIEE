# Makefile multiplateforme

CC = g++
CFLAGS = -Wall -std=c++11
SRC = main.cpp GLShader.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = main

# Détection automatique du système d'exploitation
ifeq ($(OS),Windows_NT)
	# Paramètres pour Windows
	TARGET = toto
	LDFLAGS = -lglfw3 -lglew32 -lopengl32
else
	# Paramètres pour macOS
	CFLAGS += -DGL_SILENCE_DEPRECATION
	IFLAGS = -Iglfw-3.4.bin.MACOS/include/
	LDFLAGS = -Lglfw-3.4.bin.MACOS/lib-arm64 -framework OpenGL -lglfw3 -framework cocoa -framework IOKit
endif

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(IFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean