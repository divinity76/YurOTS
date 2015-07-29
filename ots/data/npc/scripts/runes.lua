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

  	if (msgcontains(msg, 'hi') and (focus == 0)) and getDistanceToCreature(cid) < 4 then
  		selfSay('Hello ' .. creatureGetName(cid) .. '! I sell runes, wands and rods.')
  		focus = cid
  		talk_start = os.clock()

  	elseif msgcontains(msg, 'hi') and (focus ~= cid) and getDistanceToCreature(cid) < 4 then
  		selfSay('Sorry, ' .. creatureGetName(cid) .. '! I talk to you in a minute.')

	elseif focus == cid then
		talk_start = os.clock()

		if msgcontains(msg, 'runes') then
			selfSay('I sell hmms (40gps), uhs (40gps), gfbs (60gps), explosions (60gps), sds (90gps) and blank runes (5gps). To buy more runes say "10 uh" or "100 sd".')		elseif msgcontains(msg, 'wands') then
			selfSay('I sell wand of inferno (15k), plague (5k), cosmic energy (10k), vortex (500gp) and dragonbreath (1k).')
		elseif msgcontains(msg, 'rods') then
			selfSay('I sell quagmire (10k), snakebite (500gp), tempest (15k), volcanic (5k) and moonlight rod (1k).')

		elseif msgcontains(msg, 'inferno') then
			buy(cid,2187,1,15000)
		elseif msgcontains(msg, 'plague') then
			buy(cid,2188,1,5000)
		elseif msgcontains(msg, 'cosmic energy') then
			buy(cid,2189,1,10000)
		elseif msgcontains(msg, 'vortex') then
			buy(cid,2190,1,500)
		elseif msgcontains(msg, 'dragonbreath') then
			buy(cid,2191,1,1000)

		elseif msgcontains(msg, 'quagmire') then
			buy(cid,2181,1,10000)
		elseif msgcontains(msg, 'snakebite') then
			buy(cid,2182,1,500)
		elseif msgcontains(msg, 'tempest') then
			buy(cid,2183,1,15000)
		elseif msgcontains(msg, 'volcanic') then
			buy(cid,2185,1,5000)
		elseif msgcontains(msg, 'moonlight') then
			buy(cid,2186,1,1000)

		elseif msgcontains(msg, '100 hmm') then
			buy(cid,2311,100,800)
		elseif msgcontains(msg, '10 hmm') then
			buy(cid,2311,10,80)
		elseif msgcontains(msg, 'hmm') then
			buy(cid,2311,5,40)
		elseif msgcontains(msg, '100 uh') then
			buy(cid,2273,100,4000)		elseif msgcontains(msg, '10 uh') then
			buy(cid,2273,10,400)
		elseif msgcontains(msg, 'uh') then
			buy(cid,2273,1,40)

		elseif msgcontains(msg, '100 gfb') then
			buy(cid,2304,100,2000)		elseif msgcontains(msg, '10 gfb') then
			buy(cid,2304,10,200)
		elseif msgcontains(msg, 'gfb') then
			buy(cid,2304,3,60)
		elseif msgcontains(msg, '100 explosion') then
			buy(cid,2313,100,2000)		elseif msgcontains(msg, '10 explosion') then
			buy(cid,2313,10,200)
		elseif msgcontains(msg, 'explosion') then
			buy(cid,2313,3,60)
		elseif msgcontains(msg, '100 sd') then
			buy(cid,2268,100,9000)		elseif msgcontains(msg, '10 sd') then
			buy(cid,2268,10,900)
		elseif msgcontains(msg, 'sd') then
			buy(cid,2268,1,90)

		elseif msgcontains(msg, 'blank') then
			buy(cid,2260,1,5)

		elseif string.find(msg, '(%a*)bye(%a*)') and getDistanceToCreature(cid) < 4 then
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
