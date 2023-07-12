#include "bench.h"
#include "misc.h"

void StartBenchmark(SearchThread& st) {
    SearchInfo& info = st.info;

    info.depth            = 13;
    info.timeset          = false;

    uint64_t nodes        = 0;
    uint64_t count        = 0;
    uint64_t time_elapsed = 0;

    // Inspired from Koivisto

    for (auto& fen : bench_fens) {
        st.applyFen(fen);

        auto start = misc::tick();
        iterative_deepening<false>(st);
        auto end = misc::tick();

        count++;
        nodes += st.nodes_reached;
        time_elapsed += (end - start);

        printf("Position [%2d] -> cp %5d bestmove %s %12ld nodes %8d nps", int(count),
               int(info.score), convertMoveToUci(st.bestmove).c_str(), nodes,
               static_cast<int>(1000.0f * nodes / (time_elapsed + 1)));
        std::cout << std::endl;
    }

    printf("Finished: %42d nodes %8d nps\n", static_cast<int>(nodes),
           static_cast<int>(1000.0f * nodes / (time_elapsed + 1)));
    std::cout << std::flush;
}