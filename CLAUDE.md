# Reglas para Claude Code en este proyecto (MyProject7 — Unreal Engine 5.8)

## 1. Eficiencia de tokens (anti-basura)

- Nada de planes largos, introducciones ni explicaciones teóricas antes de actuar. Ir directo al
  grano: diagnosticar y ejecutar.
- **Excepción explícita:** antes de una acción grande o destructiva (borrar assets, reestructurar
  un sistema entero, tocar algo sin deshacer fácil) sí se avisa primero, breve. Esto no es un plan
  largo — es una frase de "voy a borrar X, ¿seguimos?", no una introducción.
- Al editar C++: tocar únicamente las líneas afectadas. Prohibido reescribir bloques o archivos
  enteros si no es estrictamente necesario.
- Al editar Blueprints con CodeFizz (`cfa`): el equivalente es no reconstruir un grafo entero
  cuando basta con añadir/reconectar los nodos concretos que hacen falta.
- Si un log de compilación o de consola es masivo, extraer solo la línea exacta del error
  (`grep`/`python -c` sobre la salida) y descartar el resto.

## 2. Antes de actuar: revisar decisiones previas

- Antes de procesar cualquier tarea, mirar `decisions.md` (raíz del proyecto) por si el problema
  ya se resolvió antes.
- Si ya hay una entrada que cubre el caso, ejecutar la acción/comando de `cfa` directamente — no
  volver a pensar la solución desde cero.
- `decisions.md` es un log local de este proyecto, distinto de la memoria persistente de Claude
  Code (que vive fuera del repo). Son complementarios, no lo mismo: `decisions.md` es lo que
  cualquiera que abra este repo puede leer; la memoria persistente es lo que Claude recuerda entre
  sesiones aunque cambie de máquina.

## 3. Flujo con CodeFizz

- Todo por el CLI `cfa` (`C:\Users\grego\AppData\Local\CodeFizz\bin\cfa.exe`), nunca MCP.
- Comandos lo más compactos posible: agrupar cambios relacionados en una sola llamada
  (`build_blueprint_graph` con varios nodos/conexiones a la vez) en vez de uno por uno cuando se
  pueda.
- Tras cada corrección que funcione:
  1. Añadir una línea a `decisions.md` con el resumen (una línea: qué problema, qué solución).
  2. Preguntar si se guarda también en la memoria persistente de Claude Code (para que sobreviva
     aunque se borre este repo). No guardarlo ahí sin preguntar.

## Nota de arquitectura de armas de este proyecto

Cada arma en primera persona (HK416, AK74U, FN502, Benelli, P9, …) es un Skeletal Mesh
independiente con brazos+arma ya soldados en un solo mesh (paquetes `arms@X.fbx`), montado como
componente del personaje y posicionado con `RelativeLocation`/`RelativeRotation` frente a la
cámara. **No hay un socket de mano compartido ni dedos que cerrar por separado** — ese método
(`Weapon_Socket_R`, ajustar huesos de dedos) es para personajes con un mesh de manos único que
agarran armas sueltas, y no aplica aquí a menos que el proyecto migre a esa arquitectura.

## graphify (usar SIEMPRE para preguntas de código)

Este proyecto tiene un grafo de conocimiento en `graphify-out/`.

Reglas obligatorias:
- Ante **cualquier** pregunta sobre el código, ejecutar **primero** `graphify query "<pregunta>"`
  antes de `grep`, `find` o leer archivos a ciegas. Para relaciones entre símbolos usar
  `graphify path "<A>" "<B>"`; para un concepto concreto, `graphify explain "<concepto>"`.
- `graphify-out/GRAPH_REPORT.md` solo para revisión amplia de arquitectura, cuando
  query/path/explain no den suficiente contexto.
- Después de modificar código C++, correr `graphify update .` para mantener el grafo al día
  (solo AST, sin coste de API).
- **Límite conocido:** el grafo solo cubre C++ (`Source/`). El gameplay real (armas, recarga,
  cámara) vive en Blueprints y DataTables (`BP_Player`, `DT_FPWeapons`, `ABP_M4`), que graphify
  NO indexa. Para esas preguntas, `graphify query` sirve para descartar el código C++ de
  plantilla (`Variant_Shooter/`, dormido); la respuesta buena se saca con `cfa`.
