focus = 0
talk_start = 0
target = 0
following = false
attacking = false
talk_state = 0
cname = ''
vocation = 0
mainlevel = 8

function onThingMove(creature, thing, oldpos, oldstackpos)

end


function onCreatureAppear(creature)

end


function onCreatureDisappear(cid, pos)
  	if focus == cid then
          selfSay('Good bye then.')
          focus = 0
          talk_start = 0
  	end
end


function onCreatureTurn(creature)

end
function msgcontains(txt, str)
  	return (string.find(txt, str) and not string.find(txt, '(%w+)' .. str) and not string.find(txt, str .. '(%w+)'))
end


function onCreatureSay(cid, type, msg)
  	cname = creatureGetName(cid)	msg = string.lower(msg)

  	if (msgcontains(msg, 'hi') and (focus == 0)) and getDistanceToCreature(cid) < 4 then
  		selfSay('Hello ' .. cname .. '! Are you ready to face your destiny?')
  		talk_state = 0
  		focus = cid
  		talk_start = os.clock()	elseif msgcontains(msg, 'hi') and (focus ~= cid) and getDistanceToCreature(cid) < 4 then
  		selfSay('Sorry, ' .. cname .. '! I talk to you in a minute.')

  	elseif msgcontains(msg, 'bye') and focus == cid and getDistanceToCreature(cid) < 4 then
 		selfSay('Good bye, ' .. cname .. '!')
 		talk_state = 0
 		focus = 0
 		talk_start = 0
  	elseif focus == cid then		talk_start = os.clock()
  		if talk_state == 0 then			if msgcontains(msg, 'yes') then		-- wanna go to mainland
  				level = getPlayerLevel(cname)

  				if level >= mainlevel then
 					selfSay('Great! Do you want to be a knight, a paladin, a sorcerer or a druid?')
 					talk_state = 1
  				else
  					selfSay('Sorry, you need level ' .. mainlevel .. ' to go to the mainland.')
 					talk_state = 0
  				end
 			else
 				selfSay('Come back when you are ready then.')
 				talk_state = 0
 			end

 		elseif talk_state == 1 then		-- telling vocation
 			talk_state = 2

 			if msgcontains(msg, 'sorcerer') then
 				selfSay('A mighty sorcerer! Are you sure?')
 				vocation = 1
 			elseif msgcontains(msg, 'druid') then
 				selfSay('A mysterious druid! Are you sure?')
 				vocation = 2
 			elseif msgcontains(msg, 'paladin') then
 				selfSay('A nimble paladin! Are you sure?')
 				vocation = 3
 			elseif msgcontains(msg, 'knight') then
 				selfSay('A valorous knight! Are you sure?')
 				vocation = 4
 			else
 				selfSay('Sorry, there is no such vocation.')
 				vocation = 0
 				talk_state = 1
 			end

 		elseif talk_state == 2 then		-- confirming vocation
 			if msgcontains(msg, 'yes') then
 				selfSay('Great! I can send you to The City. Where do you want to go?')
 				talk_state = 3
 			else
 				selfSay('What vocation do you want then?')
 				talk_state = 1
 			end

 		elseif talk_state == 3 then		-- telling city name
 			if msgcontains(msg, 'city') then
 				selfSay('Good luck, young adventurer!')
 				setPlayerVocation(cid,vocation)
 				setPlayerMasterPos(cid,160,54,7)
 				selfSay('/send ' .. cname .. ', 160 54 7')

 				talk_state = 0
 				focus = 0
 				talk_start = 0
 			else
 				selfSay('Sorry, there is no such city.')
 				talk_state = 3
 			end
  		end
  	end
end


function onCreatureChangeOutfit(creature)

end


function onThink()
  	if (os.clock() - talk_start) > 45 then
  		if focus > 0 then
  			selfSay('Next Please...')
  		end
  			focus = 0
  	end
 	if focus ~= 0 then
 		if getDistanceToCreature(focus) > 5 then
 			selfSay('Good bye then.')
 			focus = 0
 		end
 	end
end
