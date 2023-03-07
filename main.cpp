#include "init.h"
#include "uci.h"

int main (){
    init_all();

    uci_loop();

    return 0;
}