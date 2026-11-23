# Guion de la presentación (10 minutos)

Reparto sugerido según el rol de cada integrante (ajustar si quieren).

| Minuto | Lámina / acción | Quién |
|---|---|---|
| 0:00 – 1:00 | Problema y ODS 6: en Culiprán y San Pedro (Melipilla) no hay agua potable por tuberías; el agua sale de pozo, se guarda en estanques que rebalsan y se pierde | Mateo |
| 1:00 – 2:00 | Solución y arquitectura (ESP32 → Railway → Aiven) | Claudio |
| 2:00 – 4:00 | **Demo en vivo:** mover el flotador, ver cómo la bomba (o el LED del GPIO 2) enciende tras 10 s, y mostrar la página pública en el celular | Mateo y Claudio |
| 4:00 – 5:00 | Software: máquina de estados, envío en otro núcleo, API con clave | Claudio |
| 5:00 – 6:00 | Hardware: componentes, conexiones y separación de 220 V | Mateo |
| 6:00 – 7:00 | Diseño 3D: caja, techo con riel, tubo telescópico, 7 piezas | Diego |
| 7:00 – 8:00 | Testing: puesta en marcha y resultados | Mateo |
| 8:00 – 9:00 | Iteraciones y lecciones | Diego |
| 9:00 – 10:00 | Conclusiones y qué mejorarían | Los tres |

## Plan B

Si falla internet durante la demo, mostrar la **página local** del ESP32 (con el hotspot
del celular) y un **video grabado** de la demo completa (**POR COMPLETAR**: grabarlo).

## Preguntas probables

| Pregunta | Respuesta preparada |
|---|---|
| ¿Por qué un flotador y no un sensor ultrasónico? | Es un contacto seco: simple, sin alimentación ni calibración, y no le afectan la espuma ni el vapor. Para decidir encender o apagar basta con saber si el agua llegó al punto |
| ¿Qué pasa si se cae el WiFi? | La bomba sigue funcionando porque el control depende solo del flotador. Los datos quedan en cola, la página muestra SIN SEÑAL y todo se recupera solo al volver el WiFi |
| ¿Por qué el retardo de 10 s y el descanso de 60 s? | Evitan que la bomba prenda y apague muchas veces cuando el agua se mueve, y protegen el motor |
| ¿Cualquiera puede mandar datos falsos? | No: `POST /api/lecturas` exige la `x-api-key`; sin ella el servidor responde 401 |
| ¿Cuánto cuesta? | Unos $12.000 CLP: flotador P35 $6.000 y bomba $6.000. La placa y la impresión 3D las hizo la universidad y el cargador era nuestro. Ver el BOM |
| ¿Qué mejorarían? | **POR COMPLETAR** (por ejemplo: sensor de nivel continuo para saber los litros, avisos por correo o celular, alimentación con respaldo) |
