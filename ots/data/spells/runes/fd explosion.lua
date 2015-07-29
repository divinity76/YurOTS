area = {
{0, 1, 0},
{1, 1, 1},
{0, 1, 0}
}

attackType = ATTACK_FIRE
needDirection = false
areaEffect = NM_ME_FIRE_AREA
animationEffect = NM_ANI_FIRE

hitEffect = NM_ME_NONE
damageEffect = NM_ME_FIRE_AREA
animationColor = FIRE
offensive = true
drawblood = true

ExplosionObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

ExplosionObject.minDmg = 10
ExplosionObject.maxDmg = 50

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, ExplosionObject:ordered())
end
