#include <cassert>
#include <numeric>
#include <sstream>
#include <string>

#include <fmt/core.h>

#include <ast.hpp>
#include <ast_visitor.hpp>
#include <eval.hpp>
#include <object.hpp>

// NOTE: Consider using a JSON library for stringification.

namespace gaya::ast
{

template <typename Container, typename Transform>
std::string join(const Container& c, Transform transform)
{
    return std::transform_reduce(
        c.empty() ? c.cbegin() : c.cbegin() + 1,
        c.cend(),
        c.empty() ? std::string() : transform(c[0]),
        [](auto acc, auto x) { return acc + "," + x; },
        transform);
}

std::string program::to_string() const noexcept
{
    std::stringstream ss;
    ss << "{\"type\": \"program\","
       << "\"stmts\": ["
       << join(stmts, [](auto& stmt) { return stmt->to_string(); }) << "]}";
    return ss.str();
}

gaya::eval::object::object_ptr program::accept(ast_visitor& v)
{
    return v.visit_program(*this);
}

std::string declaration_stmt::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "declaration_stmt", "identifier":)" << ident->to_string()
       << R"(, "expression": )" << expr->to_string() << "}";
    return ss.str();
}

gaya::eval::object::object_ptr declaration_stmt::accept(ast_visitor& v)
{
    return v.visit_declaration_stmt(*this);
}

std::string expression_stmt::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "expression_stmt",)"
       << R"("expression": )" << expr->to_string() << "}";
    return ss.str();
}

gaya::eval::object::object_ptr expression_stmt::accept(ast_visitor& v)
{
    return v.visit_expression_stmt(*this);
}

std::string assignment_stmt::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "assignment_stmt", "identifier":)" << ident->to_string()
       << R"(, "expression": )" << expr->to_string() << "}";
    return ss.str();
}

gaya::eval::object::object_ptr assignment_stmt::accept(ast_visitor& v)
{
    return v.visit_assignment_stmt(*this);
}

std::string WhileStmt::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "while_stmt", "condition":)" << condition->to_string()
       << R"(, "body": [)"
       << join(body, [](auto& stmt) { return stmt->to_string(); }) << "]}";
    return ss.str();
}

gaya::eval::object::object_ptr WhileStmt::accept(ast_visitor& v)
{
    return v.visit_while_stmt(*this);
}

std::string do_expression::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "do_expression", "body": [)"
       << join(body, [](auto& node) { return node->to_string(); }) << "]}";
    return ss.str();
}

gaya::eval::object::object_ptr do_expression::accept(ast_visitor& v)
{
    return v.visit_do_expression(*this);
}

std::string case_branch::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "case_branch", "condition": )" << condition->to_string()
       << R"(, "body": )" << body->to_string() << "}";
    return ss.str();
}

std::string case_expression::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "case_expression", "branches": [)"
       << join(branches, [](auto& branch) { return branch.to_string(); })
       << "]";
    if (otherwise)
    {
        ss << ", \"otherwise\": " << otherwise->to_string();
    }
    ss << "}";
    return ss.str();
}

gaya::eval::object::object_ptr case_expression::accept(ast_visitor& v)
{
    return v.visit_case_expression(*this);
}

std::string call_expression::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "call_expression", "args": [)"
       << join(args, [](auto& arg) { return arg->to_string(); })
       << R"(], "identifier": )" << target->to_string() << "}";
    return ss.str();
}

gaya::eval::object::object_ptr call_expression::accept(ast_visitor& v)
{
    return v.visit_call_expression(*this);
}

std::string function_expression::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "function_expression", "params": [)"
       << join(params, [](auto& param) { return param.to_string(); })
       << R"(], "body": )" << body->to_string() << "}";
    return ss.str();
}

gaya::eval::object::object_ptr function_expression::accept(ast_visitor& v)
{
    return v.visit_function_expression(*this);
}

std::string let_expression::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "let_expression", "identifier": )" << ident->to_string()
       << R"(, "binding": )" << binding->to_string() << R"(, "expr": )"
       << expr->to_string() << "}";
    return ss.str();
}

gaya::eval::object::object_ptr let_expression::accept(ast_visitor& v)
{
    return v.visit_let_expression(*this);
}

gaya::eval::object::object_ptr binary_expression::accept(ast_visitor& v)
{
    return v.visit_binary_expression(*this);
}

std::string binary_expression::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "binary_expression", "op": ")"
       << op.get_span().to_string() << R"(", "lhs": )" << lhs->to_string()
       << R"(, "rhs": )" << rhs->to_string() << "}";
    return ss.str();
}

gaya::eval::object::object_ptr
cmp_expression::execute(eval::interpreter& interp)
{
    auto l = lhs->accept(interp);
    if (!l) return nullptr;

    auto r = rhs->accept(interp);
    if (!r) return nullptr;

    if (!l->is_comparable() || !r->is_comparable())
    {
        interp.interp_error(
            op.get_span(),
            fmt::format(
                "{} and {} are not both comparable",
                l->typeof_(),
                r->typeof_()));
        return nullptr;
    }

    if (l->typeof_() != r->typeof_() && op.type() == token_type::equal_equal)
    {
        return std::make_shared<eval::object::number>(op.get_span(), 0);
    }

    auto result
        = std::dynamic_pointer_cast<eval::object::comparable>(l)->cmp(r);
    if (!result)
    {
        interp.interp_error(
            op.get_span(),
            fmt::format(
                "Cannot compare {} and {}",
                l->typeof_(),
                r->typeof_()));
        return nullptr;
    }

    int ret = 0;

    switch (op.type())
    {
    case token_type::less_than: ret = result < 0; break;
    case token_type::less_than_eq: ret = result <= 0; break;
    case token_type::greater_than: ret = result > 0; break;
    case token_type::greater_than_eq: ret = result >= 0; break;
    case token_type::equal_equal: ret = result == 0; break;
    default: assert(false && "should not happen");
    }

    return std::make_shared<eval::object::number>(op.get_span(), ret);
}

gaya::eval::object::object_ptr
arithmetic_expression::execute(eval::interpreter& interp)
{
    auto l = lhs->accept(interp);
    if (!l) return nullptr;

    auto r = rhs->accept(interp);
    if (!r) return nullptr;

    if (l->typeof_() != "number" || r->typeof_() != "number")
    {
        interp.interp_error(
            op.get_span(),
            fmt::format(
                "{} expected {} and {} to be both numbers",
                op.get_span().to_string(),
                l->typeof_(),
                r->typeof_()));
        return nullptr;
    }

    auto fst = std::static_pointer_cast<eval::object::number>(l)->value;
    auto snd = std::static_pointer_cast<eval::object::number>(r)->value;

    double result = 0;

    switch (op.type())
    {
    case token_type::plus: result = fst + snd; break;
    case token_type::dash: result = fst - snd; break;
    case token_type::star: result = fst * snd; break;
    case token_type::slash:
    {
        if (snd == 0)
        {
            return std::make_shared<eval::object::unit>(op.get_span());
        }
        result = fst / snd;
        break;
    }
    default: assert(false && "should not happen");
    }

    return std::make_shared<eval::object::number>(op.get_span(), result);
}

/* Unary expressions */

gaya::eval::object::object_ptr unary_expression::accept(ast_visitor& v)
{
    return v.visit_unary_expression(*this);
}

std::string unary_expression::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "unary_expression", "operator": ")"
       << op.get_span().to_string() << R"(", "operand": )"
       << operand->to_string() << "}";
    return ss.str();
}

gaya::eval::object::object_ptr
not_expression::execute(eval::interpreter& interp)
{
    auto value = operand->accept(interp);
    if (!value) return nullptr;

    return std::make_shared<eval::object::number>(
        op.get_span(),
        !value->is_truthy());
}

/* Primary expressions */

std::string number::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "number", "value":)" << '"' << value << "\"}";
    return ss.str();
}

gaya::eval::object::object_ptr number::accept(ast_visitor& v)
{
    return v.visit_number(*this);
}

std::string string::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "string", "value": ")" << value << "\"}";
    return ss.str();
}

gaya::eval::object::object_ptr string::accept(ast_visitor& v)
{
    return v.visit_string(*this);
}

std::string identifier::to_string() const noexcept
{
    std::stringstream ss;
    ss << R"({"type": "identifier", "value":)" << '"' << value << "\"}";
    return ss.str();
}

gaya::eval::object::object_ptr identifier::accept(ast_visitor& v)
{
    return v.visit_identifier(*this);
}

std::string unit::to_string() const noexcept
{
    return R"({"type": "unit"})";
}

gaya::eval::object::object_ptr unit::accept(ast_visitor& v)
{
    return v.visit_unit(*this);
}

}
