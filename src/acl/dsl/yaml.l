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
	assert(buf[result] == 0);				\

#define yyterminate()					         return acl::yaml::parser::make_END(cyaml.get_source())
#define YY_USER_ACTION                 cyaml.move_ahead((uint32_t)yyleng); 
void * yaml_alloc (std::size_t bytes, void* yyscanner);
void * yaml_realloc (void* ptr, std::size_t bytes, void* yyscanner);
void   yaml_free (void* ptr, void* yyscanner);
%}



%%
^[ \t]*\n          ; /* Skip empty lines */
^[ \t]+            { 
                     uint32_t current_indent = static_cast<uint32_t>(yyleng);                     
                     if (current_indent > cyaml.peek_indent()) {
                         cyaml.push_indent(current_indent);
                         return acl::yaml::parser::make_INDENT(cyaml.get_source());
                     } else if (current_indent < cyaml.peek_indent()) {
                         cyaml.pop_indent();
                         return acl::yaml::parser::make_DEDENT(cyaml.get_source());
                     }
                   }                   
"["                { return acl::yaml::parser::make_LBRACKET(cyaml.get_source()); }
"]"                { return acl::yaml::parser::make_RBRACKET(cyaml.get_source()); }
","                { return acl::yaml::parser::make_COMMA(cyaml.get_source()); }                   
[-][ \t]+          { return acl::yaml::parser::make_DASH(cyaml.get_source()); }
[:][ \t]+          { return acl::yaml::parser::make_COLON(cyaml.get_source()); }
\n                 { return acl::yaml::parser::make_NEWLINE(cyaml.get_source()); }
\"[^\"]*\"         { return acl::yaml::parser::make_STRING(cyaml.get_view(static_cast<uint32_t>(yyleng)), cyaml.get_source()); }
[^ \t\n\":#][^ \t\n]*  { 
                     return acl::yaml::parser::make_STRING(cyaml.get_view(static_cast<uint32_t>(yyleng)), cyaml.get_source());
                   }
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

