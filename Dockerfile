FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

#GLM and SOIL2 are on /lib
RUN apt-get update && apt-get install -y \ 
    #Compile .cpp files
    build-essential \
    cmake \
    pkg-config \
    #OpenGL
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    #GLEW
    libglew-dev \
    #GLFW
    libglfw3-dev \
    #Assimp
    libassimp-dev \
    zlib1g-dev \
    #Drivers
    libglvnd0 \
    #System interface (X11)
    libx11-dev \
    libxcursor-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxi-dev \
    libxxf86vm-dev \
    #Cleanup
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

CMD ["./docker_entrypoint.sh"]
