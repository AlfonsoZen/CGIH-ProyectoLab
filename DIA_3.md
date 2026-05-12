# Día 3 — Sistema de rondas + Marcador + Perro reactivo + Pulido final
**Fecha:** 13 de mayo de 2026  
**Meta del día:** El proyecto está terminado y es presentable. Hay un sistema de rondas completo, marcador visual con modelos de números, el perro ríe cuando un pato escapa, dificultad progresiva, y toda la escena está integrada y estable.

---

## Prerrequisito

El Día 2 debe estar completo: disparo funcional, perro animado, cámara cenital con cazador visible, plumas al impactar. Si `make run` muestra todo lo anterior, continuar.

---

## Tarea 9 — Sistema de rondas y patos que escapan

### Concepto

Una **ronda** consiste en 3 patos. Cuando los 3 patos están en estado `MUERTO` o han **escapado** (volaron más de una vuelta completa sin ser disparados), la ronda termina. Si se derribaron patos, el perro los muestra; si alguno escapó, el perro ríe. Después de un breve pausa, comienza una nueva ronda con velocidad aumentada.

### Variables globales nuevas

```cpp
// ---- Sistema de rondas ----
int  rondaActual       = 1;
int  patosDerribadosRonda = 0; // Reinicia cada ronda
int  patosEscapadosRonda  = 0; // Reinicia cada ronda
bool rondaEnCurso      = true;
float timerPausaRonda  = 0.0f;  // Pausa entre rondas
bool enPausaRonda      = false;

// Velocidad base de los patos (aumenta con cada ronda)
float velocidadBaseRonda = 0.6f;

// Ángulos recorridos por pato (para detectar escape después de 2 órbitas)
float angulosRecorridos[NUM_PATOS] = { 0.0f, 0.0f, 0.0f };
```

### Detectar escape de pato (en el bloque de actualización de patos del Día 1)

En la sección `if (patos[i].estado == VOLANDO)`, añadir:

```cpp
// Contabilizar ángulo recorrido (para detección de escape)
angulosRecorridos[i] += patos[i].velocidad * deltaTime;

// Si el pato completó 2 órbitas sin ser disparado → escapó
if (angulosRecorridos[i] >= glm::two_pi<float>() * 2.0f) {
    patos[i].estado   = MUERTO; // Se retira de la escena
    patosEscapadosRonda++;

    // Activar risa del perro
    estadoPerro       = PERRO_RIENDO;
    timerPerro        = 0.0f;
}
```

### Función `iniciarNuevaRonda()` (definir antes de `main()`)

```cpp
void iniciarNuevaRonda() {
    rondaActual++;
    patosDerribadosRonda = 0;
    patosEscapadosRonda  = 0;
    enPausaRonda         = false;
    timerPausaRonda      = 0.0f;

    // Aumentar velocidad con cada ronda (máximo 2.5x la velocidad inicial)
    velocidadBaseRonda = glm::min(0.6f + (rondaActual - 1) * 0.15f, 1.5f);

    // Reiniciar los 3 patos con la nueva velocidad
    for (int i = 0; i < NUM_PATOS; i++) {
        patos[i].angulo          = glm::radians(120.0f * i);
        patos[i].velocidad       = velocidadBaseRonda + i * 0.05f;
        patos[i].altura          = ALTURA_VUELO + i * 0.5f;
        patos[i].estado          = VOLANDO;
        patos[i].timerAleteo     = 0.0f;
        patos[i].frameAleteo     = 0;
        patos[i].velocidadCaida  = 0.0f;
        angulosRecorridos[i]     = 0.0f;
    }

    // El perro vuelve a buscar
    estadoPerro          = PERRO_BUSCANDO;
    timerPerro           = 0.0f;
    frameBuscando        = 0;
    framePerroEncontrado = 0;
    patosEnRonda         = 0;
}
```

### Detectar fin de ronda en el game loop (antes de la sección de dibujo)

```cpp
// ---- Detectar fin de ronda ----
if (rondaEnCurso && !enPausaRonda) {
    // Contar cuántos patos ya no están volando
    int patosResueltos = 0;
    for (int i = 0; i < NUM_PATOS; i++) {
        if (patos[i].estado == MUERTO) patosResueltos++;
    }

    if (patosResueltos == NUM_PATOS) {
        // Todos los patos están resueltos → iniciar pausa antes de nueva ronda
        enPausaRonda    = true;
        timerPausaRonda = 0.0f;
    }
}

// ---- Contar pausa y lanzar nueva ronda ----
if (enPausaRonda) {
    timerPausaRonda += deltaTime;
    // Esperar 3 segundos antes de la siguiente ronda
    // (el perro termina su animación de mostrar/reír)
    if (timerPausaRonda >= 3.0f) {
        iniciarNuevaRonda();
    }
}
```

---

## Tarea 10 — Marcador visual con modelos de números

### Concepto

Se utilizan los modelos `One.glb`, `Zero.glb`, etc. que ya existen en `assets/models/`. El marcador se renderiza en **screen space** como el zapper: con `view = identity` y `projection = identity`, dibujando los dígitos en la esquina superior izquierda de la pantalla.

Cada dígito es un modelo que se posiciona con una transformación ajustada para que quede en esquina. Se muestran dos valores: **ronda actual** (arriba izquierda) y **patos derribados totales** (arriba derecha).

### Cargar modelos de dígitos

```cpp
// Dígitos del 0 al 8 (los modelos existentes en assets/models/)
Model Digito0((char*)"assets/models/Zero.glb");
Model Digito1((char*)"assets/models/One.glb");
Model Digito2((char*)"assets/models/StartGameText.003.glb");  // "2" si existe, si no usar placeholder
Model Digito3((char*)"assets/models/Three.glb");
Model Digito5((char*)"assets/models/Five.glb");
Model Digito8((char*)"assets/models/Eight.glb");

// Array de punteros (0-8; llenar con los disponibles, el resto apuntan a Digito0)
Model* digitoModels[9] = {
    &Digito0, &Digito1, &Digito2, &Digito0, // 0,1,2,3 — ajustar según modelos reales
    &Digito0, &Digito5, &Digito0, &Digito0,  // 4,5,6,7
    &Digito8                                  // 8
};
```

> **Nota práctica:** Verificar qué modelos de dígitos existen realmente con `ls assets/models/ | grep -E "^[A-Z]"`. Los disponibles confirmados son: Zero, One, Three, Five, Eight. Para los que falten, usar Digito0 como placeholder temporal o construir el dígito con texto desde los modelos `Text.glb` o `DuckseasonLogo.glb`.

### Función auxiliar para renderizar un dígito en HUD

```cpp
// Renderiza un modelo de dígito en posición de pantalla (NDC: -1 a 1)
// xPos, yPos: posición en espacio NDC de pantalla
// escala: tamaño del dígito en pantalla
void renderizarDigitoHUD(Model* digito, Shader& hudShader, float xPos, float yPos, float escala) {
    glm::mat4 matDigito = glm::mat4(1.0f);
    matDigito = glm::translate(matDigito, glm::vec3(xPos, yPos, -0.1f));
    matDigito = glm::scale(matDigito, glm::vec3(escala / ((float)SCREEN_WIDTH / SCREEN_HEIGHT), escala, escala));

    glUniformMatrix4fv(glGetUniformLocation(hudShader.Program, "model"),      1, GL_FALSE, glm::value_ptr(matDigito));
    glUniformMatrix4fv(glGetUniformLocation(hudShader.Program, "view"),       1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glUniformMatrix4fv(glGetUniformLocation(hudShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    digito->Draw(hudShader);
}
```

### Renderizar el marcador en el game loop (al final, después del crosshair)

```cpp
// ---- Marcador HUD (solo en cámara FPS) ----
if (!camaraCenital) {
    glDisable(GL_DEPTH_TEST);
    hudShader.Use();

    // Mostrar ronda actual (esquina superior izquierda)
    int rondaMostrar = glm::clamp(rondaActual, 0, 8);
    renderizarDigitoHUD(digitoModels[rondaMostrar], hudShader, -0.85f, 0.85f, 0.08f);

    // Mostrar patos derribados totales (esquina superior derecha)
    int derribadosMostrar = glm::clamp(patosDerribados, 0, 8);
    renderizarDigitoHUD(digitoModels[derribadosMostrar], hudShader, 0.80f, 0.85f, 0.08f);

    glEnable(GL_DEPTH_TEST);
}
```

---

## Tarea 11 — Perro que ríe cuando un pato escapa

La Tarea 9 ya activa `PERRO_RIENDO` cuando un pato escapa. Aquí se completa el renderizado y la duración:

### En el `switch` de renderizado del perro (Día 2), la rama `PERRO_RIENDO` ya existe. Verificar que funcione correctamente:

```cpp
case PERRO_RIENDO:
    // Ciclo de risa de 2 frames, 0.5s cada uno
    // Al terminar 2 segundos de risa, el perro vuelve a BUSCANDO
    if (timerPerro >= 2.0f) {
        estadoPerro   = PERRO_BUSCANDO;
        timerPerro    = 0.0f;
        frameBuscando = 0;
    }
    // Seleccionar frame según el timer (alterna cada 0.5s)
    if (fmod(timerPerro, 1.0f) < 0.5f)
        PerroRisa1.Draw(lightingShader);
    else
        PerroRisa2.Draw(lightingShader);
    break;
```

---

## Tarea 12 — Dificultad de Bar visual (modelo DifficultyBar)

El modelo `DifficulityBar.glb` puede usarse como indicador visual de la dificultad de la ronda, renderizado en HUD o en el mundo 3D como letrero.

### Renderizar el DifficultyBar en el mundo, posicionado frente al jugador como señal

```cpp
// Cargar (en sección de modelos):
Model BarDificultad((char*)"assets/models/DifficulityBar.glb");

// Renderizar en el juego (posición fija sobre el perro):
glm::mat4 matBar = glm::mat4(1.0f);
matBar = glm::translate(matBar, glm::vec3(0.0f, 2.5f, -7.5f)); // encima del perro
matBar = glm::scale(matBar, glm::vec3(1.0f + (rondaActual - 1) * 0.2f)); // crece con la dificultad
glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(matBar));
BarDificultad.Draw(lightingShader);
```

---

## Tarea 13 — Pulido visual: ajuste de iluminación y transparencia

### 13.1 — Verificar que los modelos GLB con transparencia se descarten correctamente

En `default.frag`, la línea:
```glsl
if(color.a < 0.1 && transparency == 1) discard;
```
ya existe. Asegurarse de que `transparency = 1` esté activo para todos los modelos que usan alfa (patos, perro, arbustos, plumas). Para el piso y el skybox, usar `transparency = 0` para evitar artefactos.

### 13.2 — Ajuste de los colores de luz para paleta Duck Hunt

Duck Hunt tiene una paleta cálida (tarde de otoño). Ajustar los valores de la DirLight para:

```cpp
// Paleta Duck Hunt: luz solar amarillo-naranja, sombras azul-púrpura suave
glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"),  0.30f, 0.25f, 0.20f);
glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"),  0.90f, 0.78f, 0.55f);
glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.specular"), 0.50f, 0.45f, 0.35f);
```

### 13.3 — Activar back-face culling solo cuando corresponda

Los modelos `.glb` de Blender suelen tener normales correctas. Activar `GL_CULL_FACE` globalmente pero desactivarlo para el skybox y los modelos de vegetación plana (pasto) que deben verse de ambos lados:

```cpp
// Al inicio del frame (antes de skybox):
glDisable(GL_CULL_FACE); // Skybox y elementos planos
// ... dibujar skybox ...

// Después del skybox y antes de los objetos sólidos:
glEnable(GL_CULL_FACE);
glCullFace(GL_BACK);
// ... dibujar piso, patos, perro, cazador ...

// Para pasto y plumas (geometría plana de doble cara):
glDisable(GL_CULL_FACE);
// ... dibujar pasto y plumas ...
glEnable(GL_CULL_FACE);
```

### 13.4 — Añadir título en pantalla con el modelo del logo de Duck Hunt

El modelo `DuckseasonLogo.glb` puede mostrarse brevemente al inicio:

```cpp
// Variable global:
float timerInicio = 0.0f;
bool mostrarLogo  = true;

// En el game loop:
if (mostrarLogo) {
    timerInicio += deltaTime;
    if (timerInicio > 4.0f) mostrarLogo = false;

    glDisable(GL_DEPTH_TEST);
    hudShader.Use();

    // Renderizar logo centrado en pantalla (HUD)
    // ... (similar a renderizarDigitoHUD pero centrado en 0,0) ...

    glEnable(GL_DEPTH_TEST);
}
```

---

## Tarea 14 — Limpieza de memoria y robustez

### 14.1 — Verificar que no haya modelos que fallen al cargar

Al inicio del proyecto añadir un chequeo básico. Los modelos `.glb` con Assimp **deben tener la extensión soportada**. Si algún modelo no carga (log de Assimp en consola), identificarlo y substituir con uno `.obj` equivalente.

Ejecutar y observar la consola:
```bash
make run 2>&1 | grep -i "error\|assimp\|failed"
```

### 14.2 — Evitar que el jump saque al jugador del claro

En el sistema de física del salto (`velocityY` / `gravity`), el jugador salta hacia arriba y la restricción XZ sigue activa. Verificar que la restricción del radio solo se aplique en XZ (no en Y):

```cpp
// Al final de DoMovement(), después de la restricción de radio:
// El clamp de Y se hace en el gameloop con groundHeight, no aquí.
// La restricción de radio ya está correcta porque solo toca .x y .z
```

### 14.3 — Limite de patos derribados mostrado en pantalla

El contador de `patosDerribados` crece indefinidamente. En `renderizarDigitoHUD`, ya se hace `glm::clamp(patosDerribados, 0, 8)`. Para rondas largas, considerar mostrar solo el puntaje de la ronda actual en lugar del total, o usar un sistema de decenas/unidades si el tiempo lo permite.

---

## Tarea 15 — Prueba completa de todos los controles y entregables

### Lista de controles a verificar

| Control | Comportamiento esperado |
|---------|------------------------|
| W A S D | Movimiento restringido al claro de tierra (radio 8) |
| Mouse | Gira la cámara libremente en FPS |
| Click izquierdo | Dispara; si apunta a un pato, lo derriba |
| Espacio | Salto con gravedad |
| C | Alterna FPS ↔ cenital; en cenital se ve el cazador, sin HUD |
| ESC | Cierra la aplicación |

### Lista de requisitos del Acta a verificar

| Requisito del Acta | ¿Cumplido? |
|-------------------|-----------|
| Escenario 3D inspirado en Duck Hunt (campo, vegetación, cielo 360°) | ✅ Skybox, piso, arbustos, pasto, nubes |
| Personaje principal visible en tercera persona | ✅ Cazador renderizado en cámara cenital |
| Modelos adicionales (patos, arbustos, ambientación) | ✅ Patos × 3, arbustos × 8, nubes × 3, pasto |
| Cámara FPS | ✅ Implementada y funcional |
| Cámara cenital (tercera persona) | ✅ Con cazador visible |
| Iluminación básica | ✅ Phong con DirLight (ambient + diffuse + specular) |
| Iluminación avanzada | ✅ 4 PointLights y SpotLight declaradas en `default.frag` (aunque desactivadas en exteriores; se puede activar una como efecto de ronda) |
| Texturizado | ✅ Todos los modelos usan sus texturas |
| Animaciones por transformaciones | ✅ Órbita circular de patos, movimiento de nubes |
| Al menos una animación por keyframes | ✅ Perro (Dog_Looking × 5 frames, Dog_Found × 3 frames) |
| Interacción simple con el entorno | ✅ Disparo con detección de impacto, respuesta del perro |
| Movimiento WASD | ✅ Restringido al claro de tierra |
| Modelado jerárquico | ✅ La transform del cazador hereda la posición de la cámara |

### Comandos finales

```bash
# Compilar limpio y ejecutar
make clean && make run

# Verificar que no hay warnings de shaders (observar consola al iniciar)
# Verificar FPS estables (el contador de FPS del Día 1 debería mostrar > 30 FPS)
```

---

## Orden de renderizado definitivo (referencia)

Para que la transparencia y la profundidad funcionen correctamente, el orden de draw calls en el game loop debe ser:

```
1. glClear(COLOR | DEPTH)
2. [lightingShader] Skybox           — glDepthMask(GL_FALSE), sin culling
3. [lightingShader] Piso             — glDepthMask(GL_TRUE), con culling
4. [lightingShader] Pasto/anillo     — sin culling (doble cara)
5. [lightingShader] Arbustos         — con culling
6. [lightingShader] Nubes            — con culling
7. [lightingShader] Patos            — con culling, transparency=1
8. [lightingShader] Plumas           — sin culling, transparency=1
9. [lightingShader] Perro            — con culling
10. [lightingShader] DifficultyBar   — con culling
11. [lightingShader] Cazador         — solo si camaraCenital, con culling
12. glClear(DEPTH)  ← limpiar profundidad para el HUD
13. [hudShader]     Zapper           — solo si !camaraCenital
14. [hudShader]     Crosshair        — solo si !camaraCenital, con blend
15. [hudShader]     Marcador         — solo si !camaraCenital
```

---

## Estado final esperado al terminar el Día 3

El proyecto es entregable y presentable. Funciona de la siguiente forma:

1. Al iniciar, aparece el escenario con piso iluminado, skybox de 360°, vegetación y 3 patos volando en órbita.
2. El perro está en el borde del claro, animándose en ciclo de búsqueda.
3. El jugador se mueve con WASD (restringido al claro), apunta con el mouse.
4. Al hacer click: si un pato está en la mira, cae con efecto de plumas y el perro celebra.
5. Al presionar C: vista cenital, se ve el mapa completo y el cazador 3D orientado hacia donde mira.
6. Cuando todos los patos son resueltos (3 segundos de pausa), nueva ronda con mayor velocidad.
7. Si un pato completa 2 órbitas, el perro ríe.
