#pragma once

#include "menu/factory.hpp"
#include "menu/interfaces/menu.hpp"

namespace menu::cli
{

class Menu : public MenuIf
{
  public:
    ~Menu();

    void run() const override;
    bool isenterpressed() const override;
    std::string info() const override;

  private:
    friend class menu::MenuFactory;
    Menu(std::shared_ptr<logging::LogIf>, const std::string&, menuentries&&);
    struct Handler;
    std::unique_ptr<Handler> handler;
};

} // namespace menu::cli
