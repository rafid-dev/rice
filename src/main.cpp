#include "init.h"
#include "uci.h"

int main (int argv, char* argc[]){
    init_all();

    uci_loop(argv, argc);
    
    return 0;
}