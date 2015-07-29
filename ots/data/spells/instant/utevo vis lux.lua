attackType = ATTACK_NONE
 animationEffect = NM_ANI_NONE
 hitEffect = NM_ME_NONE
 damageEffect = NM_ME_MAGIC_ENERGIE
 animationColor = GREEN
 offensive = false
 drawblood = false
 
 EnergieObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)
 
 function onCast(cid, creaturePos, level, maglv, var)
 	centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
 	ret = doTargetMagic(cid, centerpos, EnergieObject:ordered())
 
 	if (ret) then
 		setPlayerLightLevel(cid,11)
 	end
 
 	return ret
 end
