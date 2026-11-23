# Detalle de conexiones

![Diagrama de conexiones](diagrama_conexiones.svg)

Pines tomados del firmware (`esp32_railway.ino`, sección 3). Colores informados por el
equipo: **morado y verde** para el flotador P35 y **amarillo** para la bomba.

## Baja tensión

| Origen | Destino | Color de cable | Función |
|---|---|---|---|
| Fuente USB 5 V | ESP32 (conector USB) | — | Alimentación |
| Flotador P35, cable 1 | ESP32 GPIO 27 | Morado o verde* | Señal de nivel (entrada con pull-up interna) |
| Flotador P35, cable 2 | ESP32 GND | Verde o morado* | Referencia |
| ESP32 VIN (5 V) | Relé VCC | **POR COMPLETAR** | Alimentación de la bobina del relé |
| ESP32 GND | Relé GND | **POR COMPLETAR** | Referencia |
| ESP32 GPIO 26 | Relé IN | **POR COMPLETAR** | Orden de encendido (activo en LOW) |
| ESP32 GPIO 2 | LED integrado | — | Encendido = bomba funcionando |

\* El flotador P35 es un contacto seco sin polaridad: da lo mismo cuál de los dos cables va a
GPIO 27 y cuál a GND. **POR COMPLETAR:** anotar cuál quedó en cada pin y los colores de los
cables entre la placa y el relé.

## 220 V CA (siempre con la energía cortada al conectar)

| Origen | Destino | Color de cable | Función |
|---|---|---|---|
| Red, fase (L) | Relé COM | **POR COMPLETAR** | Fase que interrumpe el relé |
| Relé NO | Bomba, fase | Amarillo | Fase hacia la bomba solo cuando el relé está activo |
| Red, neutro (N) | Bomba, neutro | **POR COMPLETAR** | Directo |
| Red, tierra | Bomba, tierra | **POR COMPLETAR** | Protección |

## Por qué esos pines

| Pin | Motivo |
|---|---|
| GPIO 27 (flotador) | Pin de entrada normal, con pull-up interna, que no interviene en el arranque del ESP32 |
| GPIO 26 (relé) | Salida digital libre que no es pin de arranque (*strapping*), así el relé no se activa al encender |
| GPIO 2 (LED) | LED integrado de la placa, sirve como indicador sin agregar piezas |

## Esquema de la PCB

El diseño de Fusion 360 `ESP32_PCB` contiene una PCB con dos relés SRA-05VDC-CL,
dos borneras de 3 polos (T1, T2), conector micro USB, regulador SOT-223, transistores
SOT-23 (Q4, Q5), diodos, LED y un sensor de luz TEMT6000.

![Modelo 3D de la PCB](placa_pcb_fusion.png)

**POR COMPLETAR:** exportar el esquemático eléctrico de la PCB (desde Fusion Electronics
o la herramienta usada) como PNG o PDF en esta carpeta.
