area = {
{1, 1, 1},
{1, 1, 1},
{1, 1, 1}
}

attackType = ATTACK_PHYSICAL
areaEffect = NM_ME_MORT_AREA
animationEffect = NM_ANI_SUDDENDEATH

hitEffect = NM_ME_MORT_AREA
damageEffect = NM_ME_MORT_AREA
animationColor = 255
offensive = true
drawblood = true

ExplosionObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}

ExplosionObject.minDmg = 225
ExplosionObject.maxDmg = 350

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, ExplosionObject:ordered())
end
