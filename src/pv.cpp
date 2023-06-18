#include "pv.h"

void PV::load_from(Move m, const PV& rest){
    moves[0] = m;
    std::memcpy(moves + 1, rest.moves, sizeof(m) * rest.length);
    length = rest.length + 1;
}