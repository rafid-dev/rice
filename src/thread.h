#pragma once

#include "search.h"

#include <vector>
#include <thread>

class Thread {
    public:
    SearchThread searchThread;

    Thread(SearchInfo& info) : searchThread(info) {}
    Thread(SearchInfo& info, TimeMan& tm) : searchThread(info, tm) {}
};

class ThreadHandler {
    using ThreadCount = uint8_t;

    private:
    std::vector<Thread> searchers;
    std::vector<std::thread> threads;

    ThreadCount thread_count = 1;

    public:
    ThreadHandler(){
        stop();
    }

    void resize(ThreadCount thread_count){
        this->thread_count = std::max<ThreadCount>(1, thread_count);
    }

    void start(SearchThread& searchThread){
        stop();

        // start search
        threads.emplace_back(iterative_deepening<true>, std::ref(searchThread));

        // Start other threads
        for (int i = 1; i < thread_count; i++) {
            searchers.emplace_back(searchThread.info, searchThread.tm);
        }

        for (auto& searcher : searchers) {
            threads.emplace_back(iterative_deepening<false>, std::ref(searcher.searchThread));
        }
    }

    void stop(){
        for (auto& th : threads){
            if (th.joinable()){
                th.join();
            }
        }

        searchers.clear();
        threads.clear();
    }
};