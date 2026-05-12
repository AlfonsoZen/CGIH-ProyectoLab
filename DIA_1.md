# Día 1 — Iluminación Phong + Patos en vuelo + Vegetación
**Fecha:** 11 de mayo de 2026  
**Meta del día:** El escenario se ve como Duck Hunt. Hay patos volando en órbita circular con animación de alas, iluminación realista en todos los objetos 3D, y vegetación poblando el mapa.

---

## Contexto del estado actual

El proyecto renderiza: skybox, piso, zapper (HUD) y crosshair (HUD).  
Los shaders actuales (`shader.vs` / `shader.frag`) son **planos** — solo muestrean textura sin iluminación.  
Los shaders `default.vert` / `default.frag` **ya existen y están completos** (Phong con DirLight + 4 PointLights + SpotLight) pero **nunca se usan**. El `Mesh.h` ya extrae normales. Todo está listo para conectarse.

---

## Tarea 1 — Conectar el shader de iluminación Phong a los objetos 3D

### Archivos a modificar: `src/main.cpp`

El objetivo es usar **dos shaders** simultáneamente:
- `lightingShader` → `default.vert` + `default.frag` → skybox, piso, patos, perro, vegetación, cazador
- `hudShader` → `shader.vs` + `shader.frag` → zapper y crosshair (sin iluminación, en screen-space)

**1.1 — Declarar ambos shaders al inicio del `main()`, después de `glewInit()`:**

```cpp
// Shader de iluminación Phong (para todos los objetos 3D del mundo)
Shader lightingShader("shaders/default.vert", "shaders/default.frag");

// Shader plano (para HUD: zapper y crosshair)
Shader hudShader("shaders/shader.vs", "shaders/shader.frag");
```

**1.2 — Eliminar la línea existente:**
```cpp
Shader shader("shaders/shader.vs", "shaders/shader.frag");
```

**1.3 — En el game loop, configurar las matrices de proyección y vista UNA vez para el lightingShader:**

```cpp
lightingShader.Use();
glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "view"),       1, GL_FALSE, glm::value_ptr(view));
glUniform3fv(glGetUniformLocation(lightingShader.Program, "viewPos"),          1, glm::value_ptr(camera.position));
```

**1.4 — Configurar la luz direccional (sol) una vez por frame. Añadir esto justo después de activar `lightingShader`:**

```cpp
// --- Luz direccional: simula el sol de la tarde de Duck Hunt ---
glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.direction"), -0.4f, -1.0f, -0.5f);
glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"),    0.35f,  0.30f,  0.25f);
glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"),    0.85f,  0.75f,  0.60f);
glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.specular"),   0.4f,   0.4f,   0.4f);

// --- Desactivar las 4 point lights (no se usan en el exterior) ---
for (int i = 0; i < 4; i++) {
    std::string base = "pointLights[" + std::to_string(i) + "].";
    glUniform3f(glGetUniformLocation(lightingShader.Program, (base + "ambient").c_str()),  0.0f, 0.0f, 0.0f);
    glUniform3f(glGetUniformLocation(lightingShader.Program, (base + "diffuse").c_str()),  0.0f, 0.0f, 0.0f);
    glUniform3f(glGetUniformLocation(lightingShader.Program, (base + "specular").c_str()), 0.0f, 0.0f, 0.0f);
    glUniform1f(glGetUniformLocation(lightingShader.Program, (base + "constant").c_str()),  1.0f);
    glUniform1f(glGetUniformLocation(lightingShader.Program, (base + "linear").c_str()),    0.0f);
    glUniform1f(glGetUniformLocation(lightingShader.Program, (base + "quadratic").c_str()), 0.0f);
}

// --- Desactivar spotlight ---
glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.ambient"),  0.0f, 0.0f, 0.0f);
glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.diffuse"),  0.0f, 0.0f, 0.0f);
glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.specular"), 0.0f, 0.0f, 0.0f);
glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.constant"),  1.0f);
glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.linear"),    0.0f);
glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.quadratic"), 0.0f);

// Indicar que hay transparencia activa (para que el fragment shader haga discard en alfa < 0.1)
glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 1);
```

**1.5 — Dibujar el Skybox con `lightingShader` pero sin vista translacional** (elimina el movimiento de la cámara del skybox para que se quede "pegado"):

```cpp
// Skybox: usa la vista sin traslación para que nunca se llegue al borde
glDepthMask(GL_FALSE);
glDisable(GL_CULL_FACE);

glm::mat4 skyboxView = glm::mat4(glm::mat3(camera.GetViewMatrix())); // elimina traslación
glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(skyboxView));

glm::mat4 model = glm::mat4(1.0f);
model = glm::scale(model, glm::vec3(2.0f));
glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
glUniform1i(glGetUniformLocation(lightingShader.Program, "useTexture"), 1);
glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 0);
Skybox.Draw(lightingShader);

glDepthMask(GL_TRUE);
glEnable(GL_CULL_FACE);

// Restaurar la vista normal con traslación
glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 1);
```

**1.6 — Dibujar el Piso con `lightingShader`:**

```cpp
model = glm::mat4(1.0f);
model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
model = glm::scale(model, glm::vec3(100.0f, 1.0f, 100.0f));
glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
glUniform1i(glGetUniformLocation(lightingShader.Program, "useTexture"), 1);
// El uvScale no existe en default.frag; el piso usa coordenadas UV del modelo directamente.
// Si el piso queda sin tiling, habrá que ajustar la escala UV en el modelo o con una textura apropiada.
Piso.Draw(lightingShader);
```

**1.7 — El Zapper y el Crosshair siguen usando `hudShader` exactamente como están ahora.** Solo cambia el nombre de variable de `shader` a `hudShader` en esas secciones.

---

## Tarea 2 — Sistema de patos con animación esquelética real (FBX de Bruno)

### Contexto

El equipo ya tiene un sistema de animación esquelética completo en `PatoVolador_Entrega/`:
- `modelAnim.h` — carga FBX con huesos, interpola keyframes, calcula matrices de skinning
- `meshAnim.h` — mesh con bone weights (hasta 4 huesos por vértice)
- `anim.vs` / `anim.frag` — shaders que deforman los vértices en GPU según los huesos
- `Pato.fbx` — modelo con 14 huesos y animación de aleteo en loop

El aleteo es **automático**: `ModelAnim::Draw()` internamente llama `glfwGetTime()` y calcula el frame de animación correspondiente. Nosotros solo construimos la matriz `model` para posicionar y orientar cada pato en la órbita circular.

---

### Paso 2.0 — Actualizar el Makefile a C++17

El código de Bruno usa features de C++17. Abrir `Makefile` y cambiar:

```makefile
# ANTES:
CXXFLAGS = -std=c++11 -Wall -DGLEW_STATIC -fpermissive \

# DESPUÉS:
CXXFLAGS = -std=c++17 -Wall -DGLEW_STATIC -fpermissive \
```

---

### Paso 2.1 — Copiar los archivos del módulo a las rutas del proyecto

```bash
# Desde la raíz del proyecto:
cp PatoVolador_Entrega/codigo/modelAnim.h include/
cp PatoVolador_Entrega/codigo/meshAnim.h  include/
cp PatoVolador_Entrega/codigo/shaders/anim.vs   shaders/
cp PatoVolador_Entrega/codigo/shaders/anim.frag shaders/

# El FBX va junto a su textura dentro de assets/models/
# (modelAnim.h busca la textura en el mismo directorio que el FBX)
cp PatoVolador_Entrega/modelo/Pato.fbx                assets/models/
cp PatoVolador_Entrega/modelo/texture_20250901.png     assets/models/
```

> Si el amigo entregó **dos FBX** (uno por color de pato), copiar ambos:
> `PatoA.fbx` + su textura, y `PatoB.fbx` + su textura, ambos a `assets/models/`.

---

### Paso 2.2 — Añadir el include en `src/main.cpp`

```cpp
// Añadir junto a los otros includes al inicio del archivo:
#include "../include/modelAnim.h"
```

---

### Paso 2.3 — Variables globales nuevas (antes de `main()`)

```cpp
// ---- Sistema de patos ----
const int NUM_PATOS = 3;

enum EstadoPato { VOLANDO, CAYENDO, MUERTO };

struct Pato {
    float angulo;         // Ángulo actual en la órbita (radianes)
    float velocidad;      // Velocidad angular (rad/segundo)
    float altura;         // Altura Y de vuelo
    EstadoPato estado;
    float velocidadCaida; // Para la física de caída libre
};

const float RADIO_ORBITA = 20.0f; // Radio del anillo de pasto
const float ALTURA_VUELO =  6.0f;

Pato patos[NUM_PATOS];
```

---

### Paso 2.4 — Cargar modelo y shader animado (en `main()`, después de `glewInit()`)

```cpp
// Shader exclusivo para el pato animado (maneja bone transforms en GPU)
Shader animShader("shaders/anim.vs", "shaders/anim.frag");

// Un solo modelo de pato con animación esquelética y textura completa
std::cout << ">>> Cargando Pato..." << std::endl;
ModelAnim Pato("assets/models/Pato.fbx");
Pato.initShaders(animShader.Program);
std::cout << ">>> Pato listo" << std::endl;
```

> El pato cayendo **reutiliza el mismo modelo animado** — el FBX con textura, aplicando
> solo la física de caída a la posición Y. No se necesita Duck_Falling.glb.

---

### Paso 2.5 — Inicializar los patos (antes del game loop)

```cpp
// Distribuir los 3 patos equitativamente en la órbita
for (int i = 0; i < NUM_PATOS; i++) {
    patos[i].angulo        = glm::radians(120.0f * i); // 0°, 120°, 240°
    patos[i].velocidad     = 0.6f + i * 0.05f;
    patos[i].altura        = ALTURA_VUELO + i * 0.5f;
    patos[i].estado        = VOLANDO;
    patos[i].velocidadCaida = 0.0f;
}
```

---

### Paso 2.6 — Lógica de actualización en el game loop (antes del renderizado)

```cpp
// ---- Actualizar posición y estado de cada pato ----
for (int i = 0; i < NUM_PATOS; i++) {
    if (patos[i].estado == VOLANDO) {
        // Avanzar en la órbita circular
        patos[i].angulo += patos[i].velocidad * deltaTime;
        if (patos[i].angulo > glm::two_pi<float>())
            patos[i].angulo -= glm::two_pi<float>();

    } else if (patos[i].estado == CAYENDO) {
        // Caída libre con gravedad
        patos[i].velocidadCaida -= 15.0f * deltaTime;
        patos[i].altura += patos[i].velocidadCaida * deltaTime;
        if (patos[i].altura <= -1.0f)
            patos[i].estado = MUERTO;
    }
}
```

---

### Paso 2.7 — Renderizar los patos en el game loop

```cpp
// ---- Dibujar patos ----
// Configurar el shader animado una sola vez antes del loop de patos
animShader.Use();
glUniformMatrix4fv(glGetUniformLocation(animShader.Program, "view"),       1, GL_FALSE, glm::value_ptr(view));
glUniformMatrix4fv(glGetUniformLocation(animShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

// Luz direccional: igualar a la del lightingShader para coherencia visual
glUniform3f(glGetUniformLocation(animShader.Program, "viewPos"),
            camera.position.x, camera.position.y, camera.position.z);
glUniform3f(glGetUniformLocation(animShader.Program, "light.direction"), -0.4f, -1.0f, -0.5f);
glUniform3f(glGetUniformLocation(animShader.Program, "light.ambient"),    0.35f, 0.30f, 0.25f);
glUniform3f(glGetUniformLocation(animShader.Program, "light.diffuse"),    0.85f, 0.75f, 0.60f);
glUniform3f(glGetUniformLocation(animShader.Program, "light.specular"),   0.4f,  0.4f,  0.4f);
glUniform3f(glGetUniformLocation(animShader.Program, "material.specular"), 0.5f, 0.5f, 0.5f);
glUniform1f(glGetUniformLocation(animShader.Program, "material.shininess"), 32.0f);

for (int i = 0; i < NUM_PATOS; i++) {
    if (patos[i].estado == MUERTO) continue;

    float px = RADIO_ORBITA * cos(patos[i].angulo);
    float pz = RADIO_ORBITA * sin(patos[i].angulo);
    float py = patos[i].altura;

    // La tangente a la órbita circular es perpendicular al radio → pato siempre mira hacia adelante
    float yaw = patos[i].angulo + glm::half_pi<float>();

    // Misma matriz para VOLANDO y CAYENDO — el modelo animado se usa en ambos estados.
    // Cuando está cayendo, la posición Y baja por física pero el modelo (y el aleteo) es el mismo.
    glm::mat4 matPato = glm::mat4(1.0f);
    matPato = glm::translate(matPato, glm::vec3(px, py, pz));
    matPato = glm::rotate(matPato, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    matPato = glm::scale(matPato, glm::vec3(0.012f));        // ajustar según tamaño real del FBX
    matPato = glm::translate(matPato, -Pato.boundingCenter); // pivote sobre el centro del cuerpo

    glUniformMatrix4fv(glGetUniformLocation(animShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(matPato));
    // Draw() calcula bone transforms con glfwGetTime() → aleteo automático en loop
    Pato.Draw(animShader);
}
```

---

## Tarea 3 — Vegetación: arbustos, nubes y anillo de pasto

### Cargar modelos de vegetación (después de los modelos de patos)

```cpp
Model Arbusto((char*)"assets/models/Bush.glb");
Model Nube1((char*)"assets/models/Nube_1.glb");
Model Nube2((char*)"assets/models/Nube_2.glb");
Model Nube3((char*)"assets/models/Nube_3.glb");
Model PastoAnillo((char*)"assets/models/E_Grass.glb");
Model PastoAnilloRev((char*)"assets/models/E_GrassBackwards.glb");
```

### Definir posiciones de los arbustos (variables globales o constantes antes del main)

```cpp
// 8 arbustos distribuidos uniformemente en el anillo perimetral
const int NUM_ARBUSTOS = 8;
glm::vec3 posicionesArbustos[NUM_ARBUSTOS];

// Inicializar en main() antes del game loop:
for (int i = 0; i < NUM_ARBUSTOS; i++) {
    float angArbusto = glm::radians(360.0f / NUM_ARBUSTOS * i);
    posicionesArbustos[i] = glm::vec3(
        22.0f * cos(angArbusto),  // Radio ligeramente mayor que la órbita de patos
        -0.5f,
        22.0f * sin(angArbusto)
    );
}
```

### Renderizar vegetación en el game loop

```cpp
// ---- Dibujar arbustos ----
glUniform1i(glGetUniformLocation(lightingShader.Program, "useTexture"), 1);
for (int i = 0; i < NUM_ARBUSTOS; i++) {
    glm::mat4 matArbusto = glm::mat4(1.0f);
    matArbusto = glm::translate(matArbusto, posicionesArbustos[i]);
    matArbusto = glm::rotate(matArbusto, glm::radians(45.0f * i), glm::vec3(0.0f, 1.0f, 0.0f));
    matArbusto = glm::scale(matArbusto, glm::vec3(2.5f));
    glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(matArbusto));
    Arbusto.Draw(lightingShader);
}

// ---- Dibujar nubes (se mueven lentamente con el tiempo) ----
float tiempoActual = glfwGetTime();

Model* nubes[3] = { &Nube1, &Nube2, &Nube3 };
float offsetsNube[3][3] = {
    { 15.0f, 18.0f,  5.0f},
    {-10.0f, 20.0f, 12.0f},
    {  5.0f, 16.0f,-15.0f}
};
for (int i = 0; i < 3; i++) {
    glm::mat4 matNube = glm::mat4(1.0f);
    // Leve desplazamiento sinusoidal para simular movimiento de nubes
    float offsetX = sin(tiempoActual * 0.05f + i * 2.0f) * 3.0f;
    matNube = glm::translate(matNube, glm::vec3(offsetsNube[i][0] + offsetX, offsetsNube[i][1], offsetsNube[i][2]));
    matNube = glm::scale(matNube, glm::vec3(4.0f));
    glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(matNube));
    nubes[i]->Draw(lightingShader);
}

// ---- Dibujar anillo de pasto perimetral ----
// Se coloca un conjunto de segmentos de pasto alrededor del claro central
const int SEGMENTOS_PASTO = 12;
for (int i = 0; i < SEGMENTOS_PASTO; i++) {
    float angPasto = glm::radians(360.0f / SEGMENTOS_PASTO * i);
    glm::mat4 matPasto = glm::mat4(1.0f);
    matPasto = glm::translate(matPasto, glm::vec3(18.0f * cos(angPasto), -1.0f, 18.0f * sin(angPasto)));
    matPasto = glm::rotate(matPasto, angPasto, glm::vec3(0.0f, 1.0f, 0.0f));
    matPasto = glm::scale(matPasto, glm::vec3(3.0f));
    glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(matPasto));
    // Alternar entre los dos modelos de pasto para variedad visual
    if (i % 2 == 0)
        PastoAnillo.Draw(lightingShader);
    else
        PastoAnilloRev.Draw(lightingShader);
}
```

---

## Tarea 4 — Restricción de movimiento al claro central

### En la función `DoMovement()`, añadir al final de la función:

```cpp
// Restringir al jugador al círculo central de tierra (radio = 8 unidades)
const float RADIO_TIERRA = 8.0f;
glm::vec2 posXZ(camera.position.x, camera.position.z);
float distanciaAlCentro = glm::length(posXZ);
if (distanciaAlCentro > RADIO_TIERRA) {
    // Proyectar la posición de vuelta al borde del círculo
    posXZ = glm::normalize(posXZ) * RADIO_TIERRA;
    camera.position.x = posXZ.x;
    camera.position.z = posXZ.y;
}
```

---

## Verificación al final del Día 1

Al ejecutar `make run` se debe observar:

- [ ] El skybox y el piso tienen iluminación Phong (sombras y brillos visibles, no apariencia plana)
- [ ] 3 patos volando en órbita circular alrededor del jugador, sobre el anillo de pasto
- [ ] Los patos aletean con animación esquelética real (FBX de Bruno, aleteo fluido)
- [ ] 8 arbustos distribuidos en el perímetro
- [ ] 3 nubes flotando en el cielo con movimiento suave
- [ ] El anillo de pasto rodea el claro central
- [ ] El jugador no puede salir del claro central de tierra
- [ ] El zapper y el crosshair siguen funcionando correctamente como HUD

## Compilación

```bash
make run
```

Si hay errores de compilación relacionados con el shader `default.frag` y el uniform `uvScale`, es porque ese uniform no existe en `default.frag` (era exclusivo del shader plano). Eliminar cualquier llamada a `glUniform1f(..., "uvScale", ...)` cuando se use `lightingShader`.
