--Door Script Edited by Danielo--
function onUse(cid, item, frompos, item2, topos)

doorpos = {x=topos.x, y=topos.y, z=topos.z, stackpos=253}	-- mod by Yurez
doorplayer = getThingfromPos(doorpos)

if doorplayer.itemid > 0 then
	doPlayerSendCancel(cid,"Someone is blocking the door.")
	return 1
end

if item.itemid == 1209 then
if item.actionid == 0 then
doTransformItem(item.uid,1211)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1211 then
doTransformItem(item.uid,1209)
elseif item.itemid == 1250 then
if item.actionid == 0 then
doTransformItem(item.uid,1251)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1251 then
doTransformItem(item.uid,1250)
elseif item.itemid == 1232 then
if item.actionid == 0 then
doTransformItem(item.uid,1233)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1232 then
doTransformItem(item.uid,1233)
elseif item.itemid == 1253 then
if item.actionid == 0 then
doTransformItem(item.uid,1254)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1254 then
doTransformItem(item.uid,1253)

elseif item.itemid == 1235 then
if item.actionid == 0 then
doTransformItem(item.uid,1236)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1236 then
doTransformItem(item.uid,1235)

elseif item.itemid == 1212 then
if item.actionid == 0 then
doTransformItem(item.uid,1214)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1214 then
doTransformItem(item.uid,1212)

elseif item.itemid == 1219 then
if item.actionid == 0 then
doTransformItem(item.uid,1220)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1220 then
doTransformItem(item.uid,1219)

elseif item.itemid == 1221 then
if item.actionid == 0 then
doTransformItem(item.uid,1222)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1222 then
doTransformItem(item.uid,1221)

elseif item.itemid == 1223 then
if item.actionid == 0 then
doTransformItem(item.uid,1224)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1224 then
doTransformItem(item.uid,1223)
----
elseif item.itemid == 1225 then
if item.actionid == 0 then
doTransformItem(item.uid,1226)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end


-----
elseif item.itemid == 1226 then
doTransformItem(item.uid,1225)

elseif item.itemid == 1227 then
if item.actionid == 0 then
doTransformItem(item.uid,1228)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1228 then
doTransformItem(item.uid,1227)

elseif item.itemid == 1229 then
if item.actionid == 0 then
doTransformItem(item.uid,1230)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1230 then
doTransformItem(item.uid,1229)

elseif item.itemid == 1231 then
if item.actionid == 0 then
doTransformItem(item.uid,1233)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1233 then
doTransformItem(item.uid,1231)

elseif item.itemid == 1234 then
if item.actionid == 0 then
doTransformItem(item.uid,1236)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1236 then
doTransformItem(item.uid,1234)

elseif item.itemid == 1237 then
if item.actionid == 0 then
doTransformItem(item.uid,1238)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1238 then
doTransformItem(item.uid,1237)

elseif item.itemid == 1239 then
if item.actionid == 0 then
doTransformItem(item.uid,1240)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1240 then
doTransformItem(item.uid,1239)

elseif item.itemid == 1241 then
if item.actionid == 0 then
doTransformItem(item.uid,1242)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1242 then
doTransformItem(item.uid,1241)

elseif item.itemid == 1243 then
if item.actionid == 0 then
doTransformItem(item.uid,1244)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1244 then
doTransformItem(item.uid,1243)

elseif item.itemid == 1245 then
if item.actionid == 0 then
doTransformItem(item.uid,1246)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1246 then
doTransformItem(item.uid,1245)

elseif item.itemid == 1247 then
if item.actionid == 0 then
doTransformItem(item.uid,1248)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1248 then
doTransformItem(item.uid,1247)

elseif item.itemid == 1249 then
if item.actionid == 0 then
doTransformItem(item.uid,1251)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1251 then
doTransformItem(item.uid,1249)

elseif item.itemid == 1252 then
if item.actionid == 0 then
doTransformItem(item.uid,1254)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1254 then
doTransformItem(item.uid,1252)

elseif item.itemid == 1255 then
if item.actionid == 0 then
doTransformItem(item.uid,1256)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1256 then
doTransformItem(item.uid,1255)

elseif item.itemid == 1257 then
if item.actionid == 0 then
doTransformItem(item.uid,1258)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1258 then
doTransformItem(item.uid,1257)

elseif item.itemid == 1259 then
if item.actionid == 0 then
doTransformItem(item.uid,1260)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1260 then
doTransformItem(item.uid,1259)

elseif item.itemid == 1261 then
if item.actionid == 0 then
doTransformItem(item.uid,1262)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1262 then
doTransformItem(item.uid,1261)

--another simple door--
elseif item.itemid == 1634 then
if item.actionid == 0 then
doTransformItem(item.uid,1635)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1635 then
doTransformItem(item.uid,1634)

elseif item.itemid == 1636 then
if item.actionid == 0 then
doTransformItem(item.uid,1637)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1637 then
doTransformItem(item.uid,1636)

--another simple door--

elseif item.itemid == 1638 then
if item.actionid == 0 then
doTransformItem(item.uid,1639)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1639 then
doTransformItem(item.uid,1638)

elseif item.itemid == 5082 then
if item.actionid == 0 then
doTransformItem(item.uid,5083)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 5083 then
doTransformItem(item.uid,5082)

elseif item.itemid == 5084 then
if item.actionid == 0 then
doTransformItem(item.uid,5085)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 5085 then
doTransformItem(item.uid,5084)

elseif item.itemid == 4913 then
if item.actionid == 0 then
doTransformItem(item.uid,4914)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 4914 then
doTransformItem(item.uid,4913)

elseif item.itemid == 4915 then
if item.actionid == 0 then
doTransformItem(item.uid,4916)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 4916 then
doTransformItem(item.uid,4915)

elseif item.itemid == 3535 then
if item.actionid == 0 then
doTransformItem(item.uid,3537)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 3537 then
doTransformItem(item.uid,3535)

elseif item.itemid == 3536 then
if item.actionid == 0 then
doTransformItem(item.uid,3537)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 3537 then
doTransformItem(item.uid,3536)

elseif item.itemid == 3538 then
if item.actionid == 0 then
doTransformItem(item.uid,3539)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 3539 then
doTransformItem(item.uid,3538)

elseif item.itemid == 3540 then
if item.actionid == 0 then
doTransformItem(item.uid,3541)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 3541 then
doTransformItem(item.uid,3540)

elseif item.itemid == 3542 then
if item.actionid == 0 then
doTransformItem(item.uid,3543)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 3543 then
doTransformItem(item.uid,3542)

elseif item.itemid == 3544 then
if item.actionid == 0 then
doTransformItem(item.uid,3546)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 3546 then
doTransformItem(item.uid,3544)

elseif item.itemid == 3545 then
if item.actionid == 0 then
doTransformItem(item.uid,3546)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 3546 then
doTransformItem(item.uid,3545)

elseif item.itemid == 3547 then
if item.actionid == 0 then
doTransformItem(item.uid,3548)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 3548 then
doTransformItem(item.uid,3547)

elseif item.itemid == 3549 then
if item.actionid == 0 then
doTransformItem(item.uid,3550)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 3550 then
doTransformItem(item.uid,3549)

elseif item.itemid == 3551 then
if item.actionid == 0 then
doTransformItem(item.uid,3552)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 3552 then
doTransformItem(item.uid,3551)

elseif item.itemid == 1640 then
if item.actionid == 0 then
doTransformItem(item.uid,1641)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1641 then
doTransformItem(item.uid,1640)

elseif item.itemid == 1213 then
if item.actionid == 0 then
doTransformItem(item.uid,1214)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1214 then
doTransformItem(item.uid,1213)

elseif item.itemid == 1210 then
if item.actionid == 0 then
doTransformItem(item.uid,1211)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1211 then
doTransformItem(item.uid,1210)

elseif item.itemid == 1539 then
if item.actionid == 0 then
doTransformItem(item.uid,1540)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1540 then
doTransformItem(item.uid,1539)

elseif item.itemid == 1541 then
if item.actionid == 0 then
doTransformItem(item.uid,1542)
else
doPlayerSendTextMessage(cid,22,"It is locked.")
end
elseif item.itemid == 1542 then
doTransformItem(item.uid,1541)
else
return 0
end
return 1
end
