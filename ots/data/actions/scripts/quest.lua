-- simple quests based on uniqueId
-- to make quest create chest on map and set its uniqueId to id of quest item

function onUse(cid, item, frompos, item2, topos)
	prize = item.uid

	if prize > 1000 and prize < 5000 then
		queststatus = getPlayerStorageValue(cid,prize)

		if queststatus == -1 then
			doPlayerSendTextMessage(cid,22,'You have found a ' .. getItemName(prize) .. '.')
			doPlayerAddItem(cid,prize,1)
			setPlayerStorageValue(cid,prize,1)
		else
			doPlayerSendTextMessage(cid,22,"It is empty.")
		end

		return 1
	else
		return 0
	end
end
