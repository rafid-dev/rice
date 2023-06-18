#include "uci.h"
#include "bench.h"
#include "eval.h"
#include "misc.h"
#include "movescore.h"
#include "perft.h"
#include "search.h"
#include "tt.h"
#include "types.h"
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

bool TUNING = false;

void print_tuning_parameter(std::string str, int value) {
    std::cout << "option name " << str << " type spin default " << value << " min -100000 max 100000" << std::endl;
}

void print_tuning_parameters() {
    print_tuning_parameter("RFPMargin", RFPMargin);
    print_tuning_parameter("RFPImproving", RFPImprovingBonus);
    print_tuning_parameter("RFPDepth", RFPDepth);
}

static void uci_send_id() {
    std::cout << "id name " << NAME << std::endl;
    std::cout << "id author " << AUTHOR << std::endl;
    std::cout << "option name Hash type spin default 64 min 4 max " << MAXHASH << std::endl;
    std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;

    if (TUNING) {
        print_tuning_parameters();
    }

    std::cout << "uciok" << std::endl;
}

static void set_option(std::istream &is, std::string &token, std::string name, int &value) {
    if (token == name) {
        is >> std::skipws >> token;
        is >> std::skipws >> token;

        value = std::stoi(token);
    }
}

int DefaultHashSize = 64;
int CurrentHashSize = DefaultHashSize;
int LastHashSize = CurrentHashSize;

bool IsUci = false;

TranspositionTable *table;

void uci_loop(int argv, char **argc) {
    std::cout << "Rice Copyright (C) 2023  Rafid Ahsan" << std::endl;

    Board board;

    auto heap_allocated_info = std::make_unique<SearchInfo>();
    auto &info = *heap_allocated_info;

    auto ttable = std::make_unique<TranspositionTable>();
    table = ttable.get();
    table->Initialize(DefaultHashSize);

    if (argv > 1 && std::string{argc[1]} == "bench") {
        info.depth = 13;
        info.timeset = false;
        StartBenchmark(board, info);
        exit(0);
    }

    std::string command;
    std::string token;

    while (std::getline(std::cin, command)) {
        std::istringstream is(command);

        token.clear();
        is >> std::skipws >> token;

        if (token == "stop") {
            info.stopped = true;
        } else if (token == "quit") {
            info.stopped = true;
            break;
        } else if (token == "isready") {
            std::cout << "readyok" << std::endl;
            continue;
        } else if (token == "ucinewgame") {
            table->Initialize(CurrentHashSize);
            continue;
        } else if (token == "uci") {
            IsUci = true;
            uci_send_id();
            continue;
        }

        /* Handle UCI position command */
        else if (token == "position") {
            std::string option;
            is >> std::skipws >> option;
            if (option == "startpos") {
                board.applyFen(DEFAULT_POS);
            } else if (option == "fen") {
                std::string final_fen;
                is >> std::skipws >> option;
                final_fen = option;

                // Record side to move
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // Record castling
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // record enpassant square
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // record fifty move conter
                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                final_fen.push_back(' ');
                is >> std::skipws >> option;
                final_fen += option;

                // Finally, apply the fen.
                board.applyFen(final_fen);
            }
            is >> std::skipws >> option;
            if (option == "moves") {
                std::string moveString;

                while (is >> moveString) {
                    // std::cout << moveString << std::endl;
                    board.makeMove<false>(convertUciToMove(board, moveString));
                }
            }
            continue;
        }

        /* Handle UCI go command */
        else if (token == "go") {
            is >> std::skipws >> token;

            // Initialize variables
            int depth = -1;

            uint64_t nodes = -1;

            while (token != "none") {
                if (token == "infinite") {
                    depth = -1;
                    continue;
                }
                if (token == "movestogo") {
                    is >> std::skipws >> token;
                    info.tm.movestogo = stoi(token);
                    is >> std::skipws >> token;
                    continue;
                }

                // Depth
                if (token == "depth") {
                    is >> std::skipws >> token;
                    depth = std::stoi(token);
                    is >> std::skipws >> token;
                    continue;
                }

                // Time
                if (token == "wtime") {
                    is >> std::skipws >> token;
                    info.tm.wtime = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }
                if (token == "btime") {
                    is >> std::skipws >> token;
                    info.tm.btime = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }

                // Increment
                if (token == "winc") {
                    is >> std::skipws >> token;
                    info.tm.winc = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }
                if (token == "binc") {
                    is >> std::skipws >> token;
                    info.tm.binc = std::stod(token);
                    is >> std::skipws >> token;
                    continue;
                }

                if (token == "movetime") {
                    is >> std::skipws >> token;
                    info.tm.movetime = stod(token);
                    is >> std::skipws >> token;
                    continue;
                }
                if (token == "nodes") {
                    is >> std::skipws >> token;
                    nodes = stoi(token);
                    is >> std::skipws >> token;
                }
                token = "none";
            }

            if (nodes != -1) {
                info.nodes = nodes;
                info.nodeset = true;
            }

            info.depth = depth;
            if (info.tm.wtime != -1 || info.tm.btime != -1 || info.tm.movetime != -1) {
                info.timeset = true;
            }

            if (depth == -1) {
                info.depth = MAXPLY;
            }

            info.stopped = false;
            info.uci = IsUci;

            iterative_deepening<true>(board, info);
            // mainSearchThread =
            //     std::thread(iterative_deepening, std::ref(board), std::ref(info));
        }

        else if (token == "setoption") {
            is >> std::skipws >> token;
            is >> std::skipws >> token;

            set_option(is, token, "Hash", CurrentHashSize);

            // Tuner related options
            set_option(is, token, "RFPMargin", RFPMargin);
            set_option(is, token, "RFPImproving", RFPImprovingBonus);
            set_option(is, token, "RFPDepth", RFPDepth);

            set_option(is, token, "LMRBase", LMRBase);
            set_option(is, token, "LMRDivision", LMRDivision);

            set_option(is, token, "NMPBase", NMPBase);
            set_option(is, token, "NMPDivison", NMPDivision);
            set_option(is, token, "NMPMargin", NMPMargin);

            init_search();

            if (CurrentHashSize != LastHashSize) {
                CurrentHashSize = std::min(CurrentHashSize, MAXHASH);
                LastHashSize = CurrentHashSize;
                table->Initialize(CurrentHashSize);
            }
        }

        /* Debugging Commands */
        else if (token == "print") {
            std::cout << board << std::endl;
            continue;
        } else if (token == "bencheval") {

            long samples = 1000000000;
            long long timeSum = 0;
            int output;
            for (int i = 0; i < samples; i++) {
                auto start = std::chrono::high_resolution_clock::now();
                output = evaluate(board);
                auto stop = std::chrono::high_resolution_clock::now();
                timeSum += std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();
            }
            auto timeAvg = (double)timeSum / samples;
            std::cout << "Output: " << output << " , Time: " << timeAvg << "ns" << std::endl;

            continue;
        } else if (token == "eval") {
            std::cout << "Eval: " << evaluate(board) << std::endl;
        } else if (token == "repetition") {
            std::cout << board.isRepetition() << std::endl;
        } else if (token == "side") {
            std::cout << (board.sideToMove == White ? "White" : "Black") << std::endl;
        } else if (token == "bench") {

            info.depth = 13;
            info.timeset = false;
            StartBenchmark(board, info);

        }else if (token == "perft") {

            is >> std::skipws >> token;
            int depth = stoi(token);
            PerftTest(board, depth);
        }
    }

    table->clear();

    std::cout << std::endl;
    if (!info.uci) {
        std::cout << "\u001b[0m";
    }
}