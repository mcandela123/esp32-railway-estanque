# Protocolo de pruebas

**Fecha de redacción:** 22-11-2026, antes de ejecutar las pruebas formales.
**Versión del sistema:** firmware con página local, WiFiMulti y envío a Railway (commit `4f7162b`).

> El 22-11-2026, antes de este protocolo, se hizo una **prueba exploratoria de puesta en
> marcha**: se movió el flotador a mano y se verificó que los datos llegaran a la página.
> Sus datos están en [`../datos/`](../datos/) y se analizan en
> [`reporte_testing.md`](reporte_testing.md), pero **no reemplazan** las pruebas de este protocolo.

---

## Prueba A — Funcional (control de la bomba)

| Campo | Detalle |
|---|---|
| Objetivo | Verificar que la bomba se enciende con el estanque vacío y se apaga al llenarse |
| Condiciones | Estanque real (o un balde con el flotador instalado), bomba conectada al relé, ESP32 alimentado con el cargador USB |
| Procedimiento | 1. Encender el equipo y esperar el descanso inicial de 60 s. 2. Bajar el nivel hasta que el flotador marque VACÍO. 3. Medir con cronómetro el tiempo hasta que la bomba enciende. 4. Dejar llenar hasta que el flotador marque LLENO. 5. Medir el tiempo hasta que la bomba se apaga. 6. Repetir **20 ciclos** |
| Criterio de éxito | 20 de 20 ciclos correctos. Encendido entre 10 y 12 s después de marcar VACÍO (retardo programado de 10 s). Apagado en menos de 2 s después de marcar LLENO. Ningún encendido durante el descanso de 60 s |
| Evidencia | Video de al menos 2 ciclos, planilla con los 20 ciclos, exportación de la base de datos |

## Prueba B — Conectividad y datos

| Campo | Detalle |
|---|---|
| Objetivo | Verificar que los datos llegan a la base y que el sistema se recupera si se corta el WiFi |
| Condiciones | Equipo funcionando conectado al WiFi |
| Procedimiento | 1. Anotar cuántos cambios de estado registra el Monitor Serie durante 30 min. 2. Comparar con las filas guardadas en la base (`/api/lecturas`). 3. Apagar el router o el hotspot 5 min, generar un cambio del flotador y volver a encenderlo. 4. Revisar que la página muestre **SIN SEÑAL** durante el corte y que el dato pendiente llegue después |
| Criterio de éxito | ≥ 99 % de los envíos registrados en el Monitor Serie (`-> 201`) aparecen en la base. La bomba sigue funcionando durante el corte. La página avisa SIN SEÑAL en ≤ 3 min y vuelve a EN LÍNEA tras reconectar |
| Evidencia | Registro del Monitor Serie, exportación CSV de la base y capturas de la página antes, durante y después del corte |

## Prueba C — Tiempos

| Campo | Detalle |
|---|---|
| Objetivo | Medir el retardo entre un cambio del flotador y su aparición en la página publicada |
| Condiciones | Celular con la página abierta y cronómetro |
| Procedimiento | 1. Cambiar el flotador e iniciar el cronómetro. 2. Detenerlo cuando la tarjeta "Nivel" cambie en la página. 3. Repetir **10 veces** |
| Criterio de éxito | Mediana ≤ 10 s (la página se actualiza cada 5 s y cada envío tarda alrededor de 1,2 s) |
| Evidencia | Tabla con los 10 tiempos y video de una medición |

## Prueba D — Usuarios

| Campo | Detalle |
|---|---|
| Objetivo | Verificar que una persona sin conocimientos técnicos entiende la página |
| Participantes | **Mínimo 5 personas reales** (nombre o iniciales, edad aproximada y relación con el problema) |
| Procedimiento | 1. Mostrar la página publicada sin explicar nada. 2. Preguntar: ¿el estanque tiene agua?, ¿la bomba está funcionando?, ¿el equipo está conectado? 3. Medir cuánto tarda en responder. 4. Preguntar qué cambiaría |
| Criterio de éxito | Al menos 4 de 5 personas responden bien las 3 preguntas en menos de 30 s |
| Evidencia | Foto o video de cada prueba (con permiso), planilla de respuestas y tiempos, lista de comentarios |

---

## Registro de ejecución

| Prueba | Fecha | Responsable | Resultado |
|---|---|---|---|
| A | **POR COMPLETAR** | **POR COMPLETAR** | |
| B | **POR COMPLETAR** | | |
| C | **POR COMPLETAR** | | |
| D | **POR COMPLETAR** | | |
