YurOTS 0.9.4f

OTS by Yurez pod Tibie 7.6 oparty na CVS z dnia 2006-01-14.

Mo¿liwoœci (i autorzy):
0.9.0
- bp/depo save (CVS)
- bezpieczny trade (CVS)
- buy/sell z kryszta³kami (TLM)
- house system (TLM, Black Demon, ja)
- skulle i party (TLM)
- server save (TLM)
- œwiat³o (Bomb Da Brat)
- vip lista (Elementaries)
- spawn npc (Wolv119)
- anty afk (Tibia Rules)
- runy na battle (Skulldiggers, Tibia Rules)
- summony (CVS, Tibia Rules, ja)
- niewidka dla gma (TRS)
- obracanie itemów (SuperGillis)
- ice rapier (Tibia Rules)
- construction kits (JTE)
- burst arrow (Skulldiggers)
- exiva (bryan007)
- amulety: wszystkie poza garlic i bronze (ja)
- ringi: skillowe, might, time, energy (ja)
- kolejka (ja)
- gildie z npcem (ja)
- pvp arena (ja)
- boh+hur+time ring (ja)
0.9.1
- burst arrow (Skulldiggers)
- destroy field (GriZzm0)
- exiva (Bryan)
- napisy na tabliczkach (ja)
- komendy: /pvp /owner /send
- wiêcej opcji w configu
- lepsze obliczanie obra¿eñ
- energy ring
- npc na ³ódkê
- fieldy
0.9.2
- prostsze domki (DosMaster)
- nauka czarów (ja)
- promocje (ja)
- komendy: /save /ban
- oddzielny distance mul
- wielolinijkowe signy
- leczenie summonów
- poprawki obra¿eñ
- exhaust na uha
0.9.3
- system rooka (ja)
- czaty gildii (CVS)
- wygl¹d itemu dla potwora/npca (Black Demon)
- config: expmulpvp, spearlosechance
- komendy: /pos /shutdown /max !uptime
- pochodnie, œwiece itp. - w miejscu na strza³y
- wszystkie czary œwiat³a
- mniejsze zu¿ycie pamiêci
0.9.4
- protokó³ 7.6 (CVS)
- itemy OTB i mapy OTBM (CVS)
- dzieñ i noc (CVS revmagsys)
- ró¿d¿ki (Jiddo)
- system premium (ja)
- lista zgonów z czasem (Jiddo)
- wysysanie ¿ycia i many (ja)
- garlic necklace i bronze amulet (ja)
- wysokie levele (Black Demon, ja)
- utana vid i stealth ring (ja)
- kolory krwi potworów (Tjin)
- prawdziwe potwory (KaM!oOoL, Shogun)
- komendy: /clean /premmy !premmy
- config: access*, max*, queuepremmy
- exani tera (CrazyToko)
- reload configa (Smygflik)
- drzwi levelowe przez actionId
- proste questy przez uniqueId
- gm widzi id i pozycjê itemu
- wiele aren pvp
0.9.4a
- config: rodrange, wandrange, freepremmy
0.9.4d
- tworzenie plików crashy (Andy Pennell)
0.9.4e
- wbudowany kreator kont (ja)
- autowykrywanie ip (Deathplanter)


Mapa by Aquisitor, update do 7.6 by Vide (na podstawie Ent-Online):
- 512x512, 1MB binarny
- ~1000 spawnów
- 36 domków, 3 gh
- 12 questsów (+anni)
- npc: sprzedaj¹cy runy, jedzenie, meble; skupuj¹cy loot
- temple x=160 y=54 z=7
- rook temple x=85 y=211 z=7


Komendy GMa:
/a x		skok do przodu o x kratek
/B msg		wiadomoœæ do wszystkich
/b nick		ban na ip
/ban nick	ban postaci
/c nick		teleport gracza do siebie
/clean		usuwa œmieci le¿¹ce na ziemi
/i id count	tworzenie itemów
/m name		postaw potwora
/summon name	postaw summona
/t		teleport do œwi¹tyni
/goto nick	teleport obok gracza
/info nick	info o graczu
/closeserver	serwer tylko dla gmów
/openserver	serwer dla wszystkich
/getonline	pokazuje graczy i levele
/kick nick	kickniêcie gracza
/up		teleport w górê
/down		teleport w dó³
/invisible	prze³¹cz niewidkê gma
/max x		ustaw limit graczy
/pos		poka¿ moje po³o¿enie
/premmy x nick	daj graczowi x godzin premii
/promote nick	daj graczowi promocjê
/pvp		ustaw pvp na brak=0, zwyk³e=1, enforced=2
/owner nick	zmieñ w³aœciciela domku w którym stoisz
/owner		wyczyœæ w³aœciciela domku w którym stoisz
/shutdown m	wy³¹cz serwer po up³ywie m minut
/s name		ustawia npca
/save		wymuœ zapis stanu serwera
/send nick, x y z	przenieœ gracza do podanych wspó³rzêdnych


Komendy gracza:
!exp		poka¿ exp brakuj¹cy do levela
!mana		poka¿ manê brakuj¹c¹ do mlevela
!online		lista graczy online
!house		prze³aduj prawa do domków
!frags		poka¿ iloœæ zabitych (unjust)
!report msg	bug report dla hostera
!uptime		wyœwietl czas dzia³ania serwera
!premmy		zobacz iloœæ godzin premii


Komendy Guild Mastera:
found		zak³adanie nowej gildii
invite		zapraszanie ludzi do gildii
kick, exclude	wyrzucanie z gildii
join		do³aczanie sie do gildii
leave		opuszczenie gildii,
pass		przekazanie przywodztwa
vice		awansowanie na vice-lidera
member		degradacja vice-lidera
nick, title	ustawienie opisu w nawiasie


Czary do domków:
aleta gom	edycja ownera (mo¿e byæ tylko jeden)
aleta som	edycja listy sub-ownerow
aleta grav	edycja listy door-ownerow (przy drzwiach)
aleta sio	edycja listy gosci
alana sio "Nick	wyrzucenie gracza z domku
alana sio	ucieczka z domku


Uwagi:
- gildie i domki zapisuj¹ siê podczas save servera, nie w momencie wylogowania gracza,
- stawiaj¹c bossa u¿ywaj /m, poniewa¿ /summon tworzy summona (nie ma loota),
- itemy 1740,1747,1748,1749,1770 s¹ przeznaczone na skrzynki questowe (nie mo¿na ich podnosiæ ani przesuwaæ - znacznik questbox="1" w items.xml),
- ban na ip trwa do resetu serwa, ban na postaæ trwa dopóki nie ustawisz banned na 0 w pliku gracza,
- kiedy zamieniasz ring z ziemi z tym na rêce, ten który wyl¹duje na ziemi dalej b³yszczy (bez skutków ubocznych),
- kiedy niesiesz zapalon¹ pochodniê, œwiecê itp. czary œwiat³a s¹ ignorowane,
- plague-, magic-, flame- and poisonthrowers powoduj¹ debugi (nie u¿ywaj ich).



Znane b³êdy:
- kiedy bêd¹c niewidzialnym znajdziesz siê w tym samym miejscu co gracz, on dostaje debuga,
- kiedy zmieniasz œwiat z pvp na no-pvp za pomoc¹ komendy /pvp, gracz który mia³ zaznaczonego innego gracza mo¿e nadal atakowaæ,
- niektóre krawêdzie znikaj¹ pod plamami krwi.


Poprawione b³êdy:
0.9.1
- bugi w skull systemie (zbyt wczesne znikanie red skulla i skull za uhanie kogoœ),
0.9.2
- npce koñcz¹ rozmowê gdy odejdziesz od nich zbyt daleko,
- kiedy u¿yjesz komendy /owner, gracz nie musi u¿ywaæ !house aby wejœæ do domku,
- skull system dzia³a tylo na serwerach z normalnym pvp,
- stone skin amulet daje tylko 95% ochrony przed obra¿eniami,
- w³amywanie siê do domków za pomoc¹ "bugu z teleportem",
0.9.3
- nie musisz dodawaæ frontowych drzwi w houses.xml,
- nie tracisz backpacka gdy diepercent jest równe 0,
- graczom z promocj¹ regeneruje siê ¿ycie,
- drzwi od domków wypychaj¹ ciê przy zamkniêciu,
- mo¿na tworzyæ domki poza obszarem 512x512,
- w³ócznie nie znikaj¹ (Beet Da Brat),
- burst arrowy nie powoduj¹ exhausta,
- nie mo¿na strzelaæ przez drzwi,
- tusk table obraca siê poprawnie,
- dzia³aj¹ wszystkie dziury.
0.9.4
- boxy kupione w sklepie meblowym mo¿na otwieraæ od razu (Alreth),
- przed u¿yciem construction kita musisz po³o¿yæ go na ziemi (Alreth),
- rzeczy w domkach s¹ zapisywane ze wszystkimi atrybutami (Kiper),
- po zabiciu gracza dostajesz pzlocka na <whitetime> minut,
- pkowanie summonami liczy siê do skulli,
- trzeba uczyæ siê wbudowanych czarów,
- npce nie powoduj¹ crashy (Tibia Rules),
0.9.4a
- nie dostajesz debuga przy otwieraniu nie-questowej skrzynki,
0.9.4b
- mo¿na za³o¿yæ axe ring i dzia³a sword ring,
0.9.4c
- potwory trac¹ niewidkê po trafieniu magi¹,
- kiedy potwór staje siê niewidzialny atakuj¹cy traci cel,
- tutorzy otrzymuj¹ obra¿enia od potworów,
- potrzebujesz pacca ¿eby kupiæ promocjê,
- b³êdy przy konwersji mapy (Vide).
0.9.4d
- potwory atakuj¹ gracza w momencie przywo³ania (Smygflik),
- gm-owie z tym samym accessem widz¹ siê mimo niewidki,
0.9.4e
- crash gdy npc nie ma uprawnieñ do wykonania komendy,
- crash przy próbie wyjêcia rzeczy z pojemnika usuniêtego przez /clean,
- tracenie celu kiedy atakowany gracz staje siê niewidzialny,
- debugi przy rozlewaniu p³ynów z flaszek,
0.9.4f
- crash przy do³¹czaniu do dru¿yny przelogowanego gracza (Aquisitor).


Mi³ej zabawy!