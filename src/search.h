#pragma once

#include "tt.h"
#include "types.h"


extern TranspositionTable* table;

using HistoryTable = std::array<std::array<int16_t, 64>, 12>;
using ContinuationHistoryTable =
    std::array<std::array<std::array<std::array<int16_t, 64>, 12>, 64>, 12>;
using CaptureHistoryTable = std::array<std::array<std::array<int16_t, 12>, 64>, 12>;

struct SearchInfo {
    int32_t                  score;
    uint8_t                  depth {};

    HistoryTable             searchHistory;
    ContinuationHistoryTable contHist;

    uint64_t                 nodes_reached {};

    uint64_t                 start_time {};
    uint64_t                 end_time {};
    uint64_t                 stoptime_max {};
    uint64_t                 stoptime_opt {};
    uint64_t                 nodes {};

    bool                     timeset {};
    bool                     stopped {};
    bool                     uci {};
    bool                     nodeset {};

    Move                     bestmove {NO_MOVE};
    Move                     next_bestmove {NO_MOVE};
};

struct SearchStack {
    int16_t static_eval {};
    uint8_t ply {};

    Move    excluded {NO_MOVE};
    Move    move {NO_MOVE};
    Move    killers[2] = {NO_MOVE, NO_MOVE};

    Piece   moved_piece {None};
};

extern int                     RFPMargin;
extern int                     RFPImprovingBonus;
extern int                     RFPDepth;
extern int                     LMRBase;
extern int                     LMRDivision;

extern int                     NMPBase;
extern int                     NMPDivision;
extern int                     NMPMargin;

void                           init_search();

void                           clear_for_search(SearchInfo& info, TranspositionTable* table);

template<bool print_info> 
void iterative_deepening(Board& board, SearchInfo& info);

int negamax(int alpha, int beta, int depth, Board& board, SearchInfo& info, SearchStack* ss);
int qsearch(int alpha, int beta, Board& board, SearchInfo& info, SearchStack* ss);
int aspiration_window(int prevEval, int depth, Board& board, SearchInfo& info);