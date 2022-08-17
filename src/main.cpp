#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include <nlohmann/json.hpp>

#include <compiler/compiler.hpp>
#include <parser/parser.hpp>

struct Options
{
    std::string fileName;
    bool debug;
};

int main(const int argc, const char **argv)
{
    Options options;

    try
    {
        boost::program_options::options_description mainOptions("Main options");

        mainOptions.add_options()(
            "file", boost::program_options::value<std::string>(&options.fileName)->required(), "Json file name")(
            "debug", boost::program_options::value<bool>(&options.debug)->default_value(false), "Debug flag");

        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, mainOptions), vm);
        boost::program_options::notify(vm);
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << std::endl;
    }

    std::ifstream jsonGameFile(options.fileName);
    Parser parser(jsonGameFile);

    std::ofstream headerFile("reasoner.hpp");
    std::ofstream sourceFile("reasoner.cpp");

    Compiler compiler(parser, options.debug);
    compiler.compile();
    compiler.generateSourceCode(headerFile, sourceFile);

    return 0;
}
