-- Tabela spell_script_names
DELETE FROM `spell_script_names` WHERE `ScriptName` IN (
    'spell_dk_blood_plague',
    'spell_dk_frost_fever',
    'spell_dk_death_and_decay',
    'spell_dk_blood_tap',
    'spell_dk_necrotic_strike',
    'spell_dk_outbreak',
    'spell_dk_horn_of_winter',
    'spell_dk_runic_empowerment',
    'spell_dk_dark_simulacrum'
);

INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(55078, 'spell_dk_blood_plague'),
(55095, 'spell_dk_frost_fever'),
(43265, 'spell_dk_death_and_decay'),
(45529, 'spell_dk_blood_tap'),
(73975, 'spell_dk_necrotic_strike'),
(77575, 'spell_dk_outbreak'),
(57330, 'spell_dk_horn_of_winter'),
(81229, 'spell_dk_runic_empowerment'),
(77606, 'spell_dk_dark_simulacrum');

-- Atualizações básicas das spells
UPDATE `spell_dbc` SET `AttributesEx3` = `AttributesEx3` | 0x00000100 WHERE `Id` = 45529; -- Blood Tap - PvP spell
UPDATE `spell_dbc` SET `Effect1` = 6, `EffectApplyAuraName1` = 4 WHERE `Id` = 73975; -- Necrotic Strike (debuff effect)

-- Configuração dos coeficientes de AP
UPDATE `spell_bonus_data` SET `ap_bonus` = 0.50, `comments` = 'Death Knight - Blood Plague' WHERE `entry` = 55078;
UPDATE `spell_bonus_data` SET `ap_bonus` = 0.60, `comments` = 'Death Knight - Frost Fever' WHERE `entry` = 55095;
UPDATE `spell_bonus_data` SET `ap_bonus` = 0.12, `comments` = 'Death Knight - Death and Decay (per tick)' WHERE `entry` = 43265;
UPDATE `spell_bonus_data` SET `ap_bonus` = 0.50, `comments` = 'Death Knight - Necrotic Strike' WHERE `entry` = 73975;

-- Configuração dos tempos de recarga
UPDATE `spell_dbc` SET `RecoveryTime` = 60000 WHERE `Id` = 45529; -- Blood Tap (1 min)
UPDATE `spell_dbc` SET `RecoveryTime` = 30000 WHERE `Id` = 77575; -- Outbreak (30 sec)
UPDATE `spell_dbc` SET `RecoveryTime` = 60000 WHERE `Id` = 77606; -- Dark Simulacrum (1 min)
UPDATE `spell_dbc` SET `RecoveryTime` = 20000 WHERE `Id` = 57330; -- Horn of Winter (20 sec)

-- Configuração dos recursos (Runes)
UPDATE `spell_dbc` SET `RuneCostID` = 1001 WHERE `Id` IN (45477, 45462); -- Icy Touch, Plague Strike (1 Unholy)
UPDATE `spell_dbc` SET `RuneCostID` = 1002 WHERE `Id` = 49998; -- Death Strike (1 Frost, 1 Unholy)
UPDATE `spell_dbc` SET `RuneCostID` = 1003 WHERE `Id` = 43265; -- Death and Decay (1 Blood, 1 Frost, 1 Unholy)
UPDATE `spell_dbc` SET `RuneCostID` = 1004 WHERE `Id` = 47541; -- Death Coil (40 Runic Power)
UPDATE `spell_dbc` SET `RuneCostID` = 1005 WHERE `Id` = 73975; -- Necrotic Strike (1 Frost, 1 Unholy)

-- Tabela de custo de runas personalizada
DELETE FROM `spell_rune_cost` WHERE `id` BETWEEN 1001 AND 1005;
INSERT INTO `spell_rune_cost` (`id`, `blood`, `frost`, `unholy`, `rune_power`, `runic_power`) VALUES
(1001, 0, 0, 1, 0, 0),  -- 1 Unholy
(1002, 0, 1, 1, 0, 0),  -- 1 Frost, 1 Unholy
(1003, 1, 1, 1, 0, 0),  -- 1 Blood, 1 Frost, 1 Unholy
(1004, 0, 0, 0, 0, 40), -- 40 Runic Power
(1005, 0, 1, 1, 0, 0);  -- Necrotic Strike (1 Frost, 1 Unholy)

-- Configuração dos procs
DELETE FROM `spell_proc` WHERE `SpellId` IN (81229, 50422);
INSERT INTO `spell_proc` (`SpellId`, `SchoolMask`, `SpellFamilyName`, `SpellFamilyMask0`, `SpellFamilyMask1`, `SpellFamilyMask2`, `ProcFlags`, `SpellTypeMask`, `SpellPhaseMask`, `HitMask`, `Chance`, `Cooldown`, `Charges`) VALUES
(81229, 0, 15, 0x02000000, 0x00000000, 0x00000000, 0x00011000, 1, 2, 0, 45, 0, 0), -- Runic Empowerment
(50422, 0, 15, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0, 0, 0, 15, 0, 0); -- Scent of Blood

-- Glyphs relacionados
DELETE FROM `spell_script_names` WHERE `ScriptName` = 'spell_dk_glyph_of_frost_fever';
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(58635, 'spell_dk_glyph_of_frost_fever');

-- Talentos importantes
DELETE FROM `spell_script_names` WHERE `ScriptName` IN ('spell_dk_improved_blood_plague', 'spell_dk_pandemic');
INSERT INTO `spell_script_names` (`spell_id`, `ScriptName`) VALUES
(62905, 'spell_dk_improved_blood_plague'),
(929, 'spell_dk_pandemic');

-- Aura para Dark Simulacrum
DELETE FROM `spell_dbc` WHERE `Id` = 77622;
INSERT INTO `spell_dbc` (`Id`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `CastingTimeIndex`, `DurationIndex`, `RangeIndex`, `SchoolMask`, `SpellLevel`, `BaseLevel`, `MaxLevel`, `SpellName`, `DmgClass`, `PreventionType`, `Effect1`, `EffectApplyAuraName1`, `EffectImplicitTargetA1`, `EffectMiscValue1`, `EffectMiscValueB1`, `SpellFamilyName`, `SpellFamilyFlags1`, `SpellIconID`, `SpellVisual1`, `SpellPriority`) VALUES
(77622, 0x00000100, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 1, 21, 1, 0x00000000, 0, 0, 0, 'Dark Simulacrum Copy', 0, 0, 6, 4, 1, 0, 0, 0, 0x00000000, 1, 0, 0);

-- Configuração do visual do Dark Simulacrum
DELETE FROM `spell_dbc` WHERE `Id` = 77616;
INSERT INTO `spell_dbc` (`Id`, `Attributes`, `AttributesEx`, `AttributesEx2`, `AttributesEx3`, `AttributesEx4`, `AttributesEx5`, `AttributesEx6`, `AttributesEx7`, `CastingTimeIndex`, `DurationIndex`, `RangeIndex`, `SchoolMask`, `SpellLevel`, `BaseLevel`, `MaxLevel`, `SpellName`, `DmgClass`, `PreventionType`, `Effect1`, `EffectApplyAuraName1`, `EffectImplicitTargetA1`, `EffectMiscValue1`, `EffectMiscValueB1`, `SpellFamilyName`, `SpellFamilyFlags1`, `SpellIconID`, `SpellVisual1`, `SpellPriority`) VALUES
(77616, 0x00000100, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 1, 21, 1, 0x00000000, 0, 0, 0, 'Dark Simulacrum Visual', 0, 0, 6, 4, 1, 0, 0, 0, 0x00000000, 1, 0, 0);