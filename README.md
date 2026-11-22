# Llenado automático de estanque — ESP32 + Railway + Aiven

Proyecto TEI201, adaptado de la guía *ESP32 conectado a una base de datos en la nube*.

**Página publicada:** https://esp32-railway-estanque-production.up.railway.app

```
ESP32 (flotador P35 + relé) --POST HTTPS--> Railway (Express + página) --SQL con SSL--> Aiven (PostgreSQL)
```

El ESP32 controla la bomba con el flotador P35 y, además, manda a la nube
tres datos cada vez que cambian y cada 60 segundos:

| sensor   | valor                                                                 |
|----------|-----------------------------------------------------------------------|
| `nivel`  | 1 = estanque LLENO, 0 = VACÍO                                          |
| `bomba`  | 1 = bomba ENCENDIDA, 0 = APAGADA                                       |
| `estado` | 0 APAGADA, 1 ESPERANDO ARRANQUE, 2 ENCENDIDA, 3 ESPERANDO CORTE, 4 DESCANSO |

Cada dato es una fila de la tabla `lecturas` (`dispositivo`, `sensor`, `valor`, `creado_en`),
igual que en la guía.

## Archivos

```
server.js                          servidor Express (API)
package.json                       dependencias de Node
.gitignore                         archivos que NO se suben
.env.example                       plantilla de variables (sin claves reales)
public/index.html                  página con estado actual, gráfico y tabla
esp32/esp32_railway/esp32_railway.ino   código del ESP32
```

## Pasos (según la guía)

1. **Requisitos:** instalar Node.js LTS (18 o más), Git y Arduino IDE 2 con el
   paquete ESP32 y la librería **ArduinoJson** (v7).
2. **GitHub:** crear un repositorio vacío y subir el contenido de esta carpeta
   (sin `.env` ni `node_modules`).
3. **Aiven:** crear un servicio PostgreSQL con plan **Free** (no el Professional
   que viene marcado). Copiar la *Service URI* y el *CA certificate*.
4. **Probar en el computador:** copiar `.env.example` a `.env`, poner los datos
   reales y ejecutar:
   ```
   npm install
   npm start
   ```
   Abrir `http://localhost:3000/api/salud` → debe responder `{"ok":true,"db":"conectada"}`.
   Lectura de prueba (PowerShell):
   ```
   Invoke-RestMethod -Method Post -Uri http://localhost:3000/api/lecturas `
     -Headers @{ "x-api-key" = "SU_API_KEY" } -ContentType "application/json" `
     -Body '{"dispositivo":"pc-prueba","sensor":"nivel","valor":1}'
   ```
5. **Railway** (crear la cuenta recién ahora, el trial dura 30 días): nuevo
   proyecto desde el repositorio, variables `DATABASE_URL`, `DB_CA_CERT` y
   `API_KEY`, presionar **Deploy**, generar el dominio con puerto **8080** y
   activar el deploy automático.
6. **ESP32:** en `esp32_railway.ino`, sección 1, poner en `REDES_WIFI` una o
   más redes WiFi (2,4 GHz; la placa se conecta a la que esté disponible),
   la URL de Railway terminada en `/api/lecturas` y la misma `API_KEY`.
   Subir con la placa **ESP32 Dev Module**. En el Monitor Serie (115200)
   debe aparecer `-> 201`.
7. Abrir la URL de Railway: la página muestra el nivel, la bomba, el estado y
   el historial, y se actualiza sola cada 5 segundos.

## Diferencias con el código de ejemplo de la guía

- El envío a internet corre en una **tarea aparte** (núcleo 0) con una cola.
  Si el WiFi o el servidor fallan, el control de la bomba sigue funcionando
  sin pausas. Si el envío falla, reintenta hasta 3 veces.
- El WiFi se conecta **sin bloquear**: la bomba no espera al WiFi para funcionar.
- `POST /api/lecturas` también rechaza valores no finitos (NaN, infinito).
- `GET /api/lecturas` acepta `?sensor=nivel` para pedir un solo sensor.
- La página muestra si el ESP32 está **sin señal** (no llegan datos hace más de 3 minutos).
- El ESP32 también tiene una **página local** (`http://IP-de-la-placa` o
  `http://estanque.local`) que funciona dentro del mismo WiFi, aunque no haya internet.
- Se pueden guardar **varias redes WiFi**: la placa usa la que encuentre (WiFiMulti).
- En este repositorio el código del ESP32 va **sin claves**: las redes WiFi y la
  `API_KEY` se escriben solo en la copia local antes de subirlo a la placa.

## Seguridad

- `.env` nunca se sube a GitHub. Si se sube por error, cambiar la contraseña
  de Aiven y la `API_KEY`.
- No pegar en un chat la `DATABASE_URL`, la contraseña de Aiven, la `API_KEY`
  ni la clave del WiFi.
- `cliente.setInsecure()` cifra la conexión pero no verifica el certificado
  del servidor (aceptable para el curso, como dice la guía).
