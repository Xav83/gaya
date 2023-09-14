#pragma once

#include <object.hpp>

namespace gaya::eval::object::builtin::string
{

struct length final : public builtin_function
{
    length()
        : builtin_function("string.length")
    {
    }

    size_t arity() const noexcept override;

    object_ptr
    call(interpreter&, span, std::vector<object_ptr>) noexcept override;
};

struct concat final : public builtin_function
{
    concat()
        : builtin_function { "string.concat" }
    {
    }

    size_t arity() const noexcept override;

    object_ptr
    call(interpreter&, span, std::vector<object_ptr>) noexcept override;
};

}
