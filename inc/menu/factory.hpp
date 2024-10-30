#pragma once

#include "log/interfaces/logging.hpp"
#include "menu/interfaces/menu.hpp"

#include <memory>

namespace menu
{

class MenuFactory
{
  public:
    template <typename T>
    static std::shared_ptr<MenuIf> create(const std::string& title,
                                          menuentries&& entries)
    {
        std::shared_ptr<logging::LogIf> logIf;
        return create<T>(logIf, title, std::move(entries));
    }

    template <typename T>
    static std::shared_ptr<MenuIf> create(std::shared_ptr<logging::LogIf> logIf,
                                          const std::string& title,
                                          menuentries&& entries)
    {
        return std::shared_ptr<T>(new T(logIf, title, std::move(entries)));
    }
};

} // namespace menu
