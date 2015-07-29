-- Construction kits by JTE --
-- Bugfixes by Alreth --

function onUse(cid, item, frompos, item2, topos)
	if frompos.x == 65535 then
		doPlayerSendCancel(cid, "Put the construction kit on the ground first.")
		return 1
	end

	doSendMagicEffect(topos,2)	-- puff

	if item.itemid == 3901 then			-- Wooden chair
		doTransformItem(item.uid,1652)
	elseif item.itemid == 3902 then		-- Sofa chair
		doTransformItem(item.uid,1658)
	elseif item.itemid == 3903 then		-- Red cushioned chair
		doTransformItem(item.uid,1666)
	elseif item.itemid == 3904 then		-- Green cushioned chair
		doTransformItem(item.uid,1670)
	elseif item.itemid == 3905 then		-- Tusk chair
		doTransformItem(item.uid,3813)
	elseif item.itemid == 3906 then		-- Ivory chair
		doTransformItem(item.uid,3817)
	elseif item.itemid == 3908 then		-- Coal basin
		doTransformItem(item.uid,2602)
	elseif item.itemid == 3909 then		-- Big table
		doTransformItem(item.uid,1614)
	elseif item.itemid == 3910 then		-- Square table
		doTransformItem(item.uid,1615)
	elseif item.itemid == 3911 then		-- Round table
		doTransformItem(item.uid,1616)
	elseif item.itemid == 3912 then		-- Small table
		doTransformItem(item.uid,1619)
	elseif item.itemid == 3913 then		-- Stone table
		doTransformItem(item.uid,3805)
	elseif item.itemid == 3914 then		-- Tusk table
		doTransformItem(item.uid,3807)
	elseif item.itemid == 3917 then		-- Harp
		doTransformItem(item.uid,2084)
	elseif item.itemid == 3918 then		-- Birdcage
		doTransformItem(item.uid,2095)
	elseif item.itemid == 3919 then		-- Bamboo table
		doTransformItem(item.uid,3809)
	elseif item.itemid == 3926 then		-- Piano
		doTransformItem(item.uid,2080)
	elseif item.itemid == 3927 then		-- Globe
		doTransformItem(item.uid,2098)
	elseif item.itemid == 3928 then		-- Potted flower (pink one)
		doTransformItem(item.uid,2104)
	elseif item.itemid == 3929 then		-- Potted flower (the boring green one)
		doTransformItem(item.uid,2101)
	elseif item.itemid == 3931 then		-- Christmas tree
		doTransformItem(item.uid,2105)
	elseif item.itemid == 3932 then		-- Dresser
		doRemoveItem(item.uid,item.type)
		doCreateItem(1724,1,frompos)
	elseif item.itemid == 3933 then		-- Pendelum clock
		doTransformItem(item.uid,1728)
	elseif item.itemid == 3935 then		-- Trough
		doTransformItem(item.uid,1775)
	elseif item.itemid == 3937 then		-- Table lamp
		doTransformItem(item.uid,2064)
	-- containers
	elseif item.itemid == 3907 then		-- Small trunk
		doRemoveItem(item.uid,item.type)
		doCreateItem(3821,1,frompos)
	elseif item.itemid == 3915 then		-- Box
		doRemoveItem(item.uid,item.type)
		doCreateItem(1738,1,frompos)
	elseif item.itemid == 3920 then		-- Thick trunk
		doRemoveItem(item.uid,item.type)
		doCreateItem(3811,1,frompos)
	elseif item.itemid == 3921 then		-- Drawer
		doRemoveItem(item.uid,item.type)
		doCreateItem(1716,1,frompos)
	elseif item.itemid == 3923 then		-- Barrel
		doRemoveItem(item.uid,item.type)
		doCreateItem(1774,1,frompos)
	elseif item.itemid == 3934 then		-- Locker
		doRemoveItem(item.uid,item.type)
		doCreateItem(1732,1,frompos)
	elseif item.itemid == 3936 then		-- Bamboo dresser
		doRemoveItem(item.uid,item.type)
		doCreateItem(3832,1,frompos)
	elseif item.itemid == 3938 then		-- Large trunk
		doRemoveItem(item.uid,item.type)
		doCreateItem(1750,1,frompos)
	else
		return 0
	end
	return 1
end
