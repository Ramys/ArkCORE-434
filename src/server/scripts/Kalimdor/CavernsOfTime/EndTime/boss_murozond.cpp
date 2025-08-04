/*
 * Copyright (C) 2011-2016 ArkCORE <http://www.arkania.net/>
 *
 * Murozond Boss Script
 * 
 * Now living outside of time, Murozond was once the great Dragon Aspect 
 * Nozdormu the Timeless One. After the titans showed him his own death, 
 * the tormented Nozdormu was tricked by the Old Gods into trying to subvert 
 * his mortality. As a result, Nozdormu shattered the timeways and created 
 * the Infinite Dragonflight... jeopardizing the very future of Azeroth.
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
    SAY_TIME_WARP,
    SAY_TEMPORAL_BLAST,
    SAY_INFINITE_BREATH,
    SAY_SHATTER_TIME,
    SAY_ETERNAL_NIGHTMARE,
    SAY_TIME_LOOP,
};

enum Spells
{
    SPELL_MUROZOND_VISUAL = 101680, // Visual effects on boss
    
    // Time manipulation spells
    SPELL_TIME_WARP = 101681, // Time warp
    SPELL_TEMPORAL_BLAST = 101682, // Temporal blast
    SPELL_TIME_LOOP = 101683, // Time loop
    SPELL_SHATTER_TIME = 101684, // Shatter time
    SPELL_TEMPORAL_VORTEX = 101685, // Temporal vortex
    SPELL_TIME_DILATION = 101686, // Time dilation
    
    // Dragon abilities
    SPELL_INFINITE_BREATH = 101687, // Infinite breath
    SPELL_TAIL_SWEEP = 101688, // Tail sweep
    SPELL_WING_BUFFET = 101689, // Wing buffet
    SPELL_CLAW_SWIPE = 101690, // Claw swipe
    
    // Infinite Dragonflight abilities
    SPELL_ETERNAL_NIGHTMARE = 101691, // Eternal nightmare
    SPELL_INFINITE_CORRUPTION = 101692, // Infinite corruption
    SPELL_TIME_DISTORTION = 101693, // Time distortion
    SPELL_TEMPORAL_RIFT = 101694, // Temporal rift
    
    // Special abilities
    SPELL_CHRONOSPHERE = 101695, // Chronosphere
    SPELL_TIME_STOP = 101696, // Time stop
    SPELL_TEMPORAL_EXPLOSION = 101697, // Temporal explosion
    SPELL_INFINITE_PRESENCE = 101698, // Infinite presence
};

enum Events
{
    EVENT_TIME_WARP = 1,
    EVENT_TEMPORAL_BLAST,
    EVENT_TIME_LOOP,
    EVENT_SHATTER_TIME,
    EVENT_TEMPORAL_VORTEX,
    EVENT_TIME_DILATION,
    EVENT_INFINITE_BREATH,
    EVENT_TAIL_SWEEP,
    EVENT_WING_BUFFET,
    EVENT_CLAW_SWIPE,
    EVENT_ETERNAL_NIGHTMARE,
    EVENT_INFINITE_CORRUPTION,
    EVENT_TIME_DISTORTION,
    EVENT_TEMPORAL_RIFT,
    EVENT_CHRONOSPHERE,
    EVENT_TIME_STOP,
    EVENT_TEMPORAL_EXPLOSION,
    EVENT_INFINITE_PRESENCE,
    EVENT_PHASE_CHANGE,
    EVENT_LANDING,
    EVENT_TAKEOFF,
};

enum Phases
{
    PHASE_GROUND = 1,
    PHASE_AIR,
    PHASE_TIME_WARP,
    PHASE_INFINITE,
};

enum Creatures
{
    NPC_TEMPORAL_RIFT = 54578,
    NPC_TIME_DISTORTION = 54579,
    NPC_INFINITE_DRAGON = 54580,
};

class boss_murozond : public CreatureScript
{
    public:
        boss_murozond() : CreatureScript("boss_murozond") { }

        struct boss_murozondAI : public BossAI
        {
            boss_murozondAI(Creature* creature) : BossAI(creature, BOSS_MUROZOND)
            {
                introDone = false;
                instance = me->GetInstanceScript();
                currentPhase = PHASE_GROUND;
                isFlying = false;
                timeWarpActive = false;
            }

            InstanceScript* instance;
            bool introDone;
            uint8 currentPhase;
            bool isFlying;
            bool timeWarpActive;
            EventMap events;

            void Reset() override
            {
                events.Reset();
                currentPhase = PHASE_GROUND;
                isFlying = false;
                timeWarpActive = false;

                if (instance)
                {
                    instance->SetBossState(BOSS_MUROZOND, NOT_STARTED);
                    instance->HandleGameObject(4009, true); // Open doors
                    instance->HandleGameObject(4010, true);
                }

                if (!me->HasAura(SPELL_MUROZOND_VISUAL))
                    DoCast(me, SPELL_MUROZOND_VISUAL);

                me->RemoveAllAuras();
                me->SetCanFly(false);
                me->SetDisableGravity(false);
            }

            void EnterEvadeMode(EvadeReason why) override
            {
                me->GetMotionMaster()->MoveTargetedHome();
                Reset();

                me->SetHealth(me->GetMaxHealth());

                if (instance)
                {
                    instance->SetBossState(BOSS_MUROZOND, FAIL);
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
                    instance->SetBossState(BOSS_MUROZOND, DONE);
                    instance->HandleGameObject(4009, true);
                    instance->HandleGameObject(4010, true);
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
                    instance->SetBossState(BOSS_MUROZOND, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

                    instance->HandleGameObject(4009, false);
                    instance->HandleGameObject(4010, false);
                }

                ScheduleGroundPhase();
            }

            void ScheduleGroundPhase()
            {
                events.Reset();
                currentPhase = PHASE_GROUND;
                isFlying = false;
                
                me->SetCanFly(false);
                me->SetDisableGravity(false);
                
                events.ScheduleEvent(EVENT_CLAW_SWIPE, 8000);
                events.ScheduleEvent(EVENT_TAIL_SWEEP, 15000);
                events.ScheduleEvent(EVENT_TEMPORAL_BLAST, 12000);
                events.ScheduleEvent(EVENT_TIME_DILATION, 20000);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 60000);
            }

            void ScheduleAirPhase()
            {
                events.Reset();
                currentPhase = PHASE_AIR;
                isFlying = true;
                
                Talk(SAY_TAKEOFF);
                me->SetCanFly(true);
                me->SetDisableGravity(true);
                me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() + 20.0f);
                
                events.ScheduleEvent(EVENT_INFINITE_BREATH, 10000);
                events.ScheduleEvent(EVENT_WING_BUFFET, 15000);
                events.ScheduleEvent(EVENT_TEMPORAL_RIFT, 20000);
                events.ScheduleEvent(EVENT_LANDING, 45000);
            }

            void ScheduleTimeWarpPhase()
            {
                events.Reset();
                currentPhase = PHASE_TIME_WARP;
                timeWarpActive = true;
                
                Talk(SAY_TIME_WARP);
                DoCast(me, SPELL_TIME_WARP);
                
                events.ScheduleEvent(EVENT_TIME_LOOP, 10000);
                events.ScheduleEvent(EVENT_SHATTER_TIME, 15000);
                events.ScheduleEvent(EVENT_TEMPORAL_VORTEX, 20000);
                events.ScheduleEvent(EVENT_CHRONOSPHERE, 12000);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 60000);
            }

            void ScheduleInfinitePhase()
            {
                events.Reset();
                currentPhase = PHASE_INFINITE;
                
                Talk(SAY_ETERNAL_NIGHTMARE);
                DoCast(me, SPELL_ETERNAL_NIGHTMARE);
                
                events.ScheduleEvent(EVENT_INFINITE_CORRUPTION, 10000);
                events.ScheduleEvent(EVENT_TIME_DISTORTION, 15000);
                events.ScheduleEvent(EVENT_TIME_STOP, 20000);
                events.ScheduleEvent(EVENT_TEMPORAL_EXPLOSION, 25000);
                events.ScheduleEvent(EVENT_INFINITE_PRESENCE, 12000);
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
                        case EVENT_TIME_WARP:
                            Talk(SAY_TIME_WARP);
                            DoCast(me, SPELL_TIME_WARP);
                            events.ScheduleEvent(EVENT_TIME_WARP, 45000);
                            break;

                        case EVENT_TEMPORAL_BLAST:
                            Talk(SAY_TEMPORAL_BLAST);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_TEMPORAL_BLAST);
                            events.ScheduleEvent(EVENT_TEMPORAL_BLAST, 15000);
                            break;

                        case EVENT_TIME_LOOP:
                            Talk(SAY_TIME_LOOP);
                            DoCast(me, SPELL_TIME_LOOP);
                            events.ScheduleEvent(EVENT_TIME_LOOP, 30000);
                            break;

                        case EVENT_SHATTER_TIME:
                            Talk(SAY_SHATTER_TIME);
                            DoCast(me, SPELL_SHATTER_TIME);
                            events.ScheduleEvent(EVENT_SHATTER_TIME, 40000);
                            break;

                        case EVENT_TEMPORAL_VORTEX:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_TEMPORAL_VORTEX);
                            events.ScheduleEvent(EVENT_TEMPORAL_VORTEX, 25000);
                            break;

                        case EVENT_TIME_DILATION:
                            DoCast(me, SPELL_TIME_DILATION);
                            events.ScheduleEvent(EVENT_TIME_DILATION, 35000);
                            break;

                        case EVENT_INFINITE_BREATH:
                            Talk(SAY_INFINITE_BREATH);
                            DoCast(me, SPELL_INFINITE_BREATH);
                            events.ScheduleEvent(EVENT_INFINITE_BREATH, 20000);
                            break;

                        case EVENT_TAIL_SWEEP:
                            DoCast(me, SPELL_TAIL_SWEEP);
                            events.ScheduleEvent(EVENT_TAIL_SWEEP, 25000);
                            break;

                        case EVENT_WING_BUFFET:
                            DoCast(me, SPELL_WING_BUFFET);
                            events.ScheduleEvent(EVENT_WING_BUFFET, 30000);
                            break;

                        case EVENT_CLAW_SWIPE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_CLAW_SWIPE);
                            events.ScheduleEvent(EVENT_CLAW_SWIPE, 12000);
                            break;

                        case EVENT_ETERNAL_NIGHTMARE:
                            Talk(SAY_ETERNAL_NIGHTMARE);
                            DoCast(me, SPELL_ETERNAL_NIGHTMARE);
                            events.ScheduleEvent(EVENT_ETERNAL_NIGHTMARE, 60000);
                            break;

                        case EVENT_INFINITE_CORRUPTION:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_INFINITE_CORRUPTION);
                            events.ScheduleEvent(EVENT_INFINITE_CORRUPTION, 18000);
                            break;

                        case EVENT_TIME_DISTORTION:
                            DoCast(me, SPELL_TIME_DISTORTION);
                            events.ScheduleEvent(EVENT_TIME_DISTORTION, 22000);
                            break;

                        case EVENT_TEMPORAL_RIFT:
                            for (uint8 i = 0; i < 3; ++i)
                            {
                                float x = me->GetPositionX() + frand(-15.0f, 15.0f);
                                float y = me->GetPositionY() + frand(-15.0f, 15.0f);
                                float z = me->GetPositionZ();
                                
                                if (Creature* rift = me->SummonCreature(NPC_TEMPORAL_RIFT, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 30000))
                                {
                                    rift->SetFaction(me->GetFaction());
                                    rift->AI()->AttackStart(SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true));
                                }
                            }
                            events.ScheduleEvent(EVENT_TEMPORAL_RIFT, 35000);
                            break;

                        case EVENT_CHRONOSPHERE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_CHRONOSPHERE);
                            events.ScheduleEvent(EVENT_CHRONOSPHERE, 28000);
                            break;

                        case EVENT_TIME_STOP:
                            DoCast(me, SPELL_TIME_STOP);
                            events.ScheduleEvent(EVENT_TIME_STOP, 40000);
                            break;

                        case EVENT_TEMPORAL_EXPLOSION:
                            DoCast(me, SPELL_TEMPORAL_EXPLOSION);
                            events.ScheduleEvent(EVENT_TEMPORAL_EXPLOSION, 50000);
                            break;

                        case EVENT_INFINITE_PRESENCE:
                            DoCast(me, SPELL_INFINITE_PRESENCE);
                            events.ScheduleEvent(EVENT_INFINITE_PRESENCE, 25000);
                            break;

                        case EVENT_LANDING:
                            Talk(SAY_LANDING);
                            me->SetCanFly(false);
                            me->SetDisableGravity(false);
                            me->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() - 20.0f);
                            isFlying = false;
                            ScheduleGroundPhase();
                            break;

                        case EVENT_PHASE_CHANGE:
                            // Change phases based on health percentage
                            if (me->GetHealthPct() > 75.0f)
                                ScheduleAirPhase();
                            else if (me->GetHealthPct() > 50.0f)
                                ScheduleTimeWarpPhase();
                            else if (me->GetHealthPct() > 25.0f)
                                ScheduleInfinitePhase();
                            else
                                ScheduleGroundPhase(); // Return to ground phase for final stand
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_murozondAI(creature);
        }
};

// Temporal Rift AI
class npc_temporal_rift : public CreatureScript
{
    public:
        npc_temporal_rift() : CreatureScript("npc_temporal_rift") { }

        struct npc_temporal_riftAI : public ScriptedAI
        {
            npc_temporal_riftAI(Creature* creature) : ScriptedAI(creature)
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
            return new npc_temporal_riftAI(creature);
        }
};

void AddSC_boss_murozond()
{
    new boss_murozond();
    new npc_temporal_rift();
} 
