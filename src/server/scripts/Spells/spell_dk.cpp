/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Player.h"
#include "Unit.h"

enum DeathKnightSpells
{
    SPELL_DK_BLOOD_PLAGUE              = 55078,
    SPELL_DK_FROST_FEVER                = 55095,
    SPELL_DK_DEATH_AND_DECAY            = 43265,
    SPELL_DK_BLOOD_TAP                  = 45529,
    SPELL_DK_NECROTIC_STRIKE            = 73975,
    SPELL_DK_OUTBREAK                   = 77575,
    SPELL_DK_DARK_SIMULACRUM            = 77606,
    SPELL_DK_HORN_OF_WINTER             = 57330,
    SPELL_DK_RUNIC_EMPOWERMENT          = 81229,
    SPELL_DK_PESTILENCE                 = 50842,
    SPELL_DK_DEATH_COIL                 = 47541,
    SPELL_DK_DEATH_STRIKE               = 49998,
    SPELL_DK_ICY_TOUCH                  = 45477,
    SPELL_DK_PLAGUE_STRIKE              = 45462,
    SPELL_DK_RUNE_TAP                   = 48982,
    SPELL_DK_ANTI_MAGIC_SHELL           = 48707,
    SPELL_DK_ARMY_OF_THE_DEAD           = 42650,
    SPELL_DK_EMPOWER_RUNE_WEAPON        = 47568
};

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
                amount = CalculatePct(caster->GetTotalAttackPowerValue(BASE_ATTACK), 50); // 50% AP
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
                amount = CalculatePct(caster->GetTotalAttackPowerValue(BASE_ATTACK), 60); // 60% AP
        }

        void Register() override
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_dk_frost_fever_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_dk_frost_fever_AuraScript();
    }
};

class spell_dk_death_and_decay : public SpellScriptLoader
{
public:
    spell_dk_death_and_decay() : SpellScriptLoader("spell_dk_death_and_decay") { }

    class spell_dk_death_and_decay_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dk_death_and_decay_SpellScript);

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            int32 damage = GetHitDamage();
            if (Unit* caster = GetCaster())
            {
                float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
                damage += CalculatePct(ap, 12); // 12% AP per tick
            }
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

class spell_dk_blood_tap : public SpellScriptLoader
{
public:
    spell_dk_blood_tap() : SpellScriptLoader("spell_dk_blood_tap") { }

    class spell_dk_blood_tap_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dk_blood_tap_SpellScript);

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            // Converts a Blood Rune into Death Rune
            caster->ModifyPower(POWER_RUNES, RUNE_BLOOD, -1);
            caster->ModifyPower(POWER_RUNES, RUNE_DEATH, 1);
            caster->SetRuneTimer(RUNE_BLOOD, 0);
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

class spell_dk_necrotic_strike : public SpellScriptLoader
{
public:
    spell_dk_necrotic_strike() : SpellScriptLoader("spell_dk_necrotic_strike") { }

    class spell_dk_necrotic_strike_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dk_necrotic_strike_SpellScript);

        void HandleDamage(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            if (!caster || !target)
                return;

            int32 damage = GetHitDamage();
            damage += CalculatePct(caster->GetTotalAttackPowerValue(BASE_ATTACK), 50);

            int32 absorbAmount = CalculatePct(damage, 50);
            target->AddAura(SPELL_DK_NECROTIC_STRIKE, target);
            if (Aura* aura = target->GetAura(SPELL_DK_NECROTIC_STRIKE))
                aura->SetAmount(absorbAmount);
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

class spell_dk_outbreak : public SpellScriptLoader
{
public:
    spell_dk_outbreak() : SpellScriptLoader("spell_dk_outbreak") { }

    class spell_dk_outbreak_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dk_outbreak_SpellScript);

        void HandleOnHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetHitUnit();
            if (!caster || !target)
                return;

            // Applies both diseases
            caster->AddAura(SPELL_DK_BLOOD_PLAGUE, target);
            caster->AddAura(SPELL_DK_FROST_FEVER, target);
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

class spell_dk_horn_of_winter : public SpellScriptLoader
{
public:
    spell_dk_horn_of_winter() : SpellScriptLoader("spell_dk_horn_of_winter") { }

    class spell_dk_horn_of_winter_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_dk_horn_of_winter_SpellScript);

        void HandleOnHit(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            // Grants 10% Strength and Agility for 2 min
            int32 str = CalculatePct(caster->GetStat(STAT_STRENGTH), 10);
            int32 agi = CalculatePct(caster->GetStat(STAT_AGILITY), 10);
            
            caster->CastCustomSpell(caster, SPELL_DK_HORN_OF_WINTER, &str, &agi, NULL, true);
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

class spell_dk_runic_empowerment : public SpellScriptLoader
{
public:
    spell_dk_runic_empowerment() : SpellScriptLoader("spell_dk_runic_empowerment") { }

    class spell_dk_runic_empowerment_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_dk_runic_empowerment_AuraScript);

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            return eventInfo.GetDamageInfo() && eventInfo.GetDamageInfo()->GetDamage() > 0;
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            // 45% chance to convert a rune into a death rune
            if (roll_chance_i(45))
            {
                for (uint8 i = 0; i < MAX_RUNES; ++i)
                {
                    if (caster->GetRuneType(i) == RUNE_BLOOD || 
                        caster->GetRuneType(i) == RUNE_FROST || 
                        caster->GetRuneType(i) == RUNE_UNHOLY)
                    {
                        caster->SetRuneType(i, RUNE_DEATH);
                        caster->SetRuneTimer(i, 0);
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
}
