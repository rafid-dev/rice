#pragma once

#include "types.h"
#include "eval.h"
#include <fstream>
#include <map>

struct parameter_t {
    std::string name;
    std::vector<Score> values;
    parameter_t(const std::string& param_name, const Score& value){
        this->values.clear();
        this->values.resize(1);
        this->values[0].mg = value.mg;
        this->values[0].eg = value.eg;
        this->name = param_name;
    }
    parameter_t(const std::string& param_name, const Score value[], const int& size){
        this->values.clear();
        this->values.resize(size);
        for (int i = 0; i < size; i++){
            this->values[i].mg = value[i].mg;
            this->values[i].eg = value[i].eg;
        }
        this->name = param_name;
    }
};

class Tuner{
    private:
    Board currentBoard;
    std::vector<parameter_t> initial_parameters;
    std::vector<parameter_t> tuned_parameters;

    public:
    Tuner();
    double mean_square_error(double K, int number_of_positions);
    void tune(double K, int number_of_positions);
    void print_parameter(parameter_t param);
    parameter_t get_initial_parameters();
};