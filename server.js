// server.js - API + pagina web del estanque
// Recibe los datos del ESP32 (nivel, bomba, estado) y los guarda en PostgreSQL de Aiven.
// Se despliega en Railway. Basado en la guia TEI201 (ESP32 + Aiven + Railway).

require("dotenv").config();
const express = require("express");
const path = require("path");
const { Pool } = require("pg");

const app = express();
app.use(express.json());
app.use(express.static(path.join(__dirname, "public")));

// ---------- Conexion a la base de datos (Aiven exige SSL) ----------
// Se quita "?sslmode=require" de la URI porque el SSL se configura abajo.
const pool = new Pool({
  connectionString: (process.env.DATABASE_URL || "").split("?")[0],
  ssl: process.env.DB_CA_CERT
    ? { ca: process.env.DB_CA_CERT.replace(/\\n/g, "\n") }
    : { rejectUnauthorized: false },
  max: 5,
});

async function crearTabla() {
  await pool.query(`
    CREATE TABLE IF NOT EXISTS lecturas (
      id SERIAL PRIMARY KEY,
      dispositivo VARCHAR(50) NOT NULL,
      sensor VARCHAR(50) NOT NULL,
      valor REAL NOT NULL,
      creado_en TIMESTAMPTZ DEFAULT NOW()
    )
  `);
  console.log("Tabla 'lecturas' lista");
}

// ---------- Seguridad minima: clave compartida con el ESP32 ----------
function revisarClave(req, res, next) {
  if (!process.env.API_KEY || req.get("x-api-key") !== process.env.API_KEY) {
    return res.status(401).json({ error: "API key invalida" });
  }
  next();
}

// ---------- Rutas ----------
app.get("/api/salud", async (req, res) => {
  try {
    await pool.query("SELECT 1");
    res.json({ ok: true, db: "conectada" });
  } catch (err) {
    res.status(500).json({ ok: false, error: err.message });
  }
});

// El ESP32 manda, por ejemplo:
//   {"dispositivo":"esp32-estanque1","sensor":"nivel","valor":1}
// Sensores que usa este proyecto:
//   nivel  -> 1 = estanque LLENO, 0 = estanque VACIO (flotador P35)
//   bomba  -> 1 = bomba ENCENDIDA, 0 = APAGADA (rele)
//   estado -> 0 APAGADA, 1 ESPERANDO ARRANQUE, 2 ENCENDIDA, 3 ESPERANDO CORTE, 4 DESCANSO
app.post("/api/lecturas", revisarClave, async (req, res) => {
  const { dispositivo, sensor, valor } = req.body;
  if (!dispositivo || !sensor || typeof valor !== "number" || !Number.isFinite(valor)) {
    return res.status(400).json({ error: "Faltan campos o 'valor' no es numero" });
  }
  try {
    const r = await pool.query(
      "INSERT INTO lecturas (dispositivo, sensor, valor) VALUES ($1, $2, $3) RETURNING id",
      [String(dispositivo).slice(0, 50), String(sensor).slice(0, 50), valor]
    );
    res.status(201).json({ ok: true, id: r.rows[0].id });
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: "Error al guardar" });
  }
});

// La pagina web pide las ultimas lecturas.
// Opcional: ?sensor=nivel para pedir solo un sensor.
app.get("/api/lecturas", async (req, res) => {
  const limite = Math.min(parseInt(req.query.limite) || 50, 500);
  const sensor = req.query.sensor;
  try {
    const r = sensor
      ? await pool.query(
          "SELECT * FROM lecturas WHERE sensor = $1 ORDER BY id DESC LIMIT $2",
          [sensor, limite]
        )
      : await pool.query(
          "SELECT * FROM lecturas ORDER BY id DESC LIMIT $1",
          [limite]
        );
    res.json(r.rows);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: "Error al leer" });
  }
});

// ---------- Arranque ----------
const PORT = process.env.PORT || 3000; // Railway define PORT solo
app.listen(PORT, async () => {
  console.log(`Servidor escuchando en el puerto ${PORT}`);
  try {
    await crearTabla();
  } catch (err) {
    console.error("No se pudo conectar a la base de datos:", err.message);
  }
});
