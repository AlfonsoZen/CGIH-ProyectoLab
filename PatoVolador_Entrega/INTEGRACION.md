# Pato Volador - Guía de Integración

**Equipo:** OpenGL · **Versión:** 1.0
**Autor del módulo:** Bruno
**Fecha:** Mayo 2026

---

## Resumen

Este paquete contiene un **modelo de pato animado en 3D** (FBX con esqueleto y aleteo de alas), junto con todo el código C++/OpenGL necesario para integrarlo a una escena existente en el proyecto.

### ¿Qué hace la animación?

El pato hace un loop infinito así:

1. Avanza recto a lo largo del eje **Z** (4 segundos a velocidad 1.5)
2. Llega al extremo, se detiene y **gira 180° sobre su propio eje** (1.8 seg, con easing suave)
3. Avanza de regreso hacia el otro extremo (mirando en la nueva dirección — siempre de frente)
4. Llega al otro extremo, gira 180° otra vez, vuelve a empezar

Siempre **mira hacia donde se mueve**. Nunca de espaldas. Sin curvas ni desviaciones.

---

## Contenido de esta carpeta

```
PatoVolador_Entrega/
├── README.md                       Resumen rápido
├── INTEGRACION.md                  Esta guía
├── codigo/
│   ├── modelAnim.h                 Cargador de modelos animados (FBX con huesos)
│   ├── meshAnim.h                  Estructura de mesh con bone weights
│   ├── snippet_pato.cpp            Código a copiar/pegar en main .cpp
│   └── shaders/
│       ├── anim.vs                 Vertex shader (calcula transformaciones de huesos)
│       └── anim.frag               Fragment shader (textura + iluminación)
└── modelo/
    ├── Pato.fbx                    El modelo (45K vértices, 14 huesos, 909 KB)
    └── texture_20250901.png        Textura del pato (1024×1024, 2 MB)
```

---

## Pasos de integración

### Paso 1 — Copiar archivos a sus rutas

Asumiendo que el proyecto del equipo está en alguna carpeta `MiProyecto/`:

| Archivo de esta carpeta | Va en |
|---|---|
| `codigo/modelAnim.h` | `MiProyecto/MiProyecto/modelAnim.h` (junto a sus otros `.h`) |
| `codigo/meshAnim.h` | `MiProyecto/MiProyecto/meshAnim.h` |
| `codigo/shaders/anim.vs` | `MiProyecto/MiProyecto/Shader/anim.vs` |
| `codigo/shaders/anim.frag` | `MiProyecto/MiProyecto/Shader/anim.frag` |
| `modelo/Pato.fbx` | `MiProyecto/MiProyecto/Models/Pato.fbx` |
| `modelo/texture_20250901.png` | `MiProyecto/MiProyecto/Models/texture_20250901.png` |

### Paso 2 — Verificar dependencias del proyecto

**Estándar de C++ requerido: mínimo C++17** (el código usa `auto&`, `std::min/max` con vectores, range-based for con structured bindings implícitos, etc.). En el `.vcxproj`, asegurarse de tener:

```xml
<LanguageStandard>stdcpp17</LanguageStandard>
```
o superior (`stdcpp20` también funciona).

`modelAnim.h` requiere **Assimp** (para leer FBX). En el `.vcxproj` debe estar:

- **Include**: `External/assimp/include` en `AdditionalIncludeDirectories`
- **Lib**: `External/assimp/lib` en `AdditionalLibraryDirectories`
- **Linker**: `assimp-vc140-mt.lib` en `AdditionalDependencies`
- **DLL**: `assimp-vc140-mt.dll` debe estar junto al `.exe` (en `Debug/` o `Release/`)

> Si su proyecto no tiene Assimp configurado, descárguenlo de [github.com/assimp/assimp](https://github.com/assimp/assimp) y compílenlo con CMake, o usen vcpkg: `vcpkg install assimp:x86-windows`.

También requiere los headers ya existentes del proyecto:
- `Mesh.h` (debe definir `struct Vertex` con AL MENOS `Position`, `Normal`, `TexCoords`)
- `Model.h` (debe definir `TextureFromFile` y `struct Texture`)
- `Shader.h` (clase `Shader` con miembro `Program` y método `Use()`)

> **Importante sobre el tipo de `Texture.path`:** este código asume que `Texture.path` es `aiString` (versión común). Si en su proyecto es `std::string`, ver la sección de "Solución de problemas" para el ajuste de una línea en `modelAnim.h`.

### Paso 3 — Registrar los nuevos archivos en el `.vcxproj`

Abrir `MiProyecto.vcxproj` (con un editor de texto) y agregar dentro de los `<ItemGroup>` correspondientes:

```xml
<ItemGroup>
  <!-- ...otros ClInclude... -->
  <ClInclude Include="modelAnim.h" />
  <ClInclude Include="meshAnim.h" />
</ItemGroup>

<ItemGroup>
  <!-- ...otros None Include de shaders... -->
  <None Include="Shader\anim.vs" />
  <None Include="Shader\anim.frag" />
</ItemGroup>
```

### Paso 4 — Pegar el código en el `.cpp` principal

Abrir `codigo/snippet_pato.cpp` de esta carpeta. Tiene 4 bloques marcados:

1. **`#include "modelAnim.h"`** — pegarlo arriba con los otros includes
2. **Variables globales** — pegar junto a las demás variables globales del proyecto
3. **Carga del modelo y shader** — pegar dentro de `main()`, después de inicializar GLEW y antes del game loop
4. **Bloque de dibujo** — pegar dentro del game loop, antes de `glfwSwapBuffers`

> ⚠️ Asegurarse de que dentro del game loop existan las variables `view`, `projection`, `camera` y `deltaTime` antes del bloque del pato. Si el proyecto las llama distinto, ajustar los nombres.

### Paso 5 — Compilar y correr

- **Visual Studio:** `Ctrl+Shift+B` (Build) → `F5` (Run)
- **Si hay errores:** revisar la sección de "Solución de problemas" más abajo
- **Si compila pero no ven el pato:** ajustar `trajectoryCenter` y/o `duckSpeed` para que el recorrido caiga dentro de la cámara del escenario

---

## Cómo verificar que funciona

Al correr, deberían ver en la **consola** (ventana negra que aparece junto a la de OpenGL) algo similar a esto (los valores exactos varían):

```
>>> Cargando Pato.fbx ...
scene->HasAnimations() 1: 1
scene->mNumMeshes 1: 1
scene->mAnimations[0]->mNumChannels 1: 16
scene->mAnimations[0]->mDuration 1: 123
scene->mAnimations[0]->mTicksPerSecond 1: 24

                name nodes :
RootNode
Armature
Bone
Bone.001
... (lista de huesos)

bones: 14 vertices: 44904
>>> [Pato] FBX pidio: 'Pato.fbm\texture_20250901' -> basename: 'texture_20250901'
>>> [Pato] encontrado en: 'Models/texture_20250901.png'
>>> [Pato] textura id=10 (tipo: texture_diffuse)
>>> [Pato] bbox min = (...)
>>> [Pato] bbox max = (...)
>>> [Pato] bbox center = (...)
>>> [Pato] centroide (usado como pivote) = (...)
>>> [Pato] centro ANIMADO (t=0) sample=N
>>> [Pato]   centroide animado = (...)
>>> [Pato]   bbox animado min = (...)
>>> [Pato]   bbox animado max = (...)
>>> [Pato]   USADO como pivote: bbox center animado = (...)
>>> Pato listo
```

> Las líneas que empiezan con `>>>` son diagnósticos de carga del modelo y la textura. Si quieren limpiar la consola en producción, pueden comentar los `std::cout` en `modelAnim.h` que tienen el prefijo `>>> [Pato]`.

Y en la ventana del juego: el pato amarillo aleteando, avanzando recto, girando 180°, regresando.

---

## Parámetros ajustables

Todos están al inicio del bloque del pato en el `.cpp`. Cambiarlos no requiere tocar nada más.

| Parámetro | Default | Para qué sirve |
|---|---|---|
| `duckSpeed` | `1.5` | velocidad de avance (m/s) |
| `zStart` / `zEnd` | `-5.0` / `+5.0` | extremos del recorrido en eje Z |
| `spinTime` | `1.8` | duración del giro 180° (más alto = giro más lento y suave) |
| `duckYawOffset` | `0.0` | ajuste si el modelo apunta hacia otro lado (probar 90, 180, -90) |
| `trajectoryCenter` | `(0, 1.6, 0)` | X y altura Y donde vuela el pato |
| `pivotOverride` | `(0,0,0)` | dejar así para usar pivote auto. Cambiar solo si el giro se barre |

### Adaptar al escenario del equipo

- **Si el escenario es más grande** → subir `zEnd` a 10 o 15
- **Si el pato se ve muy chico/grande** → en el `Draw`, cambiar `glm::scale(model, glm::vec3(0.012f))` (subir/bajar el `0.012`)
- **Si vuela muy bajo/alto** → cambiar el `1.6` en `trajectoryCenter`
- **Si lo quieren a otro lado** (no centrado) → cambiar el `0.0` X de `trajectoryCenter`
- **Cambiar la dirección del recorrido** (que vuele en X en vez de Z) → editar el bloque para usar `px` en vez de `pz`

---

## Solución de problemas comunes

### Error de compilación: `"Tangent" no es miembro de "Vertex"`

El `Vertex` del proyecto no tiene `Tangent`/`Bitangent`. Revisar `meshAnim.h` y `modelAnim.h` — ya quitamos esas referencias en esta versión, pero si el equipo tiene su propio `Mesh.h` con esos campos, no hay problema.

### Error: `'==': binario: no se encontró un operador para 'std::string'` (al usar este código)

El `Texture.path` del proyecto es `aiString`, y este código ya está adaptado a eso usando `std::strcmp(.C_Str(), ...)`. Si compila bien para ustedes, ignorar.

### Error: `no se puede convertir 'aiString' a 'std::string'` (al usar este código en SU proyecto)

Caso opuesto al anterior: SU proyecto define `Texture.path` como `std::string`. Solución — en `modelAnim.h`, cambiar la línea ~306:

```cpp
// ANTES (este código):
if (std::strcmp(textures_loaded[j].path.C_Str(), str.C_Str()) == 0)

// DESPUES (compatible con std::string):
if (textures_loaded[j].path == std::string(str.C_Str()))
```

Y la línea ~319:

```cpp
// ANTES:
texture.path = str;

// DESPUES:
texture.path = std::string(str.C_Str());
```

### El pato carga pero se ve negro

La textura no se está encontrando. Revisar la consola:

- Si dice `>>> [Pato] NO se encontro la textura...` → falta `texture_20250901.png` en `Models/`
- Si la consola NO muestra ningún `>>> [Pato] cargando textura ...` → el FBX no tiene material asignado

El shader tiene un **color de respaldo amarillo** que se activa si la textura no carga, así que aún sin textura debería verse el pato (de color amarillo plano).

### El pato no aparece en pantalla

- Probar mover `trajectoryCenter` a la posición del centro del escenario
- Subir la escala temporalmente a `1.0f` para confirmar que sí está cargado
- Revisar la consola por errores de carga del modelo

### El giro 180° hace que el pato barre todo el escenario

Esto es por un pivote mal calculado. Ya está resuelto con el cálculo automático del centro animado, pero si por alguna razón vuelve a fallar:

- Revisar la consola: el valor de `USADO como pivote` debe ser cercano al cuerpo del pato
- Si no, ajustar manualmente con `pivotOverride` (probar valores grandes como `(0, 50, 0)` y bajar/subir hasta que el giro se vea limpio)

### Carga muy lento (~30 segundos)

El FBX tiene demasiados vértices. Verificar:
- Que el modelo sea el optimizado (~45,000 vértices)
- Si tiene 1,000,000+ vértices, hay que abrirlo en Blender, aplicar un modificador **Decimate** con ratio 0.05, y re-exportar

### Quieren reducir aún más el peso de la textura

La textura ya viene optimizada a 1024×1024 (2 MB). Si quieren bajarla más:
- Convertirla a JPEG (~300-500 KB): abrir en cualquier editor (GIMP, Photoshop, Paint.NET) y guardar como `.jpg` con calidad 85%. El cargador del módulo prueba `.jpg` automáticamente, así que no rompería nada.
- Bajar a 512×512 si la textura no necesita más detalle visible al tamaño que se ve el pato en pantalla.

---

# 🤖 Prompt para IA (Claude / GPT / Cursor / Copilot)

Si el equipo prefiere usar una IA para integrar el módulo en lugar de hacerlo a mano, copiar y pegar este prompt:

---

```
Tengo un proyecto de OpenGL en C++ usando GLFW, GLEW, GLM, Assimp y SOIL2 (Visual
Studio en Windows). Quiero integrar un módulo externo: un PATO 3D animado que vuela
en una trayectoria de ida y vuelta con giro de 180° en cada extremo.

El módulo viene en una carpeta `PatoVolador_Entrega/` con la siguiente estructura:

PatoVolador_Entrega/
├── codigo/
│   ├── modelAnim.h            (cargador de FBX animado con esqueleto)
│   ├── meshAnim.h             (mesh con bone weights)
│   ├── snippet_pato.cpp       (código de ejemplo a copiar/pegar)
│   └── shaders/
│       ├── anim.vs            (vertex shader con bone transforms)
│       └── anim.frag          (fragment shader con luz direccional)
└── modelo/
    ├── Pato.fbx               (modelo, 45K vértices, 14 huesos)
    └── texture_20250901.png   (textura del pato)

Necesito que:

1) Copies todos los archivos a sus rutas correctas dentro de mi proyecto:
   - .h en la raíz del proyecto (junto a Camera.h, Model.h, Mesh.h, Shader.h)
   - .vs y .frag en la carpeta Shader/
   - Pato.fbx y texture_20250901.png en la carpeta Models/

2) Registres modelAnim.h, meshAnim.h, anim.vs y anim.frag en el archivo .vcxproj
   del proyecto (como ClInclude y None Include respectivamente).

3) Modifiques el .cpp principal del proyecto (el que tiene int main() y el game
   loop) integrando los 4 bloques marcados de snippet_pato.cpp:
   - Bloque 1: #include "modelAnim.h" arriba
   - Bloque 2: variables globales junto a las demás globales
   - Bloque 3: carga del Shader y ModelAnim dentro de main(), después de glewInit()
     y antes del game loop
   - Bloque 4: lógica de animación + dibujo dentro del game loop, antes del
     glfwSwapBuffers

4) Verifiques que el proyecto ya tiene Assimp configurado. Si no, agregar:
   - AdditionalIncludeDirectories: la ruta a assimp/include
   - AdditionalLibraryDirectories: la ruta a assimp/lib
   - AdditionalDependencies: assimp-vc140-mt.lib
   - El DLL assimp-vc140-mt.dll junto al .exe

5) Adaptes los nombres de variables en el bloque 4 (`view`, `projection`, `camera`,
   `deltaTime`) si en mi proyecto se llaman distinto.

6) Si los headers Mesh.h o Model.h del proyecto difieren de los esperados, hacer los
   ajustes necesarios en modelAnim.h y meshAnim.h SIN romper el código existente.

REQUISITOS IMPORTANTES:

- NO modifiques el comportamiento ni los modelos existentes del proyecto.
- El pato es un módulo añadido, debe coexistir con el resto del escenario.
- Si hay conflictos de shaders (ej. el lightingShader del proyecto usa otros
  uniforms), respetar la separación: el pato usa SU PROPIO animShader.
- Antes de modificar el .cpp principal, mostrarme los cambios propuestos.
- Después de la integración, decir cómo verificar que funciona (qué buscar en la
  consola al correr).

PARÁMETROS QUE PUEDO QUERER AJUSTAR DESPUÉS:

- duckSpeed (velocidad)
- zStart / zEnd (largo del recorrido)
- spinTime (duración del giro)
- trajectoryCenter (posición y altura del pato)
- escala 0.012f (tamaño del pato)

Estos parámetros están todos juntos al inicio del bloque, listos para tweakearlos.
```

---

## Notas finales

- La textura tiene un **color de respaldo amarillo patito** en el shader, así que aún si la textura falla, el pato es visible.
- El cargador de texturas es **robusto a las rutas extrañas** que Blender pone en el FBX (ej. `Pato.fbm\nombre` sin extensión) — prueba múltiples extensiones automáticamente.
- El **pivote del giro** se calcula automáticamente del modelo animado, así que no tienen que tocar nada para que el giro se vea bien.
- Si encuentran bugs o quieren agregar features (más patos, segundo recorrido, sonido al aletear, etc.), avísenme.

---

## Cómo convertir esta guía a PDF

**Opción 1 (la más fácil) — Visual Studio Code:**
1. Instalar la extensión **"Markdown PDF"** (de yzane)
2. Abrir este archivo `INTEGRACION.md`
3. `Ctrl+Shift+P` → escribir `Markdown PDF: Export (pdf)` → Enter
4. Se genera `INTEGRACION.pdf` en esta misma carpeta

**Opción 2 — Pandoc** (si lo tienen instalado):
```bash
pandoc INTEGRACION.md -o INTEGRACION.pdf
```

**Opción 3 — Online:**
Subir el archivo a [https://md2pdf.netlify.app](https://md2pdf.netlify.app) o cualquier conversor MD→PDF.

**Opción 4 — Word:**
Abrir el `.md` en Word → Archivo → Guardar como → PDF.
