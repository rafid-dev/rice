#pragma once

#include "tt.h"
#include "types.h"
#include "misc.h"

extern TranspositionTable *table;

using HistoryTable = std::array<std::array<int16_t, 64>, 13>;
using ContinuationHistoryTable = std::array<std::array<std::array<std::array<int16_t, 64>, 13>, 64>, 13>;

struct TimeMan {
    uint64_t start_time{};
    uint64_t end_time{};
    uint64_t stoptime_max{};
    uint64_t stoptime_opt{};
    uint64_t average_time{};

    int stability{};

    Move prev_bestmove{NO_MOVE};

    void set_time(uint64_t uci_time, uint64_t inc, int moves_to_go) {
        constexpr int safety_overhead = 50;

        uci_time -= safety_overhead;

        int time_slot = average_time = uci_time / moves_to_go;

        stoptime_max = start_time + time_slot;
        stoptime_opt = start_time + time_slot;
    }

    void set_time(uint64_t uci_time, uint64_t inc) {
        uci_time /= 20;

        int time_slot = average_time = uci_time + inc;
        int basetime = (time_slot);

        int optime = basetime * 0.6;

        int maxtime = std::min<uint64_t>(uci_time, basetime * 2);
        stoptime_max = start_time + maxtime;
        stoptime_opt = start_time + optime;
    }

    bool check_time() { return (misc::tick() > stoptime_max); }

    bool stop_search() { return (misc::tick() > stoptime_opt); }

    void update_tm(Move bestmove) {
        if (prev_bestmove != bestmove) {
            prev_bestmove = bestmove;
            stability = 0;
        } else {
            stability++;
        }

        // double scale = 1.2 - 0.04 * std::min(stability, 10);

        // stoptime_opt = std::min<uint64_t>(stoptime_max, average_time * scale);
    }

    void reset(){
        stability = 0;
        prev_bestmove = NO_MOVE;
    }
};

struct SearchInfo {
    int32_t score;
    uint8_t depth{};

    HistoryTable searchHistory;
    ContinuationHistoryTable contHist;

    uint64_t nodes_reached{};
    uint64_t nodes{};

    bool timeset{};
    bool stopped{};
    bool uci{};
    bool nodeset{};

    Move bestmove{NO_MOVE};

    TimeMan tm;
};

struct SearchStack {
    int16_t static_eval{};
    uint8_t ply{};

    Move excluded{NO_MOVE};
    Move move{NO_MOVE};
    Move killers[2] = {NO_MOVE, NO_MOVE};

    Piece moved_piece{None};
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

void clear_for_search(SearchInfo &info, TranspositionTable *table);

template <bool print_info> void iterative_deepening(Board &board, SearchInfo &info);

int negamax(int alpha, int beta, int depth, Board &board, SearchInfo &info, SearchStack *ss);
int qsearch(int alpha, int beta, Board &board, SearchInfo &info, SearchStack *ss);
int aspiration_window(int prevEval, int depth, Board &board, SearchInfo &info);