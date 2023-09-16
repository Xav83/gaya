#pragma once

#include <array>
#include <ast_visitor.hpp>
#include <diagnostic.hpp>
#include <env.hpp>
#include <object.hpp>
#include <span.hpp>

namespace gaya::eval
{

class interpreter final : public ast::ast_visitor
{
public:
    interpreter(const std::string& filename, const char* source);

    [[nodiscard]] std::optional<object::object> eval() noexcept;

    /// This one is useful for REPLs.
    [[nodiscard]] std::optional<object::object>
        eval(env, ast::node_ptr) noexcept;

    [[nodiscard]] std::vector<diagnostic::diagnostic>
    diagnostics() const noexcept;

    /// Return the name of the file currently being interpreted.
    [[nodiscard]] const std::string& current_filename() const noexcept;

    /// Remove all diagnostics from this interpreter.
    void clear_diagnostics() noexcept;

    /// Get the interpreter's environment.
    [[nodiscard]] const env& get_env() const noexcept;

    /// Define a new symbol in the current scope.
    void define(key, object::object) noexcept;

    /// Begin a new scope.
    void begin_scope(env new_scope) noexcept;

    /// End a previous scope.
    void end_scope() noexcept;

    /// Add an interpreter error.
    void interp_error(span, const std::string& msg);

    /// Add an interpreter hint.
    void interp_hint(span, const std::string& hint);

    /// Evaluate the given file in the context of this interpreter.
    [[nodiscard]] bool loadfile(const std::string&) noexcept;

    /// Whether the interpreter had an error.
    [[nodiscard]] bool had_error() const noexcept;

    /**
     * @return The stack of scopes of this interpreter.
     */
    [[nodiscard]] const std::vector<env>& scopes() const noexcept;

    /* Visitor pattern */

    object::object visit_program(ast::program&) override;
    object::object visit_declaration_stmt(ast::declaration_stmt&) override;
    object::object visit_expression_stmt(ast::expression_stmt&) override;
    object::object visit_assignment_stmt(ast::assignment_stmt&) override;
    object::object visit_while_stmt(ast::while_stmt&) override;
    object::object visit_do_expression(ast::do_expression&) override;
    object::object visit_case_expression(ast::case_expression&) override;
    object::object visit_not_expression(ast::not_expression&) override;
    object::object visit_perform_expression(ast::perform_expression&) override;
    object::object visit_binary_expression(ast::binary_expression&) override;
    object::object visit_call_expression(ast::call_expression&) override;
    object::object
    visit_function_expression(ast::function_expression&) override;
    object::object visit_let_expression(ast::let_expression&) override;
    object::object visit_array(ast::array&) override;
    object::object visit_number(ast::number&) override;
    object::object visit_string(ast::string&) override;
    object::object visit_identifier(ast::identifier&) override;
    object::object visit_unit(ast::unit&) override;
    object::object visit_placeholder(ast::placeholder&) override;

private:
    [[nodiscard]] env& current_env() noexcept;

    std::string _filename;
    const char* _source = nullptr;
    std::vector<diagnostic::diagnostic> _diagnostics;
    std::vector<env> _scopes;
};

}
