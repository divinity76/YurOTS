focus = 0
talk_start = 0
target = 0
following = false
attacking = false
talk_state = 0

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

  	if (msgcontains(msg, 'hi') and (focus == 0)) and getDistanceToCreature(cid) < 4 then
 		selfSay('Hello ' .. creatureGetName(cid) .. '! I sell premiums and promotions.')
 		focus = cid
 		talk_start = os.clock()
	elseif msgcontains(msg, 'hi') and (focus ~= cid) and getDistanceToCreature(cid) < 4 then
  		selfSay('Sorry, ' .. creatureGetName(cid) .. '! I talk to you in a minute.')
  	elseif focus == cid then		talk_start = os.clock()		if msgcontains(msg, 'promotion') or msgcontains(msg, 'promote') then
 			if isPromoted(cid) then
 				selfSay('Sorry, you are already promoted.')
 				talk_state = 0
 			elseif getPlayerLevel(creatureGetName(cid)) < 20 then
				selfSay('Sorry, you need level 20 to buy promotion.')
				talk_state = 0
			elseif not isPremium(cid) then
				selfSay('Sorry, you must be premium to buy promotion.')
				talk_state = 0
			else
				selfSay('Do you want to buy promotion for 20k?')
				talk_state = 1
			end

		elseif msgcontains(msg, 'premium') or msgcontains(msg, 'premmy') then
			selfSay('Do you want to buy 10 premmy hours for 10k?')
			talk_state = 2

		elseif talk_state == 1 then
			if msgcontains(msg, 'yes') then
				if pay(cid,20000) then
					selfSay('/promote ' .. creatureGetName(cid))
					selfSay('You are now promoted!')
				else
					selfSay('Sorry, you do not have enough money.')
				end
 			end
			talk_state = 0

		elseif talk_state == 2 then
			if msgcontains(msg, 'yes') then
				if pay(cid,10000) then
					selfSay('/premmy 10 ' .. creatureGetName(cid))
					selfSay('You have 10 premmy hours more!')
				else
					selfSay('Sorry, you do not have enough money.')
				end
			end
			talk_state = 0

  		elseif msgcontains(msg, 'bye')  and getDistanceToCreature(cid) < 4 then
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
