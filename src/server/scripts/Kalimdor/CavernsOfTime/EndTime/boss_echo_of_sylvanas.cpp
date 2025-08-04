/*
 * Copyright (C) 2011-2016 ArkCORE <http://www.arkania.net/>
 *
 * Echo of Sylvanas Boss Script
 * 
 * Spawned from shattered timeways and cloistered within the Ruby Dragonshrine, 
 * a maddened fragment of the Forsaken's leader, Sylvanas Windrunner, waits 
 * restlessly. Having lost everything and unable to find peace, this tormented 
 * echo aches for a chance to unleash her dark fury at anything still living 
 * within these barren, time-twisted wastes.
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
    SAY_SHADOW_BOLT,
    SAY_WAILING_SOULS,
    SAY_BLACK_ARROW,
    SAY_DARK_FURY,
    SAY_RAISE_DEAD,
};

enum Spells
{
    SPELL_SYLVANAS_VISUAL = 101640, // Visual effects on boss
    
    // Shadow spells
    SPELL_SHADOW_BOLT = 101641, // Shadow bolt
    SPELL_SHADOW_BOLT_VOLLEY = 101642, // Shadow bolt volley
    SPELL_SHADOW_STEP = 101643, // Shadow step teleport
    SPELL_SHADOW_EMBRACE = 101644, // Shadow embrace debuff
    
    // Death spells
    SPELL_DEATH_COIL = 101645, // Death coil
    SPELL_DEATH_GRIP = 101646, // Death grip
    SPELL_RAISE_DEAD = 101647, // Raise dead
    SPELL_DEATH_AND_DECAY = 101648, // Death and decay
    
    // Wailing Souls
    SPELL_WAILING_SOULS = 101649, // Wailing souls channel
    SPELL_WAILING_SOULS_DAMAGE = 101650, // Wailing souls damage
    
    // Black Arrow
    SPELL_BLACK_ARROW = 101651, // Black arrow
    SPELL_BLACK_ARROW_DOT = 101652, // Black arrow dot
    
    // Dark Fury
    SPELL_DARK_FURY = 101653, // Dark fury buff
    SPELL_DARK_FURY_DAMAGE = 101654, // Dark fury damage
    
    // Special abilities
    SPELL_BANSHEE_WAIL = 101655, // Banshee wail
    SPELL_BANSHEE_CURSE = 101656, // Banshee curse
    SPELL_SOUL_REND = 101657, // Soul rend
    SPELL_VOID_BLAST = 101658, // Void blast
};

enum Events
{
    EVENT_SHADOW_BOLT = 1,
    EVENT_SHADOW_BOLT_VOLLEY,
    EVENT_SHADOW_STEP,
    EVENT_DEATH_COIL,
    EVENT_DEATH_GRIP,
    EVENT_RAISE_DEAD,
    EVENT_DEATH_AND_DECAY,
    EVENT_WAILING_SOULS,
    EVENT_BLACK_ARROW,
    EVENT_DARK_FURY,
    EVENT_BANSHEE_WAIL,
    EVENT_BANSHEE_CURSE,
    EVENT_SOUL_REND,
    EVENT_VOID_BLAST,
    EVENT_PHASE_CHANGE,
};

enum Phases
{
    PHASE_NORMAL = 1,
    PHASE_WAILING,
    PHASE_DARK_FURY,
    PHASE_BANSHEE,
};

enum Creatures
{
    NPC_RAISED_DEAD = 54572,
    NPC_WAILING_SOUL = 54573,
    NPC_BANSHEE_SPIRIT = 54574,
};

class boss_echo_of_sylvanas : public CreatureScript
{
    public:
        boss_echo_of_sylvanas() : CreatureScript("boss_echo_of_sylvanas") { }

        struct boss_echo_of_sylvanasAI : public BossAI
        {
            boss_echo_of_sylvanasAI(Creature* creature) : BossAI(creature, BOSS_ECHO_OF_SYLVANAS)
            {
                introDone = false;
                instance = me->GetInstanceScript();
                currentPhase = PHASE_NORMAL;
                wailingSoulsActive = false;
            }

            InstanceScript* instance;
            bool introDone;
            uint8 currentPhase;
            bool wailingSoulsActive;
            EventMap events;

            void Reset() override
            {
                events.Reset();
                currentPhase = PHASE_NORMAL;
                wailingSoulsActive = false;

                if (instance)
                {
                    instance->SetBossState(BOSS_ECHO_OF_SYLVANAS, NOT_STARTED);
                    instance->HandleGameObject(4005, true); // Open doors
                    instance->HandleGameObject(4006, true);
                }

                if (!me->HasAura(SPELL_SYLVANAS_VISUAL))
                    DoCast(me, SPELL_SYLVANAS_VISUAL);

                me->RemoveAllAuras();
            }

            void EnterEvadeMode(EvadeReason why) override
            {
                me->GetMotionMaster()->MoveTargetedHome();
                Reset();

                me->SetHealth(me->GetMaxHealth());

                if (instance)
                {
                    instance->SetBossState(BOSS_ECHO_OF_SYLVANAS, FAIL);
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
                    instance->SetBossState(BOSS_ECHO_OF_SYLVANAS, DONE);

                    if (instance->GetData(DATA_FIRST_ENCOUNTER) == IN_PROGRESS)
                        instance->SetData(DATA_FIRST_ENCOUNTER, DONE);
                    else  
                        instance->SetData(DATA_SECOND_ENCOUNTER, DONE);

                    instance->HandleGameObject(4005, true);
                    instance->HandleGameObject(4006, true);

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
                    instance->SetBossState(BOSS_ECHO_OF_SYLVANAS, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

                    instance->HandleGameObject(4005, false);
                    instance->HandleGameObject(4006, false);
                }

                ScheduleNormalPhase();
            }

            void ScheduleNormalPhase()
            {
                events.Reset();
                currentPhase = PHASE_NORMAL;
                
                events.ScheduleEvent(EVENT_SHADOW_BOLT, 5000);
                events.ScheduleEvent(EVENT_SHADOW_BOLT_VOLLEY, 15000);
                events.ScheduleEvent(EVENT_SHADOW_STEP, 25000);
                events.ScheduleEvent(EVENT_DEATH_COIL, 12000);
                events.ScheduleEvent(EVENT_DEATH_GRIP, 20000);
                events.ScheduleEvent(EVENT_BLACK_ARROW, 18000);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 60000);
            }

            void ScheduleWailingPhase()
            {
                events.Reset();
                currentPhase = PHASE_WAILING;
                wailingSoulsActive = true;
                
                Talk(SAY_WAILING_SOULS);
                DoCast(me, SPELL_WAILING_SOULS);
                
                events.ScheduleEvent(EVENT_WAILING_SOULS, 10000);
                events.ScheduleEvent(EVENT_SHADOW_BOLT, 8000);
                events.ScheduleEvent(EVENT_DEATH_COIL, 15000);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 30000);
            }

            void ScheduleDarkFuryPhase()
            {
                events.Reset();
                currentPhase = PHASE_DARK_FURY;
                
                Talk(SAY_DARK_FURY);
                DoCast(me, SPELL_DARK_FURY);
                
                events.ScheduleEvent(EVENT_DARK_FURY, 5000);
                events.ScheduleEvent(EVENT_SHADOW_BOLT_VOLLEY, 10000);
                events.ScheduleEvent(EVENT_DEATH_AND_DECAY, 15000);
                events.ScheduleEvent(EVENT_VOID_BLAST, 20000);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 45000);
            }

            void ScheduleBansheePhase()
            {
                events.Reset();
                currentPhase = PHASE_BANSHEE;
                
                events.ScheduleEvent(EVENT_BANSHEE_WAIL, 8000);
                events.ScheduleEvent(EVENT_BANSHEE_CURSE, 15000);
                events.ScheduleEvent(EVENT_SOUL_REND, 12000);
                events.ScheduleEvent(EVENT_RAISE_DEAD, 20000);
                events.ScheduleEvent(EVENT_SHADOW_STEP, 10000);
                events.ScheduleEvent(EVENT_PHASE_CHANGE, 40000);
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
                        case EVENT_SHADOW_BOLT:
                            Talk(SAY_SHADOW_BOLT);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_SHADOW_BOLT);
                            events.ScheduleEvent(EVENT_SHADOW_BOLT, 8000);
                            break;

                        case EVENT_SHADOW_BOLT_VOLLEY:
                            DoCast(me, SPELL_SHADOW_BOLT_VOLLEY);
                            events.ScheduleEvent(EVENT_SHADOW_BOLT_VOLLEY, 20000);
                            break;

                        case EVENT_SHADOW_STEP:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            {
                                DoCast(target, SPELL_SHADOW_STEP);
                                me->NearTeleportTo(target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation());
                            }
                            events.ScheduleEvent(EVENT_SHADOW_STEP, 30000);
                            break;

                        case EVENT_DEATH_COIL:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_DEATH_COIL);
                            events.ScheduleEvent(EVENT_DEATH_COIL, 15000);
                            break;

                        case EVENT_DEATH_GRIP:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_DEATH_GRIP);
                            events.ScheduleEvent(EVENT_DEATH_GRIP, 25000);
                            break;

                        case EVENT_RAISE_DEAD:
                            Talk(SAY_RAISE_DEAD);
                            for (uint8 i = 0; i < 3; ++i)
                            {
                                float x = me->GetPositionX() + frand(-10.0f, 10.0f);
                                float y = me->GetPositionY() + frand(-10.0f, 10.0f);
                                float z = me->GetPositionZ();
                                
                                if (Creature* undead = me->SummonCreature(NPC_RAISED_DEAD, x, y, z, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 30000))
                                {
                                    undead->SetFaction(me->GetFaction());
                                    undead->AI()->AttackStart(SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true));
                                }
                            }
                            events.ScheduleEvent(EVENT_RAISE_DEAD, 35000);
                            break;

                        case EVENT_DEATH_AND_DECAY:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_DEATH_AND_DECAY);
                            events.ScheduleEvent(EVENT_DEATH_AND_DECAY, 25000);
                            break;

                        case EVENT_WAILING_SOULS:
                            if (wailingSoulsActive)
                            {
                                DoCast(me, SPELL_WAILING_SOULS_DAMAGE);
                                events.ScheduleEvent(EVENT_WAILING_SOULS, 3000);
                            }
                            break;

                        case EVENT_BLACK_ARROW:
                            Talk(SAY_BLACK_ARROW);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_BLACK_ARROW);
                            events.ScheduleEvent(EVENT_BLACK_ARROW, 22000);
                            break;

                        case EVENT_DARK_FURY:
                            DoCast(me, SPELL_DARK_FURY_DAMAGE);
                            events.ScheduleEvent(EVENT_DARK_FURY, 8000);
                            break;

                        case EVENT_BANSHEE_WAIL:
                            Talk(SAY_BANSHEE_WAIL);
                            DoCast(me, SPELL_BANSHEE_WAIL);
                            events.ScheduleEvent(EVENT_BANSHEE_WAIL, 15000);
                            break;

                        case EVENT_BANSHEE_CURSE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_BANSHEE_CURSE);
                            events.ScheduleEvent(EVENT_BANSHEE_CURSE, 20000);
                            break;

                        case EVENT_SOUL_REND:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_SOUL_REND);
                            events.ScheduleEvent(EVENT_SOUL_REND, 18000);
                            break;

                        case EVENT_VOID_BLAST:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_VOID_BLAST);
                            events.ScheduleEvent(EVENT_VOID_BLAST, 12000);
                            break;

                        case EVENT_PHASE_CHANGE:
                            // Change phases based on health percentage
                            if (me->GetHealthPct() > 75.0f)
                                ScheduleWailingPhase();
                            else if (me->GetHealthPct() > 50.0f)
                                ScheduleDarkFuryPhase();
                            else if (me->GetHealthPct() > 25.0f)
                                ScheduleBansheePhase();
                            else
                                ScheduleNormalPhase(); // Return to normal phase for final stand
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_echo_of_sylvanasAI(creature);
        }
};

// Raised Dead AI
class npc_raised_dead : public CreatureScript
{
    public:
        npc_raised_dead() : CreatureScript("npc_raised_dead") { }

        struct npc_raised_deadAI : public ScriptedAI
        {
            npc_raised_deadAI(Creature* creature) : ScriptedAI(creature)
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
            return new npc_raised_deadAI(creature);
        }
};

void AddSC_boss_echo_of_sylvanas()
{
    new boss_echo_of_sylvanas();
    new npc_raised_dead();
} 
