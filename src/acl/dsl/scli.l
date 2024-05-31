%option reentrant
%option noyywrap
%option prefix="scli_"
%option never-interactive

%{
#include <cstdint>
#include <string>
#include <string_view>
#include <acl/dsl/scli.hpp>
#include "parse_impl.hpp"
#define YY_NO_UNISTD_H
#define YY_EXTRA_TYPE acl::scli*
#define YY_INPUT(buf,result,max_size)       \
	result = yyextra->read(buf, max_size);	\
	assert(result <= max_size);			    \
	assert(buf[result] == 0);				\

#define yyterminate()					   return acl::scli_parser::make_END(scli.source)
#define YY_USER_ACTION                     scli.source.step(); scli.source.columns((uint32_t)yyleng); 
void * scli_alloc (std::size_t bytes, void* yyscanner);
void * scli_realloc (void* ptr, std::size_t bytes, void* yyscanner);
void   scli_free (void* ptr, void* yyscanner);
%}

%x STATE_REGION STATE_REGION_TEXT STATE_REGION_CODE STATE_TEXT STATE_STR STATE_CONTENT STATE_COMMENT STATE_SINGLE_LINE_COMMENT 

REGND   [a-zA-Z#_$\.]+
BLANK   [ \t\r]
STRCH   [a-zA-Z0-9\._@*/\\\[\]\<\>]
%%


<STATE_TEXT>{

^"\\--" 	 scli.escape_sequence(scli.make_token()); scli.skip_len(1); scli.put(2); 
^"--"        {
	BEGIN(STATE_REGION); 

	auto content = scli.make_text(); 
	scli.skip_len(2);
	return acl::scli_parser::make_TEXT_CONTENTS(std::move(content), scli.source); 
}

<<EOF>>  {
	BEGIN(INITIAL); 
	return acl::scli_parser::make_TEXT_CONTENTS(std::move(scli.make_text()), scli.source); 
}

.		scli.put(1);

}


<STATE_STR>{
\"     			{
	auto tok = scli.make_text();
	scli.skip_len(1);
	BEGIN(STATE_CONTENT);  
	return acl::scli_parser::make_STRING_LITERAL(tok, scli.source);
}
\\\"	{
	scli.escape_sequence(scli.make_token());
	scli.skip_len(1);
	scli.put(1);
}
.		{ 
		scli.put(1);
}
}

<INITIAL,STATE_CONTENT>{
^--                                    BEGIN(STATE_REGION); scli.skip_len(2);
-                                      scli.put(yyleng);
"/*"                                   BEGIN(STATE_COMMENT); scli.skip_len(2);
"//"                                   BEGIN(STATE_SINGLE_LINE_COMMENT); scli.skip_len(2);
"{"                                    scli.skip_len(1); return acl::scli_parser::make_LBRACES(scli.source);
"}"                                    scli.skip_len(1); return acl::scli_parser::make_RBRACES(scli.source);
"import"	                             scli.skip_len(yyleng); return acl::scli_parser::make_IMPORT(scli.source);
","                                    scli.skip_len(1); return acl::scli_parser::make_COMMA(scli.source);
";"                                    scli.skip_len(1); return acl::scli_parser::make_SEMICOLON(scli.source);
"("                                    scli.skip_len(1); return acl::scli_parser::make_LPARENTHESES(scli.source);
")"                                    scli.skip_len(1); return acl::scli_parser::make_RPARENTHESES(scli.source);
"="                                    scli.skip_len(1); return acl::scli_parser::make_ASSIGN(scli.source);
":"                                    scli.skip_len(1); return acl::scli_parser::make_COLON(scli.source);
"\""                                   scli.skip_len(1); BEGIN(STATE_STR);
[\n]+  		  	                         scli.skip_len(yyleng); scli.source.lines((uint32_t)yyleng);
{BLANK}+   	                           scli.skip_len(yyleng);
{STRCH}({STRCH}|"-")*                  scli.put(yyleng); return acl::scli_parser::make_STRING(scli.make_token(), scli.source);

}

<STATE_REGION>{
{BLANK}+   	scli.skip_len(yyleng); 
{REGND}		  scli.put(yyleng);
{BLANK}*":"	{ 				
				auto id = scli.make_token();
				scli.set_current_reg_id(id);
				scli.set_region_position(scli.source.next_line());
				scli.skip_len(yyleng);
				if (scli.is_code_region(id))
				{
					BEGIN(STATE_REGION_CODE); 
				}
				else 
				{
					BEGIN(STATE_REGION_TEXT);
			  }
			}
}

<STATE_REGION_TEXT,STATE_REGION_CODE>{

"--"|\n  {
	auto value = scli.make_token();
	value = scli.trim(value);
	scli.skip_len(yyleng);
	if (YY_START == STATE_REGION_CODE) {
		BEGIN(STATE_CONTENT);
		return acl::scli_parser::make_REGION_ID(value, scli.source);
	} else {
		BEGIN(STATE_TEXT);
		return acl::scli_parser::make_TEXT_REGION_ID(value, scli.source);
	}
}

.	scli.put(1);

}

<STATE_SINGLE_LINE_COMMENT>{
[^\n]+		scli.skip_len(yyleng); 
\n		    scli.skip_len(1); scli.source.lines((uint32_t)1); BEGIN(STATE_CONTENT);
}

<STATE_COMMENT>{
"*/" 		scli.skip_len(2);BEGIN(STATE_CONTENT);
[^*\n]+		scli.skip_len(yyleng); 
\*          scli.skip_len(1); 
\n		    scli.skip_len(1); scli.source.lines((uint32_t)1);
}

<STATE_TEXT,STATE_STR>\n      {
  // no error, just increase the line number
  // @todo
  scli.source.lines((uint32_t)yyleng);
  scli.put(1);
}

%%

namespace acl 
{

void scli::begin_scan() noexcept
{
  scli_lex_init_extra(this, &scanner);
}

void scli::end_scan() noexcept
{
  scli_lex_destroy(scanner);
}

}
