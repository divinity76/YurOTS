focus = 0
talk_start = 0
target = 0
following = false
attacking = false
talk_state = 0
gstat = 0		-- guild status
grank = ''		-- guild rank
gname = ''		-- guild name
cname = ''		-- name of player who talks to us
pname = ''		-- name of some other player
maxnamelen = 30
maxranklen = 20
maxnicklen = 20
leaderlevel = 50
NONE = 0
INVITED = 1
MEMBER = 2
VICE = 3
LEADER = 4
allow_pattern = '^[a-zA-Z0-9 -]+$'

function onThingMove(creature, thing, oldpos, oldstackpos)

end


function onCreatureAppear(creature)

end


function onCreatureDisappear(cid, pos)
  	if focus == cid then
          selfSay('Good bye then.')
          focus = 0
          talk_start = 0
  	end
end


function onCreatureTurn(creature)

end
function msgcontains(txt, str)
  	return (string.find(txt, str) and not string.find(txt, '(%w+)' .. str) and not string.find(txt, str .. '(%w+)'))
end


function onCreatureSay(cid, type, msg)
  	cname = creatureGetName(cid)

  	if (msgcontains(msg, 'hi') and (focus == 0)) and getDistanceToCreature(cid) < 4 then
  		selfSay('Hello ' .. cname .. '! How can I help you?')
  		talk_state = 0
  		focus = cid
  		talk_start = os.clock()	elseif msgcontains(msg, 'hi') and (focus ~= cid) and getDistanceToCreature(cid) < 4 then
  		selfSay('Sorry, ' .. cname .. '! I talk to you in a minute.')

  	elseif msgcontains(msg, 'bye') and (focus == cid) and getDistanceToCreature(cid) < 4 then
  			selfSay('Good bye, ' .. cname .. '!')
  			talk_state = 0
  			focus = 0
  			talk_start = 0
  	elseif focus == cid then
  		if talk_state == 0 then
  			msg = string.lower(msg)			if msgcontains(msg, 'found') then	-- found a new guild
  				level = getPlayerLevel(cname)

  				if level >= leaderlevel then
  					gstat = getPlayerGuildStatus(cname)

  					if gstat == NONE or gstat == INVITED then
  						selfSay('What name your guild should have?')
  						talk_state = 1
  					elseif gstat == MEMBER or gstat == VICE or gstat == LEADER then
  						selfSay('Sorry, you are member of a guild.')
  						talk_state = 0
  					end
  				else
  					selfSay('Sorry, you need level ' .. leaderlevel .. ' to found a guild.')
  				end

  				talk_start = os.clock()

  			elseif msgcontains(msg, 'join')  then	-- join a guild when invited
  				gstat = getPlayerGuildStatus(cname)

  				if gstat == NONE then
  					selfSay('Sorry, you are not invited to any guild.')
  					talk_state = 0
  				elseif gstat == INVITED then
  					gname = getPlayerGuildName(cname)
  					selfSay('Do you want to join ' .. gname .. '?')
  					talk_state = 3
  				elseif gstat == MEMBER or gstat == VICE or gstat == LEADER then
  					selfSay('Sorry, you are a member of a guild.')
  					talk_state = 0
  				end

  				talk_start = os.clock()

  			elseif msgcontains(msg, 'exclude') or msgcontains(msg, 'kick') then		-- kick player from a guild
  				gstat = getPlayerGuildStatus(cname)

  				if gstat == VICE or gstat == LEADER then
  					selfSay('Who do you want to kick today?')
  					talk_state = 4
  				else
  					selfSay('Sorry, only leader and vice-leaders can kick players from a guild.')
  					talk_state = 0
  				end

  				talk_start = os.clock()

  			elseif msgcontains(msg, 'invite') then		-- invite player to a guild
  				gstat = getPlayerGuildStatus(cname)

  				if gstat == VICE or gstat == LEADER then
  					selfSay('Who do you want to invite to your guild?')
  					talk_state = 5
  				else
  					selfSay('Sorry, only leader and vice-leaders can invite players to a guild.')
  					talk_state = 0
  				end

  				talk_start = os.clock()

  			elseif msgcontains(msg, 'leave') then		-- leave a guild
  				gstat = getPlayerGuildStatus(cname)

  				if gstat == NONE or gstat == INVITED then
  					selfSay('You are not in a guild.')
  					talk_state = 0
  				elseif gstat == MEMBER or gstat == VICE then
  					gname = getPlayerGuildName(cname)
  					selfSay('Do you want to leave ' .. gname .. '?')
  					talk_state = 7
  				elseif gstat == LEADER then
  					selfSay('You are a leader of a guild. If you leave, no one can invite new players. Are you sure?')
  					talk_state = 7
  				end

  			elseif msgcontains(msg, 'pass') then		-- pass leadership
  				gstat = getPlayerGuildStatus(cname)

  				if gstat == LEADER then
  					selfSay('Who do you want to be a new leader?')
  					talk_state = 8
  				else
  					selfSay('Sorry, only leader can resign from his position.')
  					talk_state = 0
  				end

  			elseif msgcontains(msg, 'vice') then		-- set vice leader
  				gstat = getPlayerGuildStatus(cname)				if gstat == LEADER then					selfSay('Which member do you want to promote to vice-leader?')
  					talk_state = 9
  				else
  					selfSay('Sorry, only leader can promote member to vice-leader.')
  					talk_state = 0
  				end

  			elseif msgcontains(msg, 'member') then		-- remove vice-leader
  				gstat = getPlayerGuildStatus(cname)

  				if gstat == LEADER then
  					selfSay('Which vice-leader do you want to demote to regular member?')
  					talk_state = 10
  				else
  					selfSay('Sorry, only leader can demote vice-leaders to members.')
  					talk_state = 0
  				end

  			elseif msgcontains(msg, 'nick') or msgcontains(msg, 'title') then	-- set nick
  				gstat = getPlayerGuildStatus(cname);

  				if gstat == LEADER then
  					selfSay('Whom player do you want to change nick?')
  					talk_state = 11
  				else
  					selfSay('Sorry, only leader can change nicks.')
  					talk_state = 0
  				end
  			end

  		else	-- talk_state != 0
  			talk_start = os.clock()

  			if talk_state == 1 then		-- get name of new guild
  				gname = msg

  				if string.len(gname) <= maxnamelen then
 					if string.find(gname, allow_pattern) then
 						if foundNewGuild(gname) == 0 then							selfSay('Sorry, there is already a guild with that name.')
 							talk_state = 0
 						else
 							selfSay('And what rank do you wish to have?')
 							talk_state = 2
 						end
 					else
 						selfSay('Sorry, guild name contains illegal characters.')
 						talk_state = 0
 					end
  				else
  					selfSay('Sorry, guild name cannot be longer than ' .. maxnamelen .. ' characters.')
  					talk_state = 0
  				end

  			elseif talk_state == 2 then		-- get rank of leader
  				grank = msg

  				if string.len(grank) <= maxranklen then
 					if string.find(grank, allow_pattern) then
 						setPlayerGuild(cname,LEADER,grank,gname)
 						selfSay('You are now leader of your new guild.')
 						talk_state = 0
 					else						selfSay('Sorry, rank name contains illegal characters.')
 						talk_state = 0
 					end
  				else
  					selfSay('Sorry, rank name cannot be longer than ' .. maxranklen .. ' characters.')
  					talk_state = 0
  				end

  			elseif talk_state == 3 then		-- join a guild
  				if msg == 'yes' then
  					setPlayerGuildStatus(cname, MEMBER)
  					selfSay('You are now member of a guild.')
  					talk_state = 0
  				else
  					selfSay('What else can I do for you?')
  					talk_state = 0
  				end

  			elseif talk_state == 4 then		-- kick player
  				pname = msg
  				gname = getPlayerGuildName(cname)
  				gname2 = getPlayerGuildName(pname)

  				if cname == pname then
  					selfSay('To kick yourself say leave.')
  					talk_state = 0
  				elseif gname == gname2 then
  					gstat2 = getPlayerGuildStatus(pname)

  					if gstat > gstat2 then
  						clearPlayerGuild(pname)
  						selfSay('You kicked ' .. pname .. ' from your guild.')
  						talk_state = 0
  					else
  						selfSay('Sorry, vice-leaders can kick only regular members.')
  						talk_state = 0
  					end
  				else
  					selfSay('Sorry, ' .. pname .. ' is not in your guild.')
  					talk_state = 0
  				end

  			elseif talk_state == 5 then		-- get invited name
  				pname = msg
  				gstat = getPlayerGuildStatus(pname)

  				if gstat == MEMBER or gstat == VICE or gstat == LEADER then
  					selfSay('Sorry, ' .. pname .. ' is in another guild.')
  					talk_state = 0
  				else
  					selfSay('And what rank do you wish to give him/her?')
  					talk_state = 6
  				end

  			elseif talk_state == 6 then		-- get invited rank
  				grank = msg

  				if string.len(grank) <= maxranklen then
 					if string.find(grank, allow_pattern) then
 						gname = getPlayerGuildName(cname)
 						setPlayerGuild(pname, INVITED, grank, gname)
 						selfSay('You have invited ' .. pname .. ' to your guild.')
 						talk_state = 0
 					else
 						selfSay('Sorry, rank name contains illegal characters.')
 						talk_state = 0
 					end
  				else					selfSay('Sorry, rank name cannot be longer than ' .. maxranklen .. ' characters.')
  					talk_state = 0
  				end

  			elseif talk_state == 7 then		-- leave a guild
  				if msg == 'yes' then
  					clearPlayerGuild(cname)
  					selfSay('You have left your guild.')
  					talk_state = 0
  				else					selfSay('What else can I do for you?')
  					talk_state = 0
  				end

  			elseif talk_state == 8 then		-- pass leadership
  				pname = msg
  				level = getPlayerLevel(pname)

  				if level >= leaderlevel then
  					gname = getPlayerGuildName(cname)
  					gname2 = getPlayerGuildName(pname)

  					if gname == gname2 then
  						setPlayerGuildStatus(cname,MEMBER)
  						setPlayerGuildStatus(pname,LEADER)
  						gname = getPlayerGuildName(cname)
  						selfSay(pname .. ' is a new leader of ' .. gname .. '.')
  						talk_state = 0
  					else
  						selfSay('Sorry, ' .. pname .. ' is not in your guild.')
  						talk_state = 0;
  					end
  				else
  					selfSay('Sorry, ' .. pname .. ' is not online.')
  					talk_state = 0
  				end

  			elseif talk_state == 9 then		-- set vice-leader
  				pname = msg
  				gname = getPlayerGuildName(cname)
  				gname2 = getPlayerGuildName(pname)

  				if cname == pname then
  					selfSay('To resign from leadership say pass.')
  					talk_state = 0
  				elseif gname == gname2 then
  					gstat = getPlayerGuildStatus(pname)

  					if gstat == INVITED then						selfSay('Sorry, ' .. pname .. ' hasn\'t joined your guild yet.');						talk_state = 0
  					elseif gstat == VICE then
  						selfSay(pname .. ' is already a vice-leader.')						talk_state = 0
  					elseif gstat == MEMBER then
  						setPlayerGuildStatus(pname, VICE)
  						selfSay(pname .. ' is now a vice-leader of your guild.')						talk_state = 0
  					end
  				else
  					selfSay('Sorry, ' .. pname .. ' is not in your guild.')
  					talk_state = 0
  				end

  			elseif talk_state == 10 then	-- set member
  				pname = msg
  				gname = getPlayerGuildName(cname)
  				gname2 = getPlayerGuildName(pname)

  				if cname == pname then
  					selfSay('To resign from leadership say pass.')
  					talk_state = 0
  				elseif gname == gname2 then
  					gstat = getPlayerGuildStatus(pname)

  					if gstat == INVITED then
  						selfSay('Sorry, ' .. pname .. ' hasn\'t joined your guild yet.');
  						talk_state = 0
  					elseif gstat == VICE then
  						setPlayerGuildStatus(pname, MEMBER)
  						selfSay(pname .. ' is now a regular member of your guild.')
  						talk_state = 0
  					elseif gstat == MEMBER then
  						selfSay(pname .. ' is already a regular member.')
  						talk_state = 0
  					end
  				else
  					selfSay('Sorry, ' .. pname .. ' is not in your guild.')
  					talk_state = 0
  				end

  			elseif talk_state == 11 then	-- get name of player to change nick
  				pname = msg				gname = getPlayerGuildName(cname)
  				gname2 = getPlayerGuildName(pname)

  				if gname == gname2 then
  					selfSay('And what nick do you want him to have (say none to clear)?')
  					talk_state = 12
  				else
  					selfSay('Sorry, ' .. pname .. ' is not in your guild.')
  					talk_state = 0
  				end

  			elseif talk_state == 12 then	-- get nick
  				if msg == 'none' then
  					setPlayerGuildNick(pname, '')
  					selfSay(pname .. ' now has no nick.')
  					talk_state = 0
  				else
  					if string.len(msg) <= maxnicklen then
 						if string.find(msg, allow_pattern) then
 							setPlayerGuildNick(pname, msg)
 							selfSay('You have changed ' .. pname .. '\'s nick.')
 							talk_state = 0
 						else							selfSay('Sorry, nick contains illegal characters.')
 							talk_state = 0
 						end
  					else
  						selfSay('Sorry, nick cannot be longer than ' .. maxnicklen .. ' characters.')
  						talk_state = 0
  					end
  				end
  			end
  		end
  	end
end


function onCreatureChangeOutfit(creature)

end


function onThink()
  	if (os.clock() - talk_start) > 45 then
  		if focus > 0 then
  			selfSay('Next Please...')
  		end
  			focus = 0
  	end
 	if focus ~= 0 then
 		if getDistanceToCreature(focus) > 5 then
 			selfSay('Good bye then.')
 			focus = 0
 		end
 	end
end
