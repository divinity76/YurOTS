area = {
    {0, 1, 0},
    {1, 1, 1},
    {0, 1, 0}
    }
    attackType = ATTACK_PHYSICAL
    needDirection = false
    areaEffect = NM_ME_EXPLOSION_AREA
    animationEffect = NM_ANI_FIRE
    
    hitEffect = NM_ME_EXPLOSION_DAMAGE
    damageEffect = NM_ME_DRAW_BLOOD
    animationColor = RED
    offensive = true
    drawblood = true
    
    
    HealFriendObject = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)
    
    function onCast(cid, creaturePos, level, maglv, var)
    centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z} targetpos = getPosition(var)
    HealFriendObject.minDmg = (level * 2 + maglv * 3) * 2
    HealFriendObject.maxDmg = (level * 2 + maglv * 3) * 3.5
    
    if targetpos.x ~= nil and targetpos.z ~= nil and targetpos.y ~= nil then
    	if math.abs(targetpos.x - centerpos.x) < 18 and math.abs(targetpos.y - centerpos.y) < 14 and targetpos.z == centerpos.z then
    		return doTargetMagic(cid, targetpos, HealFriendObject:ordered())
    	end
    end
    
    return false
    end  
    
