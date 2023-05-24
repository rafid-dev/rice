#pragma once

#include <iostream>

typedef std::string FANCYCOLOR;

FANCYCOLOR FANCY_Black = "\u001b[30m";
FANCYCOLOR FANCY_Red = "\u001b[31;1m";
FANCYCOLOR FANCY_Green = "\u001b[32;1m";
FANCYCOLOR FANCY_Yellow = "\u001b[33;1m";
FANCYCOLOR FANCY_Cyan = "\u001b[36;1m";
FANCYCOLOR FANCY_Blue = "\u001b[34;1m";
FANCYCOLOR FANCY_Reset = "\u001b[0m";

template<typename T = uint64_t>
inline void FancyNumber(T x, bool uci, FANCYCOLOR col){
    if (uci){
        std::cout << x;
    }else{
        std::cout << col << x << FANCY_Reset;
    }
}