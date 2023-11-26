%option reentrant
%option noyywrap
%option prefix="macro_"
%option never-interactive

%{
#include <cstdint>
#include <string>
#include <string_view>
#include <acl/dsl/macrohpp>
#include "macroexpr_impl.hpp"
#define YY_NO_UNISTD_H
#define YY_INPUT(buf,result,max_size)  result = read(buf, max_size);	
#define yyterminate()					   return acl::macroparser::make_END(scli.source)

void* macroalloc (std::size_t bytes, void* yyscanner);
void* macrorealloc (void* ptr, std::size_t bytes, void* yyscanner);
void   macrofree (void* ptr, void* yyscanner);
%}


%%


"+"     { move(1); return '+'; }
"-"     { move(1); return '-'; }
"*"     { move(1); return '*'; }
"/"     { move(1); return '/'; }
"("     { move(1); return '('; }
")"     { move(1); return ')'; }
"!"     { move(1); return NOT; }
"~"     { move(1); return BITWISE_NOT; }
"&&"    { move(2); return LOGICAL_AND; }
"||"    { move(2); return LOGICAL_OR; }
"<"     { move(1); return LESS_THAN; }
">"     { move(1); return GREATER_THAN; }
"<="    { move(2); return LESS_THAN_EQUALS; }
">="    { move(2); return GREATER_THAN_EQUALS; }
"=="    { move(2); return EQUALS; }
"!="    { move(2); return NOT_EQUALS; }
"#"                    { move(1); return DEFINED; }
[0-9]+                 { accept(yyleng); return NUMBER; }
[a-zA-Z_][a-zA-Z_0-9]* { accept(yyleng); return IDENTIFIER; }
[ \t\n]                { move(yyleng); }
.                      { error("Invalid character"); }
