%option reentrant
%option noyywrap
%option prefix="macroxpr_"
%option never-interactive

%{
#include <cstdint>
#include <string>
#include <string_view>
#include <acl/dsl/macroxpr.hpp>
#include "macroxpr_impl.hpp"
#define YY_NO_UNISTD_H
#define YY_EXTRA_TYPE acl::macroxpr*
#define YY_INPUT(buf,result,max_size)  result = yyextra->read(buf, max_size);	
#define yyterminate()				   return acl::macroxpr_parser::make_END(xpr.location())
#ifndef YY_NULLPTR
#  define YY_NULLPTR nullptr
#endif

void* macroalloc   (std::size_t bytes, void* yyscanner);
void* macrorealloc (void* ptr, std::size_t bytes, void* yyscanner);
void  macrofree    (void* ptr, void* yyscanner);
%}


%%


"+"     { xpr.move(1); return acl::macroxpr_parser::make_ADD(xpr.location()); }
"-"     { xpr.move(1); return acl::macroxpr_parser::make_SUB(xpr.location()); }
"*"     { xpr.move(1); return acl::macroxpr_parser::make_MUL(xpr.location()); }
"/"     { xpr.move(1); return acl::macroxpr_parser::make_DIV(xpr.location()); }
"("     { xpr.move(1); return acl::macroxpr_parser::make_LBRACKET(xpr.location()); }
")"     { xpr.move(1); return acl::macroxpr_parser::make_RBRACKET(xpr.location()); }
"!"     { xpr.move(1); return acl::macroxpr_parser::make_NOT(xpr.location()); }
"~"     { xpr.move(1); return acl::macroxpr_parser::make_BITWISE_NOT(xpr.location()); }
"&&"    { xpr.move(2); return acl::macroxpr_parser::make_LOGICAL_AND(xpr.location()); }
"||"    { xpr.move(2); return acl::macroxpr_parser::make_LOGICAL_OR(xpr.location()); }
"<"     { xpr.move(1); return acl::macroxpr_parser::make_LESS_THAN(xpr.location()); }
">"     { xpr.move(1); return acl::macroxpr_parser::make_GREATER_THAN(xpr.location()); }
"<="    { xpr.move(2); return acl::macroxpr_parser::make_LESS_THAN_EQUALS(xpr.location()); }
">="    { xpr.move(2); return acl::macroxpr_parser::make_GREATER_THAN_EQUALS(xpr.location()); }
"=="    { xpr.move(2); return acl::macroxpr_parser::make_EQUALS(xpr.location()); }
"!="    { xpr.move(2); return acl::macroxpr_parser::make_NOT_EQUALS(xpr.location()); }
"$"                    { xpr.move(1); return acl::macroxpr_parser::make_DEFINED(xpr.location()); }
[0-9]+                 { return acl::macroxpr_parser::make_NUMBER(xpr.accept_int(yyleng), xpr.location()); }
[a-zA-Z_][a-zA-Z_0-9]* { return acl::macroxpr_parser::make_IDENTIFIER(xpr.accept_str(yyleng), xpr.location()); }
[ \t\n]                { xpr.move(yyleng); }
.                      { xpr.error("Invalid character"); }


%%

namespace acl 
{

void macroxpr::begin_scan() noexcept
{
  macroxpr_lex_init_extra(this, &scanner_);
}

void macroxpr::end_scan() noexcept
{
  macroxpr_lex_destroy(scanner_);
}

}
