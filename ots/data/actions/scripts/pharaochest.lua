-- pharaon quest chests

function onUse(cid, item, frompos, item2, topos)

   	if item.uid == 5003 then
   		queststatus = getPlayerStorageValue(cid,5003)
   		if queststatus == -1 then
			doPlayerSendTextMessage(cid,22,"You have found a Dragon Lance.")
			doPlayerAddItem(cid,2414,1)
			setPlayerStorageValue(cid,5003,1)
   		else
   			doPlayerSendTextMessage(cid,22,"It is empty.")
   		end
   	elseif item.uid == 5004 then
   		queststatus = getPlayerStorageValue(cid,5003)
   		if queststatus == -1 then
			doPlayerSendTextMessage(cid,22,"You have found a War Hammer.")
			doPlayerAddItem(cid,2391,1)
			setPlayerStorageValue(cid,5003,1)
   		else
   			doPlayerSendTextMessage(cid,22,"It is empty.")
   		end	elseif item.uid == 5017 then
   		queststatus = getPlayerStorageValue(cid,5003)
   		if queststatus == -1 then
			doPlayerSendTextMessage(cid,22,"You have found a Giant Sword.")
			doPlayerAddItem(cid,2393,1)
			setPlayerStorageValue(cid,5003,1)
   		else
   			doPlayerSendTextMessage(cid,22,"It is empty.")
   		end
	else
		return 0
   	end

   	return 1
end
