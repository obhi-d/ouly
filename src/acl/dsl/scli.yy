%language "C++"
%skeleton "lalr1.cc"
%require "3.2"

%defines
%define api.parser.class {scli_parser}
%define api.namespace {acl}
%define api.prefix {scli_}
%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define parse.trace
%define parse.error verbose

%code requires
{
#include "parse_impl.hpp"
#include <acl/dsl/scli.hpp>


#ifndef YY_NULLPTR
#  define YY_NULLPTR nullptr
#endif
#define YY_DECL extern acl::scli_parser::symbol_type scli_lex(acl::scli& scli, void* yyscanner)

}

%define api.location.type {acl::scli::location}
%param { acl::scli& scli }
%lex-param { void* SCANNER_PARAM  }
%locations
%initial-action
{
  @$.source_name = scli.get_file_name();
}

%code
{
#define SCANNER_PARAM scli.get_scanner()
YY_DECL;
}

%token 
	END 0 "end of file"
	SEMICOLON  ";"
	LBRACES   "{"
	RBRACES   "}"
	LABRACKET  "<"
	RABRACKET  ">"
	COMMA      ","
	LPARENTHESES    "("
	RPARENTHESES    ")"
	ASSIGN     "="
	COLON      ":"
	IMPORT	   "import"
	;

%token <std::string_view> REGION_ID TEXT_REGION_ID STRING
%token <acl::text_content> STRING_LITERAL TEXT_CONTENTS

// Types
%printer { yyoutput << $$; } <std::string_view>

%start script

%%

script:    statement           
	     | statement script      

statement: commanddecl                            
		| RBRACES                               { scli.exit_command_scope();  scli.destroy_comamnd_state();            }
		| REGION_ID                              { scli.enter_region(std::move($1));                                    }
		| TEXT_REGION_ID TEXT_CONTENTS           { scli.enter_text_region(std::move($1), std::move($2));                }
		| IMPORT STRING_LITERAL SEMICOLON        { scli.import_script(std::move($2));                                   }

commandname:   STRING         { scli.set_next_command($1); }
             | STRING COLON   { scli.set_next_command($1); }

commanddecl: 
           commandname SEMICOLON                     { scli.execute_command(); scli.destroy_comamnd_state(); }
		 | commandname parameters.1.N SEMICOLON      { scli.execute_command(); scli.destroy_comamnd_state(); }
		 | commandname parameters.1.N LBRACES       { scli.execute_command(); scli.enter_command_scope();   }
		 

parameters.1.N:     parameter	
				  | parameters.1.N parameter  
				  ;

parameter: | STRING ASSIGN                     { scli.set_next_param_name($1); }
           | STRING_LITERAL                    { scli.set_param(std::move($1)); }
           | STRING                            { scli.set_param($1); }
		   | LPARENTHESES                        { scli.enter_param_scope(); }
		   | RPARENTHESES                        { scli.exit_param_scope(); }
		   | COMMA                             
		   ;


%%
/*============================================================================*/
namespace acl
{

void scli_parser::error(const scli_parser::location_type& loc, const std::string& msg) 
{
    scli.error(loc, msg, "parse");
}

void scli::error(scli::location const& l, std::string_view err, std::string_view ctx) 
{
  sstate.error_handler(l, err, ctx);
}

void scli::parse(std::string_view src_name, std::string_view content) noexcept
{
	source_name = src_name;
	contents    = content;
	begin_scan();
	set_current_reg_id("root");
	enter_region("");
	scli_parser parser(*this);
	parser.parse();
	end_scan();	
}

}
