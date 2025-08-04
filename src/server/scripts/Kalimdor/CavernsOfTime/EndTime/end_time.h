/*
 * Copyright (C) 2011-2016 ArkCORE <http://www.arkania.net/>
 *
 * End Time Boss Scripts - Main Include File
 * 
 * This file includes all boss scripts for the End Time dungeon:
 * - Echo of Baine
 * - Echo of Jaina  
 * - Echo of Sylvanas
 * - Echo of Tyrande
 * - Murozond
 *
 * THIS particular file is NOT free software; third-party users 
 * should NOT have access to it, redistribute it or modify it. :)
 */

#include "ScriptMgr.h"
#include "end_time.h"

// Include all boss scripts
extern void AddSC_boss_echo_of_baine();
extern void AddSC_boss_echo_of_jaina();
extern void AddSC_boss_echo_of_sylvanas();
extern void AddSC_boss_echo_of_tyrande();
extern void AddSC_boss_murozond();

class end_time_bosses : public WorldScript
{
    public:
        end_time_bosses() : WorldScript("end_time_bosses") { }

        void OnStartup() override
        {
            // Register all boss scripts
            AddSC_boss_echo_of_baine();
            AddSC_boss_echo_of_jaina();
            AddSC_boss_echo_of_sylvanas();
            AddSC_boss_echo_of_tyrande();
            AddSC_boss_murozond();
        }
};

void AddSC_end_time_bosses()
{
    new end_time_bosses();
} 
