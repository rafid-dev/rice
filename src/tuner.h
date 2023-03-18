#pragma once

#include "types.h"
#include "eval.h"
#include <fstream>
#include <map>

struct Parameter{
    std::string name;
    Score score;
    Parameter(std::string paramName, Score s){
        name = paramName;
        score = s;
    }
};

class Tuner{
    private:
    Board currentBoard;
    std::vector<Parameter> parameters;
    public:
    Tuner();
    double mean_square_error(double K, int number_of_positions);
    void tune(double K, int number_of_positions);
    void add_parameter(Parameter parameter);
    void print_parameter(Parameter param);
    void print_parameter_array(Parameter params[]);
};