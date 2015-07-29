focus = 0
talk_start = 0
target = 0
following = false
attacking = false

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
  	msg = string.lower(msg)

  	if ((string.find(msg, '(%a*)hi(%a*)')) and (focus == 0)) and getDistanceToCreature(cid) < 4 then
 		if getPlayerVocation(cid) == 3 then
 			selfSay('Hello ' .. creatureGetName(cid) .. '! What spell do you want to learn?')
 			focus = cid
 			talk_start = os.clock()
 		else
 			selfSay('Sorry, I sell spells for paladins.')
 		end
	elseif string.find(msg, '(%a*)hi(%a*)') and (focus ~= cid) and getDistanceToCreature(cid) < 4 then
  		selfSay('Sorry, ' .. creatureGetName(cid) .. '! I talk to you in a minute.')
  	elseif focus == cid then		talk_start = os.clock()		if msgcontains(msg, 'light healing') then
  			learnSpell(cid,'exura',170)		elseif msgcontains(msg, 'create food') then
  			learnSpell(cid,'exevo pan',150)
 		elseif msgcontains(msg, 'intense healing') then
  			learnSpell(cid,'exura gran',350)
 		elseif msgcontains(msg, 'conjure arrow') then
  			learnSpell(cid,'exevo con',450)
 		elseif msgcontains(msg, 'heavy magic missle') then
  			learnSpell(cid,'adori gran',600)
 		elseif msgcontains(msg, 'haste') then
  			learnSpell(cid,'utani hur',600)
 		elseif msgcontains(msg, 'magic shield') then
  			learnSpell(cid,'utamo vita',450)
 		elseif msgcontains(msg, 'conjure bolt') then
  			learnSpell(cid,'exevo con mort',750)
 		elseif msgcontains(msg, 'ultimate healing') then
  			learnSpell(cid,'exura vita',1000)
 		elseif msgcontains(msg, 'conjure burst arrow') then
  			learnSpell(cid,'exevo con flam',1000)
 		elseif msgcontains(msg, 'conjure power bolt') then
  			learnSpell(cid,'exevo con vis',2000)
 		elseif msgcontains(msg, 'greater light') then
 			learnSpell(cid,'utevo gran lux',500)
 		elseif msgcontains(msg, 'light') then
 			learnSpell(cid,'utevo lux',100)		elseif msgcontains(msg, 'invisible') then
 			learnSpell(cid,'utana vid',1000)
		elseif msgcontains(msg, 'find person') then
 			learnSpell(cid,'exiva',80)
		elseif msgcontains(msg, 'magic rope') then
 			learnSpell(cid,'exani tera',200)

  		elseif string.find(msg, '(%a*)bye(%a*)')  and getDistanceToCreature(cid) < 4 then
  			selfSay('Good bye, ' .. creatureGetName(cid) .. '!')
  			focus = 0
  			talk_start = 0
  		end
  	end
end


function onCreatureChangeOutfit(creature)

end


function onThink()
  	if (os.clock() - talk_start) > 30 then
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
