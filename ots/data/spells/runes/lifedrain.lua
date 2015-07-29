area = {
{0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 1, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0}
}

attackType = ATTACK_FIRE
needDirection = false
areaEffect = NM_ME_MAGIC_BLOOD
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_MAGIC_BLOOD
damageEffect = NM_ME_MAGIC_BLOOD
animationColor = RED
offensive = true
drawblood = false

GreatFireballObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

GreatFireballObject.minDmg = 30
GreatFireballObject.maxDmg = 60

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, GreatFireballObject:ordered())
end
