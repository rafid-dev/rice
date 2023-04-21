#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>
#include "uci.h"
#include "eval.h"
#include "misc.h"
#include "movescore.h"
#include "search.h"
#include "tt.h"
#include "types.h"
#include "datagen.h"
#include "bench.h"


static void uci_send_id()
{
  std::cout << "id name " << NAME << "\n";
  std::cout << "id author " << AUTHOR << "\n";
  std::cout << "option name Hash type spin default 64 min 4 max " << MAXHASH << "\n";
  std::cout << "uciok\n";
}

static void set_option(std::istream &is, std::string &token, std::string name,
                       int &value)
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
int LastHashSize = CurrentHashSize;

bool IsUci = false;

TranspositionTable TTable;
TranspositionTable *table = &TTable;

void uci_loop(int argv, char *argc[])
{
  Board board;
  SearchInfo info;

  table->Initialize(DefaultHashSize);

  if (argv > 2 && std::string{argc[1]} == "datagen")
  {
    int threads = std::stoi(argc[2]);
    table->Initialize(64 * threads);
    generateData(1000000, threads);
  }else if (argv > 1 && std::string{argc[1]} == "bench"){
    info.depth = 13;
    info.timeset = false;
    StartBenchmark(board, info);
  }

  std::string command;
  std::string token;
  
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
    }
    else if (token == "quit")
    {
      info.stopped = true;

      break;
    }
    else if (token == "isready")
    {
      std::cout << "readyok\n";
      continue;
    }
    else if (token == "ucinewgame")
    {
      table->Initialize(CurrentHashSize);
      std::cout << "readyok\n";
      continue;
    }
    else if (token == "uci")
    {
      IsUci = true;
      uci_send_id();
      continue;
    }

    /* Handle UCI position command */
    else if (token == "position")
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
    else if (token == "go")
    {
      is >> std::skipws >> token;

      // Initialize variables
      int depth = -1;
      int uciTime = -1;
      int inc = 0;
      int movestogo = -1;
      int movetime = -1;
      int nodes = -1;

      while (token != "none")
      {
        if (token == "infinite")
        {
          depth = -1;
          continue;
        }
        if (token == "movestogo")
        {
          is >> std::skipws >> token;
          movestogo = stoi(token);
          is >> std::skipws >> token;
          continue;
        }

        // Depth
        if (token == "depth")
        {
          is >> std::skipws >> token;
          depth = stoi(token);
          is >> std::skipws >> token;
          continue;
        }

        // Time
        if (token == "wtime")
        {
          is >> std::skipws >> token;
          if (board.sideToMove == White)
          {
            uciTime = stoi(token);
          }
          is >> std::skipws >> token;
          continue;
        }
        if (token == "btime")
        {
          is >> std::skipws >> token;
          if (board.sideToMove == Black)
          {
            uciTime = stoi(token);
          }
          is >> std::skipws >> token;
          continue;
        }

        // Increment
        if (token == "winc")
        {
          is >> std::skipws >> token;
          if (board.sideToMove == White)
          {
            inc = stoi(token);
          }
          is >> std::skipws >> token;
          continue;
        }
        if (token == "binc")
        {
          is >> std::skipws >> token;
          if (board.sideToMove == Black)
          {
            inc = stoi(token);
          }
          is >> std::skipws >> token;
          continue;
        }

        if (token == "movetime")
        {
          is >> std::skipws >> token;
          movetime = stoi(token);
          is >> std::skipws >> token;
          continue;
        }
        if (token == "nodes")
        {
          is >> std::skipws >> token;
          nodes = stoi(token);
          is >> std::skipws >> token;
        }
        token = "none";
      }
      if (movetime != -1)
      {
        uciTime = movetime;
        movestogo = 1;
      }
      if (nodes != -1)
      {
        info.stopNodes = nodes;
        info.nodeset = true;
      }

      info.start_time = GetTimeMs();
      info.depth = depth;
      if (uciTime != -1)
      {
        info.timeset = true;
      }

      if (info.timeset && movestogo != -1)
      {
        int safety_overhead = 50;

        uciTime -= safety_overhead;

        int time_slot = uciTime / movestogo;

        info.stoptimeMax = info.start_time + time_slot;
        info.stoptimeOpt = info.start_time + time_slot;
      }
      else if (info.timeset)
      {

        uciTime /= 40;

        int time_slot = uciTime + inc;
        int basetime = (time_slot);

        // optime is the time we use to stop if we just cleared a depth
        int optime = basetime * 0.6;

        // max time is the time we can spend max on a search
        int maxtime = std::min(uciTime, basetime * 2);
        info.stoptimeMax = info.start_time + maxtime;
        info.stoptimeOpt = info.start_time + optime;
      }

      if (depth == -1)
      {
        info.depth = MAXPLY;
      }

      info.stopped = false;
      info.uci = IsUci;

      if (IS_DEBUG)
      {
        std::cout << "movestogo: " << movestogo << " time:" << uciTime << " start:" << info.start_time << " stop:" << info.stoptimeMax << " depth:" << info.depth << " timeset: " << info.timeset << "\n";
      }
      SearchPosition(board, info);
      // mainSearchThread =
      //     std::thread(SearchPosition, std::ref(board), std::ref(info));
    }

    else if (token == "setoption")
    {
      is >> std::skipws >> token;
      is >> std::skipws >> token;
      set_option(is, token, "Hash", CurrentHashSize);
      set_option(is, token, "RFPMargin", RFPMargin);
      set_option(is, token, "RFPDepth", RFPDepth);
      set_option(is, token, "LMRBase", LMRBase);
      set_option(is, token, "LMRDivision", LMRDivision);
      InitSearch();
      if (CurrentHashSize != LastHashSize)
      {
        CurrentHashSize = std::min(CurrentHashSize, MAXHASH);
        LastHashSize = CurrentHashSize;
        table->Initialize(CurrentHashSize);
      }
    }

    /* Debugging Commands */
    else if (token == "print")
    {
      std::cout << board << std::endl;
      continue;
    }
    else if (token == "be")
    {
      int64_t sum = 0;
      int count = 100000000;
      int64_t score = 0;
      auto start = std::chrono::high_resolution_clock::now();

      for (int i = 0; i < count; i++)
      {

        score += Evaluate(board);
      }
      auto stop = std::chrono::high_resolution_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
      sum += duration.count();
      std::cout << "Average NS: " << (sum / count) << std::endl;
      std::cout << score << std::endl;

      continue;
    }
    else if (token == "eval")
    {
      std::cout << "Eval: " << Evaluate(board) << std::endl;
    }
    else if (token == "side")
    {
      std::cout << (board.sideToMove == White ? "White" : "Black\n")
                << std::endl;
    }
    else if (token == "bench")
    {
      info.depth = 13;
      info.timeset = false;
      StartBenchmark(board, info);
    }
    else if (token == "datagen")
    {
      table->Initialize(64 * 6);
      generateData(1000000, 6);
    }
  }

  table->clear();

  std::cout << "\n";
  if (!info.uci)
  {
    std::cout << "\u001b[0m";
  }
}