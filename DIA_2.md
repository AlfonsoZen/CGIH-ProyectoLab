# Día 2 — Disparo + Perro animado + Cámara cenital + Cazador visible
**Fecha:** 12 de mayo de 2026  
**Meta del día:** El juego tiene un loop completo. El jugador puede disparar patos, el perro reacciona con keyframes al resultado, la cámara cenital muestra al cazador en el mundo, y el movimiento ya está restringido al claro.

---

## Prerrequisito

El Día 1 debe estar completo: iluminación Phong activa, patos en órbita con aleteo, vegetación en escena, restricción de movimiento. Si `make run` compila y se ven patos volando, continuar.

---

## Tarea 5 — Sistema de disparo (click izquierdo + detección de impacto)

### Concepto

Al hacer click izquierdo, se lanza un **rayo** desde el centro de la pantalla en dirección `camera.front`. Para cada pato en estado `VOLANDO`, se calcula si el rayo pasa suficientemente cerca de su posición. Si el ángulo entre el rayo y el vector (cámara → pato) es menor a 5°, el pato es impactado.

La detección por cono es correcta para un juego FPS: el crosshair está en el centro exacto de la pantalla, y el rayo parte en la dirección de vista.

### Variables globales nuevas (añadir antes de `main()`)

```cpp
// ---- Sistema de disparo ----
int patosDerribados = 0;   // Contador global de patos eliminados
int patosEscapados  = 0;   // Contador de patos que llegaron a escapar (futuro Día 3)
bool disparoRealizado = false; // Flag para el callback del mouse
```

### Callback del mouse (añadir nueva función antes de `main()`)

```cpp
// Callback invocado por GLFW al hacer click con el mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // Solo registrar el click izquierdo al presionar (no al soltar)
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        disparoRealizado = true;
    }
}
```

### Registrar el callback en `main()` (junto a los otros `glfwSet...Callback`)

```cpp
glfwSetMouseButtonCallback(window, MouseButtonCallback);
```

### Lógica de detección de impacto en el game loop (añadir justo después de `glfwPollEvents()`)

```cpp
// ---- Procesar disparo ----
if (disparoRealizado) {
    disparoRealizado = false; // Reset del flag

    // El rayo viaja desde la posición de la cámara en dirección camera.front
    glm::vec3 origenRayo    = camera.position;
    glm::vec3 direccionRayo = glm::normalize(camera.GetFront());

    const float ANGULO_MAXIMO_IMPACTO = glm::radians(5.0f); // Cono de 5 grados

    for (int i = 0; i < NUM_PATOS; i++) {
        if (patos[i].estado != VOLANDO) continue;

        // Calcular posición actual del pato en la órbita
        float px = RADIO_ORBITA * cos(patos[i].angulo);
        float pz = RADIO_ORBITA * sin(patos[i].angulo);
        glm::vec3 posPato(px, patos[i].altura, pz);

        // Vector desde la cámara al pato, normalizado
        glm::vec3 vecCamaraPato = glm::normalize(posPato - origenRayo);

        // El ángulo entre el rayo y el vector cámara-pato
        float cosAngulo = glm::dot(direccionRayo, vecCamaraPato);
        float angulo    = acos(glm::clamp(cosAngulo, -1.0f, 1.0f));

        // Distancia máxima de disparo efectivo: 40 unidades
        float distancia = glm::length(posPato - origenRayo);

        if (angulo < ANGULO_MAXIMO_IMPACTO && distancia < 40.0f) {
            // ¡Impacto! Cambiar estado del pato a CAYENDO
            patos[i].estado         = CAYENDO;
            patos[i].velocidadCaida = 0.0f;
            patosDerribados++;

            // Notificar al sistema del perro (ver Tarea 6)
            perroReaccionarDisparo(patosDerribados);

            break; // Un disparo solo puede golpear un pato a la vez
        }
    }
}
```

> **Nota:** La función `perroReaccionarDisparo()` se define en la Tarea 6 a continuación. Declararla antes de este bloque o mover la declaración al inicio del archivo.

---

## Tarea 6 — Sistema del perro con keyframes y máquina de estados

### Concepto

El perro tiene varios modelos GLB, cada uno representando una pose distinta. La **animación por keyframes** consiste en cambiar de modelo cada cierto tiempo, creando la ilusión de movimiento. Los estados del perro son:

| Estado | Modelos usados | Duración |
|--------|---------------|----------|
| `BUSCANDO` | Dog_Looking → Dog_Looking2 → ... → Dog_Looking5 | Ciclo de 0.4s por pose |
| `ENCONTRADO` | Dog_Found → Dog_Found1 → Dog_Found2 | 0.5s por pose, una vez |
| `MOSTRANDO` | Dog_OneDuck o Dog_TwwoDucks | 1.5s |
| `RIENDO` | Dog_Laugh1 → Dog_Laugh2 (ciclo) | 0.5s por pose |

### Variables globales nuevas

```cpp
// ---- Sistema del perro ----
enum EstadoPerro { PERRO_BUSCANDO, PERRO_ENCONTRADO, PERRO_MOSTRANDO, PERRO_RIENDO };

EstadoPerro estadoPerro = PERRO_BUSCANDO;
float timerPerro        = 0.0f;  // Tiempo acumulado en el estado actual
int   frameBuscando     = 0;     // Frame actual del ciclo de búsqueda (0-4)
int   patosEnRonda      = 0;     // Patos derribados en la ronda actual

// Posición fija del perro: al borde del claro, frente al jugador
const glm::vec3 POSICION_PERRO(0.0f, -1.0f, -7.5f);
```

### Cargar modelos del perro (después de los modelos de patos en `main()`)

```cpp
// Ciclo de búsqueda (5 poses de búsqueda)
Model PerroBuscando1((char*)"assets/models/Dog_Looking.glb");
Model PerroBuscando2((char*)"assets/models/Dog_Looking2.glb");
Model PerroBuscando3((char*)"assets/models/Dog_Looking3.glb");
Model PerroBuscando4((char*)"assets/models/Dog_Looking4.glb");
Model PerroBuscando5((char*)"assets/models/Dog_Looking5.glb");

// Reacción al encontrar un pato
Model PerroEncontrado1((char*)"assets/models/Dog_Found.glb");
Model PerroEncontrado2((char*)"assets/models/Dog_Found1.glb");
Model PerroEncontrado3((char*)"assets/models/Dog_Found2.glb");

// Mostrar el pato cazado
Model PerroUnPato((char*)"assets/models/Dog_OneDuck.glb");
Model PerroDsPatos((char*)"assets/models/Dog_TwwoDucks.glb");

// Risa (pato escapado — se usa en Día 3 para ronda fallida)
Model PerroRisa1((char*)"assets/models/Dog_Laugh1.glb");
Model PerroRisa2((char*)"assets/models/Dog_Laugh2.glb");

// Arrays para acceso por índice
Model* framesBuscando[5] = {
    &PerroBuscando1, &PerroBuscando2, &PerroBuscando3,
    &PerroBuscando4, &PerroBuscando5
};
Model* framesEncontrado[3] = {
    &PerroEncontrado1, &PerroEncontrado2, &PerroEncontrado3
};
```

### Función `perroReaccionarDisparo()` (declarar antes del game loop o como lambda)

```cpp
// Llamada desde el sistema de disparo cuando se derriba un pato
void perroReaccionarDisparo(int totalDerribados) {
    estadoPerro = PERRO_ENCONTRADO;
    timerPerro  = 0.0f;
    patosEnRonda++;
}
```

> Como esta función necesita acceder a las variables globales del perro, puede declararse como función libre antes de `main()` o como una lambda local. Si se usa como función libre, declarar `estadoPerro`, `timerPerro`, y `patosEnRonda` como globales.

### Lógica de la máquina de estados del perro (en el game loop, antes del renderizado)

```cpp
// ---- Actualizar máquina de estados del perro ----
timerPerro += deltaTime;

switch (estadoPerro) {
    case PERRO_BUSCANDO:
        // Ciclo de 5 poses de búsqueda, 0.4 segundos cada una
        if (timerPerro >= 0.4f) {
            timerPerro   = 0.0f;
            frameBuscando = (frameBuscando + 1) % 5;
        }
        break;

    case PERRO_ENCONTRADO:
        // 3 poses de "encontrado", 0.5s cada una, luego pasa a MOSTRANDO
        if (timerPerro >= 0.5f) {
            timerPerro = 0.0f;
            int frameActual = (int)(timerPerro / 0.5f);
            if (frameActual >= 3) {
                estadoPerro = PERRO_MOSTRANDO;
            }
        }
        break;

    case PERRO_MOSTRANDO:
        // Mostrar pato por 1.5 segundos, luego volver a BUSCANDO
        if (timerPerro >= 1.5f) {
            estadoPerro  = PERRO_BUSCANDO;
            timerPerro   = 0.0f;
            frameBuscando = 0;
        }
        break;

    case PERRO_RIENDO:
        // Ciclo de risa de 2 frames, 0.5s cada uno — se activa en Día 3
        if (timerPerro >= 0.5f) {
            timerPerro = 0.0f;
        }
        break;
}
```

> **Corrección de bug:** El cálculo de `frameActual` en `PERRO_ENCONTRADO` debe hacerse con un contador separado. Reemplazar con:

```cpp
    case PERRO_ENCONTRADO: {
        // framePerroEncontrado es una variable global int inicializada en 0
        if (timerPerro >= 0.5f) {
            timerPerro = 0.0f;
            framePerroEncontrado++;
            if (framePerroEncontrado >= 3) {
                framePerroEncontrado = 0;
                estadoPerro = PERRO_MOSTRANDO;
                timerPerro  = 0.0f;
            }
        }
        break;
    }
```

Añadir `int framePerroEncontrado = 0;` a las variables globales del perro.

### Renderizar el perro en el game loop

```cpp
// ---- Dibujar perro ----
{
    glm::mat4 matPerro = glm::mat4(1.0f);
    matPerro = glm::translate(matPerro, POSICION_PERRO);
    matPerro = glm::rotate(matPerro, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    matPerro = glm::scale(matPerro, glm::vec3(1.5f));
    glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(matPerro));
    glUniform1i(glGetUniformLocation(lightingShader.Program, "useTexture"), 1);

    switch (estadoPerro) {
        case PERRO_BUSCANDO:
            framesBuscando[frameBuscando]->Draw(lightingShader);
            break;
        case PERRO_ENCONTRADO:
            framesEncontrado[framePerroEncontrado]->Draw(lightingShader);
            break;
        case PERRO_MOSTRANDO:
            // Si hay 2 patos derribados en la ronda, mostrar pose de dos patos
            if (patosEnRonda >= 2)
                PerroDsPatos.Draw(lightingShader);
            else
                PerroUnPato.Draw(lightingShader);
            break;
        case PERRO_RIENDO:
            // Seleccionar frame de risa según el timer (alterna entre 2 frames)
            if ((int)(timerPerro / 0.5f) % 2 == 0)
                PerroRisa1.Draw(lightingShader);
            else
                PerroRisa2.Draw(lightingShader);
            break;
    }
}
```

---

## Tarea 7 — Cámara cenital (vista de pájaro) con tecla C

### Concepto

Al presionar `C`, la vista cambia a una cámara fija ubicada sobre el mapa mirando hacia abajo. En este modo, el jugador **no puede moverse** y se muestra el modelo 3D del cazador/personaje en la posición del jugador. Al presionar `C` de nuevo, regresa la cámara FPS.

### Variable global nueva

```cpp
bool camaraCenital = false; // false = FPS, true = cenital
```

### Añadir el modelo del cazador (en la sección de carga de modelos)

```cpp
// Modelo del cazador visible en vista cenital
// RedDog.obj es el modelo del perro rojo que actúa como representación del jugador
Model Cazador((char*)"assets/models/RedDog.obj");
```

### Toggle con tecla C en `KeyCallback()`

```cpp
// En el if principal del KeyCallback, añadir:
if (key == GLFW_KEY_C && action == GLFW_PRESS) {
    camaraCenital = !camaraCenital;
}
```

### Bloquear movimiento en modo cenital en `DoMovement()`

```cpp
void DoMovement() {
    // En modo cenital, el jugador no se mueve
    if (camaraCenital) return;

    // ... resto del código existente ...
}
```

### Calcular la view matrix correcta en el game loop

Reemplazar la línea `glm::mat4 view = camera.GetViewMatrix();` con:

```cpp
glm::mat4 view;
if (camaraCenital) {
    // Cámara cenital: posicionada arriba del mapa mirando hacia abajo
    // Se usa glm::lookAt(ojo, objetivo, vector_arriba)
    // El vector "arriba" de la cámara cenital apunta hacia -Z para que el norte quede arriba en pantalla
    glm::vec3 ojoCenital(0.0f, 45.0f, 0.0f);
    glm::vec3 objetivoCenital(0.0f, 0.0f, 0.0f);
    glm::vec3 arribasCenital(0.0f, 0.0f, -1.0f);
    view = glm::lookAt(ojoCenital, objetivoCenital, arribasCenital);
} else {
    view = camera.GetViewMatrix();
}
```

### Renderizar el cazador SOLO en modo cenital

Añadir en la sección de dibujo del game loop, DESPUÉS de renderizar el perro:

```cpp
// ---- Dibujar cazador (solo visible en cámara cenital) ----
if (camaraCenital) {
    glm::mat4 matCazador = glm::mat4(1.0f);
    // El cazador aparece donde está el jugador (posición de la cámara FPS)
    // Se baja ligeramente porque la posición de la cámara es la altura de los ojos
    matCazador = glm::translate(matCazador, camera.position - glm::vec3(0.0f, 0.8f, 0.0f));
    // Orientar al cazador en la dirección que mira el jugador
    // Calcular ángulo yaw de la cámara para rotar el modelo
    float yawCazador = atan2(camera.GetFront().x, camera.GetFront().z);
    matCazador = glm::rotate(matCazador, yawCazador, glm::vec3(0.0f, 1.0f, 0.0f));
    matCazador = glm::scale(matCazador, glm::vec3(1.2f));

    glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(matCazador));
    glUniform1i(glGetUniformLocation(lightingShader.Program, "useTexture"), 1);
    Cazador.Draw(lightingShader);
}
```

### Ocultar el HUD en modo cenital

La pistola y el crosshair no tienen sentido en vista cenital. En la sección del HUD (zapper + crosshair), envolver con:

```cpp
// El HUD solo se muestra en cámara FPS
if (!camaraCenital) {
    // ... código existente del zapper ...
    // ... código existente del crosshair ...
}
```

---

## Tarea 8 — Efecto de plumas al derribar un pato (visual)

### Concepto

Cuando un pato pasa de `VOLANDO` a `CAYENDO`, se puede mostrar brevemente el modelo de plumas en su posición para dar retroalimentación visual. Los modelos `feathers_1.glb` a `feathers_5.glb` sirven para esto.

### Variables para el efecto de plumas

```cpp
// ---- Efecto de plumas ----
struct EfectoPlumas {
    bool activo;
    glm::vec3 posicion;
    float timer;
    int   frame;       // 0-4, ciclo de los 5 modelos de plumas
};

EfectoPlumas efectoPlumas = { false, glm::vec3(0.0f), 0.0f, 0 };
const float DURACION_PLUMAS = 0.6f;
```

### Cargar modelos de plumas

```cpp
Model Plumas1((char*)"assets/models/feathers_1.glb");
Model Plumas2((char*)"assets/models/feathers_2.glb");
Model Plumas3((char*)"assets/models/feathers_3.glb");
Model Plumas4((char*)"assets/models/feathers_4.glb");
Model Plumas5((char*)"assets/models/feathers_5.glb");

Model* framesPlumas[5] = { &Plumas1, &Plumas2, &Plumas3, &Plumas4, &Plumas5 };
```

### Activar el efecto al impactar (en el bloque de detección de impacto del Día 1)

```cpp
// Justo después de cambiar el estado del pato a CAYENDO:
efectoPlumas.activo   = true;
efectoPlumas.posicion = glm::vec3(px, patos[i].altura, pz); // posición del pato
efectoPlumas.timer    = 0.0f;
efectoPlumas.frame    = 0;
```

### Actualizar y renderizar el efecto de plumas en el game loop

```cpp
// ---- Actualizar efecto de plumas ----
if (efectoPlumas.activo) {
    efectoPlumas.timer += deltaTime;
    efectoPlumas.frame = (int)(efectoPlumas.timer / (DURACION_PLUMAS / 5.0f));

    if (efectoPlumas.timer >= DURACION_PLUMAS) {
        efectoPlumas.activo = false;
    } else {
        glm::mat4 matPlumas = glm::mat4(1.0f);
        matPlumas = glm::translate(matPlumas, efectoPlumas.posicion);
        // Las plumas giran mientras caen para efecto visual
        matPlumas = glm::rotate(matPlumas, efectoPlumas.timer * 3.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        matPlumas = glm::scale(matPlumas, glm::vec3(2.0f));

        glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(matPlumas));
        glUniform1i(glGetUniformLocation(lightingShader.Program, "useTexture"), 1);
        glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 1);

        int frameIdx = glm::clamp(efectoPlumas.frame, 0, 4);
        framesPlumas[frameIdx]->Draw(lightingShader);
    }
}
```

---

## Verificación al final del Día 2

Al ejecutar `make run`:

- [ ] Click izquierdo → si apunta a un pato, el pato cambia a estado CAYENDO y cae con gravedad
- [ ] Al impactar un pato aparece brevemente el efecto de plumas en la posición del impacto
- [ ] El perro en pose de búsqueda (Dog_Looking, ciclo animado) en el borde del claro
- [ ] Al derribar un pato, el perro ejecuta la animación ENCONTRADO → MOSTRANDO y vuelve a BUSCANDO
- [ ] Tecla C alterna entre vista FPS y vista cenital
- [ ] En vista cenital: se ve el mapa completo desde arriba, el modelo del cazador visible en la posición del jugador, sin HUD (zapper ni crosshair)
- [ ] En vista cenital: el movimiento está bloqueado
- [ ] Al regresar a vista FPS (C de nuevo): el HUD reaparece, el movimiento funciona

## Compilación

```bash
make run
```

Si hay conflictos de nombres de función (p.ej. `perroReaccionarDisparo` accediendo a globales desde dentro de un callback), asegurarse de que todas las variables del perro (`estadoPerro`, `timerPerro`, `patosEnRonda`, `framePerroEncontrado`) sean **globales**, declaradas fuera de `main()`.
