#include <eval.hpp>
#include <types.hpp>
#include <iostream>

namespace gaya::types
{

std::optional<ForeignType> foreign_type_from_string(const std::string& s)
{
    if (s == "c.Int") return ForeignType::c_Int;
    if (s == "c.Void") return ForeignType::c_Void;
    if (s == "c.Pointer") return ForeignType::c_Pointer;
    return {};
}

TypeConstraint TypeConstraint::with_closed_over_env(
    std::shared_ptr<eval::env> env) const noexcept
{
    return TypeConstraint { env, condition };
}

TypeKind Type::kind() const noexcept
{
    return _kind;
}

bool Type::check(eval::interpreter& interp, const eval::object::object& o)
    const noexcept
{
    bool type_ok       = false;
    bool constraint_ok = true;

    switch (_kind)
    {
    case TypeKind::Any:
    {
        type_ok = true;
        break;
    }
    case TypeKind::Array:
    {
        type_ok = IS_ARRAY(o);
        break;
    }
    case TypeKind::Dictionary:
    {
        type_ok = IS_DICTIONARY(o);
        break;
    }
    case TypeKind::Function:
    {
        type_ok = IS_FUNCTION(o) || IS_BUILTIN_FUNCION(o);
        break;
    }
    case TypeKind::Number:
    {
        type_ok = IS_NUMBER(o);
        break;
    }
    case TypeKind::Sequence:
    {
        type_ok = eval::object::is_sequence(o);
        break;
    }
    case TypeKind::String:
    {
        type_ok = IS_STRING(o);
        break;
    }
    case TypeKind::Unit:
    {
        type_ok = IS_UNIT(o);
        break;
    }
    case TypeKind::Struct:
    {
        type_ok = IS_STRUCT(o) && AS_STRUCT(o).name == _declared_type_name;
        break;
    }
    }

    if (_constraint.condition != nullptr)
    {
        assert(_constraint.condition && _constraint.closed_over_env);
        using namespace gaya::eval;
        std::cout << "Type::check - before env creation" << std::endl;
        env e (_constraint.closed_over_env);
        std::cout << "Type::check - after env creation" << std::endl;
        interp.begin_scope(e);
        std::cout << "Type::check - after begin scope" << std::endl;
        interp.define(key::global("_"), o);
        std::cout << "Type::check - before accept" << std::endl;
        auto result = _constraint.condition->accept(interp);
        std::cout << "Type::check - after accept" << std::endl;
        interp.end_scope();
        std::cout << "Type::check - after end scope" << std::endl;
        if (!object::is_valid(result)) { std::cout << "Type::check - end with invalid object" << std::endl; return false; }
        constraint_ok = object::is_truthy(result);
    }

    std::cout << "Type::check - end" << std::endl;
    return type_ok && constraint_ok;
}

std::string Type::to_string() const noexcept
{
    if (!_declared_type_name.empty())
    {
        return _declared_type_name;
    }

    switch (_kind)
    {
    case TypeKind::Any: return "Any";
    case TypeKind::Array: return "Array";
    case TypeKind::Dictionary: return "Dictionary";
    case TypeKind::Function: return "Function";
    case TypeKind::Number: return "Number";
    case TypeKind::Sequence: return "Sequence";
    case TypeKind::String: return "String";
    case TypeKind::Struct: return "Struct";
    case TypeKind::Unit: return "Unit";
    }

    assert(0 && "Unhandled case in Type::to_string");
}

std::optional<Type> Type::from_string(const std::string& s) noexcept
{
    if (s == "Dictionary")
        return Type { TypeKind::Dictionary };
    else if (s == "Any")
        return Type { TypeKind::Any };
    else if (s == "Array")
        return Type { TypeKind::Array };
    else if (s == "Function")
        return Type { TypeKind::Function };
    else if (s == "Number")
        return Type { TypeKind::Number };
    else if (s == "Sequence")
        return Type { TypeKind::Sequence };
    else if (s == "String")
        return Type { TypeKind::String };
    else
        return {};
}

TypeConstraint Type::constraint() const noexcept
{
    return _constraint;
}

Type Type::with_constraint(TypeConstraint new_constraint) const noexcept
{
    return Type {
        _declared_type_name,
        _kind,
        new_constraint,
    };
}

}
