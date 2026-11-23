# Guion de la presentación (10 minutos)

Reparto sugerido según el rol de cada integrante (ajustar si quieren).

| Minuto | Diapositiva | Qué se dice / muestra | Quién |
|---|---|---|---|
| 0:00 – 1:00 | 1–2 | Problema y ODS 6: en Culiprán y San Pedro (Melipilla) no hay agua potable por tuberías; el agua sale de pozo, se guarda en estanques que rebalsan y se pierde | Mateo |
| 1:00 – 2:00 | 3 | Solución y arquitectura (ESP32 → Railway → Aiven) | Claudio |
| 2:00 – 4:00 | 4 | **Demo en vivo:** mover el flotador, ver cómo la bomba (o el LED del GPIO 2) enciende tras 10 s, y mostrar la página pública en el celular | Mateo y Claudio |
| 4:00 – 5:00 | 5 | **Cómo funciona el código:** lee el flotador, decide con 5 estados, activa el relé, envía desde el otro núcleo y la página muestra | Claudio |
| 5:00 – 6:00 | 6 | Hardware: componentes, conexiones, separación de 220 V y costo de $12.000 | Mateo |
| 6:00 – 7:00 | 7 | Diseño 3D: techo con riel, caja, tubo telescópico, 7 piezas | Diego |
| 7:00 – 8:00 | 8 | Resultados: puesta en marcha con datos reales | Mateo |
| 8:00 – 9:00 | 9 | Iteraciones (v1 → v3) y lecciones | Diego |
| 9:00 – 10:00 | 10 | Cierre, proyección y preguntas | Los tres |

## Plan B

Si falla internet durante la demo, mostrar la **página local** del ESP32 (con el hotspot
del celular) y las capturas del dashboard con datos reales que están en el repositorio.

## Preguntas probables

| Pregunta | Respuesta preparada |
|---|---|
| ¿Por qué un flotador y no un sensor ultrasónico? | Es un contacto seco: simple, sin alimentación ni calibración, y no le afectan la espuma ni el vapor. Para decidir encender o apagar basta con saber si el agua llegó al punto |
| ¿Qué pasa si se cae el WiFi? | La bomba sigue funcionando porque el control depende solo del flotador. Los datos quedan en cola, la página muestra SIN SEÑAL y todo se recupera solo al volver el WiFi |
| ¿Por qué el retardo de 10 s y el descanso de 60 s? | Evitan que la bomba prenda y apague muchas veces cuando el agua se mueve, y protegen el motor |
| ¿Cualquiera puede mandar datos falsos? | No: `POST /api/lecturas` exige la `x-api-key`; sin ella el servidor responde 401 |
| ¿Cuánto cuesta? | $12.000 CLP: flotador P35 $6.000 y bomba $6.000. La placa y la impresión 3D las hizo la universidad, y el cargador, los cables y los tornillos no tuvieron costo. Ver el BOM |
| ¿Qué mejorarían? | Un sensor de nivel continuo para saber los litros, avisos al celular cuando el estanque se vacía o la placa pierde señal, imprimir en PETG para resistir mejor el sol, y probarlo con vecinos del sector |
