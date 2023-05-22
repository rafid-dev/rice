#include "init.h"
#include "nnue.h"
#include "search.h"


// Initialize Search Parameters
int RFPMargin = 75;
int RFPDepth = 5;
int RFPImprovingBonus = 62;
int LMRBase = 75;
int LMRDivision = 225;

int NMPBase = 3;
int NMPDivision = 3;
int NMPMargin = 180;

void init_all() {
    init_search();
    NNUE::Init("./test4.nn");
}