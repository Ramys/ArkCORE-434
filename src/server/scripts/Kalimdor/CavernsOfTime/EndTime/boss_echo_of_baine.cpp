/*
 * Copyright (C) 2011-2016 ArkCORE <http://www.arkania.net/>
 *
 * Script 100% done. Features:
 *
 * - Throw back totem mechanic implemented
 * - Nozdormu teleport say added
 * - Live testing completed
 * - All mechanics working properly
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
    SAY_THROW_TOTEM,
    SAY_PULVERIZE,
    SAY_NOZDORMU_TELEPORT,
};

enum Spells
{
    SPELL_BAINE_VIS   = 101624, // Visuals on boss (the totems on the back etc.)
    SPELL_THROW_TOTEM = 101615, // Triggers missile at location, with summon totem and kb.
    SPELL_PULVERIZE_J = 101626, // Jump to target, activate platform.
    SPELL_PULVERIZE_D = 101627, // Damage spell.
    SPELL_PULV_DBM    = 101625, // DBM spell for tracking.
    SPELL_MOLTEN_AXE  = 101836, // Extra damage on melee attack when in lava.
    SPELL_MOLTEN_FIST = 101866, // Extra damage on melee for players when they touch the lava.
    SPELL_TB_TOTEM    = 101602, // Throw totem back at Baine on click.
    SPELL_TB_TOTEM_A  = 107837, // Visual aura: player has totem to throw.
    SPELL_NOZDORMU_TELEPORT = 101595, // Nozdormu teleport spell
};

enum Events
{
    EVENT_PULVERIZE = 1,
    EVENT_PULVERIZE_DAMAGE,
    EVENT_THROW_TOTEM,
    EVENT_NOZDORMU_TELEPORT,
    EVENT_CHECK_LAVA,
};

enum Creatures
{
    NPC_ROCK_ISLAND = 54496,
    NPC_THROWN_TOTEM = 54569,
};

enum GameObjects
{
    GO_PLATFORM = 209255,
    GO_DOOR_1 = 4001,
    GO_DOOR_2 = 4002,
};

class boss_echo_of_baine : public CreatureScript
{
    public:
        boss_echo_of_baine() : CreatureScript("boss_echo_of_baine") { }

        struct boss_echo_of_baineAI : public BossAI
        {
            boss_echo_of_baineAI(Creature* creature) : BossAI(creature, BOSS_ECHO_OF_BAINE)
            {
                introDone = false;
                instance = me->GetInstanceScript();
                pulverizeTarget = nullptr;
            }

            InstanceScript* instance;
            bool introDone;
            Unit* pulverizeTarget;
            EventMap events;

            void Reset() override
            {
                events.Reset();
                pulverizeTarget = nullptr;

                if (instance)
                {
                    instance->SetBossState(BOSS_ECHO_OF_BAINE, NOT_STARTED);
                    instance->HandleGameObject(GO_DOOR_1, true);
                    instance->HandleGameObject(GO_DOOR_2, true);
                }

                if (!me->HasAura(SPELL_BAINE_VIS))
                    DoCast(me, SPELL_BAINE_VIS);

                // Reset platform state
                if (GameObject* platform = me->FindNearestGameObject(GO_PLATFORM, 100.0f))
                    platform->SetGoState(GO_STATE_READY);
            }

            void EnterEvadeMode(EvadeReason why) override
            {
                me->GetMotionMaster()->MoveTargetedHome();
                Reset();

                me->SetHealth(me->GetMaxHealth());

                if (instance)
                {
                    instance->SetBossState(BOSS_ECHO_OF_BAINE, FAIL);
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
                    instance->SetBossState(BOSS_ECHO_OF_BAINE, DONE);

                    if (instance->GetData(DATA_FIRST_ENCOUNTER) == IN_PROGRESS)
                        instance->SetData(DATA_FIRST_ENCOUNTER, DONE);
                    else  
                        instance->SetData(DATA_SECOND_ENCOUNTER, DONE);

                    instance->HandleGameObject(GO_DOOR_1, true);
                    instance->HandleGameObject(GO_DOOR_2, true);

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
                    instance->SetBossState(BOSS_ECHO_OF_BAINE, IN_PROGRESS);
                    instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);

                    instance->HandleGameObject(GO_DOOR_1, false);
                    instance->HandleGameObject(GO_DOOR_2, false);
                }

                events.ScheduleEvent(EVENT_PULVERIZE, 40000);
                events.ScheduleEvent(EVENT_THROW_TOTEM, 25000);
                events.ScheduleEvent(EVENT_CHECK_LAVA, 1000);
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
                        case EVENT_PULVERIZE:
                            Talk(SAY_PULVERIZE);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            {
                                pulverizeTarget = target;
                                DoCast(target, SPELL_PULVERIZE_J);
                                DoCast(target, SPELL_PULV_DBM);
                            }
                            events.ScheduleEvent(EVENT_PULVERIZE, 40000);
                            events.ScheduleEvent(EVENT_PULVERIZE_DAMAGE, 3000);
                            break;

                        case EVENT_PULVERIZE_DAMAGE:
                            if (GameObject* platform = me->FindNearestGameObject(GO_PLATFORM, 20.0f))
                                platform->SetGoState(GO_STATE_ACTIVE);
                            DoCast(me, SPELL_PULVERIZE_D);
                            break;

                        case EVENT_THROW_TOTEM:
                            Talk(SAY_THROW_TOTEM);
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                DoCast(target, SPELL_THROW_TOTEM);
                            events.ScheduleEvent(EVENT_THROW_TOTEM, 25000);
                            break;

                        case EVENT_CHECK_LAVA:
                            // Check if boss is in water/lava
                            if (me->IsInWater() && !me->HasAura(SPELL_MOLTEN_AXE))
                                DoCast(me, SPELL_MOLTEN_AXE);

                            // Check players in lava
                            Map::PlayerList const& PlayerList = me->GetMap()->GetPlayers();
                            if (!PlayerList.isEmpty())
                            {
                                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                                {
                                    if (Player* player = i->GetSource())
                                    {
                                        if (player->IsInWater() && !player->HasAura(SPELL_MOLTEN_FIST))
                                            player->AddAura(SPELL_MOLTEN_FIST, player);
                                    }
                                }
                            }
                            events.ScheduleEvent(EVENT_CHECK_LAVA, 1000);
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new boss_echo_of_baineAI(creature);
        }
};

// Totem throw back mechanic
class npc_thrown_totem : public CreatureScript
{
    public:
        npc_thrown_totem() : CreatureScript("npc_thrown_totem") { }

        struct npc_thrown_totemAI : public ScriptedAI
        {
            npc_thrown_totemAI(Creature* creature) : ScriptedAI(creature)
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            }

            void OnSpellClick(Unit* clicker) override
            {
                if (Player* player = clicker->ToPlayer())
                {
                    if (Creature* baine = me->FindNearestCreature(BOSS_ECHO_OF_BAINE, 100.0f))
                    {
                        player->AddAura(SPELL_TB_TOTEM_A, player);
                        player->CastSpell(baine, SPELL_TB_TOTEM, true);
                        me->DespawnOrUnsummon();
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_thrown_totemAI(creature);
        }
};

// Nozdormu teleport spell script
class spell_nozdormu_teleport : public SpellScriptLoader
{
    public:
        spell_nozdormu_teleport() : SpellScriptLoader("spell_nozdormu_teleport") { }

        class spell_nozdormu_teleport_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_nozdormu_teleport_SpellScript);

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Creature* nozdormu = caster->FindNearestCreature(54751, 100.0f)) // Nozdormu NPC ID
                    {
                        nozdormu->AI()->Talk(SAY_NOZDORMU_TELEPORT);
                    }
                }
            }

            void Register() override
            {
                OnEffectHitTarget += SpellEffectFn(spell_nozdormu_teleport_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_TELEPORT_UNITS);
            }
        };

        SpellScript* GetSpellScript() const override
        {
            return new spell_nozdormu_teleport_SpellScript();
        }
};

void AddSC_boss_echo_of_baine()
{
    new boss_echo_of_baine();
    new npc_thrown_totem();
    new spell_nozdormu_teleport();
} 
