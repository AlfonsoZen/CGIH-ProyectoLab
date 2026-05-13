# Checkpoint — Duck Hunt 3D
**Fecha:** 2026-05-12  
**Estado:** Días 1 y 2 implementados, pendiente de validación y pulido antes de Día 3.

Marcar cada ítem con `[x]` una vez validado visualmente en el juego.

---

## Escena base

- [ ] Skybox se renderiza correctamente (sin bordes, sin negro)
- [ ] Piso con iluminación Phong visible (no plano/sin luz)
- [ ] Iluminación direccional simula sol de tarde (tono cálido, sombras hacia adelante)
- [ ] 8 arbustos distribuidos uniformemente en el perímetro (radio 22u)
- [ ] 3 nubes flotando en el cielo con movimiento sinusoidal suave
- [ ] 12 segmentos de pasto formando anillo (radio 18u), alternando normal/reverso
- [ ] No hay z-fighting entre piso y pasto

---

## Jugador / Cámara

- [ ] Movimiento WASD fluido
- [ ] Cámara FPS responde al mouse (sin comportamiento joystick en Windows)
- [ ] El jugador NO puede salir del claro central (radio 8u) — choca con el borde invisible
- [ ] Al intentar salir del radio, la posición se corrige suavemente (sin teletransportarse)
- [ ] Salto con ESPACIO funciona (o está intencionalmente desactivado)
- [ ] La cámara no atraviesa el piso (altura mínima respetada)

---

## Patos

- [ ] 3 patos visibles orbitando en círculo (radio 20u, altura ~6u)
- [ ] Cada pato tiene velocidad ligeramente distinta (no se mueven sincronizados)
- [ ] Animación de aleteo FBX visible y fluida
- [ ] Los patos siempre miran hacia adelante en la dirección de la órbita
- [ ] Al disparar y acertar: pato pasa a estado CAYENDO
- [ ] Pato CAYENDO cae con gravedad (aceleración visible, no velocidad constante)
- [ ] Pato CAYENDO desaparece al llegar al suelo (Y <= -1)
- [ ] `Duck_Dead.glb` se renderiza brevemente donde cayó el pato (actualmente ausente)
- [ ] Los patos reaparecen cuando todos están en MUERTO (nueva ronda) — actualmente ausente
- [ ] Pato que escapa (sale del rango sin ser derribado) activa PERRO_RIENDO — actualmente ausente

---

## Disparo

- [ ] Click izquierdo dispara (sin delay ni doble-disparo)
- [ ] En modo cenital, el click NO dispara
- [ ] El cono de detección (5°) se siente justo — ni demasiado fácil ni imposible
- [ ] Solo un pato por disparo (el más cercano al centro del crosshair)
- [ ] No se puede disparar patos ya en estado CAYENDO o MUERTO

---

## Efecto de plumas

- [ ] Las plumas aparecen en la posición exacta del impacto
- [ ] 5 frames visibles en secuencia durante 0.6s
- [ ] Las plumas rotan mientras caen
- [ ] Las plumas desaparecen solos al terminar (no se quedan en escena)
- [ ] Solo hay un efecto activo a la vez (no se acumulan)

---

## Perro

- [ ] Perro visible en escena con cuerpo jerárquico (cuerpo, cabeza, cola, 4 patas)
- [ ] **Estado BUSCANDO:** perro anima patas caminando, cola moviéndose, cabeza oscilando
- [ ] **Estado BUSCANDO:** posición del perro es correcta (borde del claro, no siguiendo al jugador)
- [ ] **Estado ENCONTRADO:** perro corre hacia donde cayó el pato (1.5s)
- [ ] **Estado MOSTRANDO:** perro se detiene, cabeza alzada, 2.0s estático
- [ ] **Estado MOSTRANDO:** distingue si derribó 1 o 2 patos en la ronda (`Dog_OneDuck` / `Dog_TwwoDucks` — actualmente no implementado con la jerarquía)
- [ ] **Estado RIENDO:** perro se ríe cuando un pato escapa — actualmente nunca se activa
- [ ] Transición ENCONTRADO → MOSTRANDO → BUSCANDO ocurre correctamente en tiempo
- [ ] El perro no atraviesa el piso ni flota

---

## HUD

- [ ] Zapper visible en esquina inferior derecha
- [ ] Crosshair visible en el centro exacto de la pantalla
- [ ] Crosshair es circular / tiene la forma correcta (no distorsionado por aspect ratio)
- [ ] HUD no se ve afectado por la profundidad (siempre encima de todo)
- [ ] En modo cenital el HUD completo desaparece (zapper Y crosshair)
- [ ] Al volver de cenital a FPS el HUD reaparece

---

## Cámara cenital

- [ ] Tecla C alterna entre FPS y cenital
- [ ] Vista cenital muestra el mapa completo desde arriba (altura 45u)
- [ ] `RedDog.obj` (cazador) visible en la posición del jugador en vista cenital
- [ ] Cazador orientado en la dirección que mira el jugador
- [ ] En cenital el movimiento del jugador está bloqueado
- [ ] El mouse no mueve la cámara en cenital
- [ ] Al volver a FPS el jugador retoma control inmediatamente

---

## Modelos disponibles no usados (evaluar si incorporar)

| Modelo | Uso potencial | Decisión |
|--------|--------------|----------|
| `Dog_Looking.glb` … `Dog_Looking5.glb` | Reemplazar jerarquía en BUSCANDO | pendiente |
| `Dog_Found.glb/1/2.glb` | Animar ENCONTRADO con keyframes | pendiente |
| `Dog_OneDuck.glb` / `Dog_TwwoDucks.glb` | MOSTRANDO según patos derribados | pendiente |
| `Dog_Laugh1/2.glb` | Estado RIENDO | pendiente |
| `Duck_Dead.glb` | Pato muerto en el suelo | pendiente |
| `Duck_Falling.glb` | Pato en caída (reemplazar modelo volando) | pendiente |

---

## Notas de bugs conocidos

- `PERRO_RIENDO` existe en el código pero **nunca se activa** — falta lógica de "pato escapó"
- `patosEscapados` se declara pero **nunca incrementa**
- Patos en estado `MUERTO` no reaparecen — el juego se queda sin patos indefinidamente
- En modo cenital, `viewPos` usa la posición del jugador en lugar de `(0, 45, 0)` — brillos especulares incorrectos en ese modo (visual menor)
- En WSL2 el cursor es visible y la cámara se comporta como joystick — esto es normal en WSL2, funcionará correctamente en Windows nativo

---

## Progreso

- [x] Día 1 implementado y auditado
- [x] Día 2 implementado y auditado
- [ ] Checkpoint validado (este documento)
- [ ] Día 3
