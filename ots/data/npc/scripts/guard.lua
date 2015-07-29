-- the id of the creature we are attacking, following, etc.
 
  target = 0
  following = false
  attacking = false
 
  function onThingMove(creature, thing, oldpos, oldstackpos)
 
  end
 
 
  function onCreatureAppear(creature)
 
  end
 
 
  function onCreatureDisappear(cid, pos)
 
  end
 
 
  function onCreatureTurn(creature)
 
  end
 
 
  function onCreatureSay(cid, type, msg)
  	msg = string.lower(msg)
 
  	if (string.find(msg, '(%a*)hi(%a*)')) and getDistanceToCreature(cid) < 4 then
  		selfSay('You do not talk about Fight Club.')
  	end
  end
 
 
  function onCreatureChangeOutfit(creature)
 
  end
 
 
  function onThink()
 
  end
 
