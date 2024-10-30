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

enum font
{
    reset = 0x00,
    bold = 0x01,
    norm = 0x02,
    italic = 0x03,
    fgblack = 0x1e,
    fgred = 0x1f,
    fggreen = 0x20,
    fgyellow = 0x21,
    fgblue = 0x22,
    fgmagenta = 0x23,
    fgcyan = 0x24,
    fgwhite = 0x25,
    bgblack = 0x28,
    bgred = 0x29,
    bggreen = 0x2a,
    bgyellow = 0x2b,
    bgblue = 0x2c,
    bgmagenta = 0x2d,
    bgcyan = 0x2e,
    bgwhite = 0x2f,
};

using entriesmap = std::map<int32_t, std::pair<int32_t, const std::string&>>;
using menulayout = std::pair<const std::string&, entriesmap>;

struct Menu::Handler
{
    Handler(std::shared_ptr<logging::LogIf> logIf, const std::string& title,
            menuentries&& entries) :
        logIf{logIf},
        title{title}, entriesall{std::move(entries)}
    {
        if (this->entriesall.empty())
        {
            log(logging::type::critical, "No menu entriesall given");
            throw std::runtime_error("No menu entriesall given");
        }
        std::ios_base::sync_with_stdio(false);
    }

    menulayout createlayout()
    {
        entriesmap entriesshown;
        int32_t entrypos{}, lastentrypos{(int32_t)entriesall.size() - 1};

        std::ranges::for_each(entriesall, [&](const auto& entry) {
            auto& tobeshown = std::get<1>(entry);
            if (tobeshown() || entrypos == lastentrypos)
            {
                auto menupos = (uint32_t)entriesshown.size() + 1;
                entriesshown.try_emplace(menupos, entrypos, std::get<0>(entry));
            }
            entrypos++;
        });
        return {title, std::move(entriesshown)};
    }

    bool execfunction(const menulayout& layout, int32_t sel)
    {
        auto& entriesshown = std::get<entriesmap>(layout);
        if (sel < (decltype(sel))entriesshown.size())
        {
            auto basepos = std::get<int32_t>(entriesshown.at(sel));
            std::cout << "Executing operation ..." << std::endl;
            auto& functoexec = std::get<2>(entriesall.at(basepos));
            if (functoexec())
            {
                std::cout << "Press enter to return to menu" << std::endl;
                waitenterpressed();
            }
            return true;
        }
        else
        {
            auto& functoexit = std::get<2>(entriesall.back());
            functoexit();
        }
        return false;
    }

    void run()
    {
        int32_t sel{};
        while (true)
        {
            auto layout = createlayout();
            sel = Selection(layout, sel).get();
            if (!execfunction(layout, sel))
            {
                break;
            }
        }
    }

    static bool isenterpressed(int32_t timeoutMs)
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
    const menuentries entriesall;

    void waitenterpressed()
    {
        isenterpressed(-1);
    }

    static int32_t getfirst(const menulayout& layout)
    {
        return std::get<entriesmap>(layout).begin()->first;
    }

    static int32_t getlast(const menulayout& layout)
    {
        return std::prev(std::get<entriesmap>(layout).end())->first;
    }

    struct Selection
    {
        Selection(const menulayout& layout, int32_t pos) :
            initpos{pos}, firstpos{getfirst(layout)}, lastpos{getlast(layout)},
            menuview{layout, pos}
        {
            system("stty raw -echo"); // disable line buffering
        }

        ~Selection()
        {
            system("stty cooked echo"); // enable line buffering
        }

        int32_t get()
        {
            std::string posstr{std::to_string(initpos)}, seqstr;
            uint8_t inch{};

            while ((inch = (decltype(inch))std::cin.get()) != endchar ||
                   posstr.empty() || !getnumfromstr(posstr))
            {
                if (!std::cin.rdbuf()->in_avail() && inch == escape)
                {
                    int32_t pos = 0;
                    posstr = std::to_string(pos);
                    menuview.refresh(pos);
                }
                else if (inch == 0x7F && !posstr.empty())
                {
                    posstr.pop_back();
                    menuview.refresh(getnumfromstr(posstr));
                }
                else if (std::isdigit(inch))
                {
                    posstr += inch;
                    auto pos = getnumfromstr(posstr);
                    pos = pos <= firstpos ? firstpos : pos;
                    pos = pos >= lastpos ? lastpos : pos;
                    posstr = std::to_string(pos);
                    menuview.refresh(pos);
                }
                else if (arrowseq.contains(inch))
                {
                    seqstr += inch;
                    if (!std::cin.rdbuf()->in_avail() &&
                        seqstr.size() >= arrowseqsize)
                    {
                        seqstr = seqstr.substr(seqstr.size() - arrowseqsize);
                        if (seqstr == arrowup)
                        {
                            auto pos = getnumfromstr(posstr);
                            pos = pos == 0          ? lastpos
                                  : pos == firstpos ? firstpos
                                                    : pos - 1;
                            posstr = std::to_string(pos);
                            menuview.refresh(pos);
                        }
                        else if (seqstr == arrowdown)
                        {
                            auto pos = getnumfromstr(posstr);
                            pos = pos >= lastpos ? lastpos : pos + 1;
                            posstr = std::to_string(pos);
                            menuview.refresh(pos);
                        }
                        seqstr.clear();
                    }
                }
            }
            return getnumfromstr(posstr);
        }

      private:
        const int32_t initpos;
        const int32_t firstpos;
        const int32_t lastpos;
        struct MenuView
        {
            MenuView(const menulayout& layout, int32_t pos) :
                layout{layout}, prevpos{pos}
            {
                std::cout << "\e[?25l"; // hide cursor
                display(pos);
            }

            ~MenuView()
            {
                std::cout << "\e[?25h" << std::endl; // show cursor
            }

            void refresh(int32_t pos)
            {
                if (pos != prevpos)
                {
                    display(pos);
                    prevpos = pos;
                }
            }

          private:
            const menulayout& layout;
            int32_t prevpos;

            void display(int32_t pos)
            {
                system("clear");
                std::cout << "== " << layout.first << " menu ==" << endchar
                          << std::endl;

                std::ranges::for_each(
                    std::get<entriesmap>(layout), [&](const auto& elem) {
                        auto& [menupos, entry] = elem;
                        auto& entryname = std::get<const std::string&>(entry);
                        if (menupos == pos)
                        {
                            std::cout << "\x1b[" << font::bold << ";"
                                      << font::fgblack << ";" << font::bgwhite
                                      << "m" << menupos << " : to " << entryname
                                      << "\x1b[" << font::reset << "m"
                                      << endchar << std::endl;
                        }
                        else
                        {
                            std::cout << menupos << " : to " << entryname
                                      << endchar << std::endl;
                        }
                    });
                pos == 0 ? std::cout << "-> " : std::cout << "-> " << pos;
            }
        } menuview;
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

std::string Menu::info() const
{
    return handler->getinfo();
}

bool Menu::isenterpressed()
{
    return Handler::isenterpressed(0);
}

} // namespace menu::cli
