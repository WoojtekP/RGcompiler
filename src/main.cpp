#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>

#include <parser/parser.hpp>
#include <printer/printer.hpp>


int main(const int argc, const char **argv)
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " [file name]" << std::endl;
        return 1;
    }

    std::ifstream jsonGameFile(argv[1]);
    Parser parser(jsonGameFile);

    std::ofstream headerFile("reasoner.hpp");
    Printer printer(parser, headerFile);

    printer.printHeaderFile();

    return 0;
}
