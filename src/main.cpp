#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include <nlohmann/json.hpp>

#include <compiler/compiler.hpp>
#include <parser/parser.hpp>


int main(const int argc, const char **argv)
{
    Options options;
    std::string fileName;

    try
    {
        boost::program_options::options_description mainOptions("Main options");

        mainOptions.add_options()(
            "file", boost::program_options::value<std::string>(&fileName)->required(), "Json file name")(
            "debug", boost::program_options::value<bool>(&options.debug)->default_value(false), "Debug flag")(
            "opt-conditions",
            boost::program_options::value<int>(&options.optConditions)->default_value(0),
            "Optimizing complementary conditions:\n 0 - off\n 1 - checking reachability\n 2 - generating moves\n 3 - all")(
            "move-compression",
            boost::program_options::value<bool>(&options.moveCompression_)->default_value(false),
            "Enable compressing moves")(
            "simple-path-compression",
            boost::program_options::value<bool>(&options.simplePathCompression_)->default_value(false),
            "Enable compressing simple paths");

        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, mainOptions), vm);
        boost::program_options::notify(vm);
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    std::ifstream jsonGameFile(fileName);
    Parser parser(jsonGameFile);

    std::ofstream headerFile("reasoner.hpp");
    std::ofstream sourceFile("reasoner.cpp");

    Compiler compiler(parser, options);
    compiler.compile();
    compiler.generateSourceCode(headerFile, sourceFile);

    return 0;
}
