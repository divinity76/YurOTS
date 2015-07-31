#include "position.h"
#include <iomanip>

std::ostream& operator<<(std::ostream& os, const Position& pos)
{
    os << "( " << std::setw(5) << std::setfill('0') << pos.x;
    os << " / " << std::setw(5) << std::setfill('0') << pos.y;
    os << " / " << std::setw(3) << std::setfill('0') << pos.z;
    os << " )";

    return os;
}

std::ostream& operator<<(std::ostream& os, const Direction& dir)
{
    switch (dir)
    {
    case NORTH:
        os << "Polnoc";
        break;
    case EAST:
        os << "Wschod";
        break;
    case WEST:
        os << "Zachod";
        break;
    case SOUTH:
        os << "Poludnie";
        break;

    //diagonal
    case SOUTHWEST:
        os << "Poludniowo-Zachodni";
        break;
    case SOUTHEAST:
        os << "Poludniowo-Wschodni";
        break;
    case NORTHWEST:
        os << "Polnocno-Zachodni";
        break;
    case NORTHEAST:
        os << "Polnocno-Wschodni";
        break;
    }

    return os;
}
