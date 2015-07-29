-- the id of the creature we are attacking, following, etc.
 
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
 
  endfunction msgcontains(txt, str)
  	return (string.find(txt, str) and not string.find(txt, '(%w+)' .. str) and not string.find(txt, str .. '(%w+)'))
  end
 
 
  function onCreatureSay(cid, type, msg)
  	msg = string.lower(msg)
 
  	if ((string.find(msg, '(%a*)hi(%a*)')) and (focus == 0)) and getDistanceToCreature(cid) < 4 then
  		selfSay('Hello, ' .. creatureGetName(cid) .. '! I sell beer and wine for 10 gp.')
  		focus = cid
  		talk_start = os.clock()
  	end	if string.find(msg, '(%a*)hi(%a*)') and (focus ~= cid) and getDistanceToCreature(cid) < 4 then
  		selfSay('Leave us alone, ' .. creatureGetName(cid) .. '!')
  	end
  	if msgcontains(msg, 'buy beer') and focus == cid then
  		buy(cid,2006,3,10)
  		talk_start = os.clock()
  	end
 
  	if msgcontains(msg, 'buy wine') and focus == cid then
  		buy(cid,2006,15,10)
  		talk_start = os.clock()
  	end
 
  	if msgcontains(msg, 'quest') and focus == cid then
  		talk_start = os.clock()
  		queststatus = getPlayerStorageValue(cid,5019)
 
  		if queststatus == -1 then
  			selfSay('When I was leaving my ship, I hide priceless artifact in one of the barrels. You could bring it to me.')
  		else
  			selfSay('Bloody adventurers, always looking for trouble!')
  		end
  	end	if msgcontains(msg, 'ship') and focus == cid then
  		selfSay('My ship is on the desert. It doesn\'t look well but still can sail. Of course if you know how to steer it.')
  		talk_start = os.clock()
  	end
 
  	if msgcontains(msg, 'steer') and focus == cid then
  		talk_start = os.clock()
  		queststatus = getPlayerStorageValue(cid,5019)
 
  		if queststatus == -1 then
  			selfSay('If you want to learn steering, bring me artifact from the ship.')
  		else
  			selfSay('You know everything I do.')
  		end
  	end
 
  	if msgcontains(msg, 'artifact') and focus == cid then
  		talk_start = os.clock()
  		queststatus = getPlayerStorageValue(cid,5019)
 
  		if queststatus == -1 then
  			itemstatus = doPlayerRemoveItem(cid,2342)
 
  			if itemstatus == -1 then
  				selfSay('It\'s in one of the barrels. Look under the deck.')
  			else
  				setPlayerStorageValue(cid,5019,1)
  				selfSay('Thank you! To steer a ship, just use a steering wheel.')
  			end
  		else			selfSay('This helmet belongs to me.')
  		end
  	end
 
  	if string.find(msg, '(%a*)bye(%a*)') and focus == cid and getDistanceToCreature(cid) < 4 then
  		selfSay('Bye, ' .. creatureGetName(cid) .. '!')
  		focus = 0
  		talk_start = 0
  	end
  end
 
 
  function onCreatureChangeOutfit(creature)
 
  end
 
 
  function onThink()
  	if (os.clock() - talk_start) > 30 then
  		if focus > 0 then
  			selfSay('Go away.')
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
 
