<div align="center">
  <h1>🦆 Duck Hunt 3D</h1>
  <p><b>Una reimaginación en 3D del clásico de NES utilizando OpenGL Moderno</b></p>
</div>

---

## 🎮 Sobre el Proyecto

Este proyecto es una reinterpretación tridimensional e interactiva del icónico videojuego *Duck Hunt*[cite: 14]. Desarrollado como proyecto final para la materia de Laboratorio de Computación Gráfica [cite: 1], el motor renderiza un entorno inmersivo utilizando **OpenGL moderno (versión 3.3+)**[cite: 10].

Para resolver los límites del mapa de manera creativa, el escenario se construyó como un domo curvo cerrado de 360 grados. El jugador puede moverse libremente en un claro de tierra central, mientras que los objetivos (los patos) orbitan estratégicamente en un anillo de pasto perimetral.

## ✨ Características Principales

* 🎥 **Sistema de Cámaras Dual:** * **Cámara FPS:** Vista en primera persona para recorrer el escenario, apuntar y disparar[cite: 24].
* **Cámara Cenital (3ra Persona):** Una vista de pájaro desde el cielo que permite visualizar el mapa completo y ubicar el modelo 3D del cazador en el mundo[cite: 25].
* 🐕 **Modelado y Animación:** Uso de modelado jerárquico [cite: 18] para los personajes. Incluye animaciones mediante transformaciones geométricas (para el vuelo circular de los patos) y animaciones por *keyframes* para las interacciones del perro sabueso[cite: 31, 32].
* 💡 **Renderizado Avanzado:** Implementación de texturizado detallado y sistemas de iluminación básica y avanzada[cite: 22].

## 🛠️ Entorno de Desarrollo y Tecnologías

El proyecto está diseñado para ser completamente independiente de extensiones de IDEs, optimizado para compilarse nativamente en un entorno **Linux (Ubuntu) a través de WSL2 en Windows 11**.

* **Lenguaje principal:** C++
* **API Gráfica:** OpenGL 3.3+
* **Librerías principales:** GLFW, GLEW, GLM, Assimp.

## 🚀 Instalación y Compilación

**1. Preparar el entorno (WSL2/Ubuntu):**
Asegúrate de tener instaladas las paqueterías necesarias ejecutando el siguiente comando:

```bash
sudo apt update
sudo apt install build-essential libglfw3-dev libglew-dev libglm-dev libassimp-dev xorg-dev
```

2. Ejecución rápida:
El proyecto cuenta con un Makefile robusto. Para compilar el código (solo si hay cambios) y ejecutar el programa automáticamente, simplemente corre:

```bash
make run
```

🕹️ Controles
- W, A, S, D: Caminar (movimiento restringido a la zona de tierra central).
- Mouse: Mover la mira y girar la cámara.
- Click Izquierdo: Disparar.
- Tecla C: Alternar entre cámara FPS y cámara Cenital.