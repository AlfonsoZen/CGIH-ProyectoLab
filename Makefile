# Makefile for Duck Hunt 3D

CXX = g++
CXXFLAGS = -std=c++11 -Wall -DGLEW_STATIC -fpermissive \
           -Iinclude \
           -Ilib/glm \
           -Ilib/SOIL2 \
           -Ilib

LDFLAGS = -lGL -lglfw -lGLEW -lassimp -lz

# Source files
SRC_DIR = src
SRC = $(SRC_DIR)/main.cpp $(SRC_DIR)/shader.cpp $(SRC_DIR)/camera.cpp $(SRC_DIR)/model.cpp

# SOIL2 source files
SOIL_DIR = lib/SOIL2
SOIL_SRC = $(SOIL_DIR)/etc1_utils.c $(SOIL_DIR)/image_DXT.c $(SOIL_DIR)/image_helper.c $(SOIL_DIR)/SOIL2.c

EXEC = duck_hunt_3d

.PHONY: all clean run

all: $(EXEC)

$(EXEC): $(SRC) $(SOIL_SRC)
	$(CXX) $(CXXFLAGS) -o $(EXEC) $(SRC) $(SOIL_SRC) $(LDFLAGS)

run: $(EXEC)
	@echo "--- Running Duck Hunt 3D ---"
	./$(EXEC)

clean:
	rm -f $(EXEC)
