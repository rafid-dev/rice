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

    std::array<Move, MAXPLY> operator[](int i){
        return array[i];
    }
};

struct SearchInfo {
    int verifPlies = 0;
    int depth = 0;

    int16_t searchHistory[NPIECES][NSQUARES] = {{0}};
    std::array<std::array<std::array<std::array<int16_t, 64>, 12>, 64>, 12> contHist;
    int16_t captureHistory[12][64][12];
    
    int64_t nodes = 0l;

    int64_t start_time = 0l;
    int64_t end_time = 0l;
    int64_t stoptimeMax = 0l;
    int64_t stoptimeOpt = 0l;
    int64_t stopNodes = 0l;

    bool timeset = false;
    bool stopped = false;
    bool uci = false;
    bool nodeset = false;

    PVTable pvTable;
    Move bestmove = NO_MOVE;
};

struct SearchStack {
    int static_eval = 0;
    int ply = 0;

    Move excluded = NO_MOVE;
    Move move = NO_MOVE;
    Move killers[2] = {NO_MOVE, NO_MOVE};
    Move counter = NO_MOVE;

    Piece movedPiece = None;
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