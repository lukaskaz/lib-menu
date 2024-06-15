#include "climenu.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>

Menu::Menu(const std::string& title,
           std::vector<std::pair<std::string, func>>&& entries) :
    title{title},
    entries{std::move(entries)}
{
    if (this->entries.empty())
    {
        throw std::runtime_error("No menu entries given");
    }
}

bool Menu::isenterpressed(int32_t timeoutMs)
{
    bool ret = false;
    auto epollfd = epoll_create1(0);
    if (epollfd >= 0)
    {
        auto fd = fileno(stdin);
        epoll_event event{.events = EPOLLIN, .data = {.fd = fd}}, revent{};

        epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
        if (0 < epoll_wait(epollfd, &revent, 1, timeoutMs) &&
            revent.events & EPOLLIN)
        {
            while (getchar() != '\n')
                ;
            ret = true;
        }
        close(epollfd);
    }
    return ret;
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
        system("clear");
        std::cout << "== " << title << " menu ==\n";

        uint32_t num{};
        for (const auto& entry : entries)
        {
            const auto& info = std::get<std::string>(entry);
            std::cout << ++num << " : to " << info << "\n";
        }
        std::cout << "-> ";

        auto sel = getusersel();
        if (sel < num)
        {
            std::get<func>(entries.at(sel - 1))();
            std::cout << "Press enter to return to menu\n";
        }
        else if (sel == num)
        {
            std::get<func>(entries.at(sel - 1))();
            break;
        }
        else
        {
            std::cerr << "Maximum selection item is " << entries.size() << "\n";
        }

        waitenterpressed();
    }
}

