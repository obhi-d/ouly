%option reentrant
%option noyywrap
%option prefix="yaml_"
%option never-interactive

%{
#include <cstdint>
#include <string>
#include <string_view>
#include <exception>
#include <acl/dsl/yaml.hpp>
#include "yaml_parser_impl.hpp"
#define YY_NO_UNISTD_H
#define YY_EXTRA_TYPE acl::yaml::istream*
#define YY_INPUT(buf,result,max_size)       \
	result = yyextra->read(buf, max_size);	\
	assert(result <= max_size);			    \
	assert(!result || buf[result] == 0);				\

#define yyterminate()					         return acl::yaml::parser::make_END(cyaml.location())
#define YY_USER_ACTION                 cyaml.set_token((uint32_t)yyleng); 
void * yaml_alloc (std::size_t bytes, void* yyscanner);
void * yaml_realloc (void* ptr, std::size_t bytes, void* yyscanner);
void   yaml_free (void* ptr, void* yyscanner);
%}



%%
^[ \t]*\n          ; /* Skip empty lines */
^[ \t]+            { return acl::yaml::parser::make_INDENT(cyaml.get_view(), cyaml.location()); } 
"["                { return acl::yaml::parser::make_LBRACKET(cyaml.location()); }
"]"                { return acl::yaml::parser::make_RBRACKET(cyaml.location()); }
","                { return acl::yaml::parser::make_COMMA(cyaml.location()); }
"|"                { return acl::yaml::parser::make_PIPE(cyaml.location()); }
">"                { return acl::yaml::parser::make_GREATER_THAN(cyaml.location()); }
[-][ \t]*          { return acl::yaml::parser::make_DASH(cyaml.get_view(), cyaml.location()); } 
[:]                { return acl::yaml::parser::make_COLON(cyaml.location()); }
\n                 { return acl::yaml::parser::make_NEWLINE(cyaml.location()); }
\"[^\"]*\"         { /* quoted string */ return acl::yaml::parser::make_STRING(cyaml.get_quoted_string(), cyaml.location()); }
[^ \t\n\":#]*      { /* string */ return acl::yaml::parser::make_STRING(cyaml.get_view(), cyaml.location()); }
#.*                ; /* Ignore comments */
[ \t]+            ; /* Ignore whitespace */
.                 { cyaml.throw_error(cyaml.location(), "Invalid character", "lexer-error"); }

%%


void acl::yaml::istream::begin_scan() 
{
  yaml_lex_init_extra(this, &scanner);
}

void acl::yaml::istream::end_scan() 
{
  yaml_lex_destroy(scanner);
}

