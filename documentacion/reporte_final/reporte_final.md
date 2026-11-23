# Reporte final — Llenado automático de estanque

TEI201 Introducción al Diseño en Ingeniería · Universidad Adolfo Ibáñez · 2026

**Equipo:** **POR COMPLETAR** (integrantes y roles)

> Borrador. Máximo 15 páginas al exportar a PDF. Las secciones marcadas
> **POR COMPLETAR** las escribe el equipo con sus datos reales.

## 1. Problema, contexto y ODS

**POR COMPLETAR:** ODS elegido, contexto y problema concreto. Por ejemplo: quién usa el
estanque de 2000 L, qué pasa hoy cuando rebalsa o se vacía y cuánta agua o tiempo se pierde.

## 2. Solución

Un ESP32 lee un flotador P35 instalado en el estanque y controla la bomba mediante un relé:
la enciende cuando el estanque se vacía y la apaga cuando se llena, para que no rebalse.
Además, envía el nivel, el estado de la bomba y el estado del control a un servidor en
Railway, que los guarda en PostgreSQL (Aiven) y los muestra en una página pública.

```
ESP32 (flotador P35 + relé) --POST HTTPS--> Railway (Express + página) --SQL con SSL--> Aiven (PostgreSQL)
```

Página: https://esp32-railway-estanque-production.up.railway.app

## 3. Software

Resumen de [`../../software/`](../../software/):

- Máquina de estados con retardo de arranque (10 s), corte inmediato y descanso del motor (60 s).
- Envío en una tarea aparte con cola y reintentos; WiFi no bloqueante con varias redes.
- API con clave (`x-api-key`), consultas parametrizadas y validación de datos.
- Dashboard con estado, gráfico, historial y alerta SIN SEÑAL.

## 4. Hardware

Resumen de [`../../hardware/`](../../hardware/): ESP32 Dev Module, flotador P35 (GPIO 27),
relé activo en LOW (GPIO 26) y bomba de 220 V. Justificación de componentes, BOM y
fotos: **POR COMPLETAR** con los datos reales.

## 5. Diseño 3D

Resumen de [`../../diseno_3d/`](../../diseno_3d/): caja de 100 × 60 mm, techo de una sola
agua con alero y riel deslizable, tubo telescópico de 3 tramos de 10 cm con pasadores,
roscas M36x2 y M10x1.5, y 7 piezas de 10 cm o menos para la impresora.

## 6. Testing

Resumen de [`../../testing/`](../../testing/): la prueba de puesta en marcha verificó el
envío (respuestas 201, 34 lecturas), el retardo de 10 s, el descanso de 60 s y la alerta
SIN SEÑAL. Pruebas formales A a D y validación con 5 usuarios: **POR COMPLETAR**.

## 7. Iteraciones y conclusiones

Ver [`../../iteraciones/`](../../iteraciones/). Conclusiones: **POR COMPLETAR**.

## 8. Referencias

Ver [`../../FUENTES.md`](../../FUENTES.md).
