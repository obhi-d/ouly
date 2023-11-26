%language "C++"
%skeleton "lalr1.cc"
%require "3.2"

%defines
%define api.parser.class {macroxpr_parser}
%define api.namespace {acl}
%define api.prefix {macroxpr_}
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define parse.trace
%define parse.error verbose

%code requires
{
#include "macroxpr_impl.hpp"
#include <acl/dsl/macroxpr.hpp>
#define YY_DECL acl::macroxpr_parser::symbol_type macroxpr_lex(acl::macroxpr& xpr, void* yyscanner)

}

%define api.location.type {acl::macroxpr::location_type}
%param { acl::macroxpr& xpr }
%lex-param { void* SCANNER_PARAM  }
%locations
%initial-action
{
}

%code
{
#define SCANNER_PARAM xpr.get_scanner()
YY_DECL;
}

%token END 0 "end of file"
	
%token <std::string_view> IDENTIFIER
%token <int> NUMBER
%token DEFINED
%token LOGICAL_AND
%token LOGICAL_OR
%token LESS_THAN
%token GREATER_THAN
%token LESS_THAN_EQUALS
%token GREATER_THAN_EQUALS
%token EQUALS
%token NOT_EQUALS
%token BITWISE_NOT
%token BITWISE_AND
%token BITWISE_OR
%token BITWISE_XOR
%token MINUS
%token NOT
%token ADD
%token SUB
%token MUL
%token DIV
%token LBRACKET
%token RBRACKET
%token QUESTION
%token COLON

%start expr
%type <int> expr term factor

%%

expr:    expr ADD term { $$ = $1 + $3; }
	     | expr SUB term { $$ = $1 - $3; }
			 | term { $$ = $1; }
    ;

term: term MUL factor { $$ = $1 * $3; }
    | term DIV factor { $$ = $1 / $3; }
    | factor { $$ = $1; }
    ;

factor: LBRACKET expr RBRACKET { $$ = $2; }
      | NUMBER { $$ = $1; }
      | IDENTIFIER { $$ = xpr.lookup($1); }
      | expr QUESTION expr COLON expr { $$ = ($1) ? ($3) : ($5); }
      | DEFINED IDENTIFIER { $$ = xpr.defined($2); }
      | expr LOGICAL_AND expr { $$ = $1 && $3; }
      | expr LOGICAL_OR expr { $$ = $1 || $3; }
      | expr LESS_THAN expr { $$ = $1 < $3; }
      | expr GREATER_THAN expr { $$ = $1 > $3; }
      | expr LESS_THAN_EQUALS expr { $$ = $1 <= $3; }
      | expr GREATER_THAN_EQUALS expr { $$ = $1 >= $3; }
      | expr EQUALS expr { $$ = $1 == $3; }
      | expr NOT_EQUALS expr { $$ = $1 != $3; }
      | expr BITWISE_OR expr { $$ = $1 | $3; }
      | expr BITWISE_AND expr { $$ = $1 & $3; }
      | expr BITWISE_XOR expr { $$ = $1 ^ $3; }
      | BITWISE_NOT expr { $$ = ~$2; }
      | MINUS expr { $$ = -$2; }
      | NOT expr { $$ = !$2; }
      ;

%%
/*============================================================================*/

namespace acl
{

void macroxpr_parser::error(const macroxpr_parser::location_type& loc, const std::string& msg) 
{
    xpr.error(msg);
}

bool macroxpr::evaluate(std::string_view content) 
{
	content_  = content;
	cursor_   = location_type();
	macroxpr_parser parser(*this);
	parser.parse();
	return result_ != 0;
}

}
