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
%define parse.trace
%define parse.error verbose

%code requires
{
#include "yaml_parser_impl.hpp"
#include <acl/dsl/yaml.hpp>


#ifndef YY_NULLPTR
#  define YY_NULLPTR nullptr
#endif
#define YY_DECL extern acl::yaml::parser::symbol_type yaml_lex(acl::yaml::istream& cyaml, void* yyscanner)

}

%define api.location.type {acl::yaml::location}
%param { acl::yaml::istream& cyaml }
%lex-param { void* SCANNER_PARAM  }
%locations
%initial-action
{
  @$.source_name = cyaml.get_file_name();
}

%code
{
#define SCANNER_PARAM cyaml.get_scanner()
YY_DECL;
}

%token 
	END 0 "end of file"

%token <acl::yaml::string_slice> STRING
%token COLON DASH INDENT DEDENT NEWLINE
%token LBRACKET RBRACKET COMMA
%type <acl::yaml::string_slice> value
%type <acl::yaml::string_slice> key
%type <acl::yaml::string_slice> array_value

%%

document:
    /* empty */
    | document line
    ;

line:
    mapping NEWLINE
    | sequence_item NEWLINE
    | NEWLINE
    ;

mapping:
    key COLON value
    {
        cyaml.start_mapping($1);
        cyaml.add_mapping_value($3);
        cyaml.end_mapping();
    }
    | key COLON array
    {
        cyaml.end_mapping();
    }
    | key COLON NEWLINE INDENT nested_sequence DEDENT
    {
        cyaml.end_mapping();
    }
    | key COLON NEWLINE INDENT nested_mappings DEDENT
    {
        cyaml.end_mapping();
    }
    ;

key:
    STRING
    {
        cyaml.start_mapping($1);
        $$ = $1;
    }
    ;

array:
    {
        cyaml.start_sequence();
    }
    LBRACKET array_values RBRACKET
    {
        cyaml.end_sequence();
        cyaml.end_mapping();
    }
    ;

array_values:
    array_value
    {
        cyaml.start_sequence_item();
        cyaml.add_sequence_item($1);
        cyaml.end_sequence_item();
    }
    | array_values COMMA array_value
    {
        cyaml.start_sequence_item();
        cyaml.add_sequence_item($3);
        cyaml.end_sequence_item();
    }
    ;

array_value:
    STRING
    {
        $$ = $1;
    }
    ;

nested_mappings:
    {
        cyaml.start_mapping(cyaml.get_current_key());
    }
    mappings
    ;

mappings:
    mapping NEWLINE
    | mappings mapping NEWLINE
    ;

nested_sequence:
    {
        cyaml.start_sequence();
    }
    sequence
    {
        cyaml.end_sequence();
    }
    ;

sequence:
    sequence_item NEWLINE
    | sequence sequence_item NEWLINE
    ;

sequence_item:
    DASH value
    {
        cyaml.start_sequence_item();
        cyaml.add_sequence_item($2);
        cyaml.end_sequence_item();
    }
    | DASH NEWLINE INDENT nested_mapping_in_sequence DEDENT
    ;

nested_mapping_in_sequence:
    {
        cyaml.start_sequence_item();
    }
    mappings
    {
        cyaml.end_sequence_item();
    }
    ;

value:
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
  parser.parse();
  end_scan();	
}

}
