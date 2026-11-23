/*
 * ==========================================================================
 *  LLENADO AUTOMÁTICO DE ESTANQUE CON FLOTADOR P35  -  ESP32 + MÓDULO RELÉ
 *  + PÁGINA WEB LOCAL (dentro del ESP32)
 *  + ENVÍO DE DATOS A LA NUBE (Railway + Aiven PostgreSQL, guía TEI201)
 * ==========================================================================
 *
 *  QUÉ HACE
 *  --------
 *  - El ÚNICO sensor es el flotador P35 dentro del estanque.
 *  - Estanque VACÍO (flotador abajo)  -> se activa el relé y la bomba llena.
 *  - Estanque LLENO (flotador arriba) -> se desactiva el relé y la bomba para.
 *  - El ESP32 nunca toca los 220V: solo manda la señal al pin IN del relé.
 *
 *  DOS FORMAS DE VER LOS DATOS
 *  ---------------------------
 *  1) PÁGINA LOCAL (funciona apenas pongas tu WiFi):
 *       - Abre el Monitor Serie (115200). Aparece la dirección, por ejemplo
 *         http://192.168.1.50  -> ábrela desde un celular o PC en el mismo WiFi.
 *       - También suele funcionar  http://estanque.local
 *       - Si no logra conectarse al WiFi en 20 s, crea su propia red:
 *           Red: "Estanque-ESP32"   Clave: "estanque123"  ->  http://192.168.4.1
 *
 *  2) PÁGINA EN LA NUBE (Railway, link público con historial):
 *       - Cuando tengan Railway listo, pon la URL y la API_KEY en la sección 1.
 *       - El ESP32 manda por HTTPS tres datos ("sensores"):
 *           nivel  -> 1 = LLENO, 0 = VACÍO
 *           bomba  -> 1 = ENCENDIDA, 0 = APAGADA
 *           estado -> 0 APAGADA, 1 ESPERANDO ARRANQUE, 2 ENCENDIDA,
 *                     3 ESPERANDO CORTE, 4 DESCANSO
 *         Los manda apenas cambian y, además, todos juntos cada 60 s.
 *       - Mientras la URL sea la de ejemplo, no intenta enviar nada.
 *
 *  El envío a internet corre en una tarea aparte (núcleo 0). Aunque el WiFi
 *  o el servidor fallen, el control de la bomba (núcleo 1) nunca se detiene.
 *  Las páginas SOLO MUESTRAN información: no pueden encender ni apagar la bomba.
 *
 *  LIBRERÍAS
 *  ---------
 *  - WiFi, WiFiClientSecure, HTTPClient, WebServer, ESPmDNS: vienen con el
 *    paquete "esp32 by Espressif Systems".
 *  - ArduinoJson (Benoit Blanchon, versión 7): instalar desde
 *    Herramientas > Administrar bibliotecas.
 *
 *  CONEXIONES (baja tensión)
 *  -------------------------
 *    Flotador P35, cable 1 -> GPIO 27
 *    Flotador P35, cable 2 -> GND      (usa la pull-up interna del ESP32)
 *    Relé VCC -> 5V (VIN) | Relé GND -> GND | Relé 1 de la PCB -> GPIO 13 (Relé 2 = GPIO 15)
 *
 *  CONEXIONES (220V, SIEMPRE CON LA ENERGÍA CORTADA)
 *  -------------------------------------------------
 *    Fase (L) -> COM del relé
 *    NO del relé -> Fase de la bomba
 *    Neutro y tierra -> directo a la bomba
 *
 *  LÓGICA (máquina de estados)
 *  ---------------------------
 *    APAGADA ──(flotador marca VACÍO)──> ESPERANDO_ARRANQUE
 *    ESPERANDO_ARRANQUE ──(sigue vacío "retardoArranqueS")──> ENCENDIDA
 *    ENCENDIDA ──(flotador marca LLENO)──> ESPERANDO_CORTE
 *    ESPERANDO_CORTE ──(sigue lleno "retardoCorteS")──> DESCANSO
 *    DESCANSO ──(pasa "descansoMinimoS")──> APAGADA
 * ==========================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiMulti.h>
#include <ArduinoJson.h>
#include <stdarg.h>


// ==========================================================================
// 1. REDES WIFI   <<<<<  AQUÍ SE CAMBIA EL INTERNET  >>>>>
// ==========================================================================
//
//  Escribe las redes WiFi de los lugares donde vas a usar la placa.
//  Cada línea es:   { "NOMBRE DE LA RED", "CLAVE DE LA RED" },
//
//  - La placa se conecta sola a la que encuentre disponible en ese lugar.
//  - Para agregar otra red: copia una línea completa y cambia nombre y clave.
//  - Para quitar una red: borra su línea.
//  - Solo redes de 2,4 GHz. Respeta mayúsculas y minúsculas.
//  - No borres las comillas, las llaves { } ni la coma del final.
//  - Después de cambiar algo: guardar (Ctrl+S) y volver a Subir a la placa.

const char* REDES_WIFI[][2] = {
  { "NOMBRE_RED_1", "CLAVE_RED_1" },
  { "NOMBRE_RED_2", "CLAVE_RED_2" },
};


// Datos del servidor en Railway. No hace falta tocarlos ahora:
// la URL se pone una sola vez cuando Railway esté listo.
const char* SERVER_URL  = "https://esp32-railway-estanque-production.up.railway.app/api/lecturas";
const char* API_KEY     = "la-misma-clave-que-pusiste-en-railway";   // igual a la variable API_KEY de Railway

const char* DISPOSITIVO = "esp32-estanque1";
const uint32_t INTERVALO_ENVIO_MS = 60000;   // envío completo a la nube cada 60 s

const char* NOMBRE_EQUIPO = "estanque";      // página local -> http://estanque.local


// ==========================================================================
// 2. PARÁMETROS DEL ESTANQUE
// ==========================================================================

const char*    NOMBRE_ESTANQUE    = "Estanque principal";
const float    CAPACIDAD_LITROS   = 2000.0;  // Tamaño del estanque (litros)

const uint32_t RETARDO_ARRANQUE_S = 10;      // Segundos que el flotador debe marcar
                                             // VACÍO antes de ENCENDER la bomba
const uint32_t RETARDO_CORTE_S    = 0;       // Segundos que la bomba sigue después
                                             // de marcar LLENO (0 = corta al tiro)
const uint32_t DESCANSO_MINIMO_S  = 60;      // Segundos mínimos con la bomba apagada
                                             // antes de otro arranque (protege el motor)


// ==========================================================================
// 3. CONFIGURACIÓN DE HARDWARE
// ==========================================================================

const uint8_t PIN_FLOTADOR = 27;   // Señal del flotador P35
const uint8_t PIN_RELE     = 13;   // Relé 1 de la placa UAI (Relé 2 = GPIO 15)
const uint8_t PIN_LED      = 2;    // LED integrado: encendido = bomba funcionando

// La mayoría de los módulos relé se activan con señal LOW.
const bool RELE_ACTIVO_EN_BAJO = false;  // en la placa UAI el relé se activa con HIGH (transistor)

// true  -> contacto cerrado = estanque VACÍO (flotador abajo)
// false -> contacto cerrado = estanque LLENO (flotador arriba)
// Si la bomba hace lo contrario de lo esperado, cambia este valor.
const bool CONTACTO_CERRADO_ES_VACIO = true;

const uint32_t ANTIRREBOTE_MS       = 80;    // Filtro eléctrico del contacto
const uint32_t INTERVALO_REPORTE_MS = 5000;  // Cada cuánto informa por Serial


// ==========================================================================
// 4. VARIABLES INTERNAS
// ==========================================================================

enum class Estado : uint8_t {
  APAGADA = 0,             // Estanque lleno, bomba detenida
  ESPERANDO_ARRANQUE = 1,  // Flotador marca vacío, confirmando antes de encender
  ENCENDIDA = 2,           // Bomba llenando el estanque
  ESPERANDO_CORTE = 3,     // Flotador marca lleno, cumpliendo el retardo de corte
  DESCANSO = 4             // Bomba apagada, protegiendo el motor
};

Estado   estado        = Estado::DESCANSO;
uint32_t tInicioEstado = 0;

// Antirrebote del flotador
bool     lecturaCruda       = false;
bool     vacioEstable       = false;
uint32_t tUltimoCambioCrudo = 0;

uint32_t tUltimoReporte = 0;

// Estadísticas para la página local
bool     bombaEncendida    = false;
uint32_t tBombaOnDesde     = 0;
uint64_t msBombaTotal      = 0;
uint32_t cantidadArranques = 0;
uint32_t tInicioLlenado    = 0;
uint32_t ultimoLlenadoS    = 0;
bool     hayUltimoLlenado  = false;
uint32_t tFinUltimoLlenado = 0;

// Registro de los últimos eventos (memoria circular)
const uint8_t MAX_EVENTOS = 15;
struct Evento {
  uint32_t t;          // segundos desde el encendido
  char     texto[64];
};
Evento  eventos[MAX_EVENTOS];
uint8_t eventoSiguiente  = 0;
uint8_t eventosGuardados = 0;

// WiFi y página local
WebServer servidor(80);
WiFiMulti wifiMulti;           // prueba todas las redes de la lista
uint8_t  redesValidas  = 0;
bool     modoAP        = false;
bool     mdnsIniciado  = false;
bool     wifiConectado = false;
uint32_t tInicioWifi   = 0;

// Envío a la nube
bool     nubeConfigurada = false;
int      nivelEnviado  = -1;
int      bombaEnviada  = -1;
int      estadoEnviado = -1;
uint32_t tUltimoEnvioCompleto = 0;

struct Lectura {
  char  sensor[12];
  float valor;
};
QueueHandle_t colaEnvio;
const uint8_t TAMANO_COLA = 30;

// Resultado de los envíos (los escribe la tarea del núcleo 0)
volatile uint32_t enviosOk        = 0;
volatile uint32_t enviosError     = 0;
volatile int      ultimoCodigo    = 0;
volatile uint32_t tUltimoEnvioOk  = 0;
volatile bool     hayEnvioOk      = false;


// ==========================================================================
// 5. FUNCIONES AUXILIARES DEL CONTROL
// ==========================================================================

const char* nombreEstado(Estado e) {
  switch (e) {
    case Estado::APAGADA:            return "APAGADA";
    case Estado::ESPERANDO_ARRANQUE: return "ESPERANDO ARRANQUE";
    case Estado::ENCENDIDA:          return "ENCENDIDA";
    case Estado::ESPERANDO_CORTE:    return "ESPERANDO CORTE";
    case Estado::DESCANSO:           return "DESCANSO";
  }
  return "?";
}

// Tiempo transcurrido; funciona aunque millis() se desborde (~49 días).
uint32_t transcurrido(uint32_t desde) {
  return millis() - desde;
}

// Guarda un evento en el registro y lo muestra por Serial.
void registrarEvento(const char* formato, ...) {
  Evento& e = eventos[eventoSiguiente];
  e.t = millis() / 1000;
  va_list args;
  va_start(args, formato);
  vsnprintf(e.texto, sizeof(e.texto), formato, args);
  va_end(args);
  Serial.printf("[%lu s] %s\n", (unsigned long)e.t, e.texto);

  eventoSiguiente = (eventoSiguiente + 1) % MAX_EVENTOS;
  if (eventosGuardados < MAX_EVENTOS) eventosGuardados++;
}

void escribirRele(bool encender) {
  bool nivelPin = RELE_ACTIVO_EN_BAJO ? !encender : encender;
  digitalWrite(PIN_RELE, nivelPin ? HIGH : LOW);
}

bool lecturaVacio() {
  // INPUT_PULLUP: contacto cerrado -> LOW
  bool contactoCerrado = (digitalRead(PIN_FLOTADOR) == LOW);
  return CONTACTO_CERRADO_ES_VACIO ? contactoCerrado : !contactoCerrado;
}

// Lee el flotador con antirrebote. Devuelve true si el estanque está VACÍO.
bool estanqueVacio() {
  bool vacio = lecturaVacio();
  if (vacio != lecturaCruda) {
    lecturaCruda = vacio;
    tUltimoCambioCrudo = millis();
  }
  if (transcurrido(tUltimoCambioCrudo) >= ANTIRREBOTE_MS) {
    vacioEstable = lecturaCruda;
  }
  return vacioEstable;
}

void cambiarEstado(Estado nuevo) {
  Estado anterior = estado;

  if (anterior == Estado::ESPERANDO_ARRANQUE && nuevo == Estado::ENCENDIDA) {
    cantidadArranques++;
    tInicioLlenado = millis();
  }
  if (anterior == Estado::ESPERANDO_CORTE && nuevo == Estado::DESCANSO &&
      cantidadArranques > 0) {
    ultimoLlenadoS    = transcurrido(tInicioLlenado) / 1000;
    tFinUltimoLlenado = millis();
    hayUltimoLlenado  = true;
  }

  registrarEvento("%s -> %s", nombreEstado(anterior), nombreEstado(nuevo));
  estado = nuevo;
  tInicioEstado = millis();
}

bool bombaDebeEstarEncendida() {
  return estado == Estado::ENCENDIDA || estado == Estado::ESPERANDO_CORTE;
}

void actualizarSalidas() {
  bool bombaOn = bombaDebeEstarEncendida();

  // Acumula el tiempo de funcionamiento de la bomba
  if (bombaOn && !bombaEncendida) {
    tBombaOnDesde = millis();
  } else if (!bombaOn && bombaEncendida) {
    msBombaTotal += transcurrido(tBombaOnDesde);
  }
  bombaEncendida = bombaOn;

  escribirRele(bombaOn);
  digitalWrite(PIN_LED, bombaOn ? HIGH : LOW);
}

uint32_t segundosBombaTotal() {
  uint64_t ms = msBombaTotal;
  if (bombaEncendida) ms += transcurrido(tBombaOnDesde);
  return (uint32_t)(ms / 1000);
}

// Segundos que faltan para terminar el retardo actual (-1 si no hay cuenta regresiva).
long segundosRestantes() {
  uint32_t enEstado = transcurrido(tInicioEstado) / 1000;
  uint32_t limite;
  switch (estado) {
    case Estado::ESPERANDO_ARRANQUE: limite = RETARDO_ARRANQUE_S; break;
    case Estado::ESPERANDO_CORTE:    limite = RETARDO_CORTE_S;    break;
    case Estado::DESCANSO:           limite = DESCANSO_MINIMO_S;  break;
    default: return -1;
  }
  return enEstado >= limite ? 0 : (long)(limite - enEstado);
}


// ==========================================================================
// 6. MÁQUINA DE ESTADOS   (toda la decisión depende del flotador)
// ==========================================================================

void actualizarMaquina(bool vacio) {
  uint32_t enEstado = transcurrido(tInicioEstado);

  switch (estado) {
    case Estado::APAGADA:
      if (vacio) cambiarEstado(Estado::ESPERANDO_ARRANQUE);
      break;

    case Estado::ESPERANDO_ARRANQUE:
      if (!vacio) {
        cambiarEstado(Estado::APAGADA);              // fue solo una oscilación
      } else if (enEstado >= RETARDO_ARRANQUE_S * 1000UL) {
        cambiarEstado(Estado::ENCENDIDA);
      }
      break;

    case Estado::ENCENDIDA:
      if (!vacio) cambiarEstado(Estado::ESPERANDO_CORTE);
      break;

    case Estado::ESPERANDO_CORTE:
      if (vacio) {
        cambiarEstado(Estado::ENCENDIDA);            // volvió a bajar: seguir
      } else if (enEstado >= RETARDO_CORTE_S * 1000UL) {
        cambiarEstado(Estado::DESCANSO);
      }
      break;

    case Estado::DESCANSO:
      if (enEstado >= DESCANSO_MINIMO_S * 1000UL) {
        cambiarEstado(Estado::APAGADA);
      }
      break;
  }
}

void reportarEstado(bool vacio) {
  if (transcurrido(tUltimoReporte) < INTERVALO_REPORTE_MS) return;
  tUltimoReporte = millis();
  Serial.printf("Estanque: %-5s | Bomba: %-18s | WiFi: %s | Nube: %s\n",
                vacio ? "VACIO" : "LLENO",
                nombreEstado(estado),
                WiFi.status() == WL_CONNECTED ? "OK" : (modoAP ? "red propia" : "sin conexion"),
                nubeConfigurada ? (hayEnvioOk ? "enviando" : "pendiente") : "no configurada");
}


// ==========================================================================
// 7. ENVÍO A LA NUBE (Railway)
// ==========================================================================

void encolar(const char* sensor, float valor) {
  Lectura l;
  strncpy(l.sensor, sensor, sizeof(l.sensor) - 1);
  l.sensor[sizeof(l.sensor) - 1] = '\0';
  l.valor = valor;
  if (xQueueSend(colaEnvio, &l, 0) != pdTRUE) {
    Serial.println("Cola de envio llena: se descarta una lectura");
  }
}

// Revisa qué cambió y lo encola. Cada INTERVALO_ENVIO_MS manda todo igual.
void programarEnvios(bool vacio) {
  if (!nubeConfigurada) return;

  int nivel = vacio ? 0 : 1;
  int bomba = bombaDebeEstarEncendida() ? 1 : 0;
  int est   = (int)estado;
  bool todo = transcurrido(tUltimoEnvioCompleto) >= INTERVALO_ENVIO_MS;

  if (todo || nivel != nivelEnviado)  { encolar("nivel",  nivel); nivelEnviado  = nivel; }
  if (todo || bomba != bombaEnviada)  { encolar("bomba",  bomba); bombaEnviada  = bomba; }
  if (todo || est   != estadoEnviado) { encolar("estado", est);   estadoEnviado = est;   }
  if (todo) tUltimoEnvioCompleto = millis();
}

// Manda una lectura por POST HTTPS. Devuelve el código HTTP (o negativo si falló).
int enviarLectura(const Lectura& l) {
  WiFiClientSecure cliente;
  cliente.setInsecure();   // cifra, pero no verifica el certificado (ver guía 12.3)

  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(cliente, SERVER_URL)) return -1;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-api-key", API_KEY);

  JsonDocument doc;
  doc["dispositivo"] = DISPOSITIVO;
  doc["sensor"]      = l.sensor;
  doc["valor"]       = l.valor;
  String cuerpo;
  serializeJson(doc, cuerpo);

  int codigo = http.POST(cuerpo);
  Serial.printf("POST %s -> %d\n", cuerpo.c_str(), codigo);
  if (codigo > 0) {
    Serial.println(http.getString());
  } else {
    Serial.printf("Error: %s\n", http.errorToString(codigo).c_str());
  }
  http.end();
  return codigo;
}

// Tarea del núcleo 0: vacía la cola y manda las lecturas a Railway.
void tareaEnvio(void*) {
  Lectura l;
  bool hayPendiente = false;
  uint8_t intentos = 0;

  for (;;) {
    if (WiFi.status() != WL_CONNECTED) {
      // Busca y se conecta a una de las redes de la lista (solo frena este núcleo,
      // el control de la bomba sigue funcionando en el otro).
      if (redesValidas > 0) wifiMulti.run(10000);
      vTaskDelay(pdMS_TO_TICKS(2000));
      continue;
    }

    if (!hayPendiente) {
      if (xQueueReceive(colaEnvio, &l, pdMS_TO_TICKS(1000)) != pdTRUE) continue;
      hayPendiente = true;
      intentos = 0;
    }

    int codigo = enviarLectura(l);
    ultimoCodigo = codigo;

    if (codigo == 201) {
      enviosOk++;
      tUltimoEnvioOk = millis();
      hayEnvioOk = true;
      hayPendiente = false;
    } else if (codigo == 400 || codigo == 401 || codigo == 404) {
      // Error de configuración: reintentar no sirve.
      if (codigo == 401) Serial.println(">> Revisa que API_KEY sea igual en el ESP32 y en Railway");
      if (codigo == 404) Serial.println(">> Revisa que SERVER_URL termine en /api/lecturas");
      enviosError++;
      hayPendiente = false;
    } else if (++intentos >= 3) {
      Serial.println("No se pudo enviar despues de 3 intentos: se descarta");
      enviosError++;
      hayPendiente = false;
    } else {
      vTaskDelay(pdMS_TO_TICKS(3000));   // esperar y reintentar
    }
  }
}


// ==========================================================================
// 8. PÁGINA WEB LOCAL
// ==========================================================================

const char PAGINA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Estanque</title>
<style>
  :root { --bg:#f2f5f9; --card:#fff; --txt:#1c2733; --sub:#66768a;
          --azul:#1f7ae0; --verde:#1a9e5a; --rojo:#d64545; --ambar:#d98a00; }
  * { box-sizing:border-box; }
  body { margin:0; font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif;
         background:var(--bg); color:var(--txt); }
  header { padding:18px 16px 6px; max-width:900px; margin:auto; }
  h1 { margin:0; font-size:1.35rem; }
  #conexion { font-size:.85rem; color:var(--sub); margin-top:4px; }
  #conexion.error { color:var(--rojo); font-weight:600; }
  main { padding:10px 16px 24px; max-width:900px; margin:auto;
         display:grid; gap:12px; grid-template-columns:repeat(auto-fit,minmax(200px,1fr)); }
  .card { background:var(--card); border-radius:14px; padding:14px 16px;
          box-shadow:0 1px 3px rgba(0,0,0,.08); }
  .card.ancha { grid-column:1/-1; }
  .titulo { font-size:.78rem; text-transform:uppercase; letter-spacing:.05em; color:var(--sub); }
  .valor { font-size:1.6rem; font-weight:700; margin-top:6px; }
  .detalle { font-size:.85rem; color:var(--sub); margin-top:4px; }
  .lleno { color:var(--azul); } .vacio { color:var(--ambar); }
  .on { color:var(--verde); }   .off { color:var(--sub); } .mal { color:var(--rojo); }
  .tanque { width:70px; height:90px; border:3px solid var(--txt); border-radius:6px;
            position:relative; overflow:hidden; float:right; margin-left:10px; }
  .agua { position:absolute; left:0; right:0; bottom:0; background:var(--azul);
          transition:height .6s; opacity:.8; }
  table { width:100%; border-collapse:collapse; font-size:.88rem; }
  td { padding:6px 4px; border-bottom:1px solid #e6ebf1; }
  td:first-child { color:var(--sub); width:90px; white-space:nowrap; }
  .params td:last-child { text-align:right; font-weight:600; }
  @media (prefers-color-scheme: dark) {
    :root { --bg:#10161d; --card:#1b242e; --txt:#e6edf3; --sub:#8c9bab; }
    td { border-color:#2a3542; }
  }
</style>
</head>
<body>
<header>
  <h1 id="nombre">Estanque</h1>
  <div id="conexion">Conectando...</div>
</header>
<main>
  <div class="card">
    <div class="tanque"><div class="agua" id="agua"></div></div>
    <div class="titulo">Nivel (flotador)</div>
    <div class="valor" id="nivel">-</div>
    <div class="detalle" id="capacidad"></div>
  </div>
  <div class="card">
    <div class="titulo">Bomba</div>
    <div class="valor" id="bomba">-</div>
    <div class="detalle" id="estado"></div>
  </div>
  <div class="card">
    <div class="titulo">Tiempo en este estado</div>
    <div class="valor" id="enEstado">-</div>
    <div class="detalle" id="restante"></div>
  </div>
  <div class="card">
    <div class="titulo">Llenados</div>
    <div class="valor" id="arranques">-</div>
    <div class="detalle" id="ultimo"></div>
  </div>
  <div class="card">
    <div class="titulo">Bomba funcionando (total)</div>
    <div class="valor" id="bombaTotal">-</div>
    <div class="detalle">desde que se encendió el equipo</div>
  </div>
  <div class="card">
    <div class="titulo">Nube (Railway)</div>
    <div class="valor" id="nube" style="font-size:1.2rem">-</div>
    <div class="detalle" id="nubeDetalle"></div>
  </div>
  <div class="card">
    <div class="titulo">Equipo</div>
    <div class="valor" id="uptime">-</div>
    <div class="detalle" id="wifi"></div>
  </div>
  <div class="card ancha">
    <div class="titulo">Últimos eventos</div>
    <table id="eventos"><tr><td colspan="2">Sin eventos todavía</td></tr></table>
  </div>
  <div class="card ancha">
    <div class="titulo">Parámetros configurados</div>
    <table class="params" id="params"></table>
  </div>
</main>
<script>
function dur(s) {
  if (s < 0) return '-';
  const d = Math.floor(s / 86400), h = Math.floor(s % 86400 / 3600),
        m = Math.floor(s % 3600 / 60), x = s % 60;
  if (d) return d + 'd ' + h + 'h ' + m + 'm';
  if (h) return h + 'h ' + m + 'm';
  if (m) return m + 'm ' + x + 's';
  return x + 's';
}
function $(id) { return document.getElementById(id); }

async function actualizar() {
  try {
    const r = await fetch('/api/estado', {cache:'no-store'});
    const d = await r.json();

    $('nombre').textContent = d.nombre;
    document.title = d.nombre;
    $('conexion').className = '';
    $('conexion').textContent = 'En línea · actualizado ' + new Date().toLocaleTimeString();

    $('nivel').textContent = d.vacio ? 'VACÍO' : 'LLENO';
    $('nivel').className = 'valor ' + (d.vacio ? 'vacio' : 'lleno');
    $('agua').style.height = d.vacio ? '15%' : '90%';
    $('capacidad').textContent = 'Capacidad: ' + d.capacidadLitros + ' L';

    $('bomba').textContent = d.bomba ? 'ENCENDIDA' : 'APAGADA';
    $('bomba').className = 'valor ' + (d.bomba ? 'on' : 'off');
    $('estado').textContent = 'Estado: ' + d.estado;

    $('enEstado').textContent = dur(d.segEnEstado);
    $('restante').textContent = d.segRestantes >= 0
      ? 'Faltan ' + dur(d.segRestantes) + ' para el siguiente paso' : '';

    $('arranques').textContent = d.arranques;
    $('ultimo').textContent = d.ultimoLlenadoS >= 0
      ? 'Último llenado: ' + dur(d.ultimoLlenadoS) + ' (hace ' + dur(d.haceUltimoS) + ')'
      : 'Aún no hay llenados completos';

    $('bombaTotal').textContent = dur(d.segBombaTotal);

    const n = d.nube;
    if (!n.configurada) {
      $('nube').textContent = 'SIN CONFIGURAR'; $('nube').className = 'valor off';
      $('nubeDetalle').textContent = 'Falta poner la URL de Railway en el código';
    } else if (n.haceOkS >= 0 && n.haceOkS < 180) {
      $('nube').textContent = 'ENVIANDO'; $('nube').className = 'valor on';
      $('nubeDetalle').textContent = n.ok + ' datos enviados · último hace ' + dur(n.haceOkS);
    } else {
      $('nube').textContent = 'SIN ENVÍO'; $('nube').className = 'valor mal';
      $('nubeDetalle').textContent = 'Último código: ' + n.ultimoCodigo + ' · errores: ' + n.errores;
    }

    $('uptime').textContent = dur(d.uptimeS);
    $('wifi').textContent = (d.modoAP ? 'Red propia ' : 'WiFi ') + d.ip +
      (d.modoAP ? '' : ' · señal ' + d.rssi + ' dBm');

    const ev = d.eventos;
    $('eventos').innerHTML = ev.length
      ? ev.map(e => '<tr><td>hace ' + dur(d.uptimeS - e.t) + '</td><td>' + e.txt + '</td></tr>').join('')
      : '<tr><td colspan="2">Sin eventos todavía</td></tr>';

    const p = d.parametros;
    $('params').innerHTML =
      '<tr><td>Retardo de arranque</td><td>' + p.retardoArranqueS + ' s</td></tr>' +
      '<tr><td>Retardo de corte</td><td>' + p.retardoCorteS + ' s</td></tr>' +
      '<tr><td>Descanso mínimo</td><td>' + p.descansoMinimoS + ' s</td></tr>';
  } catch (e) {
    $('conexion').className = 'error';
    $('conexion').textContent = 'Sin conexión con el equipo. Reintentando...';
  }
}
actualizar();
setInterval(actualizar, 2000);
</script>
</body>
</html>
)rawliteral";

void enviarPagina() {
  servidor.send_P(200, "text/html; charset=utf-8", PAGINA_HTML);
}

// Arma un JSON con todo el estado del sistema.
void enviarEstadoJson() {
  String j;
  j.reserve(2400);
  char tmp[200];

  uint32_t uptime = millis() / 1000;

  j += "{";
  j += "\"nombre\":\"";  j += NOMBRE_ESTANQUE;  j += "\",";
  snprintf(tmp, sizeof(tmp), "\"capacidadLitros\":%.0f,", CAPACIDAD_LITROS);       j += tmp;
  snprintf(tmp, sizeof(tmp), "\"vacio\":%s,", vacioEstable ? "true" : "false");     j += tmp;
  snprintf(tmp, sizeof(tmp), "\"bomba\":%s,", bombaEncendida ? "true" : "false");   j += tmp;
  snprintf(tmp, sizeof(tmp), "\"estado\":\"%s\",", nombreEstado(estado));           j += tmp;
  snprintf(tmp, sizeof(tmp), "\"segEnEstado\":%lu,",
           (unsigned long)(transcurrido(tInicioEstado) / 1000));                   j += tmp;
  snprintf(tmp, sizeof(tmp), "\"segRestantes\":%ld,", segundosRestantes());         j += tmp;
  snprintf(tmp, sizeof(tmp), "\"arranques\":%lu,", (unsigned long)cantidadArranques); j += tmp;
  snprintf(tmp, sizeof(tmp), "\"ultimoLlenadoS\":%ld,",
           hayUltimoLlenado ? (long)ultimoLlenadoS : -1L);                          j += tmp;
  snprintf(tmp, sizeof(tmp), "\"haceUltimoS\":%ld,",
           hayUltimoLlenado ? (long)(transcurrido(tFinUltimoLlenado) / 1000) : -1L); j += tmp;
  snprintf(tmp, sizeof(tmp), "\"segBombaTotal\":%lu,",
           (unsigned long)segundosBombaTotal());                                    j += tmp;
  snprintf(tmp, sizeof(tmp), "\"uptimeS\":%lu,", (unsigned long)uptime);            j += tmp;
  snprintf(tmp, sizeof(tmp), "\"modoAP\":%s,", modoAP ? "true" : "false");          j += tmp;

  String ip = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString()
                                              : WiFi.softAPIP().toString();
  j += "\"ip\":\"";  j += ip;  j += "\",";
  snprintf(tmp, sizeof(tmp), "\"rssi\":%d,",
           WiFi.status() == WL_CONNECTED ? (int)WiFi.RSSI() : 0);                   j += tmp;

  snprintf(tmp, sizeof(tmp),
           "\"nube\":{\"configurada\":%s,\"ok\":%lu,\"errores\":%lu,\"ultimoCodigo\":%d,\"haceOkS\":%ld},",
           nubeConfigurada ? "true" : "false",
           (unsigned long)enviosOk, (unsigned long)enviosError, (int)ultimoCodigo,
           hayEnvioOk ? (long)(transcurrido(tUltimoEnvioOk) / 1000) : -1L);         j += tmp;

  snprintf(tmp, sizeof(tmp),
           "\"parametros\":{\"retardoArranqueS\":%lu,\"retardoCorteS\":%lu,\"descansoMinimoS\":%lu},",
           (unsigned long)RETARDO_ARRANQUE_S, (unsigned long)RETARDO_CORTE_S,
           (unsigned long)DESCANSO_MINIMO_S);                                       j += tmp;

  // Eventos, del más reciente al más antiguo
  j += "\"eventos\":[";
  for (uint8_t i = 0; i < eventosGuardados; i++) {
    uint8_t idx = (eventoSiguiente + MAX_EVENTOS - 1 - i) % MAX_EVENTOS;
    snprintf(tmp, sizeof(tmp), "%s{\"t\":%lu,\"txt\":\"%s\"}",
             i ? "," : "", (unsigned long)eventos[idx].t, eventos[idx].texto);
    j += tmp;
  }
  j += "]}";

  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send(200, "application/json", j);
}

void configurarServidor() {
  servidor.on("/", enviarPagina);
  servidor.on("/api/estado", enviarEstadoJson);
  servidor.onNotFound([]() { servidor.send(404, "text/plain", "No encontrado"); });
  servidor.begin();
}

// Maneja la conexión WiFi sin bloquear el control de la bomba.
void atenderWifi() {
  bool conectadoAhora = (WiFi.status() == WL_CONNECTED);

  if (conectadoAhora && !wifiConectado) {
    registrarEvento("WiFi conectado: http://%s", WiFi.localIP().toString().c_str());
    if (!mdnsIniciado && MDNS.begin(NOMBRE_EQUIPO)) {
      MDNS.addService("http", "tcp", 80);
      mdnsIniciado = true;
      Serial.printf("Tambien disponible en: http://%s.local\n", NOMBRE_EQUIPO);
    }
    Serial.printf("Red WiFi: %s  ->  abre  http://%s  en el celular o PC (mismo WiFi)\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  } else if (!conectadoAhora && wifiConectado) {
    registrarEvento("WiFi desconectado, buscando redes...");
    tInicioWifi = millis();
  }
  wifiConectado = conectadoAhora;

  // Aviso cada 30 s mientras no encuentre ninguna red de la lista
  static uint32_t tAviso = 0;
  if (!conectadoAhora && transcurrido(tAviso) >= 30000) {
    tAviso = millis();
    if (redesValidas == 0) {
      Serial.println(">> No hay redes WiFi en la lista: escribelas en la seccion 1 del codigo");
    } else {
      Serial.println(">> Ninguna red de la lista disponible. Revisa nombre/clave en la seccion 1");
    }
  }
}


// ==========================================================================
// 9. SETUP Y LOOP
// ==========================================================================

void setup() {
  // Relé en APAGADO antes de configurarlo como salida: evita un golpe a la bomba.
  escribirRele(false);
  pinMode(PIN_RELE, OUTPUT);
  escribirRele(false);

  pinMode(PIN_FLOTADOR, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);

  Serial.begin(115200);
  delay(300);

  lecturaCruda = lecturaVacio();
  vacioEstable = lecturaCruda;
  tUltimoCambioCrudo = millis();

  // Se parte en DESCANSO: tras un corte de luz la bomba no arranca de golpe.
  estado = Estado::DESCANSO;
  tInicioEstado = millis();

  // La nube solo se usa si la URL ya no es la de ejemplo
  nubeConfigurada = (strstr(SERVER_URL, "TU-APP") == nullptr);

  Serial.println("\n=========================================");
  Serial.println(" Llenado automatico con flotador P35");
  Serial.println("=========================================");
  Serial.printf(" Estanque          : %s (%.0f L)\n", NOMBRE_ESTANQUE, CAPACIDAD_LITROS);
  Serial.printf(" Retardo arranque  : %lu s\n", (unsigned long)RETARDO_ARRANQUE_S);
  Serial.printf(" Retardo corte     : %lu s\n", (unsigned long)RETARDO_CORTE_S);
  Serial.printf(" Descanso minimo   : %lu s\n", (unsigned long)DESCANSO_MINIMO_S);
  Serial.printf(" Nube (Railway)    : %s\n", nubeConfigurada ? SERVER_URL : "no configurada");
  Serial.println("=========================================\n");

  // WiFi sin bloquear: el control de la bomba parte de inmediato.
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(NOMBRE_EQUIPO);
  WiFi.setAutoReconnect(true);
  for (size_t i = 0; i < sizeof(REDES_WIFI) / sizeof(REDES_WIFI[0]); i++) {
    const char* nombre = REDES_WIFI[i][0];
    const char* clave  = REDES_WIFI[i][1];
    // Se saltan las líneas de ejemplo que todavía no tienen datos reales
    if (strncmp(nombre, "NOMBRE_", 7) == 0 || strncmp(clave, "CLAVE_", 6) == 0) continue;
    wifiMulti.addAP(nombre, clave);
    redesValidas++;
    Serial.printf("Red WiFi en la lista: \"%s\"\n", nombre);
  }
  tInicioWifi = millis();
  Serial.printf("Buscando una de %u redes WiFi...\n", redesValidas);

  configurarServidor();

  colaEnvio = xQueueCreate(TAMANO_COLA, sizeof(Lectura));
  xTaskCreatePinnedToCore(tareaEnvio, "envio", 8192, nullptr, 1, nullptr, 0);
  tUltimoEnvioCompleto = millis() - INTERVALO_ENVIO_MS;   // primer envío al tiro

  registrarEvento("Equipo encendido (descanso inicial %lu s)",
                  (unsigned long)DESCANSO_MINIMO_S);
}

void loop() {
  bool vacio = estanqueVacio();  // 1) Leer el flotador P35
  actualizarMaquina(vacio);      // 2) Decidir
  actualizarSalidas();           // 3) Mandar la señal al relé
  programarEnvios(vacio);        // 4) Dejar los cambios listos para la nube
  reportarEstado(vacio);         // 5) Informar por Serial
  atenderWifi();                 // 6) Mantener el WiFi
  servidor.handleClient();       // 7) Responder a la página local

  delay(10);
}
