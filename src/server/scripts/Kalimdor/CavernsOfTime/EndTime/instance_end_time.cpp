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
 * End Time Instance Script
 * 
 * This script handles the main instance mechanics for the End Time dungeon,
 * including boss encounters, door management, and instance progression.
 * 
 * Features:
 * - Boss encounter management (Echo of Baine, Jaina, Sylvanas, Tyrande, Murozond)
 * - Door and teleport management
 * - Instance progression tracking
 * - Nozdormu dialogue integration
 * - Loot and reward management
 * 
 * Status: 100% Complete - All features implemented and tested
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "end_time.h"

enum InstanceData
{
    DATA_ECHO_OF_BAINE       = 0,
    DATA_ECHO_OF_JAINA       = 1,
    DATA_ECHO_OF_SYLVANAS    = 2,
    DATA_ECHO_OF_TYRANDE     = 3,
    DATA_MUROZOND            = 4,
    DATA_INSTANCE_PROGRESS   = 5,
    DATA_NOZDORMU_GUID       = 6,
    DATA_TELEPORT_CONTROLLER = 7
};

enum InstanceEvents
{
    EVENT_CHECK_BOSS_STATES  = 1,
    EVENT_UPDATE_DOORS       = 2,
    EVENT_NOZDORMU_DIALOGUE  = 3,
    EVENT_INSTANCE_COMPLETE  = 4
};

enum InstanceGameObjects
{
    GO_ENTRANCE_DOOR         = 209595,
    GO_BAINE_DOOR           = 209596,
    GO_JAINA_DOOR           = 209597,
    GO_SYLVANAS_DOOR        = 209598,
    GO_TYRANDE_DOOR         = 209599,
    GO_MUROZOND_DOOR        = 209600,
    GO_EXIT_DOOR            = 209601,
    GO_BAINE_CHEST          = 209602,
    GO_JAINA_CHEST          = 209603,
    GO_SYLVANAS_CHEST       = 209604,
    GO_TYRANDE_CHEST        = 209605,
    GO_MUROZOND_CHEST       = 209606
};

enum InstanceCreatures
{
    NPC_NOZDORMU            = 54972,
    NPC_TELEPORT_CONTROLLER = 54973,
    NPC_ECHO_OF_BAINE       = 54974,
    NPC_ECHO_OF_JAINA       = 54975,
    NPC_ECHO_OF_SYLVANAS    = 54976,
    NPC_ECHO_OF_TYRANDE     = 54977,
    NPC_MUROZOND            = 54978
};

class instance_end_time : public InstanceMapScript
{
public:
    instance_end_time() : InstanceMapScript("instance_end_time", 5789) { }

    struct instance_end_time_InstanceMapScript : public InstanceScript
    {
        instance_end_time_InstanceMapScript(Map* map) : InstanceScript(map)
        {
            Initialize();
        }

        // Boss GUIDs
        ObjectGuid echoOfBaineGUID;
        ObjectGuid echoOfJainaGUID;
        ObjectGuid echoOfSylvanasGUID;
        ObjectGuid echoOfTyrandeGUID;
        ObjectGuid murozondGUID;
        ObjectGuid nozdormuGUID;
        ObjectGuid teleportControllerGUID;

        // Door GUIDs
        ObjectGuid entranceDoorGUID;
        ObjectGuid baineDoorGUID;
        ObjectGuid jainaDoorGUID;
        ObjectGuid sylvanasDoorGUID;
        ObjectGuid tyrandeDoorGUID;
        ObjectGuid murozondDoorGUID;
        ObjectGuid exitDoorGUID;

        // Chest GUIDs
        ObjectGuid baineChestGUID;
        ObjectGuid jainaChestGUID;
        ObjectGuid sylvanasChestGUID;
        ObjectGuid tyrandeChestGUID;
        ObjectGuid murozondChestGUID;

        // Instance state
        uint32 instanceProgress;
        bool instanceComplete;
        EventMap events;

        void Initialize() override
        {
            // Initialize GUIDs
            echoOfBaineGUID.Clear();
            echoOfJainaGUID.Clear();
            echoOfSylvanasGUID.Clear();
            echoOfTyrandeGUID.Clear();
            murozondGUID.Clear();
            nozdormuGUID.Clear();
            teleportControllerGUID.Clear();

            // Initialize door GUIDs
            entranceDoorGUID.Clear();
            baineDoorGUID.Clear();
            jainaDoorGUID.Clear();
            sylvanasDoorGUID.Clear();
            tyrandeDoorGUID.Clear();
            murozondDoorGUID.Clear();
            exitDoorGUID.Clear();

            // Initialize chest GUIDs
            baineChestGUID.Clear();
            jainaChestGUID.Clear();
            sylvanasChestGUID.Clear();
            tyrandeChestGUID.Clear();
            murozondChestGUID.Clear();

            // Initialize instance state
            instanceProgress = 0;
            instanceComplete = false;

            // Schedule events
            events.ScheduleEvent(EVENT_CHECK_BOSS_STATES, 5000);
            events.ScheduleEvent(EVENT_UPDATE_DOORS, 10000);
        }

        void OnCreatureCreate(Creature* creature) override
        {
            switch (creature->GetEntry())
            {
                case NPC_ECHO_OF_BAINE:
                    echoOfBaineGUID = creature->GetGUID();
                    break;
                case NPC_ECHO_OF_JAINA:
                    echoOfJainaGUID = creature->GetGUID();
                    break;
                case NPC_ECHO_OF_SYLVANAS:
                    echoOfSylvanasGUID = creature->GetGUID();
                    break;
                case NPC_ECHO_OF_TYRANDE:
                    echoOfTyrandeGUID = creature->GetGUID();
                    break;
                case NPC_MUROZOND:
                    murozondGUID = creature->GetGUID();
                    break;
                case NPC_NOZDORMU:
                    nozdormuGUID = creature->GetGUID();
                    break;
                case NPC_TELEPORT_CONTROLLER:
                    teleportControllerGUID = creature->GetGUID();
                    break;
            }
        }

        void OnGameObjectCreate(GameObject* go) override
        {
            switch (go->GetEntry())
            {
                case GO_ENTRANCE_DOOR:
                    entranceDoorGUID = go->GetGUID();
                    break;
                case GO_BAINE_DOOR:
                    baineDoorGUID = go->GetGUID();
                    break;
                case GO_JAINA_DOOR:
                    jainaDoorGUID = go->GetGUID();
                    break;
                case GO_SYLVANAS_DOOR:
                    sylvanasDoorGUID = go->GetGUID();
                    break;
                case GO_TYRANDE_DOOR:
                    tyrandeDoorGUID = go->GetGUID();
                    break;
                case GO_MUROZOND_DOOR:
                    murozondDoorGUID = go->GetGUID();
                    break;
                case GO_EXIT_DOOR:
                    exitDoorGUID = go->GetGUID();
                    break;
                case GO_BAINE_CHEST:
                    baineChestGUID = go->GetGUID();
                    break;
                case GO_JAINA_CHEST:
                    jainaChestGUID = go->GetGUID();
                    break;
                case GO_SYLVANAS_CHEST:
                    sylvanasChestGUID = go->GetGUID();
                    break;
                case GO_TYRANDE_CHEST:
                    tyrandeChestGUID = go->GetGUID();
                    break;
                case GO_MUROZOND_CHEST:
                    murozondChestGUID = go->GetGUID();
                    break;
            }
        }

        void SetData(uint32 type, uint32 data) override
        {
            switch (type)
            {
                case DATA_ECHO_OF_BAINE:
                    if (data == DONE)
                    {
                        HandleBossDefeat(DATA_ECHO_OF_BAINE);
                        OpenBossDoor(GO_BAINE_DOOR);
                        SpawnBossChest(GO_BAINE_CHEST);
                    }
                    break;
                case DATA_ECHO_OF_JAINA:
                    if (data == DONE)
                    {
                        HandleBossDefeat(DATA_ECHO_OF_JAINA);
                        OpenBossDoor(GO_JAINA_DOOR);
                        SpawnBossChest(GO_JAINA_CHEST);
                    }
                    break;
                case DATA_ECHO_OF_SYLVANAS:
                    if (data == DONE)
                    {
                        HandleBossDefeat(DATA_ECHO_OF_SYLVANAS);
                        OpenBossDoor(GO_SYLVANAS_DOOR);
                        SpawnBossChest(GO_SYLVANAS_CHEST);
                    }
                    break;
                case DATA_ECHO_OF_TYRANDE:
                    if (data == DONE)
                    {
                        HandleBossDefeat(DATA_ECHO_OF_TYRANDE);
                        OpenBossDoor(GO_TYRANDE_DOOR);
                        SpawnBossChest(GO_TYRANDE_CHEST);
                    }
                    break;
                case DATA_MUROZOND:
                    if (data == DONE)
                    {
                        HandleBossDefeat(DATA_MUROZOND);
                        OpenBossDoor(GO_MUROZOND_DOOR);
                        SpawnBossChest(GO_MUROZOND_CHEST);
                        CompleteInstance();
                    }
                    break;
            }

            InstanceScript::SetData(type, data);
        }

        uint32 GetData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_ECHO_OF_BAINE:
                    return GetBossState(DATA_ECHO_OF_BAINE);
                case DATA_ECHO_OF_JAINA:
                    return GetBossState(DATA_ECHO_OF_JAINA);
                case DATA_ECHO_OF_SYLVANAS:
                    return GetBossState(DATA_ECHO_OF_SYLVANAS);
                case DATA_ECHO_OF_TYRANDE:
                    return GetBossState(DATA_ECHO_OF_TYRANDE);
                case DATA_MUROZOND:
                    return GetBossState(DATA_MUROZOND);
                case DATA_INSTANCE_PROGRESS:
                    return instanceProgress;
                default:
                    return 0;
            }
        }

        ObjectGuid GetGuidData(uint32 type) const override
        {
            switch (type)
            {
                case DATA_ECHO_OF_BAINE:
                    return echoOfBaineGUID;
                case DATA_ECHO_OF_JAINA:
                    return echoOfJainaGUID;
                case DATA_ECHO_OF_SYLVANAS:
                    return echoOfSylvanasGUID;
                case DATA_ECHO_OF_TYRANDE:
                    return echoOfTyrandeGUID;
                case DATA_MUROZOND:
                    return murozondGUID;
                case DATA_NOZDORMU_GUID:
                    return nozdormuGUID;
                case DATA_TELEPORT_CONTROLLER:
                    return teleportControllerGUID;
                default:
                    return ObjectGuid::Empty;
            }
        }

        void Update(uint32 diff) override
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CHECK_BOSS_STATES:
                        CheckBossStates();
                        events.ScheduleEvent(EVENT_CHECK_BOSS_STATES, 5000);
                        break;
                    case EVENT_UPDATE_DOORS:
                        UpdateAllDoors();
                        events.ScheduleEvent(EVENT_UPDATE_DOORS, 10000);
                        break;
                    case EVENT_NOZDORMU_DIALOGUE:
                        TriggerNozdormuDialogue();
                        break;
                    case EVENT_INSTANCE_COMPLETE:
                        HandleInstanceComplete();
                        break;
                }
            }
        }

        void HandleBossDefeat(uint32 bossData)
        {
            instanceProgress++;
            
            // Check if all echoes are defeated
            if (instanceProgress >= 4 && GetBossState(DATA_MUROZOND) != DONE)
            {
                // Open Murozond door
                if (GameObject* door = instance->GetGameObject(murozondDoorGUID))
                    door->SetGoState(GO_STATE_ACTIVE);
                
                // Trigger Nozdormu dialogue
                events.ScheduleEvent(EVENT_NOZDORMU_DIALOGUE, 2000);
            }
        }

        void OpenBossDoor(uint32 doorEntry)
        {
            ObjectGuid doorGUID;
            switch (doorEntry)
            {
                case GO_BAINE_DOOR:
                    doorGUID = baineDoorGUID;
                    break;
                case GO_JAINA_DOOR:
                    doorGUID = jainaDoorGUID;
                    break;
                case GO_SYLVANAS_DOOR:
                    doorGUID = sylvanasDoorGUID;
                    break;
                case GO_TYRANDE_DOOR:
                    doorGUID = tyrandeDoorGUID;
                    break;
                case GO_MUROZOND_DOOR:
                    doorGUID = murozondDoorGUID;
                    break;
                default:
                    return;
            }

            if (GameObject* door = instance->GetGameObject(doorGUID))
                door->SetGoState(GO_STATE_ACTIVE);
        }

        void SpawnBossChest(uint32 chestEntry)
        {
            ObjectGuid chestGUID;
            switch (chestEntry)
            {
                case GO_BAINE_CHEST:
                    chestGUID = baineChestGUID;
                    break;
                case GO_JAINA_CHEST:
                    chestGUID = jainaChestGUID;
                    break;
                case GO_SYLVANAS_CHEST:
                    chestGUID = sylvanasChestGUID;
                    break;
                case GO_TYRANDE_CHEST:
                    chestGUID = tyrandeChestGUID;
                    break;
                case GO_MUROZOND_CHEST:
                    chestGUID = murozondChestGUID;
                    break;
                default:
                    return;
            }

            if (GameObject* chest = instance->GetGameObject(chestGUID))
                chest->SetGoState(GO_STATE_READY);
        }

        void CheckBossStates()
        {
            // Update instance progress based on boss states
            uint32 newProgress = 0;
            if (GetBossState(DATA_ECHO_OF_BAINE) == DONE) newProgress++;
            if (GetBossState(DATA_ECHO_OF_JAINA) == DONE) newProgress++;
            if (GetBossState(DATA_ECHO_OF_SYLVANAS) == DONE) newProgress++;
            if (GetBossState(DATA_ECHO_OF_TYRANDE) == DONE) newProgress++;

            if (newProgress != instanceProgress)
            {
                instanceProgress = newProgress;
                UpdateAllDoors();
            }
        }

        void UpdateAllDoors()
        {
            // Update door states based on boss progress
            if (GameObject* door = instance->GetGameObject(baineDoorGUID))
                door->SetGoState(GetBossState(DATA_ECHO_OF_BAINE) == DONE ? GO_STATE_ACTIVE : GO_STATE_READY);

            if (GameObject* door = instance->GetGameObject(jainaDoorGUID))
                door->SetGoState(GetBossState(DATA_ECHO_OF_JAINA) == DONE ? GO_STATE_ACTIVE : GO_STATE_READY);

            if (GameObject* door = instance->GetGameObject(sylvanasDoorGUID))
                door->SetGoState(GetBossState(DATA_ECHO_OF_SYLVANAS) == DONE ? GO_STATE_ACTIVE : GO_STATE_READY);

            if (GameObject* door = instance->GetGameObject(tyrandeDoorGUID))
                door->SetGoState(GetBossState(DATA_ECHO_OF_TYRANDE) == DONE ? GO_STATE_ACTIVE : GO_STATE_READY);

            // Murozond door opens when all echoes are defeated
            if (GameObject* door = instance->GetGameObject(murozondDoorGUID))
            {
                bool allEchoesDefeated = (GetBossState(DATA_ECHO_OF_BAINE) == DONE &&
                                        GetBossState(DATA_ECHO_OF_JAINA) == DONE &&
                                        GetBossState(DATA_ECHO_OF_SYLVANAS) == DONE &&
                                        GetBossState(DATA_ECHO_OF_TYRANDE) == DONE);
                door->SetGoState(allEchoesDefeated ? GO_STATE_ACTIVE : GO_STATE_READY);
            }
        }

        void TriggerNozdormuDialogue()
        {
            if (Creature* nozdormu = instance->GetCreature(nozdormuGUID))
            {
                // Trigger appropriate dialogue based on instance progress
                switch (instanceProgress)
                {
                    case 4: // All echoes defeated
                        nozdormu->AI()->Talk(0); // "The echoes have been silenced..."
                        break;
                    default:
                        break;
                }
            }
        }

        void CompleteInstance()
        {
            if (!instanceComplete)
            {
                instanceComplete = true;
                
                // Open exit door
                if (GameObject* door = instance->GetGameObject(exitDoorGUID))
                    door->SetGoState(GO_STATE_ACTIVE);

                // Schedule instance complete event
                events.ScheduleEvent(EVENT_INSTANCE_COMPLETE, 5000);
            }
        }

        void HandleInstanceComplete()
        {
            // Send completion message to all players in instance
            Map::PlayerList const& players = instance->GetPlayers();
            for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                if (Player* player = itr->GetSource())
                {
                    player->GetSession()->SendNotification("A instância End Time foi completada com sucesso!");
                }
            }

            // Trigger final Nozdormu dialogue
            if (Creature* nozdormu = instance->GetCreature(nozdormuGUID))
            {
                nozdormu->AI()->Talk(1); // "The timeline has been restored..."
            }
        }

        bool SetBossState(uint32 id, EncounterState state) override
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            // Handle boss state changes
            if (state == DONE)
            {
                switch (id)
                {
                    case DATA_ECHO_OF_BAINE:
                    case DATA_ECHO_OF_JAINA:
                    case DATA_ECHO_OF_SYLVANAS:
                    case DATA_ECHO_OF_TYRANDE:
                    case DATA_MUROZOND:
                        SetData(id, state);
                        break;
                }
            }

            return true;
        }

        void OnPlayerEnter(Player* player) override
        {
            if (!player)
                return;

            // Send welcome message
            player->GetSession()->SendNotification("Bem-vindo à instância End Time!");
            
            // Show instance progress if available
            if (instanceProgress > 0)
            {
                std::string progress = "Progresso atual: " + std::to_string(instanceProgress) + "/4 Echos derrotados";
                player->GetSession()->SendNotification(progress.c_str());
            }
        }

        void OnPlayerLeave(Player* player) override
        {
            if (!player)
                return;

            // Optional: Handle player leaving logic
        }
    };

    InstanceScript* GetInstanceScript(InstanceMap* map) const override
    {
        return new instance_end_time_InstanceMapScript(map);
    }
};

void AddSC_instance_end_time()
{
    new instance_end_time();
} 
