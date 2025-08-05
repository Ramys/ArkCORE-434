-- Spawn do Deathwing (inicialmente invisível)
REPLACE INTO `creature` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseId`, `position_x`, `position_y`, `position_z`, `orientation`, `spawntimesecs`, `wander_distance`, `currentwaypoint`, `curhealth`, `curmana`, `MovementType`, `npcflag`, `unit_flags`, `dynamicflags`, `ScriptName`, `VerifiedBuild`) VALUES 
(500000, 56173, 967, 0, 0, '3,4', 0, -1755.31, -2367.19, 340.94, 0.0, 7200, 0, 0, 1, 0, 0, 0, 33554432, 0, '', 0);

-- Spawn das Plataformas (4 plataformas)
REPLACE INTO `gameobject` (`guid`, `id`, `map`, `zoneId`, `areaId`, `spawnDifficulties`, `phaseId`, `position_x`, `position_y`, `position_z`, `orientation`, `rotation0`, `rotation1`, `rotation2`, `rotation3`, `spawntimesecs`, `animprogress`, `state`, `ScriptName`, `VerifiedBuild`) VALUES 
(500001, 209547, 967, 0, 0, '3,4', 0, -1755.31, -2367.19, 340.94, 0.0, 0, 0, 0, 1, 7200, 255, 1, '', 0),
(500002, 209547, 967, 0, 0, '3,4', 0, -1851.48, -2389.55, 340.97, 0.0, 0, 0, 0, 1, 7200, 255, 1, '', 0),
(500003, 209547, 967, 0, 0, '3,4', 0, -1872.83, -2425.30, 45.31, 0.0, 0, 0, 0, 1, 7200, 255, 1, '', 0),
(500004, 209547, 967, 0, 0, '3,4', 0, -1786.69, -2393.23, 45.31, 0.0, 0, 0, 0, 1, 7200, 255, 1, '', 0);