area = {
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
 {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
 {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 }
 
 attackType = NM_ME_BLOCKHIT
 needDirection = false
 areaEffect = NM_ME_BLOCKHIT
 animationEffect = NM_ANI_NONE
 
 hitEffect = NM_ME_NONE
 damageEffect = NM_ME_BLOCKHIT
 animationColor = RED
 offensive = true
 drawblood = true
 
 UltimateExplosionObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)
 
 function onCast(cid, creaturePos, level, maglv, var)
 centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
 n = tonumber(var)   -- try to convert it to a number
 if n ~= nil then
 	-- bugged
 	-- ultimateExplosionObject.minDmg = var+0
 	-- UltimateExplosionObject.maxDmg = var+0
 
 	UltimateExplosionObject.minDmg = 0
 	UltimateExplosionObject.maxDmg = 0 
 else
 	UltimateExplosionObject.minDmg = 170
 	UltimateExplosionObject.maxDmg = 350 	
 end 
 
 return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, UltimateExplosionObject:ordered())
 end  
