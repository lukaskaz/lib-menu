#include "climenu.hpp"

#include <poll.h>

#include <iostream>

bool Menu::isenterpressed(int32_t timeoutMs)
{
    pollfd pollFd{fileno(stdin), POLLIN, 0};
    if (0 < poll(&pollFd, 1, timeoutMs) && pollFd.revents & POLLIN)
    {
        while (getchar() != '\n')
            ;
        return true;
    }
    return false;
}

inline uint32_t Menu::getusersel() const
{
    uint32_t sel = getchar() - '0';
    while ('\n' != getchar())
        ;
    return sel;
}

void Menu::run() const
{
    while (true)
    {
        uint32_t num{};

        system("clear");
        std::cout << "== " << title << " menu ==\n";
        for (const auto& entry : entries)
        {
            const auto& info = std::get<std::string>(entry);
            std::cout << ++num << " : to " << info << "\n";
        }
        std::cout << "-> ";

        if (auto sel = getusersel() - 1; sel < num)
        {
            std::get<func>(entries.at(sel))();
            std::cout << "Press enter to return to menu\n";
        }
        else
        {
            std::cerr << "Maximum selection item is " << entries.size() << "\n";
        }
        waitenterpressed();
    }
}