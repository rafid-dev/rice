#pragma once

#include "search.h"

#include <vector>
#include <thread>

class Thread {
    public:
    std::unique_ptr<SearchThread> searchThread;

    Thread(SearchInfo& info) : searchThread{std::make_unique<SearchThread>(info)} {}
    Thread(const Thread& other) : searchThread(std::make_unique<SearchThread>(*other.searchThread)) {}
    Thread(const SearchThread& other) : searchThread{std::make_unique<SearchThread>(other)} {}
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