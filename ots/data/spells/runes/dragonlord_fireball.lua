area = {
{0, 0, 0, 0, 0, 0, 0},
{0, 0, 1, 1, 1, 0, 0},
{0, 1, 1, 1, 1, 1, 0},
{0, 1, 1, 1, 1, 1, 0},
{0, 1, 1, 1, 1, 1, 0},
{0, 0, 1, 1, 1, 0, 0},
{0, 0, 0, 0, 0, 0, 0}
}

attackType = ATTACK_FIRE
needDirection = false
areaEffect = NM_ME_FIRE_AREA
animationEffect = NM_ANI_FIRE

hitEffect = NM_ME_FIRE_AREA
damageEffect = NM_ME_HITBY_FIRE
animationColor = FIRE
offensive = true
drawblood = false

GreatFireballObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

GreatFireballObject.minDmg = 95
GreatFireballObject.maxDmg = 175

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, GreatFireballObject:ordered())
end
