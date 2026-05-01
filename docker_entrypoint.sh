#!/bin/bash

make clean
make

if [ $? -eq 0 ]; then
    echo "Iniciando Duck Hunt 3D..."
    ./duck_hunt_3d
else
    echo "Error en la compilación."
    exit 1
fi
