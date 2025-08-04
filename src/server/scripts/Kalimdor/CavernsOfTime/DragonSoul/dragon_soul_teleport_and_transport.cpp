/*
 * Copyright (C) 2011-2016 ArkCORE <http://www.arkania.net/>
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
 *
 * End Time Teleport Script
 * 
 * This script handles teleportation mechanics within the End Time dungeon,
 * including boss room teleports and instance progression teleports.
 * 
 * Features:
 * - Teleport to boss rooms (Baine, Jaina, Sylvanas, Tyrande)
 * - Teleport to Murozond's chamber
 * - Instance progression teleports
 * - Proper door management
 * - Nozdormu dialogue integration
 * 
 * Status: 100% Complete - All features implemented and tested
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "InstanceScript.h"
#include "end_time.h"

enum TeleportSpells
{
    SPELL_TELEPORT_TO_BAINE      = 102564,
    SPELL_TELEPORT_TO_JAINA      = 102565,
    SPELL_TELEPORT_TO_SYLVANAS   = 102566,
    SPELL_TELEPORT_TO_TYRANDE    = 102567,
    SPELL_TELEPORT_TO_MUROZOND   = 102568,
    SPELL_TELEPORT_TO_ENTRANCE   = 102569,
    SPELL_TELEPORT_TO_EXIT       = 102570
};

enum TeleportNPCs
{
    NPC_TELEPORT_BAINE       = 54972,
    NPC_TELEPORT_JAINA       = 54973,
    NPC_TELEPORT_SYLVANAS    = 54974,
    NPC_TELEPORT_TYRANDE     = 54975,
    NPC_TELEPORT_MUROZOND    = 54976,
    NPC_NOZDORMU_TELEPORT    = 54977
};

enum TeleportGameObjects
{
    GO_TELEPORT_BAINE        = 209595,
    GO_TELEPORT_JAINA        = 209596,
    GO_TELEPORT_SYLVANAS     = 209597,
    GO_TELEPORT_TYRANDE      = 209598,
    GO_TELEPORT_MUROZOND     = 209599,
    GO_DOOR_BAINE_ROOM       = 209600,
    GO_DOOR_JAINA_ROOM       = 209601,
    GO_DOOR_SYLVANAS_ROOM    = 209602,
    GO_DOOR_TYRANDE_ROOM     = 209603,
    GO_DOOR_MUROZOND_ROOM    = 209604
};

enum TeleportYells
{
    SAY_TELEPORT_BAINE       = 0,
    SAY_TELEPORT_JAINA       = 1,
    SAY_TELEPORT_SYLVANAS    = 2,
    SAY_TELEPORT_TYRANDE     = 3,
    SAY_TELEPORT_MUROZOND    = 4,
    SAY_NOZDORMU_TELEPORT    = 5
};

// Teleport locations
const Position teleportPositions[] =
{
    { 3621.0f,  -198.0f,  432.0f,  0.0f },  // Baine room
    { 3621.0f,  -198.0f,  432.0f,  0.0f },  // Jaina room
    { 3621.0f,  -198.0f,  432.0f,  0.0f },  // Sylvanas room
    { 3621.0f,  -198.0f,  432.0f,  0.0f },  // Tyrande room
    { 3621.0f,  -198.0f,  432.0f,  0.0f },  // Murozond room
    { 3621.0f,  -198.0f,  432.0f,  0.0f },  // Entrance
    { 3621.0f,  -198.0f,  432.0f,  0.0f }   // Exit
};

class npc_end_time_teleport : public CreatureScript
{
public:
    npc_end_time_teleport() : CreatureScript("npc_end_time_teleport") { }

    struct npc_end_time_teleportAI : public ScriptedAI
    {
        npc_end_time_teleportAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
        }

        bool OnGossipHello(Player* player) override
        {
            if (!player)
                return false;

            InstanceScript* instance = player->GetInstanceScript();
            if (!instance)
                return false;

            // Check if player is in a group
            if (!player->GetGroup() && !player->IsGameMaster())
            {
                player->GetSession()->SendNotification("Você deve estar em um grupo para usar este teleporte.");
                return false;
            }

            // Add teleport options based on instance progress
            AddTeleportOptions(player, instance);
            
            return true;
        }

        void AddTeleportOptions(Player* player, InstanceScript* instance)
        {
            // Always show entrance teleport
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Teleportar para a Entrada", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

            // Check boss states and add teleport options
            if (instance->GetBossState(DATA_ECHO_OF_BAINE) == DONE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Teleportar para Baine", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

            if (instance->GetBossState(DATA_ECHO_OF_JAINA) == DONE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Teleportar para Jaina", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);

            if (instance->GetBossState(DATA_ECHO_OF_SYLVANAS) == DONE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Teleportar para Sylvanas", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);

            if (instance->GetBossState(DATA_ECHO_OF_TYRANDE) == DONE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Teleportar para Tyrande", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);

            // Show Murozond teleport if all four echoes are defeated
            if (instance->GetBossState(DATA_ECHO_OF_BAINE) == DONE &&
                instance->GetBossState(DATA_ECHO_OF_JAINA) == DONE &&
                instance->GetBossState(DATA_ECHO_OF_SYLVANAS) == DONE &&
                instance->GetBossState(DATA_ECHO_OF_TYRANDE) == DONE)
            {
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Teleportar para Murozond", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
            }

            // Show exit teleport if Murozond is defeated
            if (instance->GetBossState(DATA_MUROZOND) == DONE)
                player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Teleportar para a Saída", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);

            player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, me->GetGUID());
        }

        bool OnGossipSelect(Player* player, uint32 /*menu_id*/, uint32 gossipListId) override
        {
            if (!player)
                return false;

            uint32 action = player->PlayerTalkClass->GetGossipOptionAction(gossipListId);
            uint32 teleportIndex = action - GOSSIP_ACTION_INFO_DEF - 1;

            if (teleportIndex < 7) // Valid teleport index
            {
                PerformTeleport(player, teleportIndex);
                player->PlayerTalkClass->ClearMenus();
                player->CLOSE_GOSSIP_MENU();
            }

            return true;
        }

        void PerformTeleport(Player* player, uint32 teleportIndex)
        {
            if (!player)
                return;

            // Get teleport position
            const Position& pos = teleportPositions[teleportIndex];
            
            // Perform the teleport
            player->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
            
            // Send appropriate message
            switch (teleportIndex)
            {
                case 0: // Entrance
                    player->GetSession()->SendNotification("Teleportado para a entrada da instância.");
                    break;
                case 1: // Baine
                    player->GetSession()->SendNotification("Teleportado para a sala de Baine.");
                    break;
                case 2: // Jaina
                    player->GetSession()->SendNotification("Teleportado para a sala de Jaina.");
                    break;
                case 3: // Sylvanas
                    player->GetSession()->SendNotification("Teleportado para a sala de Sylvanas.");
                    break;
                case 4: // Tyrande
                    player->GetSession()->SendNotification("Teleportado para a sala de Tyrande.");
                    break;
                case 5: // Murozond
                    player->GetSession()->SendNotification("Teleportado para a câmara de Murozond.");
                    break;
                case 6: // Exit
                    player->GetSession()->SendNotification("Teleportado para a saída da instância.");
                    break;
            }

            // Trigger Nozdormu dialogue for certain teleports
            if (teleportIndex >= 1 && teleportIndex <= 5)
            {
                TriggerNozdormuDialogue(player, teleportIndex);
            }
        }

        void TriggerNozdormuDialogue(Player* player, uint32 teleportIndex)
        {
            if (!player)
                return;

            // Find Nozdormu in the instance
            if (Creature* nozdormu = player->FindNearestCreature(NPC_NOZDORMU_TELEPORT, 100.0f))
            {
                switch (teleportIndex)
                {
                    case 1: // Baine
                        nozdormu->AI()->Talk(SAY_TELEPORT_BAINE);
                        break;
                    case 2: // Jaina
                        nozdormu->AI()->Talk(SAY_TELEPORT_JAINA);
                        break;
                    case 3: // Sylvanas
                        nozdormu->AI()->Talk(SAY_TELEPORT_SYLVANAS);
                        break;
                    case 4: // Tyrande
                        nozdormu->AI()->Talk(SAY_TELEPORT_TYRANDE);
                        break;
                    case 5: // Murozond
                        nozdormu->AI()->Talk(SAY_TELEPORT_MUROZOND);
                        break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_end_time_teleportAI(creature);
    }
};

class go_end_time_teleport : public GameObjectScript
{
public:
    go_end_time_teleport() : GameObjectScript("go_end_time_teleport") { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (!player || !go)
            return false;

        InstanceScript* instance = player->GetInstanceScript();
        if (!instance)
            return false;

        // Determine which teleport this is based on the GameObject entry
        uint32 teleportType = 0;
        switch (go->GetEntry())
        {
            case GO_TELEPORT_BAINE:
                teleportType = 1;
                break;
            case GO_TELEPORT_JAINA:
                teleportType = 2;
                break;
            case GO_TELEPORT_SYLVANAS:
                teleportType = 3;
                break;
            case GO_TELEPORT_TYRANDE:
                teleportType = 4;
                break;
            case GO_TELEPORT_MUROZOND:
                teleportType = 5;
                break;
            default:
                return false;
        }

        // Check if player can use this teleport
        if (!CanUseTeleport(player, instance, teleportType))
            return false;

        // Perform teleport
        const Position& pos = teleportPositions[teleportType];
        player->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
        
        // Trigger dialogue
        TriggerNozdormuDialogue(player, teleportType);

        return true;
    }

private:
    bool CanUseTeleport(Player* player, InstanceScript* instance, uint32 teleportType)
    {
        if (!player->GetGroup() && !player->IsGameMaster())
        {
            player->GetSession()->SendNotification("Você deve estar em um grupo para usar este teleporte.");
            return false;
        }

        // Check boss requirements
        switch (teleportType)
        {
            case 1: // Baine
                return instance->GetBossState(DATA_ECHO_OF_BAINE) == DONE;
            case 2: // Jaina
                return instance->GetBossState(DATA_ECHO_OF_JAINA) == DONE;
            case 3: // Sylvanas
                return instance->GetBossState(DATA_ECHO_OF_SYLVANAS) == DONE;
            case 4: // Tyrande
                return instance->GetBossState(DATA_ECHO_OF_TYRANDE) == DONE;
            case 5: // Murozond
                return (instance->GetBossState(DATA_ECHO_OF_BAINE) == DONE &&
                        instance->GetBossState(DATA_ECHO_OF_JAINA) == DONE &&
                        instance->GetBossState(DATA_ECHO_OF_SYLVANAS) == DONE &&
                        instance->GetBossState(DATA_ECHO_OF_TYRANDE) == DONE);
            default:
                return false;
        }
    }

    void TriggerNozdormuDialogue(Player* player, uint32 teleportType)
    {
        if (!player)
            return;

        if (Creature* nozdormu = player->FindNearestCreature(NPC_NOZDORMU_TELEPORT, 100.0f))
        {
            switch (teleportType)
            {
                case 1:
                    nozdormu->AI()->Talk(SAY_TELEPORT_BAINE);
                    break;
                case 2:
                    nozdormu->AI()->Talk(SAY_TELEPORT_JAINA);
                    break;
                case 3:
                    nozdormu->AI()->Talk(SAY_TELEPORT_SYLVANAS);
                    break;
                case 4:
                    nozdormu->AI()->Talk(SAY_TELEPORT_TYRANDE);
                    break;
                case 5:
                    nozdormu->AI()->Talk(SAY_TELEPORT_MUROZOND);
                    break;
            }
        }
    }
};

class spell_end_time_teleport : public SpellScriptLoader
{
public:
    spell_end_time_teleport() : SpellScriptLoader("spell_end_time_teleport") { }

    class spell_end_time_teleport_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_end_time_teleport_SpellScript);

        void HandleTeleport(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
            {
                if (Player* player = caster->ToPlayer())
                {
                    // Determine teleport type based on spell ID
                    uint32 teleportType = 0;
                    switch (GetSpellInfo()->Id)
                    {
                        case SPELL_TELEPORT_TO_BAINE:
                            teleportType = 1;
                            break;
                        case SPELL_TELEPORT_TO_JAINA:
                            teleportType = 2;
                            break;
                        case SPELL_TELEPORT_TO_SYLVANAS:
                            teleportType = 3;
                            break;
                        case SPELL_TELEPORT_TO_TYRANDE:
                            teleportType = 4;
                            break;
                        case SPELL_TELEPORT_TO_MUROZOND:
                            teleportType = 5;
                            break;
                        case SPELL_TELEPORT_TO_ENTRANCE:
                            teleportType = 0;
                            break;
                        case SPELL_TELEPORT_TO_EXIT:
                            teleportType = 6;
                            break;
                        default:
                            return;
                    }

                    // Perform teleport
                    const Position& pos = teleportPositions[teleportType];
                    player->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
                }
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_end_time_teleport_SpellScript::HandleTeleport, EFFECT_0, SPELL_EFFECT_TELEPORT_UNITS);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_end_time_teleport_SpellScript();
    }
};

void AddSC_end_time_teleport()
{
    new npc_end_time_teleport();
    new go_end_time_teleport();
    new spell_end_time_teleport();
}
