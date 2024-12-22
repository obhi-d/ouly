%language "C++"
%skeleton "lalr1.cc"
%require "3.2"

%defines
%define api.parser.class {parser}
%define api.namespace {acl::yaml}
%define api.prefix {yaml_}
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define parse.error verbose

%code requires
{
// #define YYDEBUG 1
#include <acl/dsl/yaml.hpp>


#ifndef YY_NULLPTR
#  define YY_NULLPTR nullptr
#endif
#define YY_DECL extern acl::yaml::parser::symbol_type yaml_lex(acl::yaml::istream& cyaml, void* yyscanner)

}

%define api.location.type {acl::yaml::location_type}
%param { acl::yaml::istream& cyaml }
%lex-param { void* SCANNER_PARAM  }
%locations
%initial-action
{
}

%code
{
#define SCANNER_PARAM cyaml.get_scanner()
YY_DECL;
}

%token 
	END 0 "end of file"

%token <acl::yaml::string_slice> STRING INDENT DASH
%token COLON NEWLINE
%token LBRACKET RBRACKET COMMA PIPE GREATER_THAN
%type <acl::yaml::string_slice> scalar_line
%type <acl::yaml::string_slice_array> block_scalar block_scalar_content scalar_lines

%%

document:
    /* empty */
    | document line
    | end
    ;

end:
    END
    {   cyaml.close_all_mappings(); }


optional_newline:
    /* empty */
    | NEWLINE
    ;

line:
    mapping 
    | sequence_item 
    | NEWLINE
    ;

indent:
    /* empty */
    {
        cyaml.set_indention_level(0);
    }
    | INDENT
    {
        cyaml.set_indention_level($1);
    }
    ;

dash_indent:
    indent DASH
    {
        cyaml.add_dash_indention($2);
    }
    ;

key:
    indent STRING COLON 
    {
        cyaml.add_new_mapping($1);
    }
    | dash_indent STRING COLON
    {
        cyaml.add_new_mapped_sequence($3);
    }
    ;

mapping:
    key STRING optional_newline
    {
        cyaml.add_mapping_value($2);
    }
    | key block_scalar optional_newline
    {
        cyaml.add_mapping_value(std::move($3));
    }
    | key array optional_newline
    | key
    ;

array:
    {
      cyaml.begin_array();
    } 
    LBRACKET array_values RBRACKET
    {
      cyaml.end_array();
    }
    ;

array_values:
    /* empty */
    |
    STRING
    {
        cyaml.add_new_sequence_value($1);
    }
    | array_values COMMA STRING
    {
        cyaml.add_new_sequence_value($3);
    }
    ;

sequence_item:
    dash_indent STRING optional_newline
    {
      cyaml.add_new_sequence_value($3);
    }
    | dash_indent block_scalar optional_newline
    {
      cyaml.add_new_sequence_value(std::move($3));
    }
    | dash_indent array optional_newline
    ;

block_scalar:
    PIPE block_scalar_content
    {
        $$ = std::move($2);
    }
    | GREATER_THAN block_scalar_content
    {
        $$ = std::move($2);
    }
    ;

block_scalar_content:
    NEWLINE INDENT scalar_lines DEDENT
    {
        $$ = std::move($3);
    }
    ;

scalar_lines:
    scalar_line
    {
      $$.emplace_back($1);
    }
    | scalar_lines NEWLINE scalar_line
    {
      $$.emplace_back($3);
    };

scalar_line:
    STRING
  {
        $$ = $1;
  }
  ;
%%

/*============================================================================*/
namespace acl
{

void yaml::parser::error(const yaml::parser::location_type& loc, const std::string& msg) 
{
  cyaml.throw_error(loc, msg, "parser-error");
}

void yaml::istream::parse(context& handler) 
{
  this->handler_ = &handler;
  begin_scan();
  parser parser(*this);
  // parser.set_debug_level(5);
  parser.parse();
  end_scan();	
}

}
