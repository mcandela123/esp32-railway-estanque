# API del servidor

Base: `https://esp32-railway-estanque-production.up.railway.app`

| Método | Ruta | Recibe | Devuelve | Requiere `x-api-key` |
|---|---|---|---|---|
| `GET` | `/api/salud` | Nada | `200 {"ok":true,"db":"conectada"}` o `500 {"ok":false,"error":"..."}` | No |
| `POST` | `/api/lecturas` | JSON `{"dispositivo": texto, "sensor": texto, "valor": número}` | `201 {"ok":true,"id":N}` | **Sí** |
| `GET` | `/api/lecturas` | Query opcional: `limite` (por defecto 50, máximo 500) y `sensor` | `200 [ {id, dispositivo, sensor, valor, creado_en}, ... ]`, de la más nueva a la más vieja | No |
| `GET` | `/` | Nada | La página `public/index.html` | No |

## Códigos de respuesta de `POST /api/lecturas`

| Código | Significado | Qué revisar |
|---|---|---|
| 201 | Lectura guardada | — |
| 400 | Faltan campos o `valor` no es un número finito | Nombres de los campos y que el sensor no devuelva NaN |
| 401 | `x-api-key` incorrecta o ausente | Que la `API_KEY` sea igual en el ESP32 y en Railway |
| 500 | El servidor no pudo guardar | Logs de Railway y `/api/salud` |

## Valores que manda el ESP32

| `sensor` | `valor` |
|---|---|
| `nivel` | 1 = estanque LLENO, 0 = VACÍO |
| `bomba` | 1 = bomba ENCENDIDA, 0 = APAGADA |
| `estado` | 0 APAGADA, 1 ESPERANDO ARRANQUE, 2 ENCENDIDA, 3 ESPERANDO CORTE, 4 DESCANSO |

## Ejemplos

Leer las últimas 20 lecturas del nivel:

```
GET /api/lecturas?sensor=nivel&limite=20
```

Enviar una lectura de prueba (PowerShell):

```powershell
Invoke-RestMethod -Method Post -Uri https://esp32-railway-estanque-production.up.railway.app/api/lecturas `
  -Headers @{ "x-api-key" = "SU_API_KEY" } -ContentType "application/json" `
  -Body '{"dispositivo":"pc-prueba","sensor":"nivel","valor":1}'
```

Comprobado el 22-11-2026: `/api/salud` responde `{"ok":true,"db":"conectada"}` y un
`POST` con una clave incorrecta responde `401`.

## Página local del ESP32

Además, el ESP32 tiene su propia API dentro de la red WiFi:

| Método | Ruta | Devuelve |
|---|---|---|
| `GET` | `http://IP-de-la-placa/` | Página local con el estado en tiempo real |
| `GET` | `http://IP-de-la-placa/api/estado` | JSON con nivel, bomba, estado, tiempos, llenados, estado de la nube y últimos 15 eventos |
