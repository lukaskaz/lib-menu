#include "menu/interfaces/cli.hpp"

#include "menu/helpers.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <unordered_set>

namespace menu::cli
{

static constexpr auto endchar = '\r', escape = '\x1b';
static const std::string arrowup{"\x1b\x5b\x41"};
static const std::string arrowdown{"\x1b\x5b\x42"};
static const auto arrowseqsize{(uint32_t)arrowup.size()};
static const std::unordered_set<char> arrowseq{'\x1b', '\x5b', '\x41', '\x42'};

using menulayout = std::map<int32_t, int32_t>;

struct Menu::Handler
{
    Handler(std::shared_ptr<logging::LogIf> logIf, const std::string& title,
            menuentries&& entries) :
        logIf{logIf},
        title{title}, entries{std::move(entries)}
    {
        if (this->entries.empty())
        {
            log(logging::type::critical, "No menu entries given");
            throw std::runtime_error("No menu entries given");
        }
        std::ios_base::sync_with_stdio(false);
    }

    menulayout createlayout()
    {
        menulayout layout;
        uint32_t entrypos{}, lastentrypos{(uint32_t)entries.size() - 1};

        std::ranges::for_each(entries, [&](const auto& entry) {
            if (std::get<1>(entry)() || entrypos == lastentrypos)
            {
                auto menupos = (uint32_t)layout.size() + 1;
                layout.emplace(menupos, entrypos);
            }
            entrypos++;
        });
        return layout;
    }

    void displaymenu(const menulayout& layout, int32_t sel) const
    {
        system("clear");
        std::cout << "== " << title << " menu ==" << std::endl;

        std::ranges::for_each(layout, [&](auto& elem) {
            auto [menupos, entrypos] = elem;
            auto& entryname = std::get<std::string>(entries.at(entrypos));
            if (menupos == sel)
            {
                std::cout << "\x1b[47;30m" << menupos << " : to " << entryname
                          << "\x1b[0m" << std::endl;
            }
            else
            {
                std::cout << menupos << " : to " << entryname << std::endl;
            }
        });
        sel == 0 ? std::cout << "-> " : std::cout << "-> " << sel;
    }

    bool execmenufunc(const menulayout& layout, int32_t sel)
    {
        if (sel < (decltype(sel))layout.size())
        {
            auto pos = layout.at(sel);
            // get regular menufunc
            std::cout << "Executing operation ..." << std::endl;
            if (std::get<2>(entries.at(pos))())
            {
                std::cout << "Press enter to return to menu" << std::endl;
                waitenterpressed();
            }
            return true;
        }
        else
        {
            // get exit menufunc
            std::get<2>(entries.back())();
        }
        return false;
    }

    void run()
    {
        int32_t sel{};
        while (true)
        {
            auto layout = createlayout();
            displaymenu(layout, sel);
            sel = GetUserSel(this, layout, sel).get();
            if (!execmenufunc(layout, sel))
            {
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

    void log(logging::type type, const std::string& msg)
    {
        if (logIf)
        {
            logIf->log(type, module, msg);
        }
    }

  private:
    const std::string module{"libmenu"};
    std::shared_ptr<logging::LogIf> logIf;
    const std::string title;
    const menuentries entries;

    void waitenterpressed()
    {
        isenterpressed(-1);
    }

    struct GetUserSel
    {
        GetUserSel(Handler* handler, const menulayout& layout, int32_t pos) :
            handler{handler}, layout{layout}, initpos{pos},
            firstpos{layout.begin()->first},
            lastpos{std::prev(layout.end())->first}
        {
            std::cout << "\e[?25l";   // hide cursor
            system("stty raw -echo"); // disable line buffering
        }

        ~GetUserSel()
        {
            system("stty cooked echo");          // enable line buffering
            std::cout << "\e[?25h" << std::endl; // show cursor
        }

        struct MenuView
        {
            MenuView(GetUserSel* usersel) : usersel{usersel}
            {
                system("stty cooked echo");
            }
            ~MenuView()
            {
                system("stty raw -echo");
            }

            void refresh(int32_t pos)
            {
                usersel->handler->displaymenu(usersel->layout, pos);
            }

          private:
            GetUserSel* usersel;
        };

        int32_t get()
        {
            std::string posstr{std::to_string(initpos)}, seqstr;
            uint8_t inch{};

            while ((inch = (uint8_t)std::cin.get()) != endchar ||
                   posstr.empty() || !getnumfromstr(posstr))
            {
                if (!std::cin.rdbuf()->in_avail() && inch == escape)
                {
                    int32_t pos = 0;
                    posstr = std::to_string(pos);
                    MenuView(this).refresh(pos);
                }
                else if (inch == 0x7F && !posstr.empty())
                {
                    posstr.pop_back();
                    MenuView(this).refresh(getnumfromstr(posstr));
                }
                else if (std::isdigit(inch))
                {
                    posstr += inch;
                    auto pos = getnumfromstr(posstr);
                    pos = pos <= firstpos ? firstpos : pos;
                    pos = pos >= lastpos ? lastpos : pos;
                    posstr = std::to_string(pos);
                    MenuView(this).refresh(pos);
                }
                else if (arrowseq.contains(inch))
                {
                    seqstr += inch;
                    if (seqstr.size() >= arrowseqsize)
                    {
                        if (seqstr == arrowup)
                        {
                            auto pos = getnumfromstr(posstr);
                            pos = pos == 0          ? lastpos
                                  : pos == firstpos ? firstpos
                                                    : pos - 1;
                            posstr = std::to_string(pos);
                            MenuView(this).refresh(pos);
                        }
                        else if (seqstr == arrowdown)
                        {
                            auto pos = getnumfromstr(posstr);
                            pos = pos >= lastpos ? lastpos : pos + 1;
                            posstr = std::to_string(pos);
                            MenuView(this).refresh(pos);
                        }
                        seqstr.clear();
                    }
                }
            }
            return getnumfromstr(posstr);
        }

      private:
        const Handler* handler;
        const menulayout& layout;
        const int32_t initpos;
        const int32_t firstpos;
        const int32_t lastpos;
    };
};

Menu::Menu(std::shared_ptr<logging::LogIf> logIf, const std::string& title,
           menuentries&& entries) :
    handler{std::make_unique<Handler>(logIf, title, std::move(entries))}
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
