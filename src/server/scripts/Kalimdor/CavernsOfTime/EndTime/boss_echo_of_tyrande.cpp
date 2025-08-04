/*
 * Copyright (C) 2011-2016 ArkCORE <http://www.arkania.net/>
 *
 * Echo of Tyrande Boss Script
 * 
 * Once the leader of the night elves and High Priestess of Elune, this 
 * time-twisted fragment of Tyrande Whisperwind now wanders the wastes of 
 * the desolate future of Azeroth. Enveloped in a perpetual midnight, she 
 * has all but lost sight of the comforting light of Elune.
 *
 * THIS particular file is NOT free software; third-party users 
 * should NOT have access to it, redistribute it or modify it. :)
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "end_time.h"
#include "Vehicle.h"
#include "Unit.h"
#include "ScriptedEscortAI.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CombatAI.h"
#include "PassiveAI.h"
#include "ObjectMgr.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "CreatureTextMgr.h"
#include "Vehicle.h"
#include "VehicleDefines.h"
#include "Spell.h"
#include "Player.h"
#include "Map.h"
#include "InstanceScript.h"

enum Yells
{
    SAY_INTRO = 1,
    SAY_AGGRO,
    SAY_DEATH,
    SAY_KILL_1,
    SAY_KILL_2,
    SAY_KILL_3,
    SAY_STARFALL,
    SAY_MOONFIRE,
    SAY_ELUNE_WRATH,
    SAY_PERPETUAL_MIDNIGHT,
    SAY_LIGHT_OF_ELUNE,
    SAY_SHADOW_STRIKE,
};

enum Spells
{
    SPELL_TYRANDE_VISUAL = 101660, // Visual effects on boss
    
    // Nature spells
    SPELL_MOONFIRE = 101661, // Moonfire dot
    SPELL_STARFALL = 101662, // Starfall AoE
    SPELL_WRATH = 101663, // Wrath spell
    SPELL_HURRICANE = 101664, // Hurricane spell
    
    // Elune spells
    SPELL_LIGHT_OF_ELUNE = 101665, // Light of Elune heal
    SPELL_ELUNE_WRATH = 101666, // Elune's wrath
    SPELL_ELUNE_BLESSING = 101667, // Elune's blessing
    SPELL_ELUNE_PROTECTION = 101668, // Elune's protection
    
    // Shadow spells (due to perpetual midnight)
    SPELL_SHADOW_STRIKE = 101669, // Shadow strike
    SPELL_SHADOW_BOLT = 101670, // Shadow bolt
    SPELL_SHADOW_NOVA = 101671, // Shadow nova
    SPELL_SHADOW_EMBRACE = 101672, // Shadow embrace
    
    // Perpetual Midnight
    SPELL_PERPETUAL_MIDNIGHT = 101673, // Perpetual midnight aura
    SPELL_MIDNIGHT_VEIL = 101674, // Midnight veil
    SPELL_DARKNESS_EMBRACE = 101675, // Darkness embrace
    
    // Special abilities
    SPELL_ARCANE_SHOT = 101676, // Arcane shot
    SPELL_MULTI_SHOT = 101677, // Multi shot
    SPELL_TRUESHOT_AURA = 101678, // Trueshot aura
    SPELL_ASPECT_OF_THE_HAWK = 101679, // Aspect of the hawk
};

enum Events
{
    EVENT_MOONFIRE = 1,
    EVENT_STARFALL,
    EVENT_WRATH,
    EVENT_HURRICANE,
    EVENT_LIGHT_OF_ELUNE,
    EVENT_ELUNE_WRATH,
    EVENT_SHADOW_STRIKE,
    EVENT_SHADOW_BOLT,
    EVENT_SHADOW_NOVA,
    EVENT_PERPETUAL_MIDNIGHT,
    EVENT_MIDNIGHT_VEIL,
    EVENT_ARCANE_SHOT,
    EVENT_MULTI_SHOT,
    EVENT_ASPECT_OF_THE_HAWK,
    EVENT_PHASE_CHANGE,
};

enum Phases
{
    PHASE_NATURE = 1,
    PHASE_ELUNE,
    PHASE_SHADOW,
    PHASE_MIDNIGHT,
};

enum Creatures
{
    NPC_STARFALL = 54575,
    NPC_SHADOW_SPIRIT = 54576,
    NPC_ELUNE_SPIRIT = 54577,
};

class boss_echo_of_tyrande : public CreatureScript
{
    public:
        boss_echo_of_tyrande() : CreatureScript("boss_echo_of_tyrande") { }

        struct boss_echo_of_tyrandeAI : public BossAI
        {
            boss_echo_of_tyrandeAI(Creature* creature) : BossAI(creature, BOSS_ECHO_OF_TYRANDE)
            {
                introDone = false;
                instance = me->GetInstanceScript();
                currentPhase = PHASE_NATURE;
                perpetualMidnightActive = false;
            }

            InstanceScript* instance;
            bool introDone;
            uint8 currentPhase;
            bool perpetualMidnightActive;
            EventMap events;

            void Reset() override
            {
                events.Reset();
                currentPhase = PHASE_NATURE;
                perpetualMidnightActive = false;

                if (instance)
                {
                    instance->SetBossState(BOSS_ECHO_OF_TYRANDE, NOT_STARTED);
                    instance->HandleGameObject(4007, true); // Open doors
                    instance->HandleGameObject(4008, true);
                }

                if (!me->HasAura(SPELL_TYRANDE_VISUAL))
                    DoCast(me, SPELL_TYRANDE_VISUAL);

                me->RemoveAllAuras();
            }

            void EnterEvadeMode(EvadeReason why) override
            {
                me->GetMotionMaster()->MoveTargetedHome();
                Reset();

                me->SetHealth(me->GetMaxHealth());

                if (instance)
                {
                    instance->SetBossState(BOSS_ECHO_OF_TYRANDE, FAIL);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                }
            }

            void MoveInLineOfSight(Unit* who) override
            {
                if (introDone)
                    return;

                if (!me->IsWithinDistInMap(who, 40.0f, false))
                    return;

                Talk(SAY_INTRO);
                introDone = true;
            }

            void JustDied(Unit* /*killer*/) override
            {
                Talk(SAY_DEATH);

                if (instance)
                {
                    instance->SetBossState(BOSS_ECHO_OF_TYRANDE, DONE);

                    if (instance->GetData(DATA_FIRST_ENCOUNTER) == IN_PROGRESS)
                        instance->SetData(DATA_FIRST_ENCOUNTER, DONE);
                    else  
                        instance->SetData(DATA_SECOND_ENCOUNTER, DONE);

                    instance->HandleGameObject(4007, true);
                    instance->HandleGameObject(4008, true);

                    instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                }
            }

            void KilledUnit(Unit* /*victim*/) override
            {
                Talk(RAND(SAY_KILL_1, SAY_KILL_2, SAY_KILL_3));
            }

            void EnterCombat(Unit* /*who*/) override
            {
                Talk(SAY_AGGRO);

                if (instance)
                {
                    instance->SetBossState(BOSS_ECHO_OF_TYRANDE, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

                    instance->HandleGameObject(4007, false);
                    instance->HandleGameObject(4008, false);
                }

                ScheduleNaturePhase();
            }

            void ScheduleNaturePhase()
            {
                events.Reset();
                currentPhase = PHASE_NATURE;
                
                events.ScheduleEvent(EVENT_MOONFIRE, 5000);
                events.ScheduleEvent(EVENT_STARFALL, 15000);
                events.ScheduleEvent(EVENT_WRATH, 8000);
                events.ScheduleEvent(EVENT_HURRICANE, 25000);
                events.ScheduleEvent(EVENT_ARCANE_SHOT, 12000);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 60000);
            }

            void ScheduleElunePhase()
            {
                events.Reset();
                currentPhase = PHASE_ELUNE;
                
                DoCast(me, SPELL_ELUNE_BLESSING);
                
                events.ScheduleEvent(EVENT_LIGHT_OF_ELUNE, 10000);
                events.ScheduleEvent(EVENT_ELUNE_WRATH, 15000);
                events.ScheduleEvent(EVENT_STARFALL, 20000);
                events.ScheduleEvent(EVENT_MOONFIRE, 8000);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 60000);
            }

            void ScheduleShadowPhase()
            {
                events.Reset();
                currentPhase = PHASE_SHADOW;
                
                events.ScheduleEvent(EVENT_SHADOW_STRIKE, 8000);
                events.ScheduleEvent(EVENT_SHADOW_BOLT, 12000);
                events.ScheduleEvent(EVENT_SHADOW_NOVA, 20000);
                events.ScheduleEvent(EVENT_SHADOW_EMBRACE, 15000);
                events.ScheduleEvent(EVENT_MULTI_SHOT, 10000);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 60000);
            }

            void ScheduleMidnightPhase()
            {
                events.Reset();
                currentPhase = PHASE_MIDNIGHT;
                perpetualMidnightActive = true;
                
                Talk(SAY_PERPETUAL_MIDNIGHT);
                DoCast(me, SPELL_PERPETUAL_MIDNIGHT);
                
                events.ScheduleEvent(EVENT_MIDNIGHT_VEIL, 10000);
                events.ScheduleEvent(EVENT_DARKNESS_EMBRACE, 15000);
                events.ScheduleEvent(EVENT_SHADOW_STRIKE, 8000);
                events.ScheduleEvent(EVENT_ELUNE_WRATH, 20000);
                events.ScheduleEvent(EVENT_ASPECT_OF_THE_HAWK, 12000);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 60000);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_MOONFIRE:
                            Talk(SAY_MOONFIRE);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_MOONFIRE);
                            events.ScheduleEvent(EVENT_MOONFIRE, 12000);
                            break;

                        case EVENT_STARFALL:
                            Talk(SAY_STARFALL);
                            DoCast(me, SPELL_STARFALL);
                            events.ScheduleEvent(EVENT_STARFALL, 30000);
                            break;

                        case EVENT_WRATH:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_WRATH);
                            events.ScheduleEvent(EVENT_WRATH, 10000);
                            break;

                        case EVENT_HURRICANE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_HURRICANE);
                            events.ScheduleEvent(EVENT_HURRICANE, 35000);
                            break;

                        case EVENT_LIGHT_OF_ELUNE:
                            Talk(SAY_LIGHT_OF_ELUNE);
                            DoCast(me, SPELL_LIGHT_OF_ELUNE);
                            events.ScheduleEvent(EVENT_LIGHT_OF_ELUNE, 25000);
                            break;

                        case EVENT_ELUNE_WRATH:
                            Talk(SAY_ELUNE_WRATH);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_ELUNE_WRATH);
                            events.ScheduleEvent(EVENT_ELUNE_WRATH, 20000);
                            break;

                        case EVENT_SHADOW_STRIKE:
                            Talk(SAY_SHADOW_STRIKE);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_SHADOW_STRIKE);
                            events.ScheduleEvent(EVENT_SHADOW_STRIKE, 15000);
                            break;

                        case EVENT_SHADOW_BOLT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_SHADOW_BOLT);
                            events.ScheduleEvent(EVENT_SHADOW_BOLT, 12000);
                            break;

                        case EVENT_SHADOW_NOVA:
                            DoCast(me, SPELL_SHADOW_NOVA);
                            events.ScheduleEvent(EVENT_SHADOW_NOVA, 25000);
                            break;

                        case EVENT_PERPETUAL_MIDNIGHT:
                            if (perpetualMidnightActive)
                            {
                                DoCast(me, SPELL_PERPETUAL_MIDNIGHT);
                                events.ScheduleEvent(EVENT_PERPETUAL_MIDNIGHT, 5000);
                            }
                            break;

                        case EVENT_MIDNIGHT_VEIL:
                            DoCast(me, SPELL_MIDNIGHT_VEIL);
                            events.ScheduleEvent(EVENT_MIDNIGHT_VEIL, 20000);
                            break;

                        case EVENT_ARCANE_SHOT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_ARCANE_SHOT);
                            events.ScheduleEvent(EVENT_ARCANE_SHOT, 10000);
                            break;

                        case EVENT_MULTI_SHOT:
                            DoCast(me, SPELL_MULTI_SHOT);
                            events.ScheduleEvent(EVENT_MULTI_SHOT, 18000);
                            break;

                        case EVENT_ASPECT_OF_THE_HAWK:
                            DoCast(me, SPELL_ASPECT_OF_THE_HAWK);
                            events.ScheduleEvent(EVENT_ASPECT_OF_THE_HAWK, 30000);
                            break;

                        case EVENT_PHASE_CHANGE:
                            // Change phases based on health percentage
                            if (me->GetHealthPct() > 75.0f)
                                ScheduleElunePhase();
                            else if (me->GetHealthPct() > 50.0f)
                                ScheduleShadowPhase();
                            else if (me->GetHealthPct() > 25.0f)
                                ScheduleMidnightPhase();
                            else
                                ScheduleNaturePhase(); // Return to nature phase for final stand
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_echo_of_tyrandeAI(creature);
        }
};

// Starfall AI
class npc_starfall : public CreatureScript
{
    public:
        npc_starfall() : CreatureScript("npc_starfall") { }

        struct npc_starfallAI : public ScriptedAI
        {
            npc_starfallAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }

            void UpdateAI(uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_starfallAI(creature);
        }
};

void AddSC_boss_echo_of_tyrande()
{
    new boss_echo_of_tyrande();
    new npc_starfall();
} 
