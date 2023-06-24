#pragma once

#include "misc.h"
#include "tt.h"
#include "types.h"
#include "timeman.h"

#include <memory>

// Global Transposition Table
extern TranspositionTable *table;

using HistoryTable = std::array<std::array<int16_t, 64>, 13>;
using ContinuationHistoryTable = std::array<std::array<std::array<std::array<int16_t, 64>, 13>, 64>, 13>;

struct SearchInfo {
    int32_t score = 0;
    uint8_t depth = 0;

    std::atomic<uint64_t> nodes = 0;

    std::atomic<bool> timeset = 0;
    std::atomic<bool> stopped = 0;
    std::atomic<bool> nodeset = 0;
    std::atomic<bool> uci = 0;
};

struct SearchStack {
    int16_t static_eval{};
    uint8_t ply{};
    uint8_t double_extensions{};

    Move excluded{NO_MOVE};
    Move move{NO_MOVE};
    Move killers[2] = {NO_MOVE, NO_MOVE};

    Piece moved_piece{None};

    HistoryTable continuationHistory;

    SearchStack(){
        memset(continuationHistory.data(), 0, sizeof(continuationHistory));
    }
};

struct SearchThread{

    HistoryTable searchHistory;

    NNUE::Net nnue;

    Board board;
    TimeMan tm;
    SearchInfo& info;

    uint64_t nodes_reached = 0;

    Move bestmove = NO_MOVE;

    SearchThread(SearchInfo& i) : info(i), board(DEFAULT_POS, nnue){
        clear();
    }

    inline void clear(){
        nodes_reached = 0;

        memset(searchHistory.data(), 0, sizeof(searchHistory));

        board.refresh(nnue);

        tm.reset();
    }

    inline void initialize(){
        tm.start_time = misc::tick();

        if (info.timeset)
        {
            tm.set_time(board.sideToMove);
        }
    }

    inline Time start_time(){
        return tm.start_time;
    }

    // Make move on board
    template<bool UpdateNNUE>
    inline void makeMove(Move& move){
        board.makeMove<UpdateNNUE>(move, nnue);
    }

    // Unmake move on board
    template<bool UpdateNNUE>
    inline void unmakeMove(Move& move){
        board.unmakeMove<UpdateNNUE>(move, nnue);
    }

    // To make moves from UCI string.
    inline void makeMove(std::string& move_uci){
        board.makeMove<false>(convertUciToMove(board, move_uci), nnue);
    }

    // Applying fen on board
    inline void applyFen(std::string fen){
        board.applyFen(fen, nnue);
    }

    inline bool stop_early()
    {
        if (info.timeset && (tm.stop_search() || info.stopped))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    void check_time()
    {
        if ((info.timeset && tm.check_time()) || (info.nodeset && nodes_reached >= info.nodes))
        {
            info.stopped = true;
        }
    }
};

extern int RFPMargin;
extern int RFPImprovingBonus;
extern int RFPDepth;
extern int LMRBase;
extern int LMRDivision;

extern int NMPBase;
extern int NMPDivision;
extern int NMPMargin;

void init_search();

template <bool print_info> void iterative_deepening(SearchThread& st);

int negamax(int alpha, int beta, int depth, SearchThread& st, SearchStack *ss);
int qsearch(int alpha, int beta, SearchThread& st, SearchStack *ss);
int aspiration_window(int prevEval, int depth, SearchThread& st);