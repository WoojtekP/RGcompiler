#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include <nlohmann/json.hpp>

#include <compiler/Compiler.hpp>
#include <compiler/SymbolsManager.hpp>
#include <parser/Parser.hpp>

int main(const int argc, const char **argv)
{
    Options options;
    std::string inputFileName;
    std::string outputFileName;

    try
    {
        namespace po = boost::program_options;
        po::options_description mainOptions("Main options");

        mainOptions.add_options()("help,h", "Show this help message and exit")(
            "file", po::value<std::string>(&inputFileName)->required(), "Json file name with AST")(
            ",o", po::value<std::string>(&outputFileName)->default_value("reasoner"), "Output file name")(
            "print-function-names",
            po::value<bool>(&options.printOriginalNames_)->default_value(0),
            "Print original node names in function names")(
            "preserve-original-node-names",
            po::value<bool>(&options.preserveOriginalNames_)->default_value(0),
            "Preserve original node names")(
            "verification",
            po::value<bool>(&options.verification_)->default_value(0),
            "Extra verification for transducer")(
            "simple-path-compression",
            po::value<bool>(&options.simplePathCompression_)->default_value(false),
            "Enable compressing simple paths")(
            "no-cycle-detection",
            po::value<bool>(&options.noCycleDetection_)->default_value(false),
            "Disable detecting cycles in patterns")(
            "disjoint",
            po::value<bool>(&options.pragmaDisjointEnabled_)->default_value(true),
            "Enable pragma 'disjoint'")(
            "arithmetic",
            po::value<bool>(&options.arithmeticOpt_)->default_value(true),
            "Optimize arithmetic constants (when pragma 'integer' is available)")(
            "gccinline",
            po::value<int>(&options.gccInline_)->default_value(0),
            "Inlining mode:\n"
            "0: off\n"
            "1: inline methods which are used at most once\n"
            "2: force inline for functions representing 'unique' states")(
            "max-move-len",
            po::value<int>(&options.maxMoveLen_)->default_value(-1),
            "Enable setting size of static vector")(
            "all-unique", po::value<bool>(&options.allUnique_)->default_value(false), "Remove caches")(
            "remove-unecessary-funcion-arguments",
            po::value<bool>(&options.removeUnecessaryFuncionArguments_)->default_value(true),
            "Remove unecessary function arguments like passing cache if not needed")(
            "remove-unecessary-funcions",
            po::value<bool>(&options.removeUnecessaryFuncions_)->default_value(true),
            "Remove unecessary functions like state functions for pattern nodes")(
            "support-apply-move-for-keeper",
            po::value<bool>(&options.supportApplyMoveForKeepr_)->default_value(true),
            "If false ApplyMove function have extra nodes for keeper");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, mainOptions), vm);
        if (vm.count("help"))
        {
            std::cout << mainOptions << std::endl;  // Print the help message
            return 0;
        }
        po::notify(vm);

        if (options.gccInline_ < 0 || options.gccInline_ > 2)
        {
            std::cerr << "Error: inline mode must be 0, 1 or 2, but " << options.gccInline_ << " was given."
                      << std::endl;
            return 1;
        }
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    std::ifstream jsonGameFile(inputFileName);
    Parser parser(jsonGameFile);

    std::ofstream headerFile(outputFileName + ".hpp");
    std::ofstream sourceFile(outputFileName + ".cpp");

    Compiler compiler(parser, options);
    compiler.compile();
    compiler.generateSourceCode(outputFileName, headerFile, sourceFile);

    return 0;
}
