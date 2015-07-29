area = {
{0, 0, 0},
{0, 1, 0},
{0, 0, 0}
}

attackType = ATTACK_POISON
needDirection = false
areaEffect = NM_ME_POISEN_RINGS
animationEffect = NM_ANI_POISONARROW

hitEffect = NM_ME_POISEN_RINGS
damageEffect = NM_ME_POISEN_RINGS
animationColor = GREEN
offensive = true
drawblood = false
minDmg = 20
maxDmg = 20

FireBombObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)
SubFireBombObject1 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, minDmg, maxDmg)
SubFireBombObject2 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 10, 10)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

return doAreaGroundMagic(cid, centerpos, needDirection, areaEffect, area, FireBombObject:ordered(),
	0, 1, SubFireBombObject1:ordered(),
	5000, 1, SubFireBombObject2:ordered(),
	2, 60000, 1490,
	5000, 6, SubFireBombObject2:ordered(),
	1, 60000, 1490,
	0, 25000, 1490, 3)
end
