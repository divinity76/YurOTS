YurOTS 0.9.4f

Yurez's Open Tibia Server for Tibia 7.6 based on CVS from 2006-01-14.

Features (and credits):
0.9.0
- bp/depo save (CVS)
- safe trade (CVS)
- cap system (CVS)
- buy/sell with crystals (TLM)
- house system (TLM, Black Demon, me)
- skulls & party (TLM)
- server save (TLM)
- light (Bomb Da Brat)
- vip list (Elementaries)
- npc load (Wolv119)
- anti afk (Tibia Rules)
- battle window (Skulldiggers, Tibia Rules)
- summons (CVS, Tibia Rules, me)
- gm invisible (TRS)
- rotating items (SuperGillis)
- ice rapier (Tibia Rules)
- construction kits (JTE)
- amulets: all but garlic and bronze (me)
- rings: skill, might, time (me)
- login queue (me)
- guilds with npc (me)
- pvp arena (me)
- boh+hur+time ring (me)
0.9.1
- burst arrow (Skulldiggers)
- destroy field (GriZzm0)
- exiva (Bryan)
- readables (me)
- commands: /pvp /owner /send
- more options in config
- energy ring
- boat npc
- field runes
0.9.2
- simpler houses (DosMaster)
- learning spells (me)
- promotions (me)
- commands: /save /ban
- separate distance mul
- multiline readables
- healing summons
- damage fixes
- uh exhaustion
0.9.3
- rook system (me)
- guild chats (CVS)
- key system (Orzech)
- item look for monsters/npcs (Black Demon)
- config: expmulpvp, spearlosechance
- commands: /pos /shutdown /max !uptime
- torches, candles etc. - in ammo slot
- all light spells
- low memory usage
0.9.4
- 7.6 protocol (CVS)
- OTB items & OTBM maps (CVS)
- day cycle (CVS revmagsys)
- wands & rods (Jiddo)
- premium system (me)
- death list with time (Jiddo)
- lifedrain & manadrain (me)
- garlic necklace, bronze amulet (me)
- high levels (Black Demon, me)
- utana vid & stealth ring (TLM, me)
- monster blood colors (Tjin)
- real monsters (KaM!oOoL, Shogun)
- commands: /clean /premmy !premmy
- config: access*, max*, queuepremmy
- exani tera (CrazyToko)
- reload config (Smygflik)
- level doors by actionId
- simple quests by uniqueId
- gm sees item id and position
- multiple pvp arenas
0.9.4a
- config: rodrange, wandrange, freepremmy
0.9.4d
- crash file generator (Andy Pennell)
0.9.4e
- builtin account creator (me)
- ip autodetection (Deathplanter)


Map by Aquisitor, updated to 7.6 by Vide (based on Ent-Online):
- 512x512, 1MB binary
- ~1000 spawns
- 36 houses, 3 guild houses
- 12 quests (+anni)
- npcs: selling runes, food, furniture, aols, ammo; buying loot; managing guilds
- main temple x=160 y=54 z=7
- rook temple x=85 y=211 z=7


GM commands:
/a x		jump forward x squares
/B msg		broadcast message to all players
/b nick		ban player on ip
/ban nick	ban players character
/c nick		teleport player to gm
/clean		remove portable items from the ground
/closeserver	only gms can enter
/down		teleport down
/getonline	list players online with levels
/goto nick	teleport near player
/goto x y z	teleport to position
/i id count	create item
/info nick	show info about player
/invisible	switch gm invisibility
/kick nick	kick player
/max x		set max number of players online
/m name		place monster
/openserver	everyone can enter
/owner nick	set owner of the house you are in
/owner		clear owner of the house you are in
/pos		shows your position
/premmy x nick	give player x hours of premium account
/promote nick	promote player
/pvp x		set pvp to no=0, normal=1, enforced=2
/s name		place npc
/save		force server save
/send nick, x y z	teleport player to position
/shutdown m	shedule server shutdown in m minutes
/summon name	place summon
/t		go to temple
/up		teleport up


Player commands:
!exp		show exp for level
!mana		show mana for magic level
!online		show players online
!house		reload house rights
!frags		show unjustified kills
!report msg	bug report to hoster
!uptime		show server uptime
!premmy		show premium time


Guild Master keywords:
found		founding a new guild
invite		invite player to e guild
kick, exclude	exclude player from a guild
join		join guild that invited me
leave		leave my guild
pass		pass leadership to another member
vice		promote player to vice-leader
member		denote vice-leader to regular member
nick, title	change member's nick


House spells:
aleta gom	edit owner (can be only one)
aleta som	edit sub-owners
aleta grav	edit door-owner (standing near door)
aleta sio	edit guests
alana sio "Name	kick player from a house
alana sio	kick myself from a house


Notes:
- guilds and houses are saved during server save, not when player logouts,
- if you want to place boss use /m, because /summon makes him your summon (no loot),
- items 1740,1747,1748,1749,1770 are for quests and cannot be moved nor picked (attribute questbox="1" in items.xml),
- ip ban lasts until server restart, character ban lasts until you set banned to 0 in player's file,
- when you switch ring from ground with ring on you the one from you still glimmers on the ground (no side effect),
- when you change light item (torch, candle, etc.) you lose effect of light spell,
- plague-, magic-, flame- and poisonthrowers are causing debugs (don't use them),
- when you buy something, always crystal coins are changed.


Known bugs:
- when you jump into player while beeing /invisible he gets debug,
- when you change world type from pvp to no-pvp players that have selected target earlier can still attack (until they lose target),
- some edges dissappear under blood splashes.


Fixed bugs:
0.9.1
- bugs in skull system (short time of redskull and skull for uhing someone),
0.9.2
- npcs are ending talk if you walked too far away from them,
- when you set house owner with /owner command, player doesn't need to say !house,
- skull system is enabled only on normal pvp worlds,
- stone skin amulet gives you only 95% protection from damage,
- breaking into houses using "teleport bug",
0.9.3
- you don't have to include front door in houses.xml,
- you don't lose backpack if diepercent is set to 0,
- promoted players gain health properly,
- house doors push you back when closed,
- you can define houses beyond 512x512 limit,
- spears do not disappear (Beet Da Brat),
- burst arrows do not cause exhaustion,
- you cannot shoot through doors,
- tusk table rotating corretly,
- all holes are working now,
0.9.4
- containers bought in furniture shop are openable immediately (Alreth),
- you have to put construction kit on the ground before using it (Alreth),
- house items outside containers are loaded/saved with all attributes (Kiper),
- after killing player you are pzlocked for <whitetime> minutes,
- you get skull when you use summons to attack players,
- you have to learn also built-in spells,
- npc overflow fix (Tibia Rules),
0.9.4a
- you don't get debug when you open non-quest chest,
0.9.4b
- axe ring and sword ring are working properly,
0.9.4c
- monsters lose invis after being hit by magic,
- if monster becomes invis player loses target,
- tutors get physical damage from monsters,
- you need premmy to buy promotion,
- conversion bugs on map (Vide),
0.9.4d
- monsters attack when you summon them (Smygflik),
- gms with same access can see each other while invisible,
0.9.4e
- crash when npc does not have access to execute command,
- crash when moving items from /clean-ed container,
- losing target when attacked player goes invisible,
- debug using a fluid on a fluid tile (Shampo),
0.9.4f
- crash when joining party with relogged player (Aquisitor).


Enjoy!