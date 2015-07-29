area = {
 {0, 0, 0},
 {0, 1, 0},
 {0, 0, 0}
 }
 
 attackType = NONE
 animationEffect = NM_ME_NONE
 
 hitEffect = NM_ME_PUFF
 damageEffect = NM_ME_PUFF
 animationColor = BLACK_WHITE
 offensive = false
 drawblood = false
 
 HeavyMagicMissileObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)
 
 function onCast(cid, creaturePos, level, maglv, var)
 centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
 
 HeavyMagicMissileObject.minDmg = 0
 HeavyMagicMissileObject.maxDmg = 0
 
 return doTargetMagic(cid, centerpos, HeavyMagicMissileObject:ordered())
 end
