# Protocolo obligatorio — MyProject7

Usa `memoria-ue5` y `codefizz` en todas las tareas de este proyecto. Si la herramienta MCP `memoria-ue5` no está cargada (por ejemplo, en Claude Code), consulta y actualiza directamente `.claude/memoria_ue5.json`, `.claude/memoria_ue5.jsonl`, `decisions.md` y `.claude/Bugs_Solucionados.md`.

1. **Antes de editar:** consulta la memoria sobre el sistema, asset, Blueprint, mapa o bug relacionado. No asumas que no hay cambios previos.
2. **Al editar Unreal:** usa exclusivamente `codefizz`. Inspecciona primero, modifica en pasos pequeños, compila/guarda y valida el resultado.
3. **Después de cada cambio importante:** escribe en `memoria-ue5` la ruta exacta, el cambio, valores finales, validación y estado: `aprobado`, `pendiente` o `revertido`.
4. **Cuando ocurra un error:** busca primero `.claude/Bugs_Solucionados.md`. Después registra causa confirmada, solución, assets afectados y prueba de corrección. No repitas un arreglo fallido sin una razón técnica nueva.
5. **Al finalizar:** guarda un resumen de lo hecho, los pendientes y las decisiones aprobadas.

Prioridad: preservar decisiones aprobadas, evitar repetir trabajo y mantener un historial técnico útil para las siguientes sesiones.
