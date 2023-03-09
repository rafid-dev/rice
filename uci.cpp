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

void uci_loop()
{
    uci_send_id();

    

    Board board;
    SearchInfo info;
    TranspositionTable TTable;

    TTable.Initialize(16);

    TranspositionTable *table = &TTable;

    std::string command;
    std::string token;

    while (true)
    {
        std::cout.flush();
        token.clear();

        std::getline(std::cin, command);
        std::istringstream is(command);
        is >> std::skipws >> token;
        if (token == "stop"){
            info.stopped = true;
        }
        if (token == "quit")
        {
            info.stopped = true;
            break;
        }
        if (token == "isready")
        {
            std::cout << "readyok\n";
            continue;
        }
        if (token == "ucinewgame")
        {
            std::cout << "readyok\n";
            continue;
        }
        if (token == "uci")
        {
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
                std::string fen = command.erase(0, 13);
                board.applyFen(fen);
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
            is >> std::skipws >> token;

            // Initialize variables
            int depth = -1;
            int time = -1;
            int inc = 0;
            int movestogo = 30;
            int movetime = -1;

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
            if (movetime != -1){
                time = movetime;
                movestogo = 1;
            }

            info.start_time = GetTimeMs();
            info.depth = depth;
            if (time != -1){
                info.timeset = true;
                time /= movestogo;

                // Have some over head.
                if (time > 1500) time -= 50;

                info.end_time = info.start_time + time + inc;
            }
            if (depth == -1){
                info.depth = MAXPLY;
            }

            std::cout << "time:" << time << " start:" << info.start_time << " stop:" << info.end_time << " depth:" << info.depth << " timeset: " << info.timeset << "\n";

            SearchPosition(board, info, table);
            continue;
        }

        /* Debugging Commands */
        if (token == "print")
        {
            std::cout << board << std::endl;
            continue;
        }
        if (token == "eval")
        {
            std::cout << Evaluate(board) << std::endl;
            continue;
        }
        if (token == "side"){
            std::cout << (board.sideToMove == White ? "White" : "Black\n") << std::endl;
        }
        if (token == "run"){
            info.depth = MAXDEPTH;
            info.timeset = false;

            SearchPosition(board, info, table);
        }
    }

    TTable.clear();
}