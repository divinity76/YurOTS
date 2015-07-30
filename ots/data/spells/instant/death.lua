Fireball_area = {
 {0, 1, 0},
 {1, 0, 1},
 {0, 1, 0}
 }
 
 Fireball_attackType = 0--ATTACK_PHYSICAL
 Fireball_needDirection = false
 Fireball_areaEffect = 0 --NM_ME_EXPLOSION_AREA
 Fireball_animationEffect = NM_ANI_FIRE
 
 Fireball_hitEffect = NM_ME_EXPLOSION_DAMAGE
 Fireball_damageEffect = NM_ME_DRAW_BLOOD
 Fireball_animationColor = RED
 Fireball_offensive = true
 Fireball_drawblood = true
 
 Fireball_ExplosionObject = MagicDamageObject(Fireball_attackType, Fireball_animationEffect, Fireball_hitEffect, Fireball_damageEffect, Fireball_animationColor, Fireball_offensive, Fireball_drawblood, 0, 0)
 
 function onCast(cid, creaturePos, level, maglv, var)
 Fireball_centerpos = {x=creaturePos.x+3, y=creaturePos.y, z=creaturePos.z}
 
 Fireball_ExplosionObject.minDmg = (level * 2 + maglv *3) * 0.5
 Fireball_ExplosionObject.maxDmg = (level * 2 + maglv *3) * 1.1
 
 return doAreaMagic(cid, Fireball_centerpos, Fireball_needDirection, Fireball_areaEffect, Fireball_area, Fireball_ExplosionObject:ordered())
 end
