#include <graph/graph.hpp>
#include <printer/printer.hpp>
#include <program/program.hpp>


class Compiler
{
public:
    void compile();
    void generateSourceCode();

private:
    Graph graph;
    Program program;
    Printer printer;
};
