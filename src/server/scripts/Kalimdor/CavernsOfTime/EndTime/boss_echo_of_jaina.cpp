/*
 * Copyright (C) 2011-2016 ArkCORE <http://www.arkania.net/>
 *
 * Echo of Jaina Boss Script
 * 
 * The tormented fragment of Jaina Proudmoore has been split and infused within 
 * the shattered pieces of her staff. To restore balance to the timeways, this 
 * echo must be defeated. However, the devastating magical power possessed by 
 * the once-proud ruler of Theramore is hardly lost to her time-havocked spectre.
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
    SAY_FROST_NOVA,
    SAY_BLIZZARD,
    SAY_ICE_BARRIER,
    SAY_TIME_WARP,
};

enum Spells
{
    SPELL_JAINA_VISUAL = 101625, // Visual effects on boss
    
    // Frost spells
    SPELL_FROST_NOVA = 101612, // AoE frost nova
    SPELL_BLIZZARD = 101627, // Blizzard spell
    SPELL_ICE_BARRIER = 101626, // Ice barrier shield
    SPELL_FROSTBOLT = 101628, // Single target frostbolt
    SPELL_ICE_LANCE = 101629, // Ice lance spell
    
    // Arcane spells
    SPELL_ARCANE_BLAST = 101630, // Arcane blast
    SPELL_ARCANE_MISSILES = 101631, // Arcane missiles
    SPELL_ARCANE_POWER = 101632, // Arcane power buff
    
    // Time manipulation
    SPELL_TIME_WARP = 101633, // Time warp spell
    SPELL_TIME_SLOW = 101634, // Slow time effect
    
    // Staff fragments
    SPELL_STAFF_FRAGMENT = 101635, // Summon staff fragment
    SPELL_FRAGMENT_EXPLOSION = 101636, // Fragment explosion
    
    // Special abilities
    SPELL_MANA_SHIELD = 101637, // Mana shield
    SPELL_COUNTERSPELL = 101638, // Counterspell
    SPELL_POLYMORPH = 101639, // Polymorph
};

enum Events
{
    EVENT_FROST_NOVA = 1,
    EVENT_BLIZZARD,
    EVENT_ICE_BARRIER,
    EVENT_FROSTBOLT,
    EVENT_ICE_LANCE,
    EVENT_ARCANE_BLAST,
    EVENT_ARCANE_MISSILES,
    EVENT_TIME_WARP,
    EVENT_STAFF_FRAGMENT,
    EVENT_COUNTERSPELL,
    EVENT_POLYMORPH,
    EVENT_PHASE_CHANGE,
};

enum Phases
{
    PHASE_FROST = 1,
    PHASE_ARCANE,
    PHASE_TIME,
    PHASE_FRAGMENTED,
};

enum Creatures
{
    NPC_STAFF_FRAGMENT = 54570,
    NPC_TIME_RIFT = 54571,
};

class boss_echo_of_jaina : public CreatureScript
{
    public:
        boss_echo_of_jaina() : CreatureScript("boss_echo_of_jaina") { }

        struct boss_echo_of_jainaAI : public BossAI
        {
            boss_echo_of_jainaAI(Creature* creature) : BossAI(creature, BOSS_ECHO_OF_JAINA)
            {
                introDone = false;
                instance = me->GetInstanceScript();
                currentPhase = PHASE_FROST;
                fragmentCount = 0;
            }

            InstanceScript* instance;
            bool introDone;
            uint8 currentPhase;
            uint8 fragmentCount;
            EventMap events;

            void Reset() override
            {
                events.Reset();
                currentPhase = PHASE_FROST;
                fragmentCount = 0;

                if (instance)
                {
                    instance->SetBossState(BOSS_ECHO_OF_JAINA, NOT_STARTED);
                    instance->HandleGameObject(4003, true); // Open doors
                    instance->HandleGameObject(4004, true);
                }

                if (!me->HasAura(SPELL_JAINA_VISUAL))
                    DoCast(me, SPELL_JAINA_VISUAL);

                me->RemoveAllAuras();
            }

            void EnterEvadeMode(EvadeReason why) override
            {
                me->GetMotionMaster()->MoveTargetedHome();
                Reset();

                me->SetHealth(me->GetMaxHealth());

                if (instance)
                {
                    instance->SetBossState(BOSS_ECHO_OF_JAINA, FAIL);
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
                    instance->SetBossState(BOSS_ECHO_OF_JAINA, DONE);

                    if (instance->GetData(DATA_FIRST_ENCOUNTER) == IN_PROGRESS)
                        instance->SetData(DATA_FIRST_ENCOUNTER, DONE);
                    else  
                        instance->SetData(DATA_SECOND_ENCOUNTER, DONE);

                    instance->HandleGameObject(4003, true);
                    instance->HandleGameObject(4004, true);

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
                    instance->SetBossState(BOSS_ECHO_OF_JAINA, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

                    instance->HandleGameObject(4003, false);
                    instance->HandleGameObject(4004, false);
                }

                ScheduleFrostPhase();
            }

            void ScheduleFrostPhase()
            {
                events.Reset();
                currentPhase = PHASE_FROST;
                
                events.ScheduleEvent(EVENT_FROST_NOVA, 15000);
                events.ScheduleEvent(EVENT_BLIZZARD, 25000);
                events.ScheduleEvent(EVENT_ICE_BARRIER, 35000);
                events.ScheduleEvent(EVENT_FROSTBOLT, 5000);
                events.ScheduleEvent(EVENT_ICE_LANCE, 12000);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 60000); // Change phase after 1 minute
            }

            void ScheduleArcanePhase()
            {
                events.Reset();
                currentPhase = PHASE_ARCANE;
                
                DoCast(me, SPELL_ARCANE_POWER);
                
                events.ScheduleEvent(EVENT_ARCANE_BLAST, 8000);
                events.ScheduleEvent(EVENT_ARCANE_MISSILES, 15000);
                events.ScheduleEvent(EVENT_COUNTERSPELL, 20000);
                events.ScheduleEvent(EVENT_POLYMORPH, 30000);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 60000);
            }

            void ScheduleTimePhase()
            {
                events.Reset();
                currentPhase = PHASE_TIME;
                
                events.ScheduleEvent(EVENT_TIME_WARP, 10000);
                events.ScheduleEvent(EVENT_STAFF_FRAGMENT, 20000);
                events.ScheduleEvent(EVENT_BLIZZARD, 30000);
                events.ScheduleEvent(EVENT_ARCANE_BLAST, 15000);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 60000);
            }

            void ScheduleFragmentedPhase()
            {
                events.Reset();
                currentPhase = PHASE_FRAGMENTED;
                
                // Summon staff fragments
                for (uint8 i = 0; i < 3; ++i)
                {
                    float x = me->GetPositionX() + frand(-10.0f, 10.0f);
                    float y = me->GetPositionY() + frand(-10.0f, 10.0f);
                    float z = me->GetPositionZ();
                    
                    if (Creature* fragment = me->SummonCreature(NPC_STAFF_FRAGMENT, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 30000))
                    {
                        fragment->SetFaction(me->GetFaction());
                        fragment->AI()->AttackStart(SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true));
                    }
                }
                
                events.ScheduleEvent(EVENT_STAFF_FRAGMENT, 15000);
                events.ScheduleEvent(EVENT_FRAGMENT_EXPLOSION, 25000);
                events.ScheduleEvent(EVENT_BLIZZARD, 20000);
                events.ScheduleEvent(EVENT_ARCANE_MISSILES, 10000);
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
                        case EVENT_FROST_NOVA:
                            Talk(SAY_FROST_NOVA);
                            DoCast(me, SPELL_FROST_NOVA);
                            events.ScheduleEvent(EVENT_FROST_NOVA, 30000);
                            break;

                        case EVENT_BLIZZARD:
                            Talk(SAY_BLIZZARD);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_BLIZZARD);
                            events.ScheduleEvent(EVENT_BLIZZARD, 40000);
                            break;

                        case EVENT_ICE_BARRIER:
                            Talk(SAY_ICE_BARRIER);
                            DoCast(me, SPELL_ICE_BARRIER);
                            events.ScheduleEvent(EVENT_ICE_BARRIER, 45000);
                            break;

                        case EVENT_FROSTBOLT:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_FROSTBOLT);
                            events.ScheduleEvent(EVENT_FROSTBOLT, 8000);
                            break;

                        case EVENT_ICE_LANCE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_ICE_LANCE);
                            events.ScheduleEvent(EVENT_ICE_LANCE, 15000);
                            break;

                        case EVENT_ARCANE_BLAST:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_ARCANE_BLAST);
                            events.ScheduleEvent(EVENT_ARCANE_BLAST, 12000);
                            break;

                        case EVENT_ARCANE_MISSILES:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_ARCANE_MISSILES);
                            events.ScheduleEvent(EVENT_ARCANE_MISSILES, 20000);
                            break;

                        case EVENT_TIME_WARP:
                            Talk(SAY_TIME_WARP);
                            DoCast(me, SPELL_TIME_WARP);
                            events.ScheduleEvent(EVENT_TIME_WARP, 60000);
                            break;

                        case EVENT_STAFF_FRAGMENT:
                            DoCast(me, SPELL_STAFF_FRAGMENT);
                            events.ScheduleEvent(EVENT_STAFF_FRAGMENT, 25000);
                            break;

                        case EVENT_COUNTERSPELL:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_COUNTERSPELL);
                            events.ScheduleEvent(EVENT_COUNTERSPELL, 30000);
                            break;

                        case EVENT_POLYMORPH:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_POLYMORPH);
                            events.ScheduleEvent(EVENT_POLYMORPH, 40000);
                            break;

                        case EVENT_PHASE_CHANGE:
                            // Change phases based on health percentage
                            if (me->GetHealthPct() > 75.0f)
                                ScheduleArcanePhase();
                            else if (me->GetHealthPct() > 50.0f)
                                ScheduleTimePhase();
                            else if (me->GetHealthPct() > 25.0f)
                                ScheduleFragmentedPhase();
                            else
                                ScheduleFrostPhase(); // Return to frost phase for final stand
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_echo_of_jainaAI(creature);
        }
};

// Staff Fragment AI
class npc_staff_fragment : public CreatureScript
{
    public:
        npc_staff_fragment() : CreatureScript("npc_staff_fragment") { }

        struct npc_staff_fragmentAI : public ScriptedAI
        {
            npc_staff_fragmentAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }

            void JustDied(Unit* /*killer*/) override
            {
                DoCast(me, SPELL_FRAGMENT_EXPLOSION, true);
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
            return new npc_staff_fragmentAI(creature);
        }
};

void AddSC_boss_echo_of_jaina()
{
    new boss_echo_of_jaina();
    new npc_staff_fragment();
} 
