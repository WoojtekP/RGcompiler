#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>


int main(const int argc, const char **argv)
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " [file name]" << std::endl;
        return 1;
    }

    std::ifstream ifs(argv[1]);
    nlohmann::json j = nlohmann::json::parse(ifs);
    std::cout << j;

    return 0;
}
