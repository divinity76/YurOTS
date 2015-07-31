#ifdef YUR_PVP_ARENA

#ifndef PVPARENA_H
#define PVPARENA_H
#include "position.h"
class Game;

class PvpArena
{
private:
    static bool AddArenaTile(Game* game, const Position& pos, const Position& exit);
public:
    static bool Load(Game* game);
};
#endif //PVP_ARENA_H
#endif //YUR_PVP_ARENA
