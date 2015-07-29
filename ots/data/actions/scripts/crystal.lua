function onUse(cid, item, frompos, item2, topos)
  	if doRemoveItem(item.uid,1) then
  		doPlayerSendTextMessage(cid,22,"You have changed 1 crystal coin to 100 platinum coins")
  		doPlayerAddItem(cid,2152,100)
  	end
  end
