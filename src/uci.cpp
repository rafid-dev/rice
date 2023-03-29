#include <iostream>
#include <sstream>
#include <thread>
#include "uci.h"
#include "types.h"
#include "eval.h"
#include "search.h"
#include "misc.h"
#include "movescore.h"
#include "tt.h"


static void uci_send_id()
{
    std::cout << "id name " << NAME << "\n";
    std::cout << "id author " << AUTHOR << "\n";
    std::cout << "uciok\n";
}

static void set_option(std::istream &is, std::string &token, std::string name, int &value)
{
    if (token == name)
    {
        is >> std::skipws >> token;
        is >> std::skipws >> token;

        value = std::stoi(token);
    }
}


int DefaultHashSize = 64;
int CurrentHashSize = DefaultHashSize;

bool is_uci = false;

void uci_loop()
{

    Board board;
    SearchInfo info;
    TranspositionTable TTable;

    TTable.Initialize(DefaultHashSize);

    TranspositionTable *table = &TTable;

    std::string command;
    std::string token;

    std::thread mainSearchThread;   

    while (true)
    {
        token.clear();
        command.clear();

        std::getline(std::cin, command);
        std::istringstream is(command);

        is >> std::skipws >> token;

        if (token == "stop")
        {
            info.stopped = true;
            if (mainSearchThread.joinable()){
                mainSearchThread.join();
            }
        }
        if (token == "quit")
        {
            info.stopped = true;
            if (mainSearchThread.joinable()){
                mainSearchThread.join();
            }
            break;
        }
        if (token == "isready")
        {
            std::cout << "readyok\n";
            continue;
        }
        if (token == "ucinewgame")
        {
            TTable.clear();
            TTable.Initialize(CurrentHashSize);
            info.pawnTable.clear();
            info.pawnTable.Reinitialize();
            std::cout << "readyok\n";
            continue;
        }
        if (token == "uci")
        {
            is_uci = true;
            uci_send_id();
            continue;
        }

        /* Handle UCI position command */
        if (token == "position")
        {
            std::string option;
            is >> std::skipws >> option;
            if (option == "startpos")
            {
                board.applyFen(DEFAULT_POS);
            }
            else if (option == "fen")
            {
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
            if (option == "moves")
            {
                std::string moveString;

                while (is >> moveString)
                {
                    // std::cout << moveString << std::endl;
                    board.makeMove(convertUciToMove(board, moveString));
                }
            }
            continue;
        }

        /* Handle UCI go command */
        if (token == "go")
        {
            if (mainSearchThread.joinable()){
                mainSearchThread.join();
            }
            is >> std::skipws >> token;

            // Initialize variables
            int depth = -1;
            int time = -1;
            int inc = 0;
            int movestogo = 40;
            int movetime = -1;

            if (token == "infinite"){
                depth = -1;
            }
            if (token == "depth")
            {
                is >> std::skipws >> token;
                depth = stoi(token);
                is >> std::skipws >> token;
            }
            if (token == "wtime")
            {
                is >> std::skipws >> token;
                if (board.sideToMove == White)
                {
                    time = stoi(token);
                }
                is >> std::skipws >> token;
            }
            if (token == "btime")
            {
                is >> std::skipws >> token;
                if (board.sideToMove == Black)
                {
                    time = stoi(token);
                }
                is >> std::skipws >> token;
            }
            if (token == "winc")
            {
                is >> std::skipws >> token;
                if (board.sideToMove == White)
                {
                    inc = stoi(token);
                }
                is >> std::skipws >> token;
            }
            if (token == "binc")
            {
                is >> std::skipws >> token;
                if (board.sideToMove == Black)
                {
                    inc = stoi(token);
                }
                is >> std::skipws >> token;
            }
            if (token == "movestogo")
            {
                is >> std::skipws >> token;
                movestogo = stoi(token);
                is >> std::skipws >> token;
            }
            if (token == "movetime")
            {
                is >> std::skipws >> token;
                movetime = stoi(token);
                is >> std::skipws >> token;
            }
            if (movetime != -1)
            {
                time = movetime;
                movestogo = 1;
            }

            info.start_time = GetTimeMs();
            info.depth = depth;
            if (time != -1)
            {
                info.timeset = true;

                int safety_overhead = 50;

                time -= safety_overhead;

                int time_slot = time / movestogo + inc;
                int basetime = (time_slot);

                // optime is the time we use to stop if we just cleared a depth
                int optime = basetime * 0.6;

                // max time is the time we can spend max on a search
                int maxtime = std::min(time, basetime * 2);

                info.stoptimeMax = info.start_time + maxtime;
                info.stoptimeOpt = info.start_time + optime;
            }
            
            if (depth == -1)
            {
                info.depth = MAXPLY;
            }

            info.stopped = false;
            info.uci = is_uci;

            // std::cout << "time:" << time << " start:" << info.start_time << " stop:" << info.end_time << " depth:" << info.depth << " timeset: " << info.timeset << "\n";
            //SearchPosition(board, info, table);
            mainSearchThread = std::thread(SearchPosition, std::ref(board), std::ref(info), table);
        }

        if (token == "setoption")
        {
            is >> std::skipws >> token;
            is >> std::skipws >> token;
            set_option(is, token, "Hash", CurrentHashSize);
            set_option(is, token, "RFPMargin", RFPMargin);
            set_option(is, token, "RFPDepth", RFPDepth);
            set_option(is, token, "LMRBase", LMRBase);
            set_option(is, token, "LMRDivision", LMRDivision);
            InitSearch();
            if (DefaultHashSize != CurrentHashSize)
            {
                TTable.clear();
                TTable.Initialize(CurrentHashSize);
                CurrentHashSize = DefaultHashSize;
            }
        }

        /* Debugging Commands */
        if (token == "print")
        {
            std::cout << board << std::endl;
            continue;
        }
        if (token == "eval")
        {
            std::cout << Evaluate(board, info.pawnTable) << std::endl;
            continue;
        }
        if (token == "side")
        {
            std::cout << (board.sideToMove == White ? "White" : "Black\n") << std::endl;
        }
        if (token == "run")
        {
            info.depth = MAXDEPTH;
            info.timeset = false;
            
            mainSearchThread = std::thread(SearchPosition, std::ref(board), std::ref(info), table);
        }
    }

    TTable.clear();
    info.pawnTable.clear();
    std::cout << "\n"
              << "\u001b[0m";
}