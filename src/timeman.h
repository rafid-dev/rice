#pragma once

#include "search.h"

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