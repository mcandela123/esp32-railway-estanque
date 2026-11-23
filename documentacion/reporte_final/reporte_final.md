# Reporte final — Llenado automático de estanque

TEI201 Introducción al Diseño en Ingeniería · Universidad Adolfo Ibáñez · Noviembre de 2026

**Equipo:** Claudio Martínez (código), Diego Poupin (diseño 3D) y Mateo Candela (electrónica y testing)

## Archivos

| Formato | Archivo |
|---|---|
| **PDF (entrega, 13 páginas)** | [`reporte_final.pdf`](reporte_final.pdf) |
| HTML (fuente editable, con las mismas imágenes del repositorio) | [`reporte_final.html`](reporte_final.html) |

Para regenerar el PDF después de editar el HTML: abrirlo en el navegador → Imprimir →
Guardar como PDF (tamaño A4, sin encabezados).

## Contenido

1. **Problema, contexto y ODS:** en Culiprán y San Pedro (Melipilla) no hay agua potable
   por tuberías; el agua se saca de pozo y se guarda en estanques que rebalsan porque la
   bomba se maneja a mano. ODS 6, metas 6.4 y 6.1.
2. **Solución propuesta:** ESP32 + flotador P35 + relé, con envío a Railway y Aiven.
3. **Software:** captura de datos, control y comunicación en núcleos separados, API, dashboard y seguridad.
4. **Hardware:** componentes justificados, conexiones, reparación y BOM ($12.000 CLP).
5. **Diseño 3D:** 7 piezas de 10 cm o menos, techo de una sola agua con riel, tubo telescópico y planos.
6. **Testing:** prueba de puesta en marcha (34 lecturas), modos de falla y pruebas formales pendientes.
7. **Iteraciones** y lecciones aprendidas.
8. **Conclusiones** y trabajo futuro.
9. **Referencias** y declaración de uso de IA.
