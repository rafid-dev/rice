#pragma once

#include "misc.h"
#include "tt.h"
#include "types.h"
#include <cmath>

extern TranspositionTable *table;

using HistoryTable = std::array<std::array<int16_t, 64>, 13>;
using ContinuationHistoryTable = std::array<std::array<std::array<std::array<int16_t, 64>, 13>, 64>, 13>;

struct TimeMan {
    int movestogo = -1;

    Time wtime = -1;
    Time btime = -1;

    Time winc = 0;
    Time binc = 0;

    Time movetime = -1;

    Time start_time{};
    Time end_time{};
    Time stoptime_max{};
    Time stoptime_opt{};
    Time average_time{};

    int stability{};

    Move prev_bestmove{NO_MOVE};

    void set_time(Color side) {
        constexpr int safety_overhead = 50;
        Time uci_time = (side == White ? wtime : btime);

        if (movestogo != -1) {

            uci_time -= safety_overhead;

            Time time_slot = average_time = uci_time / (double)movestogo;

            stoptime_max = time_slot;
            stoptime_opt = time_slot;
        } else if (movetime == -1) {
            Time inc = (side == White ? winc : binc);
            uci_time -= safety_overhead;

            uci_time /= 20;

            Time time_slot = average_time = uci_time + inc;
            Time basetime = (time_slot);

            Time optime = basetime * 0.6;

            Time maxtime = std::min<Time>(uci_time, basetime * 2);
            stoptime_max = maxtime;
            stoptime_opt = optime;

        } else if (movetime != -1) {
            movetime -= safety_overhead;
            stoptime_max = stoptime_opt = average_time = movetime;
        }
    }

    bool check_time() { return (misc::tick() > (start_time + stoptime_max)); }

    bool stop_search() { return (misc::tick() > (start_time + stoptime_opt)); }

    void update_tm(Move bestmove) {

        // Stability scale from Stash
        constexpr double stability_scale[5] = {2.50, 1.20, 0.90, 0.80, 0.75};

        if (prev_bestmove != bestmove) {
            prev_bestmove = bestmove;
            stability = 0;
        } else {
            stability = std::min(stability + 1, 4);
        }

        double scale = stability_scale[stability];

        stoptime_opt = std::min<Time>(stoptime_max, average_time * scale);
    }

    void reset() {
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