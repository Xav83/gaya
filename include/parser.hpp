#pragma once

#include <exception>
#include <vector>

#include <robin_hood.h>

#include <ast.hpp>
#include <diagnostic.hpp>
#include <lexer.hpp>

namespace gaya
{

class parser
{
public:
    parser();

    [[nodiscard]] ast::node_ptr parse(const char* source) noexcept;

    [[nodiscard]] std::vector<diagnostic::diagnostic>
    diagnostics() const noexcept;

    /// Returning the remaining, unparsed tokens.
    [[nodiscard]] std::vector<token> remaining_tokens() noexcept;

    /// Merge the lexer and parser diagnostics.
    void merge_diagnostics() noexcept;

private:
    std::vector<ast::stmt_ptr> stmts() noexcept;

    [[nodiscard]] ast::stmt_ptr toplevel_stmt() noexcept;
    [[nodiscard]] ast::stmt_ptr declaration_stmt(token identifier);

    [[nodiscard]] ast::stmt_ptr local_stmt(token token) noexcept;
    [[nodiscard]] ast::stmt_ptr assignment_stmt(token identifier) noexcept;
    [[nodiscard]] ast::stmt_ptr while_stmt(token while_) noexcept;
    [[nodiscard]] ast::stmt_ptr expression_stmt(token discard);

    [[nodiscard]] ast::expression_ptr expression(token);
    [[nodiscard]] ast::expression_ptr let_expression(token let);
    [[nodiscard]] ast::expression_ptr case_expression(token cases);
    [[nodiscard]] ast::expression_ptr do_expression(token token);

    [[nodiscard]] ast::expression_ptr logical_expression(token) noexcept;
    [[nodiscard]] ast::expression_ptr comparison_expression(token) noexcept;
    [[nodiscard]] ast::expression_ptr pipe_expression(token) noexcept;
    [[nodiscard]] ast::expression_ptr term_expression(token) noexcept;
    [[nodiscard]] ast::expression_ptr factor_expression(token) noexcept;

    [[nodiscard]] ast::expression_ptr unary_expression(token) noexcept;
    [[nodiscard]] ast::expression_ptr not_expression(token) noexcept;
    [[nodiscard]] ast::expression_ptr perform_expression(token) noexcept;

    [[nodiscard]] ast::expression_ptr call_expression(token token);

    [[nodiscard]] ast::expression_ptr primary_expression(token);
    [[nodiscard]] ast::expression_ptr function_expression(token lcurly);
    [[nodiscard]] ast::expression_ptr string(token) noexcept;
    [[nodiscard]] ast::expression_ptr array(token) noexcept;

    void parser_error(span, const std::string&);
    void parser_hint(span, const std::string&);

    [[nodiscard]] bool match(std::optional<token>, token_type) const noexcept;
    [[nodiscard]] bool is_local_stmt(token);

    /* Resolving identifier locations */
    void begin_scope() noexcept;
    void end_scope() noexcept;

    void define(ast::identifier&) noexcept;
    void define(eval::key) noexcept;

    [[nodiscard]] bool assign_scope(std::shared_ptr<ast::identifier>&) noexcept;
    [[nodiscard]] bool assign_scope(std::unique_ptr<ast::identifier>&) noexcept;

    lexer _lexer;
    std::vector<diagnostic::diagnostic> _diagnostics;

    using scope = robin_hood::unordered_set<size_t>;
    std::vector<scope> _scopes;
};

}
