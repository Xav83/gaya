#pragma once

#include <exception>
#include <vector>

#include <ast.hpp>
#include <diagnostic.hpp>
#include <lexer.hpp>

namespace gaya
{

class parser
{
  public:
    parser(const char* source);

    [[nodiscard]] ast::node_ptr parse() noexcept;

    [[nodiscard]] ast::expression_ptr parse_expression() noexcept;

    [[nodiscard]] ast::stmt_ptr parse_stmt() noexcept;

    [[nodiscard]] std::vector<diagnostic::diagnostic>
    diagnostics() const noexcept;

    /// Returning the remaining, unparsed tokens.
    [[nodiscard]] std::vector<token> remaining_tokens() noexcept;

    /// Merge the lexer and parser diagnostics.
    void merge_diagnostics() noexcept;

  private:
    std::vector<ast::stmt_ptr> stmts() noexcept;

    [[nodiscard]] ast::stmt_ptr declaration_stmt(token identifier);
    [[nodiscard]] ast::stmt_ptr expression_stmt(token discard);
    [[nodiscard]] ast::expression_ptr expression(token);
    [[nodiscard]] ast::expression_ptr function_expression(token lcurly);
    [[nodiscard]] ast::expression_ptr let_expression(token let);
    [[nodiscard]] ast::expression_ptr case_expression(token case_);
    [[nodiscard]] ast::expression_ptr
    comparison_expression(token token) noexcept;
    [[nodiscard]] ast::expression_ptr call_expression(token token);
    [[nodiscard]] ast::expression_ptr do_expression(token token);
    [[nodiscard]] ast::expression_ptr primary_expression(token);

    void parser_error(span, const std::string&);
    void parser_hint(span, const std::string&);
    [[nodiscard]] bool match(std::optional<token>, token_type) const noexcept;

    lexer _lexer;
    std::vector<diagnostic::diagnostic> _diagnostics;
};

}
