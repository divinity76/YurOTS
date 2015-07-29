 -- Diging up scarabs, crabs and scarab coins from sand!
 -- Now they will attack the one digged them out
 -- By Roman edited by Shampo for Utopia 7.6
 -- To get it to working replace this code with your old shovel lua
 function onUse(cid, item, frompos, item2, topos)
 pos = getPlayerPosition(cid)
 if item2.itemid == 0 then
  return 0
 end
 if item2.itemid == 468 then
  doTransformItem(item2.uid,469)
  doDecayItem(item2.uid)
 elseif item2.itemid == 481 then
  doTransformItem(item2.uid,482)
  doDecayItem(item2.uid)
 elseif item2.itemid == 483 then
  doTransformItem(item2.uid,484)
  doDecayItem(item2.uid)
 elseif item2.itemid == 231 then
  rand = math.random(1,30)
  if rand < 6 then
   doSummonCreature("Scarab", topos)
   doTeleportThing(cid,topos)
   doTeleportThing(cid,pos)
  elseif rand == 6 then
   doSummonCreature("Crab", topos)
   doTeleportThing(cid,topos)
   doTeleportThing(cid,pos)
  elseif rand == 15 then
   doPlayerAddItem(cid,2159,1)
  else
   doSendMagicEffect(topos,2)
  end
 else
  return 0
 end
 return 1
end
