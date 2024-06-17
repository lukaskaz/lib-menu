#pragma once

#include <functional>
#include <string>
#include <vector>

using func = std::function<void()>;

class Menu
{
  public:
    Menu(const std::string&, std::vector<std::pair<std::string, func>>&&);

    void run() const;

    static void waitenterpressed()
    {
        isenterpressed(-1);
    }

    static bool isenterpressed(int32_t timeoutMs = 0);

  private:
    const std::string title;
    const std::vector<std::pair<std::string, func>> entries;

    uint32_t getusersel() const;
};
