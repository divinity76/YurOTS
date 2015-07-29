function onUse(cid, item, frompos, item2, topos)

   	-- from main to island
   	if item.uid == 7002 then
   		queststatus = getPlayerStorageValue(cid,5019)

   		if queststatus == -1 then
   			doPlayerSendTextMessage(cid,22,"You don't know how to steer.")
   		else			gopos = {x=188, y=150, z=7}
   			doTeleportThing(cid, gopos)		end
   	-- from island to main
   	elseif item.uid == 7003 then
   		backpos = {x=236, y=126, z=7}
   		doTeleportThing(cid, backpos)

	else
		return 0
   	end

   	return 1
end
