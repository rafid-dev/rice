#include "init.h"
#include "search.h"
#include "eval.h"


// Initialize Search Parameters
int RFPMargin = 75;
int RFPImprovingBonus = 173;
int RFPDepth = 5;
int LMRBase = 100;
int LMRDivision = 190;

void init_all(){
    InitSearch();
    InitPestoTables();
    InitEvaluationMasks();
    
}