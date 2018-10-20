
-- Trainer First Aid, Teach Profession

DELETE FROM npc_trainer WHERE entry = 200303 and spell IN (3279, 3280, 19903, 10847, 54255, 50299, 74560);
INSERT INTO `npc_trainer` VALUES 
(200303, 3279,     100,   0,   0,  0),
(200303, 3280,     500, 129,  50,  5),
(200303, 19903,  10000, 129, 125, 10),
(200303, 10847,  25000, 129, 200, 25),
(200303, 54255, 100000, 129, 275, 40),
(200303, 50299, 150000, 129, 350, 55),
(200303, 74560, 500000, 129, 425, 75);

DELETE FROM npc_trainer WHERE spell=-200303 and entry IN
(2326, 2329, 2798, 3181, 4211, 4591, 5150, 5759, 5939, 5943, 6094, 12939, 16272, 
16662, 16731, 17214, 17424, 18990, 18991, 19184, 19478, 22477, 23734, 26956, 
26992, 28706, 29233, 33589, 33621, 36615, 45540, 49879, 50574, 56796, 2325, 2327, 3373, 5024, 29628);

INSERT INTO npc_trainer (entry, spell) VALUES 
(2325, -200303),
(2327, -200303),
(3373, -200303),
(5024, -200303),
(2326, -200303),
(2329, -200303),
(2798, -200303),
(3181, -200303),
(4211, -200303),
(4591, -200303),
(5150, -200303),
(5759, -200303),
(5939, -200303),
(5943, -200303),
(6094, -200303),
(12939, -200303),
(16272, -200303),
(16662, -200303),
(16731, -200303),
(17214, -200303),
(17424, -200303),
(18990, -200303),
(18991, -200303),
(19184, -200303),
(19478, -200303),
(22477, -200303),
(23734, -200303),
(26956, -200303),
(26992, -200303),
(28706, -200303),
(29233, -200303),
(33589, -200303),
(33621, -200303),
(36615, -200303),
(45540, -200303),
(49879, -200303),
(50574, -200303),
(56796, -200303);



