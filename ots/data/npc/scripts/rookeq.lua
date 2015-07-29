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
  		selfSay('Hello, ' .. creatureGetName(cid) .. '! I sell Katana(20gp), Mace(20gp), Hatchet(20gp), Studded Armor(30gp), Chain Armor(90gp), Brass Armor(300gp), Brass Helmet(20gp), Leather Helmet(5gp), Brass Shield(15gp), Copper Shield(50gp),Leather Legs(8gp), Studded Legs(20gp), Leather Boots(5gp), Torch(2gp).')
  		focus = cid
  		talk_start = os.clock()

  	elseif msgcontains(msg, 'hi') and (focus ~= cid) and getDistanceToCreature(cid) < 4 then
  		selfSay('Sorry, ' .. creatureGetName(cid) .. '! I talk to you in a minute.')

 	elseif focus == cid then
 		talk_start = os.clock()

 		if msgcontains(msg, 'hatchet') then
 			buy(cid,2388,1,20)
 		elseif msgcontains(msg, 'katana') then
 			buy(cid,2412,1,20)
 		elseif msgcontains(msg, 'mace') then
 			buy(cid,2398,1,20)
 		elseif msgcontains(msg, 'studded armor') then
 			buy(cid,2484,1,30)
 		elseif msgcontains(msg, 'chain armor') then
 			buy(cid,2464,1,90)
 		elseif msgcontains(msg, 'brass armor') then
 			buy(cid,2465,1,300)
 		elseif msgcontains(msg, 'leather boots') then
 			buy(cid,2643,1,5)
 		elseif msgcontains(msg, 'brass helmet') then
 			buy(cid,2460,1,20)
 		elseif msgcontains(msg, 'leather helmet') then
 			buy(cid,2461,1,5)
 		elseif msgcontains(msg, 'brass shield') then
 			buy(cid,2511,1,15)
 		elseif msgcontains(msg, 'copper shield') then
 			buy(cid,2530,1,50)
 		elseif msgcontains(msg, 'torch') then
 			buy(cid,2050,1,2)
 		elseif msgcontains(msg, 'leather legs') then
 			buy(cid,2649,1,8)
 		elseif msgcontains(msg, 'studded legs') then
 			buy(cid,2468,1,20)

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
