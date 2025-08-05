/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * Updated for Cataclysm (4.3.4) by [Your Name]
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Player.h"
#include "Unit.h"

enum DeathKnightSpells
{
    SPELL_DK_BLOOD_PLAGUE              = 55078,
    SPELL_DK_FROST_FEVER               = 55095,
    SPELL_DK_DEATH_AND_DECAY           = 43265,
    SPELL_DK_BLOOD_TAP                 = 45529,
    SPELL_DK_NECROTIC_STRIKE           = 73975,
    SPELL_DK_NECROTIC_STRIKE_DEBUFF    = 73975,
    SPELL_DK_OUTBREAK                  = 77575,
    SPELL_DK_DARK_SIMULACRUM           = 77606,
    SPELL_DK_HORN_OF_WINTER            = 57330,
    SPELL_DK_HORN_OF_WINTER_BUFF       = 57623,
    SPELL_DK_RUNIC_EMPOWERMENT         = 81229,
    SPELL_DK_PESTILENCE                = 50842,
    SPELL_DK_DEATH_COIL                = 47541,
    SPELL_DK_DEATH_STRIKE              = 49998,
    SPELL_DK_ICY_TOUCH                 = 45477,
    SPELL_DK_PLAGUE_STRIKE             = 45462,
    SPELL_DK_RUNE_TAP                  = 48982,
    SPELL_DK_ANTI_MAGIC_SHELL          = 48707,
    SPELL_DK_ARMY_OF_THE_DEAD          = 42650,
    SPELL_DK_EMPOWER_RUNE_WEAPON       = 47568,
    SPELL_DK_SCENT_OF_BLOOD            = 50422,
    SPELL_DK_IMPROVED_BLOOD_TAP        = 62905
};

// 55078 - Blood Plague
class spell_dk_blood_plague : public SpellScriptLoader
{
public:
    spell_dk_blood_plague() : SpellScriptLoader("spell_dk_blood_plague") { }

    class spell_dk_blood_plague_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_dk_blood_plague_AuraScript);

        void CalculateAmount(AuraEffect const* aurEff, int32& amount, bool& canBeRecalculated)
        {
            if (Unit* caster = GetCaster())
            {
                // Blood Plague deals 50% of AP as damage every 3 sec (7 ticks total)
                float attackPower = caster->GetTotalAttackPowerValue(BASE_ATTACK);
                amount = CalculatePct(attackPower, 50);
                
                // Improved Blood Plague (Talent) increases damage by 30%
                if (AuraEffect const* impBloodPlague = caster->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_DEATHKNIGHT, 2634, EFFECT_0))
                    AddPct(amount, impBloodPlague->GetAmount());
            }
            canBeRecalculated = true;
        }

        void Register() override
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dk_blood_plague_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_dk_blood_plague_AuraScript();
    }
};

// 55095 - Frost Fever
class spell_dk_frost_fever : public SpellScriptLoader
{
public:
    spell_dk_frost_fever() : SpellScriptLoader("spell_dk_frost_fever") { }

    class spell_dk_frost_fever_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_dk_frost_fever_AuraScript);

        void CalculateAmount(AuraEffect const* aurEff, int32& amount, bool& canBeRecalculated)
        {
            if (Unit* caster = GetCaster())
            {
                // Frost Fever deals 60% of AP as damage every 3 sec (7 ticks total)
                float attackPower = caster->GetTotalAttackPowerValue(BASE_ATTACK);
                amount = CalculatePct(attackPower, 60);
                
                // Glyph of Frost Fever increases damage by 20%
                if (caster->HasAura(58635)) // Glyph ID
                    AddPct(amount, 20);
            }
            canBeRecalculated = true;
        }

        void HandleEffectApply(AuraEffect const* aurEff, AuraEffectHandleModes mode)
        {
            Unit* target = GetTarget();
            if (!target)
                return;

            // Reduces attack speed by 14% (base effect)
            target->ApplyAttackTimePercentMod(BASE_ATTACK, -14, true);
        }

        void HandleEffectRemove(AuraEffect const* aurEff, AuraEffectHandleModes mode)
        {
            Unit* target = GetTarget();
            if (!target)
                return;

            // Restore attack speed when aura is removed
            target->ApplyAttackTimePercentMod(BASE_ATTACK, 14, true);
        }

        void Register() override
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dk_frost_fever_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            AfterEffectApply += AuraEffectApplyFn(spell_dk_frost_fever_AuraScript::HandleEffectApply, EFFECT_1, SPELL_AURA_MOD_ATTACKSPEED, AURA_EFFECT_HANDLE_REAL);
            AfterEffectRemove += AuraEffectRemoveFn(spell_dk_frost_fever_AuraScript::HandleEffectRemove, EFFECT_1, SPELL_AURA_MOD_ATTACKSPEED, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_dk_frost_fever_AuraScript();
    }
};

// 43265 - Death and Decay
class spell_dk_death_and_decay : public SpellScriptLoader
{
public:
    spell_dk_death_and_decay() : SpellScriptLoader("spell_dk_death_and_decay") { }

    class spell_dk_death_and_decay_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dk_death_and_decay_SpellScript);

        void HandleDamage(SpellEffIndex effIndex)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            // Base damage + 12% AP per tick (5 ticks total = 60% AP)
            int32 damage = GetSpellInfo()->Effects[effIndex].CalcValue(caster);
            float attackPower = caster->GetTotalAttackPowerValue(BASE_ATTACK);
            damage += CalculatePct(attackPower, 12);

            // T10 4P Bonus (Shadowfrost) increases damage by 20%
            if (AuraEffect const* aurEff = caster->GetAuraEffect(70656, EFFECT_0))
                AddPct(damage, aurEff->GetAmount());

            SetHitDamage(damage);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_dk_death_and_decay_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_dk_death_and_decay_SpellScript();
    }
};

// 45529 - Blood Tap
class spell_dk_blood_tap : public SpellScriptLoader
{
public:
    spell_dk_blood_tap() : SpellScriptLoader("spell_dk_blood_tap") { }

    class spell_dk_blood_tap_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dk_blood_tap_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_DK_BLOOD_TAP, SPELL_DK_IMPROVED_BLOOD_TAP });
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                return;

            Player* player = caster->ToPlayer();

            // Converts a Blood Rune into a Death Rune
            for (uint8 i = 0; i < MAX_RUNES; ++i)
            {
                if (player->GetRuneType(i) == RUNE_BLOOD)
                {
                    player->SetRuneType(i, RUNE_DEATH);
                    player->SetRuneTimer(i, 0); // Activate immediately
                    player->ResyncRunes();
                    
                    // Improved Blood Tap talent reduces cooldown
                    if (player->HasAura(SPELL_DK_IMPROVED_BLOOD_TAP))
                        player->GetSpellHistory()->ModifyCooldown(SPELL_DK_BLOOD_TAP, -player->GetAura(SPELL_DK_IMPROVED_BLOOD_TAP)->GetEffect(EFFECT_0)->GetAmount() * IN_MILLISECONDS);
                    
                    break;
                }
            }
        }

        void Register() override
        {
            OnEffectHit += SpellEffectFn(spell_dk_blood_tap_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_dk_blood_tap_SpellScript();
    }
};

// 73975 - Necrotic Strike
class spell_dk_necrotic_strike : public SpellScriptLoader
{
public:
    spell_dk_necrotic_strike() : SpellScriptLoader("spell_dk_necrotic_strike") { }

    class spell_dk_necrotic_strike_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dk_necrotic_strike_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_DK_NECROTIC_STRIKE_DEBUFF });
        }

        void HandleDamage(SpellEffIndex effIndex)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            if (!caster || !target)
                return;

            // Calculate damage: weapon damage + 50% AP
            int32 damage = GetSpellInfo()->Effects[effIndex].CalcValue(caster);
            float attackPower = caster->GetTotalAttackPowerValue(BASE_ATTACK);
            damage += CalculatePct(attackPower, 50);
            SetHitDamage(damage);

            // Apply absorption shield equal to 50% of damage dealt
            int32 absorbAmount = CalculatePct(damage, 50);
            caster->CastCustomSpell(SPELL_DK_NECROTIC_STRIKE_DEBUFF, SPELLVALUE_BASE_POINT0, absorbAmount, target, true);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_dk_necrotic_strike_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_dk_necrotic_strike_SpellScript();
    }
};

// 77575 - Outbreak
class spell_dk_outbreak : public SpellScriptLoader
{
public:
    spell_dk_outbreak() : SpellScriptLoader("spell_dk_outbreak") { }

    class spell_dk_outbreak_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dk_outbreak_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_DK_BLOOD_PLAGUE, SPELL_DK_FROST_FEVER });
        }

        void HandleOnHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            if (!caster || !target)
                return;

            // Applies both diseases with extended duration (21 sec base)
            caster->CastSpell(target, SPELL_DK_BLOOD_PLAGUE, true);
            caster->CastSpell(target, SPELL_DK_FROST_FEVER, true);

            // Pandemic talent increases duration by 3 sec
            if (AuraEffect const* aurEff = caster->GetAuraEffect(SPELL_AURA_ADD_FLAT_MODIFIER, SPELLFAMILY_DEATHKNIGHT, 2719, EFFECT_0))
            {
                if (Aura* bloodPlague = target->GetAura(SPELL_DK_BLOOD_PLAGUE, caster->GetGUID()))
                    bloodPlague->SetDuration(bloodPlague->GetDuration() + aurEff->GetAmount() * IN_MILLISECONDS);
                
                if (Aura* frostFever = target->GetAura(SPELL_DK_FROST_FEVER, caster->GetGUID()))
                    frostFever->SetDuration(frostFever->GetDuration() + aurEff->GetAmount() * IN_MILLISECONDS);
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_dk_outbreak_SpellScript::HandleOnHit, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_dk_outbreak_SpellScript();
    }
};

// 57330 - Horn of Winter
class spell_dk_horn_of_winter : public SpellScriptLoader
{
public:
    spell_dk_horn_of_winter() : SpellScriptLoader("spell_dk_horn_of_winter") { }

    class spell_dk_horn_of_winter_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dk_horn_of_winter_SpellScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_DK_HORN_OF_WINTER_BUFF });
        }

        void HandleOnHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            // Grants 10% Strength and Agility for 2 min to all party/raid members within 20 yards
            int32 str = CalculatePct(caster->GetStat(STAT_STRENGTH), 10);
            int32 agi = CalculatePct(caster->GetStat(STAT_AGILITY), 10);
            
            caster->CastCustomSpell(SPELL_DK_HORN_OF_WINTER_BUFF, SPELLVALUE_BASE_POINT0, str, SPELLVALUE_BASE_POINT1, agi, caster, true);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_dk_horn_of_winter_SpellScript::HandleOnHit, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_dk_horn_of_winter_SpellScript();
    }
};

// 81229 - Runic Empowerment
class spell_dk_runic_empowerment : public SpellScriptLoader
{
public:
    spell_dk_runic_empowerment() : SpellScriptLoader("spell_dk_runic_empowerment") { }

    class spell_dk_runic_empowerment_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_dk_runic_empowerment_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_DK_SCENT_OF_BLOOD });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            // Only proc on direct damage spells and abilities
            return eventInfo.GetDamageInfo() && eventInfo.GetDamageInfo()->GetSpellInfo() && 
                   (eventInfo.GetDamageInfo()->GetSpellInfo()->DmgClass == SPELL_DAMAGE_CLASS_MELEE || 
                    eventInfo.GetDamageInfo()->GetSpellInfo()->DmgClass == SPELL_DAMAGE_CLASS_RANGED ||
                    eventInfo.GetDamageInfo()->GetSpellInfo()->DmgClass == SPELL_DAMAGE_CLASS_MAGIC);
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            Unit* caster = GetCaster();
            if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                return;

            Player* player = caster->ToPlayer();

            // 45% chance to convert a rune into a death rune
            if (roll_chance_i(45))
            {
                // Scent of Blood talent increases chance by 15%
                if (AuraEffect const* scentOfBlood = player->GetAuraEffect(SPELL_DK_SCENT_OF_BLOOD, EFFECT_0))
                    if (roll_chance_i(scentOfBlood->GetAmount()))
                        return; // Additional roll for extra chance

                // Find first available non-death rune
                for (uint8 i = 0; i < MAX_RUNES; ++i)
                {
                    if (player->GetRuneType(i) != RUNE_DEATH && player->GetRuneTimer(i) > 0)
                    {
                        player->SetRuneType(i, RUNE_DEATH);
                        player->SetRuneTimer(i, 0); // Activate immediately
                        player->ResyncRunes();
                        break;
                    }
                }
            }
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_dk_runic_empowerment_AuraScript::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_dk_runic_empowerment_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_dk_runic_empowerment_AuraScript();
    }
};

// 77606 - Dark Simulacrum
class spell_dk_dark_simulacrum : public SpellScriptLoader
{
public:
    spell_dk_dark_simulacrum() : SpellScriptLoader("spell_dk_dark_simulacrum") { }

    class spell_dk_dark_simulacrum_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dk_dark_simulacrum_SpellScript);

        SpellCastResult CheckCast()
        {
            Unit* target = GetExplTargetUnit();
            if (!target || target->GetTypeId() != TYPEID_PLAYER)
                return SPELL_FAILED_BAD_TARGETS;

            // Can only copy spells with cast time or channeled spells
            if (!target->IsNonMeleeSpellCast(false, false, true))
                return SPELL_FAILED_BAD_TARGETS;

            return SPELL_CAST_OK;
        }

        void HandleScriptEffect(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            if (!caster || !target)
                return;

            // Copy the spell the target is currently casting
            if (Spell* spell = target->GetCurrentSpell(CURRENT_GENERIC_SPELL))
            {
                if (SpellInfo const* spellInfo = spell->GetSpellInfo())
                {
                    // Can't copy certain spell types
                    if (spellInfo->HasAttribute(SPELL_ATTR0_CANT_CANCEL) ||
                        spellInfo->IsPassive() ||
                        spellInfo->HasAttribute(SPELL_ATTR0_HIDDEN_CLIENTSIDE) ||
                        spellInfo->HasAttribute(SPELL_ATTR1_UNAFFECTED_BY_SCHOOL_IMMUNE))
                        return;

                    // Add the spell to DK's spellbook for 8 sec
                    caster->AddAura(77616, caster); // Visual
                    caster->AddAura(77622, caster); // Spell copy aura
                    caster->SetUInt32Value(PLAYER_FIELD_TRACK_CREATURES, spellInfo->Id);
                }
            }
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_dk_dark_simulacrum_SpellScript::CheckCast);
            OnEffectHitTarget += SpellEffectFn(spell_dk_dark_simulacrum_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_dk_dark_simulacrum_SpellScript();
    }
};

void AddSC_deathknight_spell_scripts()
{
    new spell_dk_blood_plague();
    new spell_dk_frost_fever();
    new spell_dk_death_and_decay();
    new spell_dk_blood_tap();
    new spell_dk_necrotic_strike();
    new spell_dk_outbreak();
    new spell_dk_horn_of_winter();
    new spell_dk_runic_empowerment();
    new spell_dk_dark_simulacrum();
}
