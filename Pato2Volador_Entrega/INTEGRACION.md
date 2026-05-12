# Pato 2 Volador — Guia de Integracion

**Equipo:** OpenGL · **Version:** 1.0
**Fecha:** Mayo 2026

---

## Resumen

Este paquete contiene un **segundo modelo de pato animado en 3D** (FBX con esqueleto, animacion de aleteo), que vuela en **trayectoria cuadrada** con derrape suave y banking en cada esquina. Es la misma animacion que el modulo del Aguila pero con otro modelo.

### Que hace la animacion

1. El pato2 recorre los 4 lados de un cuadrado de 16x16 m a velocidad constante (5 m/s)
2. En cada esquina hace un giro de 90 grados con:
   - **Curva bezier** que redondea la esquina (no es un giro punto)
   - **Easing suave** (smoothstep) para el cambio de direccion
   - **Banking lateral** (ala interior baja al girar)
3. Loop infinito, siempre mirando hacia adelante

---

## Contenido de esta carpeta

```
Pato2Volador_Entrega/
├── README.md                   Resumen rapido
├── INTEGRACION.md              Esta guia
├── codigo/
│   ├── modelAnim.h             v1.1 — soporta texturas embebidas en FBX
│   ├── meshAnim.h              mesh con bone weights
│   ├── snippet_pato2.cpp       codigo a copiar/pegar
│   └── shaders/
│       ├── anim.vs
│       └── anim.frag
└── modelo/
    └── Pato2.fbx               modelo con textura EMBEBIDA dentro del FBX
```

---

## Pasos de integracion

### Paso 1 — Copiar archivos a sus rutas

Asumiendo proyecto en `MiProyecto/`:

| Archivo de esta carpeta | Va en |
|---|---|
| `codigo/modelAnim.h` | `MiProyecto/MiProyecto/modelAnim.h` |
| `codigo/meshAnim.h` | `MiProyecto/MiProyecto/meshAnim.h` |
| `codigo/shaders/anim.vs` | `MiProyecto/MiProyecto/Shader/anim.vs` |
| `codigo/shaders/anim.frag` | `MiProyecto/MiProyecto/Shader/anim.frag` |
| `modelo/Pato2.fbx` | `MiProyecto/MiProyecto/Models/Pato2.fbx` |

> **No hay archivos de textura sueltos** — la textura del pato2 esta dentro del propio `.fbx`. El cargador `modelAnim.h v1.1+` la extrae automaticamente.

> Si ya tienen integrado el modulo del Pato Volador o del Aguila Voladora con `modelAnim.h` **anterior**, **reemplazar esa version** por la de este paquete. Es 100% compatible con los modelos anteriores y agrega soporte de texturas embebidas.

### Paso 2 — Verificar dependencias del proyecto

**Estandar de C++ requerido: minimo C++17.**

`modelAnim.h` requiere:
- **Assimp** (para leer FBX). En el `.vcxproj`:
  - Include: `External/assimp/include` en `AdditionalIncludeDirectories`
  - Lib: `External/assimp/lib` en `AdditionalLibraryDirectories`
  - Linker: `assimp-vc140-mt.lib` en `AdditionalDependencies`
  - DLL: `assimp-vc140-mt.dll` junto al `.exe`
- **SOIL2** (para cargar imagenes desde memoria — texturas embebidas). Tipicamente ya esta en el proyecto si tienen `Model.h` funcionando con texturas.

> Si el proyecto ya integra el modulo del Pato o del Aguila, las dependencias ya estan listas.

Tambien requiere los headers existentes:
- `Mesh.h` (`struct Vertex` con AL MENOS Position, Normal, TexCoords)
- `Model.h` (define `TextureFromFile` y `struct Texture`)
- `Shader.h`

### Paso 3 — Registrar archivos en el `.vcxproj`

Si NO tienen integrado ningun modulo previo del proyecto, agregar al `.vcxproj`:

```xml
<ItemGroup>
  <ClInclude Include="modelAnim.h" />
  <ClInclude Include="meshAnim.h" />
</ItemGroup>

<ItemGroup>
  <None Include="Shader\anim.vs" />
  <None Include="Shader\anim.frag" />
</ItemGroup>
```

> Si ya integraron Pato o Aguila, saltar este paso.

### Paso 4 — Pegar el codigo en el .cpp principal

Abrir `codigo/snippet_pato2.cpp`. Tiene 4 bloques marcados:

1. **`#include "modelAnim.h"`** — pegarlo arriba con los includes
2. **Variables globales** — pegar junto a las demas globales (los nombres tienen prefijo `p2_` para no chocar con otros modulos)
3. **Carga del modelo y shader** — dentro de `main()`, despues de `glewInit()` y antes del game loop.
   > Si ya tienen el `animShader` cargado (por integracion del Pato o Aguila), **NO redeclararlo**. Solo cargar el `Pato2` y pasarle el shader existente.
4. **Bloque de trayectoria + dibujo** — dentro del game loop, antes de `glfwSwapBuffers`.
   > Si tienen el Aguila tambien volando, ambos pueden coexistir (cada uno usa su propio bloque, comparten shader).

> Asegurarse de que existan `view`, `projection`, `camera` y `deltaTime` antes del bloque.

### Paso 5 — Compilar y correr

- `Ctrl+Shift+B` (Build) → `F5` (Run)
- Si compila pero no se ve el pato2: ajustar `p2_trajectoryCenter` para que el cuadrado caiga frente a la camara
- Si sale chiquito o gigante: ajustar `p2_scale`

---

## Como verificar que funciona

En la consola al cargar:

```
>>> Cargando Pato2.fbx ...
scene->HasAnimations() 1: 1
scene->mNumMeshes 1: 1
...
bones: 13 vertices: 44718
>>> [Pato] FBX pidio: '*0' -> basename: '*0'
>>> [Pato] textura embebida #0 hint='png' size=N x M
>>> [Pato] embebida comprimida cargada, id=N
>>> Pato2 listo
```

La linea **`textura embebida #0 ... cargada`** confirma que la textura del FBX fue extraida correctamente.

En la ventana del juego: pato2 aleteando, volando en cuadrado, girando con derrape suave en cada esquina.

---

## Parametros ajustables

### Mas comunes

| Parametro | Default | Para que sirve |
|---|---|---|
| `p2_sideLength` | `16.0` | tamanio del cuadrado (m) |
| `p2_centerY` | `6.0` | **altura de vuelo** |
| `p2_speed` | `5.0` | velocidad (m/s) |
| `p2_scale` | `0.01` | tamanio del modelo |
| `p2_trajectoryCenter` | `(0,0,0)` | posicion X,Z del centro del cuadrado |

### Giro y derrape

| Parametro | Default | Para que sirve |
|---|---|---|
| `p2_spinTime` | `0.7` | segundos del giro de 90 |
| `p2_cornerDrift` | `1.8` | metros de "redondeo" en cada esquina (0 = giro perfecto en punto) |
| `p2_maxBankDeg` | `22.0` | inclinacion lateral maxima (grados) |

### Ajustes del modelo (normalmente no tocar)

| Parametro | Default | Para que sirve |
|---|---|---|
| `p2_axisFixYaw` | `0.0` | probar 90/180/270 si el pato vuela de lado |
| `p2_axisFixPitch` | `0.0` | corregir si sale acostado |
| `p2_axisFixRoll` | `0.0` | corregir si sale acostado de costado |
| `p2_rollSign` | `-1` | invertir si se inclina al lado contrario |

---

## Solucion de problemas

### El pato2 sale amarillo (sin textura)

Significa que el shader esta usando el color de respaldo porque no se cargo la textura. Posibles causas:
- **Esta usando una version vieja de `modelAnim.h`** que no soporta texturas embebidas. Reemplazarla por la de este paquete (v1.1).
- En la consola, si dice `>>> [Pato] FBX pidio: '*0'` seguido de `NO se encontro la textura`, confirma que el modelAnim.h es viejo.

### Vuela de lado / mal orientado

Probar valores 90, 180, 270 en `p2_axisFixYaw`.

### Se inclina al lado equivocado al girar

Cambiar `p2_rollSign` de `-1` a `+1`.

### No aparece en pantalla

- Verificar consola sin errores de carga
- Subir `p2_scale` temporalmente a `0.1` o `1.0`
- Cambiar `p2_trajectoryCenter` para que quede frente a la camara

### Error al compilar con `aiString` vs `std::string`

Ver `INTEGRACION.md` del Pato/Aguila — mismo workaround en `modelAnim.h`.

---

# Prompt para IA (Claude / GPT / Cursor / Copilot)

```
Tengo un proyecto OpenGL en C++ (Visual Studio en Windows) y quiero integrar
un modulo: un segundo PATO 3D animado (Pato2) que vuela en TRAYECTORIA CUADRADA
con derrape suave en cada esquina y banking lateral.

El modulo viene en `Pato2Volador_Entrega/`:

Pato2Volador_Entrega/
├── codigo/
│   ├── modelAnim.h               v1.1 - soporta texturas embebidas en FBX
│   ├── meshAnim.h
│   ├── snippet_pato2.cpp
│   └── shaders/
│       ├── anim.vs
│       └── anim.frag
└── modelo/
    └── Pato2.fbx                 con textura embebida adentro

Necesito que:

1) Copies los archivos a sus rutas:
   - .h a la raiz del proyecto
   - shaders a Shader/
   - Pato2.fbx a Models/
   (NO hay archivo de textura suelto — esta embebida en el .fbx)

2) Si el proyecto ya integra el modulo del Pato o del Aguila con un
   modelAnim.h anterior, REEMPLAZAR esa version por la de este paquete (v1.1).
   Es 100% compatible con los modelos anteriores y agrega soporte de texturas
   embebidas en FBX.

3) Registres modelAnim.h, meshAnim.h, anim.vs y anim.frag en el .vcxproj.
   SI ya estan registrados (por modulo previo), omitir.

4) Modifiques el .cpp principal integrando los 4 bloques de snippet_pato2.cpp:
   - Bloque 1: #include "modelAnim.h"
   - Bloque 2: variables globales (todas con prefijo p2_)
   - Bloque 3: carga del Pato2. SI YA existe `animShader` (por modulo previo),
     reusarlo en vez de redeclararlo.
   - Bloque 4: trayectoria + dibujo en el game loop, antes de glfwSwapBuffers

5) Verifiques que Assimp esta configurado. Si no:
   - Include: assimp/include
   - Lib: assimp/lib + assimp-vc140-mt.lib
   - DLL junto al .exe

6) Adaptes nombres si view, projection, camera, deltaTime se llaman distinto.

PARAMETROS QUE PUEDO QUERER AJUSTAR DESPUES:

- p2_sideLength, p2_centerY, p2_speed, p2_scale
- p2_cornerDrift (redondeo de esquinas)
- p2_trajectoryCenter (posicion del centro del cuadrado)
```

---

## Notas finales

- La textura va **embebida en el FBX**, asi que el equipo no tiene que andar copiando archivos sueltos. Solo el `.fbx` ya tiene todo.
- Este modulo es **compatible con los otros modulos** del proyecto (Pato Volador, Aguila Voladora) — pueden coexistir.
- El cargador `modelAnim.h v1.1` agrega soporte para texturas embebidas sin romper la compatibilidad con texturas externas.
