# Bugs solucionados — MyProject7

Registro técnico de errores ya investigados. Consultar este archivo antes de intentar arreglar un bug para evitar repetir soluciones fallidas.

## Formato para cada registro

### [Fecha] — Título corto del bug

- **Sistema / asset afectado:**
- **Síntoma:**
- **Causa comprobada:**
- **Solución aplicada:**
- **Archivos o Blueprints modificados:**
- **Validación realizada:**
- **Estado:** aprobado / pendiente / revertido

---

### [2026-09-09] — Cuelgue al disparar en pasillos largos (Backrooms_Level_0)

- **Sistema / asset afectado:** BP_Bullet_AK47, BP_Bullet, BP_Bullet1, BP_Bullet_Shotgun (componente `Sphere`)
- **Síntoma:** stall del render thread ~18s al disparar varias balas seguidas por un pasillo largo.
- **Causa comprobada:** `Sphere.bVisibleInRayTracing=true` en el mesh de la bala; varias balas vivas a la vez (LifeSpan 3s) disparaban recreación completa del Persistent SBT.
- **Solución aplicada:** `Sphere.bVisibleInRayTracing=false` en las 4 balas.
- **Validación realizada:** consola `r.RayTracing.Geometry.StaticMeshes 0` como test de confirmación; dejó de colgar.
- **Estado:** aprobado

### [2026-09-09] — "Accessed None" en UpdateFPWeaponHUD

- **Sistema / asset afectado:** BP_Player (UpdateFPWeaponHUD)
- **Síntoma:** error "Accessed None" en el log al iniciar (HUD null).
- **Causa comprobada:** SetText se llamaba antes de que el HUD estuviera válido.
- **Solución aplicada:** chequeo `IsValid(HUD)` antes de SetText.
- **Validación realizada:** log limpio tras el fix.
- **Estado:** aprobado

### [2026-09-09] — AK74M sin textura (material gris)

- **Sistema / asset afectado:** Materiales `ak74m` y `arms` (`/Game/Anims/FPS_AK74M/Raw/`)
- **Síntoma:** el mesh del AK se veía gris (material por defecto), sin textura.
- **Causa comprobada:** proyecto usa Substrate; los TextureSample de Metallic/Roughness tenían `SamplerType=Linear Color` en vez de `Color` → error de compilación de shader → cae al material gris.
- **Solución aplicada:** `set_material_expression_property SamplerType=SAMPLERTYPE_Color` en los 3 nodos afectados (2 en ak74m, 1 en arms).
- **Validación realizada:** `get_material_errors` en 0 tras el fix; confirmado visualmente en PIE.
- **Estado:** aprobado

### [2026-09-09] — AK74M sin disparo real (bala no salía)

- **Sistema / asset afectado:** DT_FPWeapons (fila "AK74M")
- **Síntoma:** al disparar no salía ninguna bala; `Bullet` quedaba `None` en tiempo de ejecución.
- **Causa comprobada:** `add_data_table_row`/`update_data_table_row` del CLI no escriben campos tipo class/object/FText en este struct (quedan en "None"/"" sin error visible) — BulletClass, FireSound y DisplayName quedaron vacíos.
- **Solución aplicada:** `dt.export_to_json_string()` para ver el formato exacto de referencia (`"/Script/Engine.BlueprintGeneratedClass'/path/Asset.Asset_C'"`), reconstruir el JSON completo con ambas filas y `dt.fill_from_json_string(json)` vía `cfa execute_python`.
- **Validación realizada:** `get_data_table_row` mostrando los valores correctos; bala real confirmada en PIE (`BP_Bullet_AK47_C` spawneado).
- **Estado:** aprobado

### [2026-09-09] — AK74M: disparo desviado a la izquierda

- **Sistema / asset afectado:** BP_Player (componentes AK74M_MuzHip / AK74M_MuzAim)
- **Síntoma:** apuntando al centro, la bala salía desviada hacia la izquierda.
- **Causa comprobada:** dos causas combinadas. (1) La rotación del disparo usa `PlayerCameraManager.GetCameraRotation()` directo, no la rotación del socket — solo importa la LOCATION de MuzHip/MuzAim. (2) El offset local original (X=55,Y=5,Z=8) se calculó mal por la rotación Yaw=-89 del componente AK74M_Arms; después, al recalibrar, el offset lateral (R=9 a la derecha) se notaba porque el proyectil vuela paralelo al eje de cámara sin converger a la mira.
- **Solución aplicada:** recalibrado con deproyección (`cam_loc + fwd*D - up*U`, sin componente lateral) convertido a espacio local con `inverse_transform_location`.
- **Validación realizada:** confirmado en juego por el usuario.
- **Estado:** aprobado

### [2026-09-09] — Ninguna arma recargaba de verdad (no solo AK74M)

- **Sistema / asset afectado:** BP_Player, composite "Reload (R)"
- **Síntoma:** la animación/lógica de recarga real nunca se ejecutaba para ningún arma.
- **Causa comprobada:** toda la lógica (`Set Reloading true → Delay → Set Reloading false`) colgaba del pin "else" de un `Branch` cuyo `Condition` nunca tenía nada conectado (default `true` literal) — la rama `then`, siempre tomada, no iba a ningún lado.
- **Solución aplicada:** reconectado `Sequence.then_0` directo al nodo de lógica real, saltando el Branch roto.
- **Validación realizada:** confirmado en juego; recarga se ejecuta y anima.
- **Estado:** aprobado

### [2026-09-09] — AK74M: recarga se disparaba 3 veces por cada R

- **Sistema / asset afectado:** BP_Player, composite "Reload (R)"
- **Síntoma:** al presionar R, la animación de recarga se reiniciaba 3 veces en vez de jugar una sola.
- **Causa comprobada:** existían 3-4 cadenas viejas de recarga (restos de HK416 y otras armas borradas) todas gateadas por guardas rotas (mismo patrón `Branch` con `Condition` sin conectar) → todas se ejecutaban a la vez, hacían toggle de `Reloading` varias veces.
- **Solución aplicada:** desconectadas las cadenas viejas; se dejó una sola cadena limpia (Set Reloading true → Delay 2.75s, igualado a la duración real de la animación → Set Reloading false).
- **Validación realizada:** confirmado en juego por el usuario ("el reload esta bien" tras el ajuste de duración).
- **Estado:** aprobado

### [2026-09-09] — AK74M sin retroceso visual al disparar

- **Sistema / asset afectado:** BP_Player, evento "Fire Event"; `MyProject7CameraManager::AddShotImpulse` (C++)
- **Síntoma:** el AK no producía ningún kick de cámara al disparar.
- **Causa comprobada:** `AddShotImpulse` solo se llamaba desde 3 ramas viejas hardcodeadas (Strength=2.5 cada una), ninguna alcanzable para el AK74M.
- **Solución aplicada:** nueva llamada a `AddShotImpulse(Strength=7.5)` enganchada en el Sequence del Fire Event (GetPlayerCameraManager → Cast a MyProject7CameraManager → AddShotImpulse).
- **Validación realizada:** pendiente de confirmación final del usuario tras el último ajuste.
- **Estado:** pendiente

### [2026-09-09] — Animación de disparo se repetía 2-3 veces por click

- **Sistema / asset afectado:** ABP_FPS_AK74M y ABP_FPS_FN502, Sequence Player del estado "Fire"
- **Síntoma:** tras un solo disparo, la animación (y con ella la sensación de retroceso animado) se veía repetirse 2-3 veces.
- **Causa comprobada:** `bLoopAnimation=true` (default) en el Sequence Player de "Fire"; el estado se mantiene activo por un umbral de tiempo fijo en el AnimBP (no ligado a la duración real del clip), y como la animación de disparo es más corta que ese umbral, volvía a arrancar sola.
- **Solución aplicada:** `Node.bLoopAnimation=false` en ambos Sequence Player de "Fire".
- **Validación realizada:** compilación en 0 errores en ambos AnimBP. Falta confirmación visual del usuario.
- **Estado:** pendiente

### [2026-09-09] — Retroceso "desde cero" (código) reemplazando el de animación

- **Sistema / asset afectado:** BP_Player (`AK74M_KickOffset`/`FN502_KickOffset`, `Fire Event`, `Event Tick`)
- **Contexto:** a pedido del usuario, en vez de depender de la animación/del `AddShotImpulse` (C++, sin control fino), se construyó un kick de posición del arma 100% Blueprint.
- **Solución aplicada:** en `Fire Event` (gateado por `FPWeaponIndex`), `Set KickOffset = (-20,0,7)` fuerte en cada disparo. En `Event Tick` (mismo Sequence donde vive el ADS de cada arma), el offset decae con `VInterpTo(..., InterpSpeed=15)` y se suma al resultado del `VInterpTo` de hip/ADS antes del `SetRelativeLocation` final — no pisa el ADS, se monta encima.
- **Validación realizada:** compilación en 0 errores. Falta confirmación de sensación en juego.
- **Estado:** pendiente

### [2026-09-09] — Retroceso del AK desincronizado de la cadencia de disparo

- **Sistema / asset afectado:** DT_FPWeapons (campo `RecoilRecovery`), `AddShotImpulse` (C++, `MyProject7CameraManager`)
- **Síntoma:** el kick de cámara al disparar el AK se sentía lento — tras 2 disparos seguidos, el retroceso del primero todavía no había terminado de recuperarse.
- **Causa comprobada (por inferencia, sin acceso al C++):** `AddShotImpulse` solo recibe `Strength` por Blueprint; su tooltip ("Impulso visual sincronizado con el disparo") sugiere que lee `CurRecoilRecovery` (variable de instancia poblada desde la DataTable) para la velocidad de recuperación. El AK tenía `RecoilRecovery=6`, casi igual al Benelli (`5`) pese a disparar 8.5x más rápido (0.1s vs 0.85s de `FireDelay`) — la recuperación quedaba desproporcionadamente lenta para la cadencia.
- **Solución aplicada:** subido `RecoilRecovery` a 40 (AK) y 32 (FN502, mismo problema), escalando desde el valor del Benelli por la razón inversa de `FireDelay`.
- **Validación realizada:** ninguna todavía — hace falta confirmación visual/sensación del usuario en juego (no hay forma de medir la curva de recuperación real sin el código C++).
- **Estado:** pendiente

### [2026-09-09] — ADS del AK74M no movía el arma

- **Sistema / asset afectado:** BP_Player, `Event Tick` / ADS del AK74M
- **Síntoma:** al apuntar (ADS) con el AK, el arma no se movía a la posición de mira (antes sí funcionaba, según el usuario).
- **Causa comprobada:** el nodo `Set Relative Location` del ADS del AK (con su `SelectVector`+`VInterpTo` ya armados) tenía el pin `execute` sin ninguna conexión entrante en todo el Blueprint — confirmado con BFS sobre el grafo exportado, no solo grep manual. El ADS del FN502 (construido en paralelo, aparentemente por otra sesión) sí estaba conectado directo a `Event Tick`, pero al agregar el del AK no se insertó un `Sequence` para bifurcar — quedó huérfano.
- **Solución aplicada:** insertado un `Sequence` entre `Event Tick` y el `Set Relative Location` del FN502 (movido a `then_0`), con `then_1` nuevo hacia el `Set Relative Location` del AK.
- **Validación realizada:** compilación en 0 errores. Falta confirmación visual del usuario en juego.
- **Estado:** pendiente (de confirmación visual)

### [2026-09-09] — Todas las armas disparaban desviadas de la mira (no solo el AK)

- **Sistema / asset afectado:** BP_Player, composite "Spawn Bullet" (anidado en "Fire (LMB)/Fire Event")
- **Síntoma:** apuntando al centro con cualquier arma, la bala impactaba corrida (confirmado con captura: 3 impactos a la izquierda del punto de mira).
- **Causa comprobada:** la rotación del disparo usaba `PlayerCameraManager.GetCameraRotation()`, que en este proyecto NO es igual a la rotación real de `FirstPersonCamera` (medido en vivo con el pawn quieto: pitch 66.6° vs 52.1°, yaw 6.95° vs 6.01°, hasta un roll falso de -0.33° que no debería existir) — el CameraManager custom del proyecto tiene comportamiento propio, distinto de hacia dónde apunta la cámara que se renderiza.
- **Solución aplicada:** redirigidas las 4 conexiones internas que usaban `GetCameraRotation()` (Select hipfire, Select ADS, ComposeRotators de recoil, GetForwardVector de trayectoria) a `FirstPersonCamera.K2_GetComponentRotation()` real, sin tocar cámara ni posición de ningún arma. Nota técnica: el composite que las contiene no tiene nombre propio — para `connect_blueprint_nodes`/`build_blueprint_graph` en un composite anidado sin título hay que usar `--function-name` con el título del composite CONTENEDOR más cercano que sí lo tenga (en este caso "Spawn Bullet"), no un path intermedio.
- **Validación realizada:** trace desde cámara vs. posición del MuzHip con el forward de cámara confirmado puro (1,0,0) — coinciden con diferencia de redondeo de float.
- **Estado:** aprobado

### [2026-09-09] — FN502: materiales auto-generados del importador FBX sin textura

- **Sistema / asset afectado:** `/Game/Anims/FPS_FN502/Raw/arms` y `/gun` (MaterialInstanceConstant)
- **Síntoma:** riesgo de mesh gris si se usaban los materiales que trae el propio import del FBX.
- **Causa comprobada:** el FBX no embebe las texturas reales en el material — el importador genera un MaterialInstanceConstant con todos los mapas apuntando a texturas placeholder del engine (DefaultTexture, DefaultNormal, BaseFlattenLinearColor).
- **Solución aplicada:** crear materiales propios (`M_FN502_Arms`, `M_FN502_Gun`) con `build_material_graph` conectando las texturas reales importadas, y asignarlos como `OverrideMaterials` del componente en vez de usar los auto-generados.
- **Validación realizada:** `get_material_instance_parameters` en los auto-generados confirmó el problema antes de aplicar el fix; materiales propios con `get_material_errors` en 0.
- **Estado:** aprobado

### [2026-09-09] — AnimBP: asignar la animación a un nodo "Sequence Player" falla con property-name simple

- **Sistema / asset afectado:** cualquier AnimBlueprint (confirmado en ABP_FPS_FN502)
- **Síntoma:** `set_blueprint_node_property` con `--property-name "Sequence"` y `set_pin_default` con `--pin-name "Sequence"` fallan ambos ("property not supported" / "pin not found").
- **Causa comprobada:** el nodo `AnimGraphNode_SequencePlayer` solo expone el pin "Pose" (output); la animación es una propiedad del struct interno `FAnimNode_SequencePlayer` (campo `Node`), no un pin ni una propiedad de primer nivel.
- **Solución aplicada:** `set_blueprint_node_property` con `--property-name "Node.Sequence"` (path anidado).
- **Validación realizada:** confirmado funcionando en los 4 estados del AnimBP del FN502.
- **Estado:** aprobado

### [2026-09-09] — Mapa CQB_Wood_Houses: spawn del jugador no fijo

- **Sistema / asset afectado:** Nivel CQB_Wood_Houses; BP_Player (SaveGame "Slot 1")
- **Síntoma:** el punto de aparición cambiaba según la posición de la cámara del editor al darle Play.
- **Causa comprobada:** el mapa no tenía `PlayerStart`. Además, BP_Player guarda un `Transform` en el SaveGame "Slot 1" y lo reaplica en BeginPlay — mover/crear el PlayerStart no alcanza si ya existe un save previo.
- **Solución aplicada:** creado `PlayerStart_Spawn`; borrado el save previo con `unreal.GameplayStatics.delete_game_in_slot('Slot 1', 0)`.
- **Validación realizada:** confirmado con `play_in_editor --new-window` (el modo viewport activo puede dar falsos positivos de cámara).
- **Estado:** aprobado

### [2026-09-09] — FN502: la animación de recarga se reproduce 2 veces al presionar R una sola vez

- **Sistema / asset afectado:** `/Game/Anims/FPS_FN502/ABP_FPS_FN502`, estado "Reload" del state machine FN502SM.
- **Síntoma:** al presionar R con la FN502 equipada, la animación de recarga se ve reproducirse 2 veces seguidas. El AK con el mismo sistema no lo manifiesta.
- **Causa comprobada:** el `CustomEvent Reload` de `BP_Player` mantiene `Reloading=true` durante 2.75s (Delay fijo, compartido por todas las armas). El Sequence Player del estado "Reload" de la FN502 usa la animación `FPS_AnimsArmature_FN_Reload`, que dura solo 1.98s, con `Node.bLoopAnimation=true` (valor por defecto, nunca seteado explícitamente). Al terminar el clip antes que la ventana de 2.75s, el Sequence Player lo reinicia solo (loop), dando la sensación de "recarga doble". El AK no lo mostraba porque su animación (`FpsAnims_AnimRig_AK_Reload`) dura 2.65s, casi idéntica a los 2.75s, así que el loop apenas alcanza a asomar. Se confirmó además que solo hay 2 llamadas `Reload` en todo `BP_Player` (la de la tecla R vía `IA_Reload.Triggered`, y un auto-trigger de "Reload If Ammo Out" dentro de Fire Event que no se activa con R) — descartando que el bug viniera de un doble disparo del evento.
- **Solución aplicada:** `set_blueprint_node_property --blueprint-path "/Game/Anims/FPS_FN502/ABP_FPS_FN502" --function-name "AnimGraph/FN502SM/Reload" --node-id "454A18CC486D35865DF86AA0EAA90037" --property-name "Node.bLoopAnimation" --property-value "false"`.
- **Validación realizada:** `validate_blueprint_compile` en 0 errores; `save_asset` en ABP_FPS_FN502 y BP_Player.
- **Estado:** aprobado (pendiente confirmación visual del usuario en PIE)

### [2026-09-10] — Feature: punto rojo de láser (no haz) para las 4 armas

- **Sistema / asset afectado:** `BP_Player` (nuevo componente `LaserDot`), material reusado `/Game/Super_Simple_FPS_Pack/Character/Weapons/M_LaserDot`.
- **Pedido:** un punto rojo en la superficie apuntada, no una línea, para todas las armas.
- **Solución aplicada:** un único componente compartido (no depende del arma equipada) que se posiciona cada Tick con un LineTraceSingle desde la cámara real (FirstPersonCamera), reusando la misma fuente de rotación ya corregida para la trayectoria de bala. Rango 20000 uu; se oculta si no hay impacto (ej. mirando un pasillo largo sin fin visible, como el mapa Backrooms).
- **Validación realizada:** `validate_blueprint_compile` 0 errores; `save_asset`. La verificación visual automática en PIE no fue concluyente porque el `AMyProject7CameraManager` custom (free-aim/bodycam) ignora `SetControlRotation` scripteado desde Python — no es indicativo de un bug en la lógica del láser, solo que no se puede forzar el ángulo de cámara por script en este proyecto para pruebas automatizadas.
- **Estado:** aprobado (pendiente confirmación visual del usuario jugando de verdad, con mouse)
\n### [2026-09-09] — Viñeta bodycam no se percibía como borde circular limpio\n\n- **Sistema / asset afectado:** `/Game/BODYCAM_VFX/PP_BodycamLens`.\n- **Síntoma:** el borde no se distinguía como una viñeta circular negra y se percibía como una capa sucia.\n- **Causa comprobada:** los parámetros dejaban el borde demasiado cerca de las esquinas; el material no contiene ninguna textura de suciedad.\n- **Solución aplicada:** `EdgeStart=0.56`, `EdgeEnd=0.82`, `VignetteIntensity=1.0`.\n- **Validación realizada:** grafo saludable, sin nodos huérfanos ni enlaces rotos; asset guardado.\n- **Estado:** pendiente de confirmación visual en juego.\n
### [2026-09-10] — Regresión: ADS del AK y la FN502 dejaron de moverse

- **Sistema / asset afectado:** `BP_Player`, `Event Tick`.
- **Síntoma:** el ADS de ambas armas (AK y FN502) dejó de reaccionar a la tecla de apuntar.
- **Causa comprobada:** `Event Tick.then` terminaba en un callejón sin salida — el primer nodo de la cadena (`SetScalarParameterValue`, sistema de bodycam motion) tenía su pin `then` sin ninguna conexión de salida. El `Sequence` que reparte ADS FN502/AK + decay de retroceso + el punto de láser (agregado en este mismo turno) tenía su pin `execute` sin ninguna conexión de entrada. Es decir: TODA la lógica por-frame del jugador quedó desconectada de Tick, sin ningún error de compilación (0/0). Probablemente producto de las recompilaciones en caliente durante PIE hechas al depurar el láser (ver [[myproject7-armas-fp-datatable]], nota sobre PIE inestable).
- **Solución aplicada:** `connect_blueprint_nodes` reconectando `SetScalarParameterValue.then → Sequence(ADS).execute`.
- **Validación realizada:** `validate_blueprint_compile` 0 errores; `save_asset`.
- **Estado:** aprobado (pendiente confirmación del usuario jugando)
\n### [2026-09-09] — Cámara bodycam con aspecto sucio en primera persona\n\n- **Sistema / asset afectado:** `BP_Player.FirstPersonCamera`, `PostProcessSettings`.\n- **Causa comprobada:** `FilmGrainIntensity=0.4` estaba activo en la cámara.\n- **Solución aplicada:** grano de película desactivado (`0.0`) y viñeta estándar desactivada, conservando únicamente `PP_BodycamLens` como viñeta circular.\n- **Validación realizada:** `BP_Player` compilado con 0 errores y 0 advertencias; guardado.\n- **Estado:** pendiente de confirmación visual en juego.\n\n### [2026-09-09] — Reversión solicitada: viñeta bodycam dinámica\n\n- **Pedido:** volver al borde negro fijo sin movimiento.\n- **Solución aplicada:** pulso temporal y modulación por movimiento anulados dentro de `PP_BodycamLens`; se mantiene la viñeta negra circular estática.\n- **Validación realizada:** grafo de material saludable y guardado.\n- **Estado:** aprobado.\n
### [2026-09-10] — Bodycam: movimiento por velocidad no funcionaba + viñeta invasiva + imagen "sucia"

- **Sistema / asset afectado:** `/Game/BODYCAM_VFX/PP_BodycamLens`, `BP_Player.FirstPersonCamera`, `Config/DefaultEngine.ini`, `MPC_BodycamMotion`.
- **Síntoma:** la viñeta no se movía con la velocidad del jugador aunque BP_Player calculaba y seteaba el valor; la viñeta cubría demasiada pantalla (peor en aspecto ultra-wide); la imagen se veía con "grano/suciedad".
- **Causa comprobada:** (a) el nodo `CollectionParameter` del material tenía `ParameterName="None"` y `ParameterId` en ceros — nunca leía `MPC_BodycamMotion.BodycamMotion`, así que el término de movimiento era siempre 0 aunque el `SetScalarParameterValue` de BP_Player funcionaba bien. (b) `EdgeStart/EdgeEnd` dejaban la viñeta empezando muy adentro. (c) el "grano" venía de `FilmGrainIntensity` de la cámara y de percibir el material OSB de las paredes del mapa CQB como suciedad de lente (el material `PP_BodycamLens` no tiene ninguna textura).
- **Solución aplicada:** `ParameterName="BodycamMotion"` + `ParameterId` = GUID real del parámetro del MPC (setear solo el nombre no basta, hay que setear el GUID). Movimiento reactivado con amplitud 0.035 y frecuencia 1.65→(1.65 + speed*2.5). Viñeta a `EdgeStart=0.70`, `EdgeEnd=0.92`. Desenfoque leve de 5 muestras mezclado solo por la máscara del borde (`Lerp(sharp, blur, EdgeMask*0.6)`). `FilmGrainIntensity=0`, `VignetteIntensity=0` (viñeta estándar off), `SceneFringeIntensity=1.5`, `ChromaticAberrationStartOffset=0.75`. `r.Tonemapper.Sharpen=0.5`.
- **Validación realizada:** `recompile_material` + `get_material_errors` = 0 errores; `validate_blueprint_compile` BP_Player = 0 errores / 0 advertencias; assets guardados. Prueba visual en PIE: centro limpio y nítido, sin grano, borde sutil.
- **Estado:** aprobado (pendiente confirmación del usuario en el editor ultra-wide)

### [2026-09-10] — Bodycam: los ajustes de post-proceso en la cámara no tenían efecto en juego

- **Sistema / asset afectado:** `AMyProject7CameraManager` (C++), `/Game/BODYCAM_VFX/BP_Project7BodycamCameraManager`.
- **Síntoma:** bajar `SceneFringeIntensity` / `VignetteIntensity` en `BP_Player.FirstPersonCamera.PostProcessSettings` no cambiaba nada en juego.
- **Causa comprobada:** el C++ del CameraManager, con `Bodycam.bPostProcessEnabled=true`, reescribe cada frame `POV.PostProcessSettings` desde el struct `Bodycam` del BP del manager (gana sobre el componente y sobre PostProcessVolumes). La viñeta nativa estaba forzada a `Bodycam.Vignette=1.0` (encima del material `PP_BodycamLens`), y la CA a `0.42`.
- **Solución aplicada:** editar los defaults del BP del manager: `Bodycam.Vignette=0.4`, `Bodycam.ChromaticAberration=0.28`, `Bodycam.ChromaticAberrationStartOffset=0.82`. NO existe `Bodycam.FilmGrain`, así que `FilmGrainIntensity=0` en el componente sí funciona. Guardar el BP del manager exige `compile_blueprint` antes de `save_asset` (si no: "Save failed / read-only" aunque el .uasset sea escribible).
- **Validación realizada:** BP del manager compila 0/0, guardado; prueba visual en PIE: viñeta sutil, centro limpio y nítido.
- **Estado:** aprobado (pendiente confirmación del usuario en su editor ultra-wide)
\n## Menú de inicio — 2026-09-09T22:45:24.928951-04:00\n- Estado: aprobado. El juego ya no entra directamente a un mapa; carga un menú UMG de selección de mapas.\n\n## Portada y selector — 2026-09-09T22:48:39.954409-04:00\n- Estado: aprobado. La entrada ahora muestra portada antes del selector de mapas.\n
### [2026-09-10] — Bodycam: la viñeta "no cambiaba nada" pasara lo que pasara

- **Sistema / asset afectado:** `AMyProject7CameraManager` (C++, BP `/Game/BODYCAM_VFX/BP_Project7BodycamCameraManager`), material `/Game/BODYCAM_VFX/PP_BodycamLens`.
- **Síntoma:** ningún ajuste de viñeta (ni en el componente de cámara, ni en los nodos del material, ni en `Bodycam.EdgeDarkening`) se veía en juego.
- **Causa comprobada (leída del `.cpp`, no adivinada):** `ApplyBodycam()` (líneas 456-598) reescribe cada frame `OutVT.POV.PostProcessSettings` y **también los parámetros escalares del material** vía un `UMaterialInstanceDynamic`: `SetScalarParameterValue("EdgeStart", Bodycam.LensEdgeStart)`, `"EdgeEnd"`, `"VignetteIntensity"` (= `max(Bodycam.Vignette, 0.05)`), `"LensDistortion"`. Editar los `DefaultValue` de los nodos del material NO tiene efecto. `Bodycam.Vignette` tiene un **piso de 0.05** en el C++ (`FMath::Max(Bodycam.Vignette, 0.05f)`), así que ponerlo en 0 lo dejaba en 0.05 (5%, invisible). `Bodycam.EdgeDarkening` **no se referencia en ningún lado** — es un knob muerto. Además, una sesión anterior había roto el cálculo del radio del material quitando el `*aspect` sobre X (el nodo del cuadrado pasó a `X*X` en vez de `(X*aspect)^2`), y el `.h` (líneas 55-56) documenta que `r` debe ir de 0 a ~1.02 en 16:9.
- **Solución aplicada:** (1) reconectar `node9 (X*aspect) -> node10 A/B` para restaurar el radial que el C++ asume. (2) **Desacoplar** el borde del material del C++: renombrar el `ScalarParameter` `VignetteIntensity` -> `VigStrength` (el `SetScalarParameterValue("VignetteIntensity", ...)` del C++ pasa a no-op) y fijar su `DefaultValue=0.9` (borde 90% negro, solo en el extremo). (3) Knobs finales del BP: `Bodycam.Vignette=0.2` (solo viñeta nativa suave), `LensEdgeStart=0.82`, `LensEdgeEnd=1.05`, `ChromaticAberration=0.15`, `ChromaticAberrationStartOffset=0.80`, `FilmGrain=0`, `LensDirtIntensity=0`, `EdgeDarkening=0`.
- **Validación realizada:** `recompile_material` + `get_material_errors` = 0; `validate_blueprint_compile` BP = 0/0; ambos `save_asset` OK. Prueba visual en PIE no realizada (PIE del proyecto se cierra solo de forma repetida).
- **Estado:** aprobado técnicamente, pendiente confirmación visual del usuario.
\n## Menú: cursor libre al entrar al mapa — 2026-09-09T22:58:03.098702-04:00\n- Causa: el selector dejaba UI Only y cursor visible al ejecutar OpenLevel.\n- Solución: cada selección reactiva Game Only, oculta cursor y limpia input antes de cargar el mapa.\n
### [2026-09-09] — Selector de mapas mostraba la escena/sky y botones verticales

- **Causa:** el selector anterior no usaba un fondo UMG opaco ni tarjetas visuales; la navegación no incluía retorno a portada.
- **Solución aplicada:** `WBP_MapSelect` reconstruido con Brush de fondo `Box` negro, tres tarjetas panorámicas con texturas importadas y botón `← VOLVER`.
- **Validación realizada:** Blueprint 0 errores, 0 advertencias y 0 cadenas exec colgantes.
- **Estado:** aprobado.
\n### [2026-09-10T00:25:46-04:00] — Botón cambiaba de pantalla sin mostrar selección\n- **Causa:** navegación inmediata; el estado activo desaparecía antes de renderizar.\n- **Solución:** `Anim_SelectPlay` y `Anim_SelectOptions` (0.25 s) animan overlay rojo y bracket; Delay de 0.25 s antes de navegar.\n- **Validación:** `WBP_MainMenuHome` compilado 0/0, sin exec colgantes y guardado.\n- **Estado:** aprobado técnicamente.\n
### [2026-09-10] — Bodycam: el pulso de la viñeta ligado a la velocidad nunca funcionó

- **Sistema / asset afectado:** `/Game/BODYCAM_VFX/PP_BodycamLens`.
- **Síntoma:** aunque `BP_Player` calculaba y publicaba correctamente la velocidad normalizada (`GetCharacterMovement→GetVelocity→VectorLengthXY→MapRangeClamped→SetScalarParameterValue` sobre `MPC_BodycamMotion.BodycamMotion`), la viñeta nunca reaccionaba al movimiento del jugador.
- **Causa comprobada:** el nodo `CollectionParameter` dentro del material tenía `ParameterName="None"` — nunca se había asociado realmente al parámetro `BodycamMotion` de la colección (a pesar de que el nombre visualmente parecía correcto en ediciones previas). Al recompilar con ese estado, el material tiraba el error explícito "CollectionParameter has invalid parameter BodycamMotion".
- **Solución aplicada:** `set_material_expression_property` fijando tanto `ParameterName="BodycamMotion"` como `ParameterId` (los 4 componentes A/B/C/D del GUID real del parámetro, obtenidos de `get_material_parameter_collection` y convertidos a int32 con signo).
- **Validación realizada:** `recompile_material` → `get_material_errors` en 0; confirmado visualmente en PIE.
- **Estado:** aprobado.

### [2026-09-10] — Bodycam: viñeta desproporcionadamente grande en pantallas anchas

- **Sistema / asset afectado:** `/Game/BODYCAM_VFX/PP_BodycamLens`.
- **Síntoma:** con `EdgeStart=0.42/EdgeEnd=0.60` la viñeta se veía apenas visible en una ventana PIE 1286×760, pero cubría gran parte de la pantalla en el viewport del editor con relación de aspecto ultra-wide.
- **Causa comprobada:** el radio efectivo de la viñeta (calculado con corrección de aspecto) varía con la resolución/relación de aspecto de la ventana; los mismos valores de borde producen resultados visualmente muy distintos según el aspecto.
- **Solución aplicada:** valores llevados hacia el borde exterior (`EdgeStart=0.70`, `EdgeEnd=0.92`) para un borde fino y menos invasivo en general, en vez de intentar afinar por resolución específica.
- **Validación realizada:** confirmado en PIE con capturas de esquina (borde visible pero fino) y de centro (limpio).
- **Estado:** aprobado, pendiente confirmación del usuario en su propia resolución de juego.

### [2026-09-10] — Editar PostProcessSettings de FirstPersonCamera no tiene efecto en juego

- **Sistema / asset afectado:** `BP_Player.FirstPersonCamera`, `AMyProject7CameraManager` (C++).
- **Síntoma:** cambiar `FilmGrainIntensity`, `VignetteIntensity`, `SceneFringeIntensity` etc. directamente en el componente de cámara compila sin errores pero no cambia nada visualmente en PIE.
- **Causa comprobada:** `AMyProject7CameraManager::ApplyBodycam()` reescribe `OutVT.POV.PostProcessSettings` TODOS los frames a partir de los defaults `Bodycam.*` de `/Game/BODYCAM_VFX/BP_Project7BodycamCameraManager`, pisando cualquier valor puesto en el componente de la cámara del jugador.
- **Solución aplicada:** editar `Bodycam.*` (ej. `Bodycam.FilmGrain`) en `BP_Project7BodycamCameraManager` en vez de `FirstPersonCamera.PostProcessSettings`. El BP del manager necesita `compile_blueprint` antes de `save_asset` (si no: "Save failed / read-only").
- **Validación realizada:** `compile_blueprint` 0 errores; `save_asset` OK.
- **Estado:** aprobado.

### [2026-09-10] — Feature: impacto en hierro (decal de cráter quemado + chispas Niagara ultra-realistas)

- **Sistema / asset afectado:** `/Game/BODYCAM_VFX/Decals/M_Impact_MetalHole` (material deferred_decal nuevo), `/Game/BODYCAM_VFX/FX/NS_Impact_Sparks_Metal` (sistema Niagara nuevo), `BP_Bullet_AK47` y `BP_Bullet1` (EventGraph), `/Game/BODYCAM_VFX/PhysMats/PM_Metal`, `M_Metal` de la estructura de la nave.
- **Pedido:** al disparar al hierro, que aparezca un impacto de cráter quemado como la foto de referencia, y chispas pequeñas redondas amarillo/naranja/rojo vivo, con fuerza en varias direcciones, densas al inicio, y algunas brasas que caen al piso y se apagan poco a poco.
- **Solución aplicada:**
  - **Decal:** material deferred_decal translúcido. `TextureSample /Game/Decals/T_BulletHole_Metal` × `Constant3(0.85,0.85,0.9)` → BaseColor; máscara circular irregular procedural (`Distance` a (0.5,0.5) × 2.3 = r; `Noise(uv*10)` con `AppendVector(uv, 0)` para pasar float3; `saturate((1.02 - r + (noise-0.5)*0.55) * 6)`) → Opacity; Metallic 0.25, Roughness 0.7. Compila 0 errores.
  - **Chispas:** sistema Niagara con 2 emisores. "Sparks" — `SpawnBurst_Instantaneous` Count 110, `InitializeParticle` Lifetime 0.45–1.6 s / Sprite Size 1.2–4.5 / Color `[9,2.8,0.4,1]` (rojo-naranja al rojo vivo), `AddVelocity` cono 68° vel 650, `Drag` 3.2, `Collision`, curva `ScaleColor` que enfría a rojo profundo y se apaga; renderer `M_Spark`, `Alignment=Unaligned` (sprites redondos, no estrías). "Embers" — `SpawnBurst` Count 24, Lifetime 3.5–7.5 s / Size 0.5–1.7 / Color `[4.5,1,0.12,1]`, `AddVelocity` cono 130° vel 175, `GravityForce` + `Drag` 5 + `Collision` (caen y se posan), curva que baja el brillo gradual hasta 0.
  - **Enganche:** en `BP_Bullet_AK47` y `BP_Bullet1`, en el overlap, `BreakHitResult` → `EqualEqual_ObjectObject(PhysMat, PM_Metal)` → `Branch`. Rama `then` (impacto en metal): `SpawnDecalAtLocation(M_Impact_MetalHole)` y luego `SpawnSystemAtLocation(NS_Impact_Sparks_Metal, ImpactPoint, MakeRotFromX(ImpactNormal))`. Rama `else`: `SpawnDecalAtLocation(M_Impact_MetalHole)` también.
  - **2026-09-10 (ajuste pedido "reemplaza la que tiene por esa nueva"):** el decal viejo de la rama `else` (nodo `8B8B54F2…` en AK / `7E85AB8C…` en B1) usaba `/Game/BODYCAM_VFX/M_BulletHole_Black`. Se cambió su pin `DecalMaterial` a `M_Impact_MetalHole` (y `DecalSize` a `12,8,8` para igualar). Ahora **todos** los impactos muestran el cráter quemado nuevo; el `Branch` solo sigue gateando las chispas (solo en metal).
  - `PM_Metal.surface_type = SURFACE_TYPE1` ("Metal" en DefaultEngine.ini); `M_Metal.phys_material = PM_Metal` para que el trace devuelva la superficie correcta.
- **Notas técnicas Niagara (aprendidas a la fuerza):** `set_niagara_module_input` usa nombres SIN espacios (`InitializeParticle`, `AddVelocity`, `SpawnBurst_Instantaneous`), no los display names. `reorder_niagara_module` ROMPE el parameter-map (1700+ errores "Parameter Maps must be created via an Input Node") → nunca reordenar, `add_niagara_module` agrega al final y eso sirve. Color como array JSON `[r,g,b,a]`, no `(R=..,G=..)`. `Directional Burst` trae el renderer en `VelocityAligned` (estría) → hay que ponerlo `Unaligned` para chispas redondas. Nodo `Noise` de material necesita float3 en Position → `AppendVector(uv_float2, Constant 0)`.
- **Validación realizada:** `get_material_errors` M_Impact_MetalHole = 0; `compile_niagara_system` NS_Impact_Sparks_Metal = válido, 0 errores; `validate_blueprint_compile` BP_Bullet_AK47 y BP_Bullet1 = 0/0; todos `save_asset` OK.
- **Estado:** aprobado técnicamente, pendiente confirmación visual del usuario disparando al hierro en juego.
\n### [2026-09-10T00:39:09-04:00] — Color de cámara demasiado intenso\n- **Solución:** Saturación de FirstPersonCamera ajustada a RGB 0.7 (70%). 0 se reserva para imagen gris.\n- **Validación:** BP_Player 0 errores/0 advertencias, asset guardado y valor releído.\n\n### [2026-09-10T00:40:18-04:00] — Saturación ajustada a imagen muy sobria\n- **Solución:** FirstPersonCamera a RGB 0.25 (25%), conservando una pequeña cantidad de color.\n- **Validación:** BP_Player 0 errores/0 advertencias, guardado y valor confirmado.\n\n### [2026-09-10T00:41:17-04:00] — Saturación eliminada por solicitud\n- **Solución:** FirstPersonCamera RGB 0.0; imagen blanco y negro.\n- **Validación:** BP_Player 0 errores/0 advertencias, guardado y valor confirmado.\n
### [2026-09-10] — La bala se iba "un poco a la izquierda" / bailaba respecto a la mira

- **Sistema / asset afectado:** `BP_Player`, composite colapsado **"Spawn Bullet"** (dentro de `Fire (LMB)/Fire Event`). Diagnóstico contra `AMyProject7CameraManager` (C++, solo lectura).
- **Síntoma:** apuntando con FN502/AK, la bala impactaba corrida hacia un lado (izquierda) y el error variaba (parecía "bailar" alrededor de la retícula), pese a que el arma y la mira se veían centradas.
- **Causa comprobada (medida en PIE, no inferida):** una sesión anterior había cambiado la dirección del disparo para que usara la rotación **cruda** de `FirstPersonCamera` (`K2_GetComponentRotation`). Pero la retícula = centro de pantalla = **POV FINAL del `PlayerCameraManager`**, y `AMyProject7CameraManager::ApplyBodycam()` le SUMA cada frame a `OutVT.POV.Rotation`: micro-sway (`MicroSwayYaw/Pitch`, activo incluso quieto porque `Stillness` es alto), `TurnLag` (hasta `MaxTurnLagDegrees` al girar), lean, judder, shot-impulse. La bala ignoraba todos esos offsets → caía al lado de la retícula. Medido con el jugador quieto a 34 m: la fórmula vieja erraba **1–2 cm lateral** y oscilaba con la fase del sway (se midió −2.03 cm y luego −1.04 cm en muestras seguidas).
- **Solución aplicada (solo lógica de por dónde sale/va la bala; nada de arma/cámara/ADS/posición/mapa):** en "Spawn Bullet" se insertó un `LineTraceByChannel` que sale de `GetCameraLocation` del **PlayerCameraManager** en la dirección `GetForwardVector(GetCameraRotation del PlayerCameraManager)` (el POV final, con sway) × 100000; un `SelectVector` toma el `ImpactPoint` si hubo `bBlockingHit`, si no el punto lejano; ese punto alimenta el `Target` de los 2 `FindLookAtRotation` (hip y ADS). El `Start` sigue siendo la boca de cañón (`MuzHip`/`MuzAim`). Así la bala **sale del cañón** y apunta **exactamente al punto que está bajo la retícula**, a cualquier distancia. Exec: el trace se insertó entre el nodo `Inputs` del composite y el `Branch` que testea si la bala es de escopeta (corre una vez por disparo, antes del bucle de perdigones). Nodos nuevos: `GetForwardVector` `D164D593`, `Multiply_VectorVector` `FA347F93`, `Add_VectorVector` `D787FA46`, `LineTraceSingle` `2B057FC9`, `BreakHitResult` `B55B0FCA`, `SelectVector` `2DEA1DA4`. Reutiliza los nodos que estaban muertos `GetCameraRotation` `7A9ECDDE` y `GetCameraLocation` `F6BFBB64` (ambos del manager). Queda huérfano (inerte) el viejo `vector+vector` `73B997BE`.
- **Validación realizada:** en PIE, misma sesión: la fórmula vieja seguía errando 1–2 cm lateral oscilando con el sway; la fórmula nueva (idéntica a lo que arma el grafo) da **0.00 cm** de error en todas las muestras, inmune a la fase del sway. `BP_Player` compila 0 errores / 0 advertencias, sin nuevas `dangling_exec_chains` (siguen 39), cadena `Event Tick → SetScalarParameterValue → Sequence(ADS)` intacta tras compilar. `save_asset` OK.
- **Estado:** aprobado técnicamente, pendiente confirmación del usuario disparando con mouse real.

### [2026-09-10] — Texto "DBG_RELOAD_IF_OUT_ENTRA" en pantalla + bucle infinito de recarga al quedarse sin balas

- **Sistema / asset afectado:** `BP_Player` — composite **"Reload If Ammo Out"** (dentro de `Fire Event`), función **`Reload`** (grafo "Reload (R)"), variables `Ammo`/`Ammo2`/`Magazine`/`Reloading`.
- **Síntoma:** (1) al disparar/recargar aparecía en pantalla `DBG_RELOAD_IF_OUT_ENTRA`. (2) tras vaciar el cargador, cada vez que se pulsaba disparar el arma **solo recargaba** (una y otra vez), nunca volvía a disparar.
- **Causa comprobada:**
  1. Un nodo `PrintString("DBG_RELOAD_IF_OUT_ENTRA")` de depuración quedó en el composite "Reload If Ammo Out": `Inputs → PrintString → Reload`.
  2. La función `self.Reload()` (la llama tanto `IA_Reload.Triggered` como "Reload If Ammo Out") **nunca reponía la munición**. El único sitio que hacía `Set Ammo = Magazine` estaba en el sub-grafo `Delay for Animation`, y ese sub-grafo era **inalcanzable** (su pin `execute` de entrada venía de un `Branch` sin ninguna conexión de exec — resto muerto del sistema del HK416, junto con `AttachMag`/`DetachMag`/`Set HK_Reloading`/etc). El spine realmente ejecutado de `Reload` era solo: `Set States (Reloading=true, MaxWalkSpeed=250) → Delay 2.75 s → Set Reloading=false`. Sin reponer `Ammo`/`Ammo2`. Así que tras vaciar, `Ammo`/`Ammo2` quedaban en 0; `Adjust Ammo Count` (en `Fire Event`) devuelve "sin balas" cuando `Ammo<=0 OR Ammo2<=0`, disparando `Reload If Ammo Out` → `self.Reload()` en **cada** pulsación de disparo. Bucle.
- **Solución aplicada (solo `cfa`):**
  - **(A)** Borrado el `PrintString` de "Reload If Ammo Out" (y el `Get AnimBP` huérfano). Queda `Inputs.execute → Reload.execute`.
  - **(B)** En el spine real de la función `Reload`, entre `Delay 2.75 s` y `Set Reloading=false`, se insertó: `Set Ammo = Magazine` → `Set Ammo2 = Magazine` → refresco del `TextBlock` `UI_HUD.Ammo` (`SetText(ToText(Ammo))`). Nodos nuevos en el grafo "Reload (R)": `var_get Magazine` `8AA8CE59`, `var_set Ammo` `E0E1C24B`, `var_set Ammo2` `057AE671`, `var_get HUD` `A86BC36A`, `var_get Ammo` (target `UI_HUD_C`) `67C5F1C5`, `var_get Ammo` (float) `90CC8F7F`, `Conv_DoubleToText` `5810B495`, `TextBlock.SetText` `2AD66EB8`.
  - El bucle se rompe solo: `Set States` pone `Reloading=true` en el mismo frame, y con `Reloading=true` el `Branch(Get Reloading).then` de `Fire Event` no tiene salida → disparar no hace nada durante los 2.75 s; al terminar, `Ammo=Magazine` y `Reloading=false` → se vuelve a disparar.
- **Validación realizada:** en PIE — `pawn.call_method("Reload")` → `Reloading=True` de inmediato; ~2.75 s después `Ammo=Ammo2=Magazine (30)` y `Reloading=False`. `BP_Player` compila 0 errores / 0 advertencias, sin nuevas `dangling_exec_chains` (39), guardado.
- **Nota:** `Ammo2` (reserva) ahora se repone a `Magazine` en cada recarga → reserva efectivamente ilimitada (aceptable en un mapa de tiro; una economía de reserva limitada sería otra tarea).
- **Estado:** aprobado técnicamente; pendiente confirmación del usuario jugando (vaciar cargador, recargar con R, volver a disparar).

### [2026-09-10] — Escopeta Benelli (tecla 3) sin textura (material gris)

- **Sistema / asset afectado:** mesh `/Game/Anims/FPS_Benelli/Raw/HandWithGloves_AnimateRDY` (brazos+escopeta del arma de la tecla 3), materiales `/Game/Anims/FPS_Benelli/Materials/M_Benelli{Arms,Body,Glove,Match,Shell}`.
- **Síntoma:** con la escopeta equipada (tecla 3), brazos y arma se veían grises/sin textura.
- **Causa comprobada:** los 5 slots del mesh apuntaban a los **materiales auto-generados por el importador FBX** (`sleeve_st6_generalist`, `glove_hardknuckle`, `12gauge`, `TTI_Benelli_M4`, `matchsaverz` — placeholders). El pack **ya incluía 5 materiales propios hechos a mano** (`M_Benelli*`) sin asignar. Además `M_BenelliArms` y `M_BenelliGlove` daban error de compilación ("Missing input texture"): un `TextureSample` vacío con `SamplerType=MASKS` para un mapa packed (roughness/metallic/AO) que el pack no trae.
- **Solución aplicada (solo `cfa`):**
  1. En `M_BenelliArms` y `M_BenelliGlove`: borrado el `TextureSample` vacío; conectado el `Constant` existente (0.75 / 0.80) a `Roughness`; recompilado → 0 errores.
  2. Asignados los 5 `M_Benelli*` a los slots del mesh por nombre: `sleeve_st6_generalist → M_BenelliArms`, `glove_hardknuckle → M_BenelliGlove`, `12gauge → M_BenelliShell`, `TTI_Benelli_M4 → M_BenelliBody`, `matchsaverz → M_BenelliMatch`.
- **Validación realizada:** `get_material_errors` = 0 en los 5; en PIE `EquipFP(1)` → `FPWeaponIndex=1`, `Benelli_Arms` visible **con textura** (screenshot). 6 assets guardados (5 materiales + mesh).
- **Mapeo de teclas confirmado:** `IA_Weapon1 → EquipFP(2) → AK74M`; `IA_Weapon2 → EquipFP(3) → FN502`; `IA_Weapon3 → EquipFP(1) → Benelli` (fila `Benelli` de `DT_FPWeapons`).
- **Estado:** aprobado (pendiente confirmación visual del usuario en juego).

### [2026-09-10] — SCAR pack (scar (2).zip) no se puede importar por CLI/Python

- **Sistema / asset afectado:** import de `scar (2).zip` → destino previsto `/Game/Anims/FPS_SCAR/`, arma pedida en la **tecla 4**.
- **Síntoma:** el import automático (`AssetImportTask` + `FbxImportUI` con skeletal mesh + animaciones) "funciona" pero produce assets basura, no un arma usable.
- **Causa comprobada:** `source/SCAR.fbx` (4.8 MB) es un **FBX crudo de Blender** con el rig completo. El importador crea:
  - `Circle_001_low` (el mesh de brazos+arma, con los 3 slots correctos `Material` / `glove_hardknuckle` / `sleeve_st6_generalist`) pero **skineado a 3 huesos de cámara** (`camera1`, `Camera`, `camera_end`) y a escala gigante (bounds extent ~`(2863,1573,1222)`).
  - 7 `AnimSequence` (`SCARArmature_Hide/Idle/Reload/Run/Shoot/Take/Walk`, con duraciones reales — Reload 4.58 s) **atadas a `Circle_001_low_Skeleton`** (los 3 huesos de cámara) → inservibles.
  - 8 `SkeletalMesh`/`Skeleton` basura de los **objetos widget del rig de Blender** (`shape_circle`, `shape_sphere`, `shape_plane`, `shape_finger`, `shape_pose`, …) + `crosshair1`.
  - Mismo tipo de fallo documentado en [[myproject7-akm-zip-no-importa]] (Saiga = solo un Skeleton; aquí = mesh mal skineado + widgets).
- **Solución aplicada:** borrado `/Game/Anims/FPS_SCAR` completo. FBX + PNGs dejados a mano en `D:\Unreal Projects\MyProject7\Saved\_scar_import\`.
- **Camino correcto (pendiente, requiere un paso manual del usuario):**
  1. En el editor: arrastrar `Saved\_scar_import\SCAR.fbx` al Content Browser (carpeta `/Game/Anims/FPS_SCAR/Raw`). En el diálogo: **Skeletal Mesh**, **Import All**, skeleton = None (crear nuevo), **Import Animations** ON. Revisar en el diálogo que el árbol de huesos tenga brazos/dedos/arma (no solo cámara). Si sigue saliendo cámara → re-exportar de Blender solo con el mesh + su armature de deform (borrar cámara y widgets del rig, aplicar transforms, escala 1, "Only Deform Bones", "Add Leaf Bones" OFF).
  2. Ya con el mesh bueno, Claude hace el resto por `cfa`: reusar `M_BenelliArms` / `M_BenelliGlove` para los brazos (el pack trae **las mismas texturas** `glove_hardknuckle` / `sleeve_st6_generalist` que la Benelli), material propio para el cuerpo (`SCARL_LPForNormalFix_*`), fila nueva en `DT_FPWeapons`, `IA_Weapon4 → EquipFP(4)`, AnimBP con state machine (Idle/Walk/Run/Fire/Reload), componentes `SCAR_MuzHip`/`SCAR_MuzAim`, luz de fogonazo, ADS.
- **Estado:** bloqueado — pendiente del import GUI del usuario.

### [2026-09-10] — No se podía bajar la saturación de la cámara

- **Sistema / asset afectado:** `AMyProject7CameraManager::ApplyBodycam()` (C++, líneas 496-512), material `/Game/BODYCAM_VFX/PP_BodycamLens`.
- **Síntoma:** el juego se veía sobresaturado y ni `BP_Project7BodycamCameraManager` ni `FirstPersonCamera.PostProcessSettings.ColorSaturation` lo bajaban.
- **Causa comprobada (leída del `.cpp`):** cada frame el C++ hace `PP.bOverride_ColorSaturation = true; PP.ColorSaturation = FVector4(FMath::Clamp(FMath::Max(Bodycam.Saturation, 1.3f), 0, 2), ...)`. El `FMath::Max(..., 1.3f)` es un **suelo duro de 1.3** (+30% de color). Poner `Bodycam.Saturation` a 1.0 no baja de 1.3, y el valor del componente de cámara lo pisa el C++ igual. Editar el C++ está bloqueado (ver [[myproject7-cpp-build-blocked]]).
- **Solución aplicada (solo `cfa`, sin tocar C++):** `PP_BodycamLens` ya es un material **post-process** en los blendables de la cámara y corre **después del tonemapper** (donde se aplica `ColorSaturation`). Se insertó un nodo `Desaturation` entre el `Multiply` final (node 27 = imagen compuesta) y `EmissiveColor`, con `Fraction` = nuevo `ScalarParameter` **`DesatAmount`** (DefaultValue **0.30**). El C++ solo setea `LensDistortion`/`EdgeStart`/`EdgeEnd`/`VignetteIntensity` en el MID — **no toca `DesatAmount`**, así que su default manda. Neto: −30% de saturación sobre el +30% forzado → color casi neutro.
- **Ajuste:** subir `DesatAmount` (hacia 0.5) = menos color; bajar (hacia 0) = más color. Es el `DefaultValue` del nodo `DesatAmount` en `PP_BodycamLens`.
- **Validación realizada:** `recompile_material` = 0 errores; confirmado en PIE (escena visiblemente menos saturada).
- **Estado:** aprobado (pendiente que el usuario diga si quiere más o menos).
\n### [2026-09-10T12:12:49-04:00] — Diagnóstico pendiente: AK dispara visualmente a la izquierda\n- **Causa comprobada:** `ProjectileOffset` es cero y la trayectoria se calcula correctamente; el origen visible es `AK74M_MuzHip/MuzAim` en (0,50,13), desalineado frente al mesh.\n- **Pendiente:** recalibrar ambos puntos de boca de cañón tras confirmación del usuario.\n\n### [2026-09-10T12:15:59-04:00] — AK: origen visual de bala a la izquierda\n- **Solución:** Se movieron solo `AK74M_MuzHip` y `AK74M_MuzAim` de `(0,50,13)` a `(-5,50,13)`, 5 cm hacia la derecha visual.\n- **Validación:** BP_Player 0 errores/0 advertencias, ambos puntos releídos y guardado.\n- **Estado:** aprobado técnicamente; confirmar visualmente disparando con tecla 1.\n