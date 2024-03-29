#include <fmt/core.h>

#include <builtins/core.hpp>
#include <eval.hpp>
#include <object.hpp>

namespace gaya::eval::object::builtin::core
{

gaya::eval::object::object typeof_(
    interpreter& interp,
    span span,
    const std::vector<object>& args) noexcept
{
    auto& o   = args[0];
    auto type = gaya::eval::object::typeof_(o);
    return create_string(interp, span, type);
}

gaya::eval::object::object assert_(
    interpreter& interp,
    span span,
    const std::vector<object>& args) noexcept
{
    if (is_truthy(args[0]))
    {
        return create_unit(span);
    }
    else
    {
        interp.interp_error(
            span,
            fmt::format(
                "assertion failed at {}:{}",
                interp.current_filename(),
                span.lineno()));
        return gaya::eval::object::invalid;
    }
}

gaya::eval::object::object tostring(
    interpreter& interp,
    span span,
    const std::vector<object>& args) noexcept
{
    if (IS_STRING(args[0]))
    {
        return args[0];
    }
    return create_string(
        interp,
        span,
        gaya::eval::object::to_string(interp, args[0]));
}

/* issequence */

gaya::eval::object::object
issequence(interpreter&, span span, const std::vector<object>& args) noexcept
{
    return create_number(span, gaya::eval::object::is_sequence(args[0]));
}

/* tosequence */

gaya::eval::object::object tosequence(
    interpreter& interp,
    span span,
    const std::vector<object>& args) noexcept
{
    if (!is_sequence(args[0]))
    {
        interp.interp_error(
            span,
            fmt::format(
                "{} can't be converted to a sequence",
                to_string(interp, args[0])));
    }

    auto o        = args[0];
    auto sequence = to_sequence(interp, o);

    return sequence;
}

}
