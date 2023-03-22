#include <cmath>
#include "tuner.h"

#define S Score

static int Eval(Board &board)
{
}

static float get_result(const std::string line)
{
    auto idx = line.find('[');
    std::string lineExtracted = line.substr(idx + 1);

    lineExtracted.resize(lineExtracted.size() - 1);
    if (lineExtracted == "1.0")
    {
        return 1.0;
    }
    else if (lineExtracted == "0.5")
    {
        return 0.5;
    }
    else if (lineExtracted == "0.0")
    {
        return 0.0;
    }
    return 0.0;
}

Tuner::Tuner()
{
    std::cout << "Initial parameters: \n";
    for (auto param : initial_parameters)
    {
        print_parameter(param);
    }
    if (initial_parameters.size() == 0)
    {
        std::cout << "No initial parameters\n";
    }
}

double Tuner::mean_square_error(double K, int number_of_positions)
{
    double error = 0;
    std::fstream position_file;
    position_file.open("positions.txt", std::ios::in);
    int lineCount = 1;
    if (position_file.is_open())
    {
        std::string line;
        while (getline(position_file, line))
        {
            if (lineCount >= number_of_positions)
            {
                break;
            }
            currentBoard.applyFen(line);
            int score = Eval(currentBoard);
            double sigmoid = 1 / (1 + pow(10, -K * score / 400));
            float result = get_result(line);
            error += pow(result - sigmoid, 2);
            lineCount++;
        }
    }

    return error / number_of_positions;
}

void Tuner::tune(double K, int number_of_positions)
{
    int adjust_value = 1;
    double best_error = mean_square_error(K, number_of_positions);

    bool improved = true;

    while (improved)
    {
        improved = false;
    }
}

parameter_t Tuner::get_initial_parameters(){
    
}

void Tuner::print_parameter(parameter_t param){
    int size = param.values.size();
    std::string ending = (size > 0 ? "), " : ")");
    for (int i = 0; i < size; i++){
        std::cout << "Score " << param.name << " = S(" << param.values[i].mg << ", " << param.values[i].eg << ending;
        if (i%8 == 0){
            std::cout << "\n";
        }
    }
    std::cout << ";\n";
}