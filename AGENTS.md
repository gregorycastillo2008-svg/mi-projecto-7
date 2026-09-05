# Reglas permanentes — MyProject7 (Unreal Engine 5.8)

## Alcance

- Trabaja únicamente dentro de `D:\Unreal Projects\MyProject7` salvo que el usuario indique otro destino.
- Trata este proyecto como producción de Unreal Engine 5.8.
- Antes de modificar algo, identifica el sistema que realmente está activo: mapa, `GameMode`, `Pawn`, `PlayerController`, Blueprint, AnimBP, arma y componente implicados.
- No supongas que un archivo C++ o Blueprint es el que se ejecuta: compruébalo mediante configuración, referencias, log o prueba controlada.

## Seguridad de los cambios

- Nunca cierres Unreal Editor.
- No modificar armas, personaje, animaciones, cámara, mapa, iluminación o materiales sin autorización explícita del usuario para ese cambio.
- No eliminar, reemplazar ni sobrescribir assets existentes sin identificar el destino exacto y explicarlo antes.
- Mantén los cambios pequeños, reversibles y relacionados con una única mejora.
- No alteres sistemas que ya funcionan al corregir otro sistema.

## Verificación visual obligatoria

Para cualquier ajuste visual o de gameplay visible:

1. Toma una captura antes del cambio cuando sea posible.
2. Aplica un único cambio controlado.
3. Compila solo si el cambio lo requiere.
4. Prueba el resultado en el juego o en una vista equivalente, sin cerrar el Editor.
5. Toma una captura después.
6. Indica qué cambió, qué se comprobó y qué queda pendiente.

No declares que un ajuste visual está terminado sin evidencia visual o de ejecución.

## Armas, brazos y animaciones

Al revisar o modificar un arma, comprueba siempre:

- entrada de disparo y de recarga;
- selección del arma correcta;
- animación específica por tipo de arma;
- manos, sockets, huesos e IK;
- Hip Fire, ADS, caminar, correr, disparo y recarga;
- proyectil y sonido desde el cañón;
- dirección del disparo alineada con cámara y mira;
- munición, HUD y estados de recarga;
- impacto en pared, piso, metal, madera y objetos.

Reglas por tipo:

- La escopeta usa animación y lógica de escopeta; su recarga debe poder insertar cartuchos de forma coherente.
- M4 y SCAR usan su propia recarga de rifle; no reutilizar una animación de escopeta.
- Pistola usa su propia recarga y manipulación de corredera.
- Ningún arma, mano o cargador debe flotar, atravesar geometría, teletransportarse o quedar desalineado.

## Impactos y realismo

- Para cada superficie, verificar decal, partículas y sonido apropiados: concreto/pared/piso, metal, madera y props.
- Verificar que el sistema de impacto está conectado al arma y proyectil que usa el `GameMode` activo.
- Las marcas de bala deben quedar orientadas por la normal del impacto, con tamaño y duración controlados.
- Priorizar realismo, rendimiento y estabilidad; no usar efectos exagerados ni cambios que reduzcan notablemente los FPS.

## Compilación y registro

- Antes de compilar, comprobar que el editor pueda aceptar la compilación en caliente; preferir Live Coding cuando corresponda.
- Tras compilar, revisar resultado y logs relevantes antes de afirmar que funciona.
- Antes de cambios importantes, revisar el estado de Git. Cuando el usuario lo autorice, crear commits claros y hacer push al remoto configurado.
- Mantener una explicación breve y concreta del estado actual: hecho, comprobado, pendiente y bloqueos reales.
