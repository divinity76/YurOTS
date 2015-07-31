# Makefile przygotowany przez Huczu

CC=gcc

CFLAGS=-I. -I/usr/include/libxml2  -I/usr/include/lua50 -I/usr/local/include

LIBLINK=-L/usr/lib64 -lxml2 -lz -lpthread -lm -llualib50 -llua50 -ldl -lboost_thread -lcurlpp -lmysqlclient

ENDFLAGS=-Werror -g -ggdb -Winvalid-pch -include "preheaders.h"


OTSFLAGS=-D__LINUX__ -DHUCZU_PVP -DHUCZU_SKULLS -DHUCZU_RECORD -DHUCZU_FOLLOW -DHUCZU_LOOT_INFO -DHUCZU_EXHAUSTED -DHUCZU_RRV -DHUCZU_SERVER_LOG -DHUCZU_PARCEL_SYSTEM -DHUCZU_BAN_SYSTEM -DHUCZU_STAGE_EXP -DHUCZU_PAY_SYSTEM -DHUCZU_NAPISY -D__MIZIAK_SUPERMANAS__ -D__MIZIAK_CREATURESCRIPTS__ -D__MIZIAK_TALKACTIONS__ -D__MIZIAK_TASKS__ -DYUR_CVS_MODS -DYUR_LOGIN_QUEUE -DYUR_BOH -DYUR_PVP_ARENA -DYUR_RINGS_AMULETS -DYUR_HIGH_LEVELS -DYUR_PREMIUM_PROMOTION -D_NG_BBK_SPAMSPELL__ -D__KIRO_AKT__ -DTLM_HOUSE_SYSTEM -DTRS_GM_INVISIBLE -DCVS_DAY_CYCLE -DFIXY -DTR_SUMMONS
OBJ = actions.o allocator.o chat.o commands.o container.o creature.o database.o databasemanager.o databasemysql.o fileloader.o game.o guilds.o houses.o ioaccountsql.o iomapotbm.o ioplayersql.o item.o itemloader.o items.o logger.o loginqueue.o luascript.o magic.o map.o monster.o monsters.o networkmessage.o npc.o otserv.o player.o position.o protocol.o protocol76.o pvparena.o scheduler.o spawn.o spells.o status.o summons.o task.o thing.o tile.o tools.o town.o

all: tibia

tibia: $(OBJ)
	$(CC) $(CFLAGS) $(OTSFLAGS) $(ENDFLAGS) -o otserv $(OBJ) $(LIBLINK)

%.o:%.cpp
	$(CC) $(CFLAGS) $(OTSFLAGS) $(ENDFLAGS) -c $+



