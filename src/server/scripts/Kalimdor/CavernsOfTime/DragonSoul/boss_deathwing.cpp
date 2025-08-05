/*
 * Copyright (C) 2008-2018 TrinityCore <https://www.trinitycore.org/>
 * Copyright (C) 2010-2015 Rising-Gods <https://www.rising-gods.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "dragon_soul.h"
#include "InstanceScript.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"

enum Spells
{
    // Phase 1: The Assault of the Destroyer
    SPELL_ASSAULT_ASPECTS          = 107018, // Initial cast when engaging
    SPELL_CATACLYSM                = 106523, // Main raid damage ability
    SPELL_ELEMENTIUM_BOLT          = 105651, // Targets random players
    SPELL_ELEMENTIUM_BLAST         = 105723, // Damage from Elementium Bolt
    SPELL_HEMORRHAGE               = 105863, // Applied to platform when tentacle dies
    SPELL_IMPALE                   = 106400, // Tentacle spike attack
    
    // Phase 2: The Maelstrom
    SPELL_TAIL_LASH                = 106385, // Tail attack
    SPELL_SHRAPNEL                 = 106794, // When armor plate is destroyed
    SPELL_BURNING_BLOOD            = 105401, // Stacking debuff on tank
    SPELL_DEGRADATION              = 106005, // When corruption tentacle spawns
    
    // Phase 3: The Hour of Twilight
    SPELL_TWILIGHT_ERUPTION        = 106388, // Massive damage after Phase 2
    SPELL_HOUR_OF_TWILIGHT         = 106371, // Main mechanic in phase 3
    SPELL_TIME_ZONE                = 105799, // Player-created safe zones
    
    // Misc
    SPELL_AGONIZING_PAIN           = 106548, // When players fail mechanics
    SPELL_SPAWN_BLOOD              = 105554, // Spawns corrupted blood
    SPELL_CORRUPTED_BLOOD          = 106834, // Corrupted blood debuff
};

enum Events
{
    EVENT_CATACLYSM = 1,
    EVENT_ELEMENTIUM_BOLT,
    EVENT_CHECK_PLATFORM,
    EVENT_PHASE_TRANSITION,
    EVENT_TAIL_LASH,
    EVENT_SHRAPNEL,
    EVENT_BURNING_BLOOD,
    EVENT_HOUR_OF_TWILIGHT,
    EVENT_TWILIGHT_ERUPTION,
};

enum Phases
{
    PHASE_ASSAULT = 1,
    PHASE_MAELSTROM,
    PHASE_TWILIGHT,
    PHASE_DEATH
};

enum Actions
{
    ACTION_PLATFORM_DESTROYED = 1,
    ACTION_ARMOR_PLATE_DESTROYED,
};

Position const DeathwingPositions[4] =
{
    {-1755.31f, -2367.19f, 340.94f, 0.0f}, // Assault Phase Position
    {-1786.69f, -2393.23f, 45.31f,  0.0f}, // Maelstrom Phase Position
    {-1851.48f, -2389.55f, 340.97f, 0.0f}, // Twilight Phase Position
    {-1872.83f, -2425.30f, 45.31f,  0.0f}, // Death Position
};

class boss_deathwing : public CreatureScript
{
public:
    boss_deathwing() : CreatureScript("boss_deathwing") { }

    struct boss_deathwingAI : public BossAI
    {
        boss_deathwingAI(Creature* creature) : BossAI(creature, BOSS_DEATHWING)
        {
            Initialize();
        }

        void Initialize()
        {
            phase = PHASE_ASSAULT;
            platformsDestroyed = 0;
            armorPlatesDestroyed = 0;
        }

        void Reset() override
        {
            _Reset();
            Initialize();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            DoCast(SPELL_ASSAULT_ASPECTS);
            events.SetPhase(PHASE_ASSAULT);
            events.ScheduleEvent(EVENT_CATACLYSM, 30000, 0, PHASE_ASSAULT);
            events.ScheduleEvent(EVENT_ELEMENTIUM_BOLT, 15000, 0, PHASE_ASSAULT);
            events.ScheduleEvent(EVENT_CHECK_PLATFORM, 5000);
        }

        void DoAction(int32 action) override
        {
            switch (action)
            {
                case ACTION_PLATFORM_DESTROYED:
                    platformsDestroyed++;
                    if (platformsDestroyed >= 4)
                    {
                        events.SetPhase(PHASE_MAELSTROM);
                        events.ScheduleEvent(EVENT_PHASE_TRANSITION, 1000);
                    }
                    break;
                case ACTION_ARMOR_PLATE_DESTROYED:
                    armorPlatesDestroyed++;
                    if (armorPlatesDestroyed >= 4)
                    {
                        events.SetPhase(PHASE_TWILIGHT);
                        events.ScheduleEvent(EVENT_PHASE_TRANSITION, 1000);
                    }
                    break;
            }
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            // Todo: handle loot and cinematic
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CATACLYSM:
                        DoCastAOE(SPELL_CATACLYSM);
                        events.ScheduleEvent(EVENT_CATACLYSM, 30000, 0, events.GetPhaseMask());
                        break;
                    case EVENT_ELEMENTIUM_BOLT:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            DoCast(target, SPELL_ELEMENTIUM_BOLT);
                        events.ScheduleEvent(EVENT_ELEMENTIUM_BOLT, 15000, 0, events.GetPhaseMask());
                        break;
                    case EVENT_CHECK_PLATFORM:
                        // Check platform status
                        events.ScheduleEvent(EVENT_CHECK_PLATFORM, 5000);
                        break;
                    case EVENT_PHASE_TRANSITION:
                        HandlePhaseTransition();
                        break;
                    case EVENT_TAIL_LASH:
                        DoCastVictim(SPELL_TAIL_LASH);
                        events.ScheduleEvent(EVENT_TAIL_LASH, 20000, 0, PHASE_MAELSTROM);
                        break;
                    case EVENT_SHRAPNEL:
                        DoCastAOE(SPELL_SHRAPNEL);
                        break;
                    case EVENT_BURNING_BLOOD:
                        if (Unit* target = me->GetVictim())
                            DoCast(target, SPELL_BURNING_BLOOD);
                        events.ScheduleEvent(EVENT_BURNING_BLOOD, 15000, 0, PHASE_MAELSTROM);
                        break;
                    case EVENT_HOUR_OF_TWILIGHT:
                        {
                        std::list<Unit*> targets;
                        SelectTargetList(targets, 5, SELECT_TARGET_RANDOM, 500.0f, true);
                        for (Unit* target : targets)
                            DoCast(target, SPELL_HOUR_OF_TWILIGHT);
                        events.ScheduleEvent(EVENT_HOUR_OF_TWILIGHT, 45000, 0, PHASE_TWILIGHT);
                        break;
                    }
                    case EVENT_TWILIGHT_ERUPTION:
                        DoCastAOE(SPELL_TWILIGHT_ERUPTION);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void HandlePhaseTransition()
        {
            switch (events.GetPhaseMask())
            {
                case PHASE_ASSAULT:
                    // Transition to Maelstrom phase
                    me->NearTeleportTo(DeathwingPositions[1]);
                    events.ScheduleEvent(EVENT_TAIL_LASH, 10000, 0, PHASE_MAELSTROM);
                    events.ScheduleEvent(EVENT_BURNING_BLOOD, 5000, 0, PHASE_MAELSTROM);
                    break;
                case PHASE_MAELSTROM:
                    // Transition to Twilight phase
                    DoCastAOE(SPELL_TWILIGHT_ERUPTION);
                    me->NearTeleportTo(DeathwingPositions[2]);
                    events.ScheduleEvent(EVENT_HOUR_OF_TWILIGHT, 30000, 0, PHASE_TWILIGHT);
                    break;
                case PHASE_TWILIGHT:
                    // Transition to Death phase
                    me->NearTeleportTo(DeathwingPositions[3]);
                    break;
            }
        }

    private:
        uint8 phase;
        uint8 platformsDestroyed;
        uint8 armorPlatesDestroyed;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new boss_deathwingAI(creature);
    }
};

// Elementium Bolt target selector
class spell_elementium_bolt_targeting : public SpellScriptLoader
{
public:
    spell_elementium_bolt_targeting() : SpellScriptLoader("spell_elementium_bolt_targeting") { }

    class spell_elementium_bolt_targeting_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_elementium_bolt_targeting_SpellScript);

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            if (targets.empty())
                return;

            // Select random target
            WorldObject* target = Trinity::Containers::SelectRandomContainerElement(targets);
            targets.clear();
            targets.push_back(target);
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_elementium_bolt_targeting_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_elementium_bolt_targeting_SpellScript();
    }
};

void AddSC_boss_deathwing()
{
    new boss_deathwing();
    new spell_elementium_bolt_targeting();
}
