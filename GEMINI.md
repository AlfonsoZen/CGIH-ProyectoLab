# Especificaciones de Desarrollo: Proyecto Final OpenGL "Duck Hunt 3D"

## Rol del Agente
Actúa como un desarrollador experto en C++ y gráficos por computadora usando OpenGL. Tu objetivo es generar el código fuente y la estructura de un proyecto final para la materia de Laboratorio de Computación Gráfica.

## Entorno de Desarrollo y Ejecución
* **Sistema Operativo:** Windows 11 utilizando WSL2 con una distribución de Ubuntu.
* **Dependencias:** Tienes total libertad para indicar los comandos `sudo apt install` necesarios para instalar librerías gráficas (como GLFW, GLEW, GLM, Assimp, etc.).
* **Compilación:** El proyecto no usará extensiones de IDE para compilar. Debes generar un archivo `Makefile` robusto. 
* **Ejecución:** El `Makefile` debe incluir un target explícito llamado `run` para que el comando `make run` compile (si hay cambios) y ejecute el programa automáticamente.
* **Estructura del Directorio:** Este proyecto estará ubicado en una carpeta que tiene como directorios hermanos las prácticas de laboratorio anteriores. Puedes asumir que el usuario tiene conocimientos de la estructura básica, pero tu código debe ser completamente autosuficiente.

## Requerimientos Técnicos Críticos
El código debe cumplir estrictamente con los siguientes lineamientos:
* **Tecnología Base:** Utilizar OpenGL moderno (versión 3.3 en adelante).
* **Visualización:** Implementar proyección en perspectiva, iluminación (básica y avanzada) y texturizado.
* **Modelado:** Uso de modelado jerárquico donde aplique. El escenario deberá incluir modelos adicionales, como enemigos y objetos de ambientación.
* **Sistema de Cámaras:** Es obligatorio implementar al menos dos tipos de cámara:
    1.  **Cámara Primera Persona (FPS):** Para recorrer el escenario y apuntar.
    2.  **Cámara Tercera Persona (Cenital):** Una vista desde el cielo hacia abajo. **Regla estricta:** Al activar esta cámara, se debe renderizar y mostrar el modelo 3D del personaje principal (el cazador) en el entorno.
* **Animación e Interacción:**
    * Navegación libre dentro del escenario usando teclado (WASD).
    * Animaciones básicas mediante transformaciones geométricas (ej. rotación/traslación de los patos).
    * Al menos una animación por keyframes (ej. el perro asomándose).

## Diseño del Escenario y Lógica del Juego
* **Mundo:** El mapa no es infinito. Debe ser un domo o cilindro cerrado a 360 grados con textura de cielo. 
* **Zonas:** El piso se divide en dos: un círculo central de "tierra" y un anillo perimetral de "pasto".
* **Movimiento del Jugador:** La navegación del jugador está restringida únicamente a la zona de tierra.
* **Comportamiento de los Patos:** Para evitar que salgan del mapa, los patos no vuelan en línea recta. Deben volar en un patrón circular (orbitando el centro del mapa) exclusivamente sobre la zona de pasto, permitiendo al jugador seguirlos con la vista y disparar.

## Estilo de Código y Documentación
* El código fuente debe estar excelentemente estructurado y comentado.
* **Nota Importante:** El usuario y su equipo deberán presentar este proyecto y realizar una demostración en vivo. Por lo tanto, **cada función, shader y bloque de lógica matemática o gráfica debe estar explicado con comentarios detallados en español** dentro del mismo código fuente para facilitar su estudio y comprensión.