#include <fmt/core.h>

#include <builtins/io.hpp>
#include <eval.hpp>
#include <object.hpp>

namespace gaya::eval::object::builtin::io
{

gaya::eval::object::maybe_object
println(interpreter&, span span, std::vector<object> args) noexcept
{
    using namespace gaya::eval::object;

    auto x = args[0];
    auto s = to_string(x);

    if (x.type == object_type_string)
    {
        s.erase(s.cbegin());
        s.erase(s.cend() - 1);
    }

    fmt::println("{}", s);

    return create_unit(span);
}

}
