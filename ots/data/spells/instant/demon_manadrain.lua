area = {
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
{0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0},
{0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
}

attackType = ATTACK_MANADRAIN
needDirection = false
areaEffect = NM_ME_NONE
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_LOOSE_ENERGY
damageEffect = NM_ME_LOOSE_ENERGY
animationColor = DARK_BLUE
offensive = true
drawblood = false

DemonManaDrain = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
	centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

	DemonManaDrain.minDmg = 55
	DemonManaDrain.maxDmg = 125

	return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, DemonManaDrain:ordered())
end
