#pragma once

#include "types.h"
#include "tt.h"

extern TranspositionTable* table;

enum SearchType {
    QSEARCH,
    ABSEARCH
};

struct PVTable {
    std::array<int, MAXPLY> length;
    std::array<std::array<Move, MAXPLY>, MAXPLY> array;

    PVTable(){
        for (auto& i : array){
            for (auto& element : i){
                element = NO_MOVE;
            }
        }
    }

    inline void print(){
        std::cout << " pv";
        for (int i = 0; i < length[0]; i++) {
            std::cout << " " << convertMoveToUci(array[0][i]);
        }
        std::cout << std::endl;
    }

    inline void clear(){
        memset(length.data(), 0, sizeof(length));
        memset(array.data(), 0, sizeof(array));
    }

    std::array<Move, MAXPLY>& operator[](int i){
        return array[i];
    }
};

using HistoryTable = std::array<std::array<int16_t, 64>, 12>;
using ContinuationHistoryTable = std::array<std::array<std::array<std::array<int16_t, 64>, 12>, 64>, 12>;
using CaptureHistoryTable = std::array<std::array<std::array<int16_t, 12>, 64>, 12>;

struct SearchInfo {
    uint8_t verifPlies{};
    uint8_t depth{};

    HistoryTable searchHistory;
    ContinuationHistoryTable contHist;
    CaptureHistoryTable captureHistory;
    
    uint64_t nodes{};

    int64_t start_time{};
    int64_t end_time{};
    int64_t stoptimeMax{};
    int64_t stoptimeOpt{};
    int64_t stopNodes{};

    bool timeset{};
    bool stopped{};
    bool uci{};
    bool nodeset{};
    
    Move bestmove {NO_MOVE};
    Move next_bestmove {NO_MOVE};
};

struct SearchStack {
    int16_t static_eval{};
    uint8_t ply{};

    Move excluded {NO_MOVE};
    Move move {NO_MOVE};
    Move killers[2] = {NO_MOVE, NO_MOVE};
    Move counter {NO_MOVE};

    Piece movedPiece {None};
};

extern int RFPMargin;
extern int RFPImprovingBonus;
extern int RFPDepth;
extern int LMRBase;
extern int LMRDivision;

extern int NMPBase;
extern int NMPDivision;
extern int NMPMargin;

void InitSearch();

void ClearForSearch(SearchInfo& info, TranspositionTable *table);

void SearchPosition(Board& board, SearchInfo& info);
int AlphaBeta(int alpha, int beta, int depth, Board& board, SearchInfo& info, SearchStack *ss);
int Quiescence(int alpha, int beta, Board &board, SearchInfo &info, SearchStack *ss);
int AspirationWindowSearch(int prevEval, int depth, Board& board, SearchInfo& info);