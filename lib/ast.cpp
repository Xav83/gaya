#include <cassert>
#include <numeric>
#include <sstream>
#include <string>

#include <fmt/core.h>

#include <ast.hpp>
#include <ast_visitor.hpp>
#include <eval.hpp>
#include <object.hpp>

namespace gaya::ast
{

object program::accept(ast_visitor& v)
{
    return v.visit_program(*this);
}

object declaration_stmt::accept(ast_visitor& v)
{
    return v.visit_declaration_stmt(*this);
}

object expression_stmt::accept(ast_visitor& v)
{
    return v.visit_expression_stmt(*this);
}

object assignment_stmt::accept(ast_visitor& v)
{
    return v.visit_assignment_stmt(*this);
}

object while_stmt::accept(ast_visitor& v)
{
    return v.visit_while_stmt(*this);
}

object for_in_stmt::accept(ast_visitor& v)
{
    return v.visit_for_in_stmt(*this);
}

object include_stmt::accept(ast_visitor& v)
{
    return v.visit_include_stmt(*this);
}

object TypeDeclaration::accept(ast_visitor& v)
{
    return v.visit_type_declaration(*this);
}

object ForeignDeclaration::accept(ast_visitor& v)
{
    return v.visit_foreign_declaration(*this);
}

object StructDeclaration::accept(ast_visitor& v)
{
    return v.visit_struct_declaration(*this);
}

/* Expressions */

object do_expression::accept(ast_visitor& v)
{
    return v.visit_do_expression(*this);
}

object case_expression::accept(ast_visitor& v)
{
    return v.visit_case_expression(*this);
}

object match_expression::accept(ast_visitor& v)
{
    return v.visit_match_expression(*this);
}

object call_expression::accept(ast_visitor& v)
{
    return v.visit_call_expression(*this);
}

object get_expression::accept(ast_visitor& v)
{
    return v.visit_get_expression(*this);
}

object function_expression::accept(ast_visitor& v)
{
    return v.visit_function_expression(*this);
}

object let_expression::accept(ast_visitor& v)
{
    return v.visit_let_expression(*this);
}

object binary_expression::accept(ast_visitor& v)
{
    return v.visit_binary_expression(*this);
}

object lnot_expression::accept(ast_visitor& v)
{
    return v.visit_lnot_expression(*this);
}

object not_expression::accept(ast_visitor& v)
{
    return v.visit_not_expression(*this);
}

object perform_expression::accept(ast_visitor& v)
{
    return v.visit_perform_expression(*this);
}

object array::accept(ast_visitor& v)
{
    return v.visit_array(*this);
}

object dictionary::accept(ast_visitor& v)
{
    return v.visit_dictionary(*this);
}

object number::accept(ast_visitor& v)
{
    return v.visit_number(*this);
}

object string::accept(ast_visitor& v)
{
    return v.visit_string(*this);
}

object identifier::accept(ast_visitor& v)
{
    return v.visit_identifier(*this);
}

object unit::accept(ast_visitor& v)
{
    return v.visit_unit(*this);
}

object placeholder::accept(ast_visitor& v)
{
    return v.visit_placeholder(*this);
}

}
