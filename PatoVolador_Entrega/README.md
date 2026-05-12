# Pato Volador — Módulo de entrega

Este paquete contiene un pato 3D animado con su animación de vuelo (recta + giro 180° en cada extremo, en loop), listo para integrarse al proyecto OpenGL del equipo.

## ¿Por dónde empezar?

📖 **Lee primero `INTEGRACION.md`** — es la guía completa con:
- Pasos de integración al proyecto
- Configuración del .vcxproj
- Solución de problemas
- Parámetros ajustables
- Prompt para IA (si quieren usar Claude/GPT/Cursor para la integración)

## Estructura

```
PatoVolador_Entrega/
├── README.md                 ← este archivo
├── INTEGRACION.md            ← guía completa (convertir a PDF, ver al final del archivo)
├── codigo/
│   ├── modelAnim.h
│   ├── meshAnim.h
│   ├── snippet_pato.cpp      ← código a copiar/pegar
│   └── shaders/
│       ├── anim.vs
│       └── anim.frag
└── modelo/
    ├── Pato.fbx
    └── texture_20250901.png
```

## Resumen de la animación

- Pato avanza recto en eje Z (4 seg)
- Llega al extremo, gira 180° sobre su eje (1.8 seg, easing suave)
- Avanza de regreso (siempre mirando hacia adelante)
- Loop infinito

## Contacto

Cualquier duda en la integración, avisar a Bruno.
