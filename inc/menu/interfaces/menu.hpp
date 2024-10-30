#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace menu
{

using showfunc = std::function<bool()>;
using menufunc = std::function<bool()>;
using menuentries = std::vector<std::tuple<std::string, showfunc, menufunc>>;

class MenuIf
{
  public:
    virtual ~MenuIf() = default;

    virtual void run() const = 0;
    virtual std::string info() const = 0;
};

} // namespace menu
