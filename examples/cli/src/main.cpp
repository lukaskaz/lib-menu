#include "menu/interfaces/cli.hpp"

#include <iostream>

bool func1(bool isshown)
{
    // show menu?
    if (isshown)
        return true;

    std::cout << "Operation num 1\n";
    // wait for enter when done?
    return false;
}

bool func2(bool isshown)
{
    if (isshown)
        return true;

    std::cout << "Operation num 2\n";
    return false;
}

bool cleanup(bool isshown)
{
    if (isshown)
        return {};

    std::cout << "Cleaning up and exitting\n";
    return true;
}

int main()
{
    try
    {
        auto menu = menu::MenuFactory::create<menu::cli::Menu>(
            "Test", {{"Run first operation", std::bind(func1, true),
                      std::bind(func1, false)},
                     {"Run second operation", std::bind(func2, true),
                      std::bind(func2, false)},
                     {"Exit menu", std::bind(cleanup, true),
                      std::bind(cleanup, false)}});
        // menu->run();
    }
    catch (std::exception& err)
    {
        std::cerr << "[ERROR] " << err.what() << '\n';
    }

    return 0;
}
