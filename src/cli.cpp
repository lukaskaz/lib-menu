#include "menu/interfaces/cli.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <charconv>
#include <iostream>
#include <stdexcept>

namespace menu::cli
{

struct Menu::Handler
{
    Handler(const std::string& title, menuentries&& entries) :
        title{title}, entries{std::move(entries)}
    {
        if (this->entries.empty())
        {
            throw std::runtime_error("No menu entries given");
        }
    }

    void run()
    {
        const auto lastentrypos = entries.size() - 1;
        while (true)
        {
            system("clear");
            std::cout << "== " << title << " menu ==\n";

            uint32_t num{}, pos{};
            std::unordered_map<decltype(num), decltype(pos)> numtoentry;
            for (const auto& entry : entries)
            {
                // get showfunc
                if (std::get<1>(entry)() || pos == lastentrypos)
                {
                    auto& info = std::get<std::string>(entry);
                    std::cout << ++num << " : to " << info << "\n";
                    numtoentry.emplace(num, pos);
                }
                pos++;
            }
            std::cout << "-> ";

            auto sel = getusersel();
            if (sel < num)
            {
                auto pos = numtoentry.at(sel);
                // get normal menufunc
                if (std::get<2>(entries.at(pos))())
                {
                    std::cout << "Press enter to return to menu\n";
                    waitenterpressed();
                }
            }
            else if (sel > num)
            {
                std::cerr << "Maximum selection item is " << entries.size()
                          << "\n";
                waitenterpressed();
            }
            else
            {
                // get exit menufunc
                std::get<2>(entries.at(sel - 1))();
                break;
            }
        }
    }

    bool isenterpressed(int32_t timeoutMs)
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

    std::string getinfo()
    {
        return "menu using cli";
    }

  private:
    const std::string module{"libmenu"};
    const std::string title;
    const menuentries entries;

    void waitenterpressed()
    {
        isenterpressed(-1);
    }

    int32_t getnumfromstr(std::string_view sv)
    {
        int32_t num{};
        const auto sstart = sv.data(), send = sv.data() + sv.size();
        if (const auto ret = std::from_chars(sstart, send, num);
            ret.ec == std::errc{} && ret.ptr == send)
        {
            return num;
        }
        throw std::runtime_error(std::string(__func__) +
                                 ": string not contain number");
    }

    uint32_t getusersel()
    {
        char inch{};
        std::string instr;
        while ((inch = (char)getchar()) != '\n' || instr.empty())
        {
            if (inch != '\n')
            {
                instr += inch;
            }
        }
        return getnumfromstr(instr);
    }
};

Menu::Menu(const std::string& title, menuentries&& entries) :
    handler{std::make_unique<Handler>(title, std::move(entries))}
{}

Menu::~Menu() = default;

void Menu::run() const
{
    handler->run();
}

bool Menu::isenterpressed() const
{
    return handler->isenterpressed(0);
}

std::string Menu::info() const
{
    return handler->getinfo();
}

} // namespace menu::cli
