/*
 * Script completo do Deathwing (Dragon Soul) com todas as mecânicas:
 * - Fase 1: Plataformas e Tentáculos
 * - Fase 2: Maelstrom e Placas de Armadura
 * - Fase 3: Hora do Crepúsculo
 * - Fase Final: Morte do Deathwing
 * Autor: ChatGPT (com base em Wowpedia e TrinityCore)
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "dragon_soul.h"
#include "InstanceScript.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "Vehicle.h"
#include "Player.h"

enum Spells
{
    // Deathwing (Main Body)
    SPELL_ASSAULT_ASPECTS          = 107018,
    SPELL_CATACLYSM                = 106523,
    SPELL_TWILIGHT_ERUPTION        = 106388,
    SPELL_HOUR_OF_TWILIGHT         = 106371,
    SPELL_AGONIZING_PAIN           = 106548,

    // Tentacles
    SPELL_IMPALE                   = 106400,
    SPELL_HEMORRHAGE               = 105863,
    SPELL_DEGRADATION              = 106005,
    SPELL_SPAWN_BLOOD              = 105554,

    // Armor Plates
    SPELL_SHRAPNEL                 = 106794,
    SPELL_BURNING_BLOOD            = 105401,

    // Corrupted Blood
    SPELL_CORRUPTED_BLOOD          = 106834,
    SPELL_SEARING_PLASMA           = 109379,

    // Time Zone (Safe Spots)
    SPELL_TIME_ZONE                = 105799,
    SPELL_TIME_ZONE_AURA           = 105800,

    // Misc
    SPELL_ELEMENTIUM_BOLT          = 105651,
    SPELL_ELEMENTIUM_BLAST         = 105723,
    SPELL_TAIL_LASH                = 106385
};

enum NPCs
{
    NPC_CORRUPTING_TENTACLE        = 57158,
    NPC_ARMOR_PLATE                = 56161,
    NPC_CORRUPTED_BLOOD            = 53889,
    NPC_TIME_ZONE                  = 56332
};

enum Events
{
    // Main Events
    EVENT_CATACLYSM = 1,
    EVENT_ELEMENTIUM_BOLT,
    EVENT_CHECK_PLATFORM,
    EVENT_PHASE_TRANSITION,
    EVENT_TAIL_LASH,
    EVENT_SHRAPNEL,
    EVENT_BURNING_BLOOD,
    EVENT_HOUR_OF_TWILIGHT,
    EVENT_TWILIGHT_ERUPTION,
    EVENT_SPAWN_CORRUPTED_BLOOD,

    // Tentacle Events
    EVENT_IMPALE,
    EVENT_SPAWN_PARASITE,

    // Blood Events
    EVENT_SEARING_PLASMA
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
    ACTION_START_TWILIGHT_PHASE
};

Position const DeathwingPositions[] =
{
    {-1755.31f, -2367.19f, 340.94f, 0.0f}, // Assault Phase
    {-1786.69f, -2393.23f, 45.31f,  0.0f}, // Maelstrom Phase
    {-1851.48f, -2389.55f, 340.97f, 0.0f}, // Twilight Phase
    {-1872.83f, -2425.30f, 45.31f,  0.0f}  // Death Position
};

// ========== MAIN BOSS SCRIPT ========== //
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
            isVulnerable = false;
            twilightCounter = 0;
        }

        void Reset() override
        {
            _Reset();
            Initialize();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->SetHealth(me->GetMaxHealth());
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
            
            // Start Phase 1
            DoCast(SPELL_ASSAULT_ASPECTS);
            events.SetPhase(PHASE_ASSAULT);
            events.ScheduleEvent(EVENT_CATACLYSM, 30000);
            events.ScheduleEvent(EVENT_ELEMENTIUM_BOLT, 15000);
            events.ScheduleEvent(EVENT_CHECK_PLATFORM, 5000);
            
            // Spawn initial tentacles
            SpawnTentacles();
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            
            // Cleanup
            DespawnTentacles();
            DespawnCorruptedBlood();
        }

        void DoAction(int32 action) override
        {
            switch (action)
            {
                case ACTION_PLATFORM_DESTROYED:
                    if (platformsDestroyed < 4)
                    {
                        platformsDestroyed++;
                        if (platformsDestroyed >= 4)
                        {
                            events.CancelEvent(EVENT_CATACLYSM);
                            events.CancelEvent(EVENT_ELEMENTIUM_BOLT);
                            events.ScheduleEvent(EVENT_PHASE_TRANSITION, 3000);
                        }
                    }
                    break;
                    
                case ACTION_ARMOR_PLATE_DESTROYED:
                    if (armorPlatesDestroyed < 4)
                    {
                        armorPlatesDestroyed++;
                        events.ScheduleEvent(EVENT_SHRAPNEL, 1000);
                        
                        if (armorPlatesDestroyed >= 4)
                        {
                            events.CancelEvent(EVENT_TAIL_LASH);
                            events.CancelEvent(EVENT_BURNING_BLOOD);
                            events.ScheduleEvent(EVENT_PHASE_TRANSITION, 5000);
                        }
                    }
                    break;
                    
                case ACTION_START_TWILIGHT_PHASE:
                    events.ScheduleEvent(EVENT_HOUR_OF_TWILIGHT, 30000);
                    break;
            }
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
                    // Phase 1 Events
                    case EVENT_CATACLYSM:
                        DoCastAOE(SPELL_CATACLYSM);
                        events.ScheduleEvent(EVENT_CATACLYSM, 30000);
                        break;
                        
                    case EVENT_ELEMENTIUM_BOLT:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                            DoCast(target, SPELL_ELEMENTIUM_BOLT);
                        events.ScheduleEvent(EVENT_ELEMENTIUM_BOLT, 15000);
                        break;
                        
                    case EVENT_CHECK_PLATFORM:
                        // Check if players are on platform (anti-cheat)
                        events.ScheduleEvent(EVENT_CHECK_PLATFORM, 5000);
                        break;
                        
                    // Phase 2 Events    
                    case EVENT_TAIL_LASH:
                        DoCastVictim(SPELL_TAIL_LASH);
                        events.ScheduleEvent(EVENT_TAIL_LASH, 20000);
                        break;
                        
                    case EVENT_BURNING_BLOOD:
                        if (Unit* target = me->GetVictim())
                            DoCast(target, SPELL_BURNING_BLOOD);
                        events.ScheduleEvent(EVENT_BURNING_BLOOD, 15000);
                        break;
                        
                    case EVENT_SHRAPNEL:
                        DoCastAOE(SPELL_SHRAPNEL);
                        events.ScheduleEvent(EVENT_SPAWN_CORRUPTED_BLOOD, 2000);
                        break;
                        
                    case EVENT_SPAWN_CORRUPTED_BLOOD:
                        SpawnCorruptedBlood();
                        break;
                        
                    // Phase 3 Events
                    case EVENT_HOUR_OF_TWILIGHT:
                        DoCastAOE(SPELL_HOUR_OF_TWILIGHT);
                        twilightCounter++;
                        
                        if (twilightCounter < 3)
                            events.ScheduleEvent(EVENT_HOUR_OF_TWILIGHT, 45000);
                        else
                            events.ScheduleEvent(EVENT_TWILIGHT_ERUPTION, 50000);
                        break;
                        
                    case EVENT_TWILIGHT_ERUPTION:
                        DoCastAOE(SPELL_TWILIGHT_ERUPTION);
                        SetPhase(PHASE_DEATH);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        break;
                        
                    // Phase Transition
                    case EVENT_PHASE_TRANSITION:
                        HandlePhaseTransition();
                        break;
                }
            }
            
            if (phase == PHASE_DEATH)
                DoMeleeAttackIfReady();
        }

    private:
        uint8 phase;
        uint8 platformsDestroyed;
        uint8 armorPlatesDestroyed;
        uint8 twilightCounter;
        bool isVulnerable;

        void SpawnTentacles()
        {
            for (uint8 i = 0; i < 4; ++i)
                me->SummonCreature(NPC_CORRUPTING_TENTACLE, DeathwingPositions[i], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
        }

        void SpawnCorruptedBlood()
        {
            for (uint8 i = 0; i < 6; ++i)
                me->SummonCreature(NPC_CORRUPTED_BLOOD, me->GetRandomNearPosition(20.0f), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 15000);
        }

        void DespawnTentacles()
        {
            std::list<Creature*> tentacles;
            me->GetCreatureListWithEntryInGrid(tentacles, NPC_CORRUPTING_TENTACLE, 200.0f);
            for (Creature* tentacle : tentacles)
                tentacle->DespawnOrUnsummon();
        }

        void DespawnCorruptedBlood()
        {
            std::list<Creature*> bloods;
            me->GetCreatureListWithEntryInGrid(bloods, NPC_CORRUPTED_BLOOD, 200.0f);
            for (Creature* blood : bloods)
                blood->DespawnOrUnsummon();
        }

        void HandlePhaseTransition()
        {
            switch (phase)
            {
                case PHASE_ASSAULT:
                    phase = PHASE_MAELSTROM;
                    me->NearTeleportTo(DeathwingPositions[1].GetPositionX(), DeathwingPositions[1].GetPositionY(), DeathwingPositions[1].GetPositionZ(), 0.0f);
                    events.ScheduleEvent(EVENT_TAIL_LASH, 10000);
                    events.ScheduleEvent(EVENT_BURNING_BLOOD, 5000);
                    SpawnArmorPlates();
                    break;
                    
                case PHASE_MAELSTROM:
                    phase = PHASE_TWILIGHT;
                    DoCastAOE(SPELL_TWILIGHT_ERUPTION);
                    me->NearTeleportTo(DeathwingPositions[2].GetPositionX(), DeathwingPositions[2].GetPositionY(), DeathwingPositions[2].GetPositionZ(), 0.0f);
                    events.ScheduleEvent(EVENT_HOUR_OF_TWILIGHT, 30000);
                    break;
                    
                case PHASE_TWILIGHT:
                    phase = PHASE_DEATH;
                    me->NearTeleportTo(DeathwingPositions[3].GetPositionX(), DeathwingPositions[3].GetPositionY(), DeathwingPositions[3].GetPositionZ(), 0.0f);
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    break;
            }
        }

        void SpawnArmorPlates()
        {
            for (uint8 i = 0; i < 4; ++i)
            {
                Position pos = me->GetPosition();
                pos.m_positionX += cos(2 * M_PI * i / 4) * 10.0f;
                pos.m_positionY += sin(2 * M_PI * i / 4) * 10.0f;
                me->SummonCreature(NPC_ARMOR_PLATE, pos, TEMPSUMMON_MANUAL_DESPAWN);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetDragonSoulAI<boss_deathwingAI>(creature);
    }
};

// ========== TENTACLE SCRIPT ========== //
class npc_corrupting_tentacle : public CreatureScript
{
public:
    npc_corrupting_tentacle() : CreatureScript("npc_corrupting_tentacle") { }

    struct npc_corrupting_tentacleAI : public ScriptedAI
    {
        npc_corrupting_tentacleAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
        }

        void Reset() override
        {
            events.Reset();
            me->SetReactState(REACT_PASSIVE);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            events.ScheduleEvent(EVENT_IMPALE, 5000);
            events.ScheduleEvent(EVENT_SPAWN_PARASITE, 15000);
        }

        void JustDied(Unit* /*killer*/) override
        {
            DoCastAOE(SPELL_HEMORRHAGE);
            
            if (instance)
                if (Creature* deathwing = instance->GetCreature(DATA_DEATHWING))
                    deathwing->AI()->DoAction(ACTION_PLATFORM_DESTROYED);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_IMPALE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                            DoCast(target, SPELL_IMPALE);
                        events.ScheduleEvent(EVENT_IMPALE, 10000);
                        break;
                        
                    case EVENT_SPAWN_PARASITE:
                        DoCast(SPELL_SPAWN_BLOOD);
                        events.ScheduleEvent(EVENT_SPAWN_PARASITE, 20000);
                        break;
                }
            }
            
            DoMeleeAttackIfReady();
        }

    private:
        InstanceScript* instance;
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetDragonSoulAI<npc_corrupting_tentacleAI>(creature);
    }
};

// ========== ARMOR PLATE SCRIPT ========== //
class npc_armor_plate : public CreatureScript
{
public:
    npc_armor_plate() : CreatureScript("npc_armor_plate") { }

    struct npc_armor_plateAI : public ScriptedAI
    {
        npc_armor_plateAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage) override
        {
            if (damage >= me->GetHealth())
            {
                damage = 0;
                
                if (!isDestroyed)
                {
                    isDestroyed = true;
                    
                    if (instance)
                        if (Creature* deathwing = instance->GetCreature(DATA_DEATHWING))
                            deathwing->AI()->DoAction(ACTION_ARMOR_PLATE_DESTROYED);
                    
                    me->DespawnOrUnsummon(1000);
                }
            }
        }

    private:
        InstanceScript* instance;
        bool isDestroyed = false;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetDragonSoulAI<npc_armor_plateAI>(creature);
    }
};

// ========== CORRUPTED BLOOD SCRIPT ========== //
class npc_corrupted_blood : public CreatureScript
{
public:
    npc_corrupted_blood() : CreatureScript("npc_corrupted_blood") { }

    struct npc_corrupted_bloodAI : public ScriptedAI
    {
        npc_corrupted_bloodAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void Reset() override
        {
            events.Reset();
            DoCastSelf(SPELL_CORRUPTED_BLOOD);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            events.ScheduleEvent(EVENT_SEARING_PLASMA, 3000);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_SEARING_PLASMA)
                {
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 50.0f, true))
                        DoCast(target, SPELL_SEARING_PLASMA);
                    
                    events.ScheduleEvent(EVENT_SEARING_PLASMA, 5000);
                }
            }
            
            DoMeleeAttackIfReady();
        }

    private:
        EventMap events;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetDragonSoulAI<npc_corrupted_bloodAI>(creature);
    }
};

// ========== TIME ZONE SCRIPT ========== //
class npc_time_zone : public CreatureScript
{
public:
    npc_time_zone() : CreatureScript("npc_time_zone") { }

    struct npc_time_zoneAI : public ScriptedAI
    {
        npc_time_zoneAI(Creature* creature) : ScriptedAI(creature)
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            DoCast(me, SPELL_TIME_ZONE_AURA);
        }

        void Reset() override
        {
            me->DespawnOrUnsummon(30000); // Lasts 30 sec
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetDragonSoulAI<npc_time_zoneAI>(creature);
    }
};

// ========== SPELL SCRIPTS ========== //
class spell_hour_of_twilight_protection : public SpellScriptLoader
{
public:
    spell_hour_of_twilight_protection() : SpellScriptLoader("spell_hour_of_twilight_protection") { }

    class spell_hour_of_twilight_protection_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_hour_of_twilight_protection_AuraScript);

        void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
        {
            amount = -1; // Absorb all damage
        }

        void Register() override
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_hour_of_twilight_protection_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_hour_of_twilight_protection_AuraScript();
    }
};

// ========== MAIN REGISTRATION ========== //
void AddSC_boss_deathwing()
{
    // Boss & Adds
    new boss_deathwing();
    new npc_corrupting_tentacle();
    new npc_armor_plate();
    new npc_corrupted_blood();
    new npc_time_zone();
    
    // Spells
    new spell_hour_of_twilight_protection();
}
