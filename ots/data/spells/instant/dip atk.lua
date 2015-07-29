area = {
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
{0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
{0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
{0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0},
{0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0},
{0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0},
{0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
}

attackType = ATTACK_DRUNKNESS
needDirection = false
areaEffect = NM_ME_LOOSE_ENERGY
animationEffect = NM_ANI_NONE

hitEffect = NM_ME_NONE
damageEffect = NM_ME_NONE
animationColor = ENERGY
offensive = true
drawblood = true

DiptrahDrunkWave = MagicDamageObject(attackType, animationEffect, hitEffect, damageEffect, animationColor, offensive, drawblood, 0, 0)

function onCast(cid, creaturePos, level, maglv, var)
centerpos = {x=creaturePos.x, y=creaturePos.y, z=creaturePos.z}
n = tonumber(var)   -- try to convert it to a number
if n ~= nil then
	-- bugged
	-- DiptrahDrunkWave.minDmg = var+0
	-- DiptrahDrunkWave.maxDmg = var+0

	DiptrahDrunkWave.minDmg = 0
	DiptrahDrunkWave.maxDmg = 0 
else
	DiptrahDrunkWave.minDmg = 0
	DiptrahDrunkWave.maxDmg = 1 	
end 

return doAreaMagic(cid, centerpos, needDirection, areaEffect, area, DiptrahDrunkWave:ordered())
end  
