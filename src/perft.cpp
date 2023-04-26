#include "perft.h"

std::atomic<uint64_t>leafNodes(0);

void Perft(Board& board, int depth){
    if (depth == 0){
        leafNodes++;
        return;
    }

    Movelist moves;
    
    Movegen::legalmoves<ALL>(board, moves);

    for (auto extmove : moves){
        board.makeMove(extmove.move);

        Perft(board, depth - 1);

        board.unmakeMove(extmove.move);
    }
}

void PerftTest(Board& board, int depth){
    leafNodes = 0;

    Movelist moves;
    
    Movegen::legalmoves<ALL>(board, moves);

    for (auto extmove : moves){
        board.makeMove(extmove.move);

        uint64_t cumnodes = leafNodes;

        Perft(board, depth - 1);

        board.unmakeMove(extmove.move);

        uint64_t oldnodes = leafNodes - cumnodes;
        std::cout << convertMoveToUci(extmove.move) << ": " << oldnodes << std::endl;
    }

    std::cout << "Total: " << leafNodes << std::endl;
}