#include "init.h"
#include "uci.h"
#include <iostream>

int main (int argv, char* argc[]){
    init_all();

    uci_loop(argv, argc);
    
    return 0;
}