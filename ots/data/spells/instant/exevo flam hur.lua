
 area = {
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 2, 2, 2, 0, 0, 1, 0, 0, 3, 3, 3, 0, 0, 0},
 {0, 0, 0, 2, 2, 2, 2, 2, 0, 3, 3, 3, 3, 3, 0, 0, 0},
 {0, 0, 0, 2, 2, 2, 0, 0, 4, 0, 0, 3, 3, 3, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0},
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
 }
 
 -- area = {
    -- {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    -- {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    -- {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
    -- {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    -- {2, 2, 2, 0, 0, 1, 0, 0, 3, 3, 3},
    -- {2, 2, 2, 2, 2, 0, 3, 3, 3, 3, 3},
    -- {2, 2, 2, 0, 0, 4, 0, 0, 3, 3, 3},
    -- {0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0},
    -- {0, 0, 0, 0, 4, 4, 4, 0, 0, 0, 0},
    -- {0, 0, 0, 0, 4, 4, 4, 0, 0, 0, 0},
    -- {0, 0, 0, 0, 4, 4, 4, 0, 0, 0, 0}
    -- }
 
    attackType = ATTACK_FIRE
    needDirection = true
    areaEffect = NM_ME_FIRE_AREA
    animationEffect = NM_ANI_FIRE
 
    hitEffect = NM_ME_FIRE_AREA
    damageEffect = NM_ME_HITBY_FIRE
    animationColor = RED
    offensive = true
    drawblood = true
 
    EnergyWaveObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)
 
    function onCast(cid, creaturePos, level, maglv, var)
    centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
    EnergyWaveObject.minDmg = (level * 2 + maglv * 3) * 1.3
    EnergyWaveObject.maxDmg = (level * 2 + maglv * 3) * 1.7
 
    return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, EnergyWaveObject:ordered())
    end
 
