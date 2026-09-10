#!/usr/bin/env node
/**
 * Inicializa una memoria local para Claude Desktop y el servidor MCP Memory.
 * Ejecutar desde la raíz del proyecto: node setup-memory.js
 */
const fs = require('fs');
const path = require('path');

const projectRoot = __dirname;
const memoryDir = path.join(projectRoot, '.claude');
const manifestPath = path.join(memoryDir, 'memoria_ue5.json');
const memoryPath = path.join(memoryDir, 'memoria_ue5.jsonl');

const entities = [
  {
    name: 'Armas',
    entityType: 'sistema_ue5',
    observations: [
      'Contiene configuración, ADS, retroceso, munición, sonidos y efectos de las armas.',
      'Documentar el arma, la tecla, el Blueprint y los valores aprobados antes de cambiarla.'
    ]
  },
  {
    name: 'Animaciones',
    entityType: 'sistema_ue5',
    observations: [
      'Contiene animaciones de manos, armas, recarga, disparo e idle.',
      'Registrar el asset, esqueleto, frames y estado de importación.'
    ]
  },
  {
    name: 'Blueprints',
    entityType: 'sistema_ue5',
    observations: [
      'Contiene Blueprints de jugador, cámara, puertas, mapas y armas.',
      'Guardar rutas de assets y el propósito de cada cambio validado.'
    ]
  },
  {
    name: 'CodeFizz',
    entityType: 'herramienta_obligatoria',
    observations: [
      'Para editar Unreal Engine 5.8 se debe usar CodeFizz mediante MCP/cfa.',
      'Usar CodeFizz para inspeccionar, modificar, compilar, guardar y validar mapas, materiales, Blueprints, animaciones y assets.',
      'No reemplazar CodeFizz por ediciones manuales cuando la tarea afecte Unreal.'
    ]
  },
  {
    name: 'Protocolo_Memoria_Obligatorio',
    entityType: 'reglas_de_trabajo',
    observations: [
      'ANTES de modificar: buscar en memoria el sistema, asset, Blueprint, mapa o bug relacionado; no asumir que no existe historial.',
      'DURANTE la modificación: usar exclusivamente CodeFizz para cambios de Unreal Engine y trabajar en pasos pequeños que se puedan verificar.',
      'DESPUÉS de cada cambio importante: guardar en memoria la ruta exacta, qué se cambió, valores finales, cómo se validó y el estado (aprobado, pendiente o revertido).',
      'CUANDO HAYA ERROR: guardar causa comprobada, solución aplicada, asset afectado y prueba de que dejó de ocurrir; consultar este registro antes de intentar otro arreglo.',
      'No repetir cambios ya rechazados, revertidos o marcados como fallidos sin explicar por qué el nuevo intento es diferente.',
      'Antes de terminar una tarea: resumir cambios, riesgos o pendientes y escribir ese resumen en memoria.'
    ]
  },
  {
    name: 'Bugs_Solucionados',
    entityType: 'registro_tecnico',
    observations: [
      'Guardar causa, arreglo, asset afectado y cómo se verificó cada bug solucionado.'
    ]
  },
  {
    name: 'MyProject7',
    entityType: 'proyecto_unreal',
    observations: [
      `Ruta local: ${projectRoot}`,
      'Proyecto Unreal Engine 5.8 administrado con CodeFizz.'
    ]
  }
];

const relations = [
  { from: 'MyProject7', to: 'Armas', relationType: 'contiene' },
  { from: 'MyProject7', to: 'Animaciones', relationType: 'contiene' },
  { from: 'MyProject7', to: 'Blueprints', relationType: 'contiene' },
  { from: 'MyProject7', to: 'CodeFizz', relationType: 'se_edita_con' },
  { from: 'MyProject7', to: 'Protocolo_Memoria_Obligatorio', relationType: 'se_rige_por' },
  { from: 'MyProject7', to: 'Bugs_Solucionados', relationType: 'registra' }
];

fs.mkdirSync(memoryDir, { recursive: true });

// Archivo legible para humanos y herramientas del proyecto.
if (!fs.existsSync(manifestPath)) {
  fs.writeFileSync(manifestPath, JSON.stringify({
    schemaVersion: 1,
    projectRoot,
    entities,
    relations
  }, null, 2) + '\n', 'utf8');
  console.log(`Creado: ${manifestPath}`);
} else {
  console.log(`Ya existe, no se reemplazó: ${manifestPath}`);
}

// El servidor oficial MCP Memory usa JSONL, no un JSON único.
if (!fs.existsSync(memoryPath)) {
  const lines = [
    ...entities.map((entity) => JSON.stringify({ type: 'entity', ...entity })),
    ...relations.map((relation) => JSON.stringify({ type: 'relation', ...relation }))
  ];
  fs.writeFileSync(memoryPath, lines.join('\n') + '\n', 'utf8');
  console.log(`Creado: ${memoryPath}`);
} else {
  console.log(`Ya existe, no se reemplazó: ${memoryPath}`);
}

console.log('Memoria UE5 inicializada correctamente.');
