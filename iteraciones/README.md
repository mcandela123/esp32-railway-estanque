# Iteraciones

| | [Iteración 1](iteracion_1/) | [Iteración 2](iteracion_2/) | [Iteración 3](iteracion_3/) |
|---|---|---|---|
| Fecha | hasta 16-11-2026 | 16-11-2026 | 21 y 22-11-2026 |
| Control de la bomba | Flotador + relé con máquina de estados | Igual | Igual |
| Dónde se ven los datos | Monitor Serie (cable USB) | Página local (mismo WiFi) | Página local **y** página pública en Railway |
| Historial | No | No | Sí, en PostgreSQL (Aiven) |
| WiFi | — | Una red | Varias redes (WiFiMulti), no bloqueante |
| Envío | — | — | Tarea aparte (núcleo 0), cola y reintentos |
| Encapsulado | Caja con tapa plana | v2 techo a dos aguas → v3 techo de una sola agua | v3 dividida en 7 piezas ≤ 10 cm |
| Tubo | 3 tramos, rosca M20 para el sensor | Pasadores, unión M36x2 y rosca M10 | Igual |

## Lecciones aprendidas

1. **Separar el control de la comunicación.** Si el envío a internet está en el mismo
   ciclo que el control, una falla del WiFi puede retrasar el corte de la bomba. Moverlo a
   otro núcleo hizo el sistema más seguro.
2. **Diseñar para la impresora disponible.** Un límite de 10 cm obliga a pensar desde el
   principio cómo se unen las piezas (roscas, casquillos, lengüetas).
3. **El techo es parte de la funcionalidad.** El primer diseño protegía la placa pero no
   las entradas de cables. El alero de la versión 3 resolvió eso.
4. **Los errores de configuración son los más comunes.** Las fallas de la puesta en
   marcha no fueron del código sino de configuración: placa equivocada en el IDE, nombre
   de WiFi y variables de Railway. Documentarlas ahorra tiempo.
5. **No subir claves.** Mantener las claves fuera del repositorio desde el principio
   evita tener que cambiarlas después.
