#!/bin/bash

make

if [ -f "./duck_hunt_3d" ]; then
    echo "--- Iniciando Duck Hunt 3D ---"
    ./duck_hunt_3d
else
    echo "Error: El binario no se construyó correctamente."
fi