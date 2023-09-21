#include <cassert>
#include <climits>

#include <fmt/core.h>

#include <builtins/array.hpp>
#include <builtins/core.hpp>
#include <builtins/dict.hpp>
#include <builtins/io.hpp>
#include <builtins/sequence.hpp>
#include <builtins/string.hpp>
#include <eval.hpp>
#include <file_reader.hpp>
#include <parser.hpp>
#include <span.hpp>

#define UNUSED(e) ((void)(e))
#define RETURN_IF_INVALID(o) \
    if (!object::is_valid(o)) return o;

namespace gaya::eval
{

using namespace std::string_literals;

template <typename To, typename From>
To bit_cast(const From& src) noexcept
{
    To dst;
    std::memcpy(&dst, &src, sizeof(To));
    return dst;
}

interpreter::interpreter()
{
#define BUILTIN(name, arity, func) \
    define(name, create_builtin_function(*this, name, arity, func))

    begin_scope(env {});

    using namespace object::builtin;

    BUILTIN("typeof"s, 1, core::typeof_);
    BUILTIN("assert"s, 1, core::assert_);
    BUILTIN("tostring"s, 1, core::tostring);
    BUILTIN("tosequence"s, 1, core::tosequence);
    BUILTIN("issequence"s, 1, core::issequence);

    BUILTIN("io.println"s, 1, io::println);
    BUILTIN("io.print"s, 1, io::print);
    BUILTIN("io.readline"s, 0, io::readline);
    BUILTIN("io.readfile"s, 1, io::readfile);

    BUILTIN("string.length"s, 1, string::length);
    BUILTIN("string.concat"s, 2, string::concat);
    BUILTIN("string.tonumber"s, 1, string::tonumber);
    BUILTIN("string.index"s, 2, string::index);
    BUILTIN("string.substring"s, 3, string::substring);
    BUILTIN("string.startswith"s, 2, string::startswith);
    BUILTIN("string.endswith"s, 2, string::endswith);
    BUILTIN("string.trim"s, 1, string::trim);

    BUILTIN("array.length"s, 1, array::length);
    BUILTIN("array.concat"s, 2, array::concat);
    BUILTIN("array.push"s, 2, array::push);
    BUILTIN("array.pop"s, 1, array::pop);
    BUILTIN("array.sort"s, 2, array::sort);

    BUILTIN("dict.length"s, 1, dict::length);
    BUILTIN("dict.set"s, 3, dict::set);
    BUILTIN("dict.remove"s, 2, dict::remove);
    BUILTIN("dict.contains"s, 2, dict::contains);

    BUILTIN("seq.next"s, 1, sequence::next);
    BUILTIN("seq.make"s, 1, sequence::make);
#undef BUILTIN
}

parser& interpreter::get_parser() noexcept
{
    return _parser;
}

std::optional<object::object>
interpreter::eval(const std::string& filename, const char* source) noexcept
{
    auto ast = _parser.parse(filename, source);
    if (!ast || !_parser.diagnostics().empty())
    {
        _diagnostics = _parser.diagnostics();
        return {};
    }

    return eval(filename, ast);
}

[[nodiscard]] std::optional<object::object>
interpreter::eval(const std::string& filename, ast::node_ptr ast) noexcept
{
    _filename = filename;
    if (auto result = ast->accept(*this); object::is_valid(result))
    {
        return result;
    }
    else
    {
        return {};
    }
}

std::vector<diagnostic::diagnostic> interpreter::diagnostics() const noexcept
{
    return _diagnostics;
}

const std::string& interpreter::current_filename() const noexcept
{
    return _filename;
}

void interpreter::clear_diagnostics() noexcept
{
    _parser.diagnostics().clear();
    _diagnostics.clear();
}

void interpreter::interp_error(span s, const std::string& msg)
{
    _diagnostics.emplace_back(s, msg, diagnostic::severity::error, _filename);
}

void interpreter::interp_hint(span s, const std::string& hint)
{
    _diagnostics.emplace_back(s, hint, diagnostic::severity::hint, _filename);
}

bool interpreter::had_error() const noexcept
{
    return !_diagnostics.empty();
}

const std::vector<env>& interpreter::scopes() const noexcept
{
    return _scopes;
}

env& interpreter::environment() noexcept
{
    return _scopes.back();
}

void interpreter::define(key k, object::object v) noexcept
{
    environment().set(std::move(k), v);
}

void interpreter::begin_scope(env new_scope) noexcept
{
    _scopes.push_back(new_scope);
}

void interpreter::end_scope() noexcept
{
    assert(_scopes.size() > 0);
    _scopes.pop_back();
}

object::object interpreter::visit_program(ast::program& program)
{
    for (const auto& stmt : program.stmts)
    {
        stmt->accept(*this);
        if (had_error()) return object::invalid;
    }
    return object::invalid;
}

object::object
interpreter::visit_declaration_stmt(ast::declaration_stmt& declaration_stmt)
{
    auto value = declaration_stmt.expr->accept(*this);
    RETURN_IF_INVALID(value);

    define(key::global(declaration_stmt.ident->value), value);

    return object::invalid;
}

object::object
interpreter::visit_expression_stmt(ast::expression_stmt& expression_stmt)
{
    expression_stmt.expr->accept(*this);
    /*
     * NOTE:
     *
     * For statements I may want to return object::none or something like that.
     */
    return object::invalid;
}

object::object
interpreter::visit_assignment_stmt(ast::assignment_stmt& assignment)
{
    auto new_val = assignment.expr->accept(*this);
    RETURN_IF_INVALID(new_val);

    auto depth = assignment.ident->depth;
    auto& key  = assignment.ident->key;

    /*
     * The parser already checks that the identifier is found and that it is
     * a valid assignment target.
     */
    [[maybe_unused]] auto ok
        = environment().update_at(std::move(key), new_val, depth);
    assert(ok);

    return object::invalid;
}

object::object interpreter::visit_while_stmt(ast::while_stmt& while_stmt)
{
    begin_scope(env { std::make_shared<env>(environment()) });
    for (;;)
    {
        auto test = while_stmt.condition->accept(*this);
        RETURN_IF_INVALID(test);

        if (!object::is_truthy(test)) break;

        for (auto& stmt : while_stmt.body)
        {
            stmt->accept(*this);
            if (had_error()) return object::invalid;
        }

        if (while_stmt.continuation)
        {
            while_stmt.continuation->accept(*this);
            if (had_error()) return object::invalid;
        }
    }
    end_scope();
    return object::invalid;
}

object::object interpreter::visit_include_stmt(ast::include_stmt& include_stmt)
{
    auto filename = _filename;

    UNUSED(eval(include_stmt.file_path, include_stmt.parsed_file));
    if (had_error())
    {
        interp_error(
            include_stmt.span_,
            fmt::format(
                "There were errors while evaluating {}",
                include_stmt.file_path.string()));
    }

    _filename = filename;
    return object::invalid;
}

object::object interpreter::visit_do_expression(ast::do_expression& do_expr)
{
    begin_scope(env { std::make_shared<env>(environment()) });
    for (size_t i = 0; i < do_expr.body.size() - 1; i++)
    {
        do_expr.body[i]->accept(*this);
        if (had_error()) return object::invalid;
    }
    auto result = do_expr.body.back()->accept(*this);
    end_scope();
    return result;
}

object::object interpreter::visit_case_expression(ast::case_expression& cases)
{
    for (const auto& branch : cases.branches)
    {
        auto condition = branch.condition->accept(*this);
        RETURN_IF_INVALID(condition);

        if (object::is_truthy(condition))
        {
            return branch.body->accept(*this);
        }
    }

    if (cases.otherwise)
    {
        return cases.otherwise->accept(*this);
    }

    return object::create_unit(cases.span_);
}

[[nodiscard]] static bool match_pattern(
    interpreter& interp,
    object::object& target,
    const ast::match_pattern& pattern) noexcept
{
    switch (pattern.kind)
    {
    case ast::match_pattern::kind::wildcard:
    {
        return true;
    }
    case ast::match_pattern::kind::capture:
    {
        auto& value     = pattern.value;
        auto identifier = std::static_pointer_cast<ast::identifier>(value);
        interp.define(identifier->value, target);
        return true;
    }
    case ast::match_pattern::kind::expr:
    {
        auto expr = pattern.value->accept(interp);
        if (!object::is_valid(expr)) return false;

        return object::equals(target, expr);
    }
    }
}

[[nodiscard]] static bool match_case_branch(
    interpreter& interp,
    object::object& target,
    const ast::match_branch& branch,
    object::object* out) noexcept
{
    auto pattern_matches = match_pattern(interp, target, branch.pattern);
    auto condition_holds = true;

    if (branch.condition != nullptr)
    {
        auto c          = branch.condition->accept(interp);
        condition_holds = object::is_truthy(c);
    }

    if (pattern_matches && condition_holds)
    {
        auto result = branch.body->accept(interp);
        if (!object::is_valid(result)) return false;

        *out = result;
        return true;
    }

    return false;
}

object::object interpreter::visit_match_expression(ast::match_expression& expr)
{
    auto target = expr.target->accept(*this);
    RETURN_IF_INVALID(target);

    object::object result = object::invalid;
    for (const auto& branch : expr.branches)
    {
        begin_scope(env { std::make_shared<env>(environment()) });

        if (match_case_branch(*this, target, branch, &result))
        {
            end_scope();
            return result;
        }

        end_scope();
    }

    if (expr.otherwise != nullptr)
    {
        return expr.otherwise->accept(*this);
    }

    return object::create_unit(expr.span_);
}

static inline object::object interpret_arithmetic_expression(
    interpreter& interp,
    ast::binary_expression& expr) noexcept
{
    auto l = expr.lhs->accept(interp);
    RETURN_IF_INVALID(l);

    auto r = expr.rhs->accept(interp);
    RETURN_IF_INVALID(r)

    if (!IS_NUMBER(l) || !IS_NUMBER(r))
    {
        interp.interp_error(
            expr.op.span,
            fmt::format(
                "{} expected {} and {} to be both numbers",
                expr.op.span.to_string(),
                object::typeof_(l),
                object::typeof_(r)));
        return object::invalid;
    }

    auto fst = AS_NUMBER(l);
    auto snd = AS_NUMBER(r);

    double result = 0;

    switch (expr.op.type)
    {
    case token_type::plus: result = fst + snd; break;
    case token_type::dash: result = fst - snd; break;
    case token_type::star: result = fst * snd; break;
    case token_type::slash:
    {
        if (snd == 0)
        {
            return object::create_unit(expr.op.span);
        }
        result = fst / snd;
        break;
    }
    default: assert(false && "should not happen");
    }

    return object::create_number(expr.op.span, result);
}

static inline object::object interpret_comparison_expression(
    interpreter& interp,
    ast::binary_expression& expr) noexcept
{
    auto l = expr.lhs->accept(interp);
    RETURN_IF_INVALID(l);

    auto r = expr.rhs->accept(interp);
    RETURN_IF_INVALID(r);

    int result = 0;

    switch (expr.op.type)
    {
    case token_type::equal_equal:
    {
        result = object::equals(l, r) ? 1 : 0;
        break;
    }
    case token_type::not_equals:
    {
        result = !object::equals(l, r) ? 1 : 0;
        break;
    }
    default:
    {
        if (!object::is_comparable(l) || !object::is_comparable(r))
        {
            interp.interp_error(
                expr.op.span,
                fmt::format(
                    "{} and {} are not both comparable",
                    object::typeof_(l),
                    object::typeof_(r)));
            return object::invalid;
        }

        int cmp;
        if (!object::cmp(l, r, &cmp))
        {
            interp.interp_error(
                expr.op.span,
                fmt::format(
                    "cant' compare {} and {}",
                    object::typeof_(l),
                    object::typeof_(r)));
            return object::invalid;
        }

        switch (expr.op.type)
        {
        case token_type::less_than: result = cmp < 0 ? 1 : 0; break;
        case token_type::less_than_eq: result = cmp <= 0 ? 1 : 0; break;
        case token_type::greater_than: result = cmp > 0 ? 1 : 0; break;
        case token_type::greater_than_eq: result = cmp >= 0 ? 1 : 0; break;
        default: assert(false && "unreachable");
        }
    }
    }

    return object::create_number(expr.op.span, result);
}

static inline object::object interpret_logical_expression(
    interpreter& interp,
    ast::binary_expression& expr) noexcept
{

    auto l = expr.lhs->accept(interp);
    RETURN_IF_INVALID(l);

    if (expr.op.type == token_type::and_)
    {
        if (object::is_truthy(l))
        {
            return expr.rhs->accept(interp);
        }
        else
        {
            return l;
        }
    }

    if (expr.op.type == token_type::or_)
    {
        if (object::is_truthy(l))
        {
            return l;
        }
        else
        {
            return expr.rhs->accept(interp);
        }
    }

    assert(false && "unreachable");
}

object::object
interpreter::visit_binary_expression(ast::binary_expression& binop)
{
    switch (binop.op.type)
    {
    case token_type::less_than:
    case token_type::less_than_eq:
    case token_type::greater_than:
    case token_type::greater_than_eq:
    case token_type::equal_equal:
    case token_type::not_equals:
    {
        return interpret_comparison_expression(*this, binop);
    }
    case token_type::plus:
    case token_type::star:
    case token_type::dash:
    case token_type::slash:
    {
        return interpret_arithmetic_expression(*this, binop);
    }
    case token_type::pipe:
    {
        auto replacement = binop.lhs->accept(*this);
        RETURN_IF_INVALID(replacement);

        begin_scope(env { std::make_shared<env>(environment()) });

        static key underscore = key::local("_");
        define(underscore, replacement);
        _placeholders_in_use += 1;

        auto result = binop.rhs->accept(*this);
        end_scope();

        if (_placeholders_in_use != 0 && !_had_unused_placeholders)
        {
            interp_error(binop.op.span, "The result of a pipe was not used");
            interp_hint(binop.op.span, "Maybe you forgot to use a '_'?");
            _had_unused_placeholders = true;
            return object::invalid;
        }

        return result;
    }
    case token_type::and_:
    case token_type::or_:
    {
        return interpret_logical_expression(*this, binop);
    }
    case token_type::diamond:
    {
        auto s1 = binop.lhs->accept(*this);
        if (!IS_STRING(s1))
        {
            interp_error(binop.op.span, "Expected lhs to '<>' to be a string");
            return object::invalid;
        }

        auto s2 = binop.rhs->accept(*this);

        std::string result = AS_STRING(s1)
            + (IS_STRING(s2) ? AS_STRING(s2) : object::to_string(*this, s2));

        return object::create_string(*this, binop.op.span, std::move(result));
    }
    case token_type::land:
    case token_type::lor:
    case token_type::xor_:
    case token_type::lshift:
    case token_type::rshift:
    {
        auto span = binop.op.span;
        auto x    = binop.lhs->accept(*this);
        RETURN_IF_INVALID(x);

        if (!IS_NUMBER(x))
        {
            interp_error(
                binop.op.span,
                fmt::format(
                    "Expected left of '{}' to be a number",
                    span.to_string()));
            return object::invalid;
        }

        auto y = binop.rhs->accept(*this);
        RETURN_IF_INVALID(y);

        if (!IS_NUMBER(y))
        {
            interp_error(
                binop.op.span,
                fmt::format(
                    "Expected right of '{}' to be a number",
                    span.to_string()));
            return object::invalid;
        }

        auto i = static_cast<int>(AS_NUMBER(x));
        auto j = static_cast<int>(AS_NUMBER(y));

        switch (binop.op.type)
        {
        case token_type::land: return object::create_number(span, i & j);
        case token_type::lor: return object::create_number(span, i | j);
        case token_type::xor_: return object::create_number(span, i ^ j);
        case token_type::lshift: return object::create_number(span, i << j);
        case token_type::rshift: return object::create_number(span, i >> j);
        default: assert("unreachable");
        }
    }
    default:
    {
        assert(0 && "unhandled case in visit_binary_expression");
    }
    }

    assert(0 && "unhandled case in visit_binary_expression");
}

object::object interpreter::visit_lnot_expression(ast::lnot_expression& e)
{
    auto n = e.operand->accept(*this);
    RETURN_IF_INVALID(n);

    if (!IS_NUMBER(n))
    {
        interp_error(e.op.span, "Expected operand to '~' to be a number");
        return object::invalid;
    }

    int num           = static_cast<unsigned int>(AS_NUMBER(n));
    int num_flipped   = ~num;
    double num_double = static_cast<double>(num_flipped);

    return object::create_number(e.op.span, num_double);
}

object::object interpreter::visit_not_expression(ast::not_expression& expr)
{
    auto value = expr.operand->accept(*this);
    RETURN_IF_INVALID(value);

    return gaya::eval::object::create_number(
        expr.op.span,
        !gaya::eval::object::is_truthy(value));
}

object::object
interpreter::visit_perform_expression(ast::perform_expression& expr)
{
    expr.stmt->accept(*this);
    if (had_error()) return object::invalid;

    return gaya::eval::object::create_unit(expr.op.span);
}

object::object interpreter::visit_call_expression(ast::call_expression& cexpr)
{
    auto o = cexpr.target->accept(*this);
    RETURN_IF_INVALID(o);

    if (!object::is_callable(o))
    {
        interp_error(
            cexpr.span_,
            fmt::format(
                "Expected a callable but got: {}",
                object::to_string(*this, o)));

        interp_hint(
            cexpr.span_,
            "To define a function, do f :: { <args> => <expr> }");

        return object::invalid;
    }

    auto arity   = object::arity(o);
    size_t nargs = cexpr.args.size();

    if (arity != nargs)
    {
        interp_error(
            cexpr.span_,
            fmt::format(
                "Wrong number of arguments provided to callable, {} {} "
                "expected",
                arity,
                arity == 1 ? "was" : "were"));
        return object::invalid;
    }

    for (size_t i = 0; i < nargs; i++)
    {
        auto result = cexpr.args[i]->accept(*this);
        RETURN_IF_INVALID(result);

        cexpr.oargs[i] = std::move(result);
    }

    return object::call(o, *this, cexpr.span_, cexpr.oargs);
}

object::object
interpreter::visit_function_expression(ast::function_expression& fexpr)
{
    std::vector<key> params;
    std::transform(
        fexpr.params.begin(),
        fexpr.params.end(),
        std::back_inserter(params),
        [](auto& param) { return key::param(param.value); });

    return object::create_function(
        *this,
        fexpr._span,
        std::make_unique<env>(environment()),
        std::move(params),
        fexpr.body);
}

object::object
interpreter::visit_let_expression(ast::let_expression& let_expression)
{
    begin_scope(env { std::make_shared<env>(environment()) });

    for (auto& binding : let_expression.bindings)
    {
        auto value = binding.value->accept(*this);
        RETURN_IF_INVALID(value);

        define(key::local(binding.ident->value), value);
    }

    auto result = let_expression.expr->accept(*this);

    end_scope();

    return result;
}

object::object interpreter::visit_array(ast::array& ary)
{
    std::vector<object::object> elems;

    for (auto& elem : ary.elems)
    {
        auto o = elem->accept(*this);
        RETURN_IF_INVALID(o);

        elems.push_back(o);
    }

    return object::create_array(*this, ary.span_, std::move(elems));
}

object::object interpreter::visit_dictionary(ast::dictionary& dict_expr)
{
    robin_hood::unordered_map<object::object, object::object> dict;

    for (size_t i = 0; i < dict_expr.keys.size(); i++)
    {
        auto key = dict_expr.keys[i]->accept(*this);
        RETURN_IF_INVALID(key);

        auto value = dict_expr.values[i]->accept(*this);
        RETURN_IF_INVALID(value);

        dict.insert({ key, value });
    }

    return object::create_dictionary(*this, dict_expr.span_, std::move(dict));
}

object::object interpreter::visit_number(ast::number& n)
{
    return object::create_number(n._span, n.value);
}

object::object interpreter::visit_string(ast::string& s)
{
    return object::create_string(*this, s._span, s.value);
}

object::object interpreter::visit_identifier(ast::identifier& identifier)
{
    /* The parser already checks for undefined identifiers. */
    auto o = environment().get_at(identifier.key, identifier.depth);
    assert(object::is_valid(o));
    return o;
}

object::object interpreter::visit_unit(ast::unit& u)
{
    return object::create_unit(u._span);
}

object::object interpreter::visit_placeholder(ast::placeholder& p)
{
    static key underscore = key::local("_");
    if (auto value = environment().get(underscore); object::is_valid(value))
    {
        if (_placeholders_in_use > 0)
        {
            _placeholders_in_use -= 1;
        }
        return value;
    }
    else
    {
        interp_error(p.span_, "Failed to replace placeholder");
        interp_hint(p.span_, "Placeholders can only be used in pipelines");
        return object::invalid;
    }
}

}
