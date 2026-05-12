# Pato 2 Volador — Modulo de entrega

Segundo pato 3D animado con su animacion de vuelo en **trayectoria cuadrada** con derrape suave y banking en cada esquina. Igual que el modulo del Aguila pero con otro modelo. Listo para integrarse al proyecto OpenGL del equipo.

## ¿Por donde empezar?

**Lee primero `INTEGRACION.md`** — es la guia completa.

## Estructura

```
Pato2Volador_Entrega/
├── README.md                ← este archivo
├── INTEGRACION.md           ← guia completa
├── codigo/
│   ├── modelAnim.h          (cargador de FBX animado con huesos, v1.1 con soporte de texturas embebidas)
│   ├── meshAnim.h           (mesh con bone weights)
│   ├── snippet_pato2.cpp    (codigo a copiar/pegar)
│   └── shaders/
│       ├── anim.vs
│       └── anim.frag
└── modelo/
    └── Pato2.fbx            (modelo con animacion de aleteo, textura embebida)
```

> **Nota:** la textura del Pato2 esta **embebida dentro del FBX**, asi que no hay archivo de textura suelto. El cargador `modelAnim.h v1.1+` la extrae automaticamente.

## Resumen de la animacion

- Pato2 vuela en cuadrado de **16 m de lado** a **6 m de altura** (ajustable)
- 4 lados rectos a velocidad 5 m/s
- 4 giros de 90 grados con curva bezier suave (derrape sutil)
- Banking lateral como un avion (ala interior baja al girar)
- Siempre mira hacia adelante en la direccion del movimiento
- Loop infinito

## Compatibilidad con los otros modulos

Este modulo comparte `modelAnim.h`, `meshAnim.h` y los shaders con los modulos del **Pato Volador** y el **Aguila Voladora**. Si el equipo ya integro alguno de esos, **gran parte del trabajo ya esta hecho** — solo hay que cargar el `Pato2.fbx` y pegar el bloque 4 del snippet.

> Si tienen una version vieja de `modelAnim.h` (de la entrega del Pato o de una entrega previa del Aguila sin soporte embebido), **reemplazarla por esta version** para que el Pato2 cargue su textura. El nuevo cargador es 100% compatible con los modelos viejos.

## Parametros mas comunes a tocar

| Parametro | Default | Para que sirve |
|---|---|---|
| `p2_sideLength` | `16.0` | tamanio del cuadrado |
| `p2_centerY` | `6.0` | altura de vuelo |
| `p2_speed` | `5.0` | velocidad |
| `p2_scale` | `0.01` | tamanio del modelo |
| `p2_cornerDrift` | `1.8` | cuanto se "redondea" cada esquina |
| `p2_trajectoryCenter` | `(0,0,0)` | donde queda el centro del cuadrado |
