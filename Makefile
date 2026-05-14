# Makefile for Duck Hunt 3D

CXX = g++
CXXFLAGS = -std=c++17 -Wall -DGLEW_STATIC -fpermissive \
           -Iinclude \
           -Ilib/glm \
           -Ilib/SOIL2 \
           -Ilib

ifeq ($(OS),Windows_NT)
    LDFLAGS = -lglfw3 -lglew32 -lassimp -lz -lopengl32 -lgdi32
    EXEC = duck_hunt_3d.exe
else
    LDFLAGS = -lGL -lglfw -lGLEW -lassimp -lz
    EXEC = duck_hunt_3d
endif

# Source files
SRC_DIR = src
SRC = $(SRC_DIR)/main.cpp $(SRC_DIR)/shader.cpp $(SRC_DIR)/camera.cpp $(SRC_DIR)/model.cpp

# SOIL2 source files
SOIL_DIR = lib/SOIL2
SOIL_SRC = $(SOIL_DIR)/etc1_utils.c $(SOIL_DIR)/image_DXT.c $(SOIL_DIR)/image_helper.c $(SOIL_DIR)/SOIL2.c

.PHONY: all clean run

all: $(EXEC)

HEADERS = $(wildcard include/*.h)

$(EXEC): $(SRC) $(SOIL_SRC) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $(EXEC) $(SRC) $(SOIL_SRC) $(LDFLAGS)

run: $(EXEC)
	@echo "--- Running Duck Hunt 3D ---"
	./$(EXEC)

clean:
	rm -f $(EXEC)
