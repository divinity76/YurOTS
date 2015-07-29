-- fluids by atolon --
-- fluids fixed by Shampo --

   function onUse(cid, item, frompos, item2, topos)
   	-- itemid means that is a creature
if frompos.x == 65535 then
	if item2.itemid == 1 then
   		if item.type == 0 then
   			doPlayerSendCancel(cid,"It is empty.")
   		else
   			if item2.uid == cid then
   				doChangeTypeItem(item.uid,0)
   				if item.type == 2 then
   					doPlayerSay(cid,"it was blood....",16)
   				elseif item.type == 4 then
   					doPlayerSay(cid,"Yuk! slime!!",16)
                                           doSendMagicEffect(topos,8)
   				elseif item.type == 3 then
   					doPlayerSay(cid,"hit! hit! fresh beer!!",16)
   				elseif item.type == 5 then
   					doPlayerSay(cid,"it was fresh lemonade!!",16)
   				elseif item.type == 11 then
   					doPlayerSay(cid,"arrgh is oil!!",16)
   				elseif item.type == 15 then
   					doPlayerSay(cid,"hit! hit! is wine",16)
   				elseif item.type == 6 then
   					doPlayerSay(cid,"oh!, yummy milk!",16)
   				elseif item.type == 10 then
   					doPlayerAddHealth(cid,100)
                                           doSendMagicEffect(topos,12)
   				elseif item.type == 13 then
   					doPlayerSay(cid,"Eww.. urine!",16)
   				elseif item.type == 7 then
   					doPlayerAddMana(cid,100)
                                           doSendMagicEffect(topos,12)
   					doPlayerSay(cid,"Aaaaah...",1)
   				elseif item.type == 19 then
   					doPlayerSay(cid,"Arrgh its mud!",16)
   				elseif item.type == 26 then
   					doPlayerSay(cid,"HOT!",16)
                                           doSendMagicEffect(topos,6)
   				elseif item.type == 28 then
   					doPlayerSay(cid,"Argh, swamp water!",16)
                                           doSendMagicEffect(topos,8)
   				else
   					doPlayerSay(cid,"Gulp.",1)
   				end
   			else
   				splash = doCreateItem(2025,item.type,topos)
   				doChangeTypeItem(item.uid,0)
   				doDecayItem(splash)
   			end
   		end
   --water--
   	elseif (item2.itemid >= 490 and item2.itemid <= 493) or
   		(item2.itemid >= 618 and item2.itemid <= 629) then
   		doChangeTypeItem(item.uid,1)

   --mud--
   	elseif item2.itemid == 103 then
   		doChangeTypeItem(item.uid,19)
   --lava--
   	elseif (item2.itemid >= 598 and item2.itemid < 712) or item2.itemid == 1509 then
   		doChangeTypeItem(item.uid,26)
   --mud--
   	elseif (item2.itemid >= 351 and item2.itemid <= 355) then
   		doChangeTypeItem(item.uid,19)

   --swamp--
   	elseif (item2.itemid >= 602 and item2.itemid <= 605) then
   		doChangeTypeItem(item.uid,28)
   --cask--
   	elseif item2.itemid == 1771 then
   		doChangeTypeItem(item.uid,1)  --water--
   	elseif item2.itemid == 1772 then
   		doChangeTypeItem(item.uid,3)  --beer--
   	elseif item2.itemid == 1773 then
   		doChangeTypeItem(item.uid,15) --wine--

   --end cask--

   -- Blood/swamp in decayto corpse --NO FINISH--

   	elseif item2.itemid > 3922 and item2.itemid < 4327 then
   		doChangeTypeItem(item.uid,2)

   -- End Blood/swamp in decayto corpse --NO FINISH--

   	else
   		if item.type == 0 then
   			doPlayerSendCancel(cid,"It is empty.")
   		else
   			splash = doCreateItem(2025,item.type,topos)
   			doChangeTypeItem(item.uid,0)
   			doDecayItem(splash)
   		end
   	end
else
doPlayerSendCancel(cid, "Put it inside your inventory first.")
return 1
end

   	return 1
end