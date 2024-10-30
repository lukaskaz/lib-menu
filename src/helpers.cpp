#include "menu/helpers.hpp"

#include <charconv>

namespace menu
{

int32_t getnumfromstr(std::string_view sv)
{
    int32_t num{};
    const auto sstart = sv.data(), send = sv.data() + sv.size();
    if (const auto ret = std::from_chars(sstart, send, num);
        ret.ec == std::errc{} && ret.ptr == send)
    {
        return num;
    }
    return 0;
}

} // namespace menu
