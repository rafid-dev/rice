#include "init.h"
#include "search.h"
#include "eval.h"


// Initialize Search Parameters
int RFPMargin = 75;
int RFPDepth = 5;
int LMRBase = 77;
int LMRDivision = 236;

void init_all(){
    InitSearch();
    InitPsqTables();
    InitEvaluationMasks();
}