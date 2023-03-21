/* Coming soon */

// #include <cmath>
// #include "tuner.h"

// std::vector<Parameter> Parameters = {
//     Parameter("bishop_pair_bonus", Score(30, 34)),
// };

// static inline float get_result(const std::string line){
    
// }

// Tuner::Tuner(){
//     std::cout << "Initial parameters: \n";
//     for (auto param : Parameters){
//         parameters.push_back(param);
//         print_parameter(param);
//     }
//     if (parameters.size() == 0){
//         std::cout << "No parameters\n";
//     }
// }

// double Tuner::mean_square_error(double K, int number_of_positions){
//     double error = 0;
//     std::fstream position_file;
//     position_file.open("quiet-labeled.epd", std::ios::in);
//     int lineCount = 1;
//     if (position_file.is_open()){
//         std::string line;
//         while(getline(position_file, line)){
//             if (lineCount >= number_of_positions){
//                 break;
//             }
//             currentBoard.applyFen(line);
//             int score = Evaluate(currentBoard);
//             double sigmoid = 1/(1 + pow(10, -K * score/400));
//             float result = get_result(line);
//             error += pow(result - sigmoid, 2);
//             lineCount++;
//         }
//     }

//     return error/number_of_positions;
// }

// void Tuner::tune(double K, int number_of_positions){
//     int adjust_value = 1;
//     double best_error = mean_square_error(K, number_of_positions);
    
//     bool improved = true;

//     // while (improved){
//     //     improved = false;
//     // }
// }

// void Tuner::add_parameter(Parameter parameter){
//     parameters.push_back(parameter);
// }

// void Tuner::print_parameter(Parameter param){
//     std::cout << "Score " << param.name << "(" << param.score.mg << "," << param.score.eg << ");\n";
// }