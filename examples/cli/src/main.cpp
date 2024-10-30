#include "menu/interfaces/cli.hpp"

#include <iostream>

template <int32_t num, bool show = true, bool waitenter = true>
bool func(bool isshown)
{
    if (isshown)
        return show;

    std::cout << "Operation num " << num << std::endl;
    if (!waitenter)
        sleep(1);
    return waitenter;
}

bool cleanup(bool isshown)
{
    if (isshown)
        return {};

    std::cout << "Cleaning up and exitting" << std::endl;
    return true;
}

int main()
{
    try
    {
        auto menu = menu::MenuFactory::create<menu::cli::Menu>(
            "Test",
            {{"Run first operation", std::bind(func<1>, true),
              std::bind(func<1>, false)},
             {"Run second operation", std::bind(func<2>, true),
              std::bind(func<2>, false)},
             {"Run third operation", std::bind(func<3, false>, true),
              std::bind(func<3, false>, false)},
             {"Run forth operation", std::bind(func<4>, true),
              std::bind(func<4>, false)},
             {"Run fifth operation", std::bind(func<5>, true),
              std::bind(func<5>, false)},
             {"Run sixth operation", std::bind(func<6>, true),
              std::bind(func<6>, false)},
             {"Run seventh operation", std::bind(func<7>, true),
              std::bind(func<7>, false)},
             {"Run eight operation", std::bind(func<8>, true),
              std::bind(func<8>, false)},
             {"Run ninth operation", std::bind(func<9>, true),
              std::bind(func<9>, false)},
             {"Run tenth operation", std::bind(func<10, true, false>, true),
              std::bind(func<10, true, false>, false)},
             {"Run eleventh operation", std::bind(func<11>, true),
              std::bind(func<11>, false)},
             {"Run twelvth operation", std::bind(func<12>, true),
              std::bind(func<12>, false)},
             {"Run thirteenth operation", std::bind(func<13>, true),
              std::bind(func<13>, false)},
             {"Run fourteenth operation", std::bind(func<14>, true),
              std::bind(func<14>, false)},
             {"Run fifteenth operation", std::bind(func<15>, true),
              std::bind(func<15>, false)},
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
