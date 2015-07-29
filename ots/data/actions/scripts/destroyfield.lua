function onUse(cid, item, frompos, item2, topos)
 	fieldpos = topos
 	fieldpos.stackpos = 254
 	fielditem = getThingfromPos(fieldpos)
 
 	if getPlayerMagLevel(cid) >= 3 then
 		if fielditem.itemid > 0 and fielditem.itemid ~= 1497 and fielditem.itemid ~= 1498 then
 			doSendMagicEffect(topos,2)
 			doRemoveItem(fielditem.uid,1)
 
 			if item.type > 1 then
 				doChangeTypeItem(item.uid,item.type-1)
 			else
 				doRemoveItem(item.uid,1)
 			end
 		else
 			doSendMagicEffect(frompos,2)
 			return 0
 		end
 	else
 		doSendMagicEffect(frompos,2)
 		doPlayerSendCancel(cid,"You don't have the required magic level to use that rune.")
 	end
 	return 1
 end
