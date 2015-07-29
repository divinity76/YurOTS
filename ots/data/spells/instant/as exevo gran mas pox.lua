area = {
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
{0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
{0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
{0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0},
{0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
{0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
{0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
}

attackType = ATTACK_POISON
needDirection = false
areaEffect = NM_ME_POISONCLOUD
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_POISEN
damageEffect = NM_ME_POISEN_RINGS
animationColor = GREEN
offensive = true
needDirection = false
drawblood = false
minDmg = 21
maxDmg = 23

PoisonStormObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, minDmg, maxDmg)
SubPoisonStormObject1 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 20, 20)
SubPoisonStormObject2 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 19, 19)
SubPoisonStormObject3 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 18, 18)
SubPoisonStormObject4 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 17, 17)
SubPoisonStormObject5 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 16, 16)
SubPoisonStormObject6 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 15, 15)
SubPoisonStormObject7 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 14, 14)
SubPoisonStormObject8 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 13, 13)
SubPoisonStormObject9 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 12, 12)
SubPoisonStormObject10 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 11, 11)
SubPoisonStormObject11 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 10, 10)
SubPoisonStormObject12 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 5, 5)
SubPoisonStormObject13 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 4, 4)
SubPoisonStormObject14 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 3, 3)
SubPoisonStormObject15 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 2, 2)
SubPoisonStormObject16 = MagicDamageObject(attackType, NM_ANI_NONE, NM_ME_NONE, damageEffect, animationColor, offensive, drawblood, 1, 1)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

return doAreaExMagic(cid, centerpos, needDirection, areaEffect, area, PoisonStormObject:ordered(),
	2000, 1, SubPoisonStormObject1:ordered(),
	2000, 1, SubPoisonStormObject2:ordered(),
	2000, 1, SubPoisonStormObject3:ordered(),
	2000, 1, SubPoisonStormObject4:ordered(),
	2000, 1, SubPoisonStormObject5:ordered(),
	2000, 1, SubPoisonStormObject6:ordered(),
	2000, 1, SubPoisonStormObject7:ordered(),
	2000, 1, SubPoisonStormObject8:ordered(),
	2000, 1, SubPoisonStormObject9:ordered(),
	2000, 1, SubPoisonStormObject10:ordered(),
	2000, 1, SubPoisonStormObject11:ordered(),
	2000, 3, SubPoisonStormObject12:ordered(),
	2000, 5, SubPoisonStormObject13:ordered(),
	2000, 7, SubPoisonStormObject14:ordered(),
	2000, 9, SubPoisonStormObject15:ordered(),
	2000, 11, SubPoisonStormObject16:ordered(),
	16)
end  

