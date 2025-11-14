#pragma once
#include "../ResultWriter/ResultWriter.hpp"
#include "../Structure/Data.hpp"
// #define MAXFLOAT 9999999

class Legalizer
{
    LegalizerInput *input;

    void divideRow();
    int getRowIdx(Cell const *cell);
    int getSubRowIdx(Row const *row, Cell const *cell);
    int placeRowTrail(int const &rowIdx, Cell *cell);
    void placeRowFinal(int const &rowIdx, int const &subRowIdx, Cell *cell);
    double calCost(Cell const *cell);
    void determinePosition();
    void abacusProcess();
    void calDisplacement(std::string phase);

public:
    Legalizer(LegalizerInput *input) : input(input) {}
    ResultWriter *solve(std::string design_name,std::string phase, bool remap_flag);
   
};
