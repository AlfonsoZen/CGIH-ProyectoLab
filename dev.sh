#!/bin/bash

xhost +local:docker

IMAGE_NAME="duck_hunt_3d"

docker build -t $IMAGE_NAME .

docker run -it --rm \
    --env="DISPLAY=$DISPLAY" \
    --volume="/tmp/.X11-unix:/tmp/.X11-unix:rw" \
    --device /dev/dri:/dev/dri \
    -v $(pwd):/app \
    $IMAGE_NAME \
    ./docker_entrypoint.sh

xhost -local:docker
