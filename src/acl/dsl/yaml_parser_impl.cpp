// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.


// Take the name prefix into account.
#define yylex   yaml_lex



#include "yaml_parser_impl.hpp"


// Unqualified %code blocks.
#line 36 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"

#define SCANNER_PARAM cyaml.get_scanner()
YY_DECL;

#line 53 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif


// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Enable debugging if requested.
#if YAML_DEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yy_stack_print_ ();                \
  } while (false)

#else // !YAML_DEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YY_USE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YAML_DEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 7 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
namespace acl { namespace yaml {
#line 146 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"

  /// Build a parser object.
  parser::parser (acl::yaml::istream& cyaml_yyarg)
#if YAML_DEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      cyaml (cyaml_yyarg)
  {}

  parser::~parser ()
  {}

  parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------.
  | symbol.  |
  `---------*/



  // by_state.
  parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  parser::symbol_kind_type
  parser::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  parser::stack_symbol_type::stack_symbol_type ()
  {}

  parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_STRING: // STRING
      case symbol_kind::S_key: // key
      case symbol_kind::S_array_value: // array_value
      case symbol_kind::S_scalar_line: // scalar_line
        value.YY_MOVE_OR_COPY< acl::yaml::string_slice > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_block_scalar: // block_scalar
      case symbol_kind::S_block_scalar_content: // block_scalar_content
      case symbol_kind::S_scalar_lines: // scalar_lines
        value.YY_MOVE_OR_COPY< acl::yaml::string_slice_array > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_STRING: // STRING
      case symbol_kind::S_key: // key
      case symbol_kind::S_array_value: // array_value
      case symbol_kind::S_scalar_line: // scalar_line
        value.move< acl::yaml::string_slice > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_block_scalar: // block_scalar
      case symbol_kind::S_block_scalar_content: // block_scalar_content
      case symbol_kind::S_scalar_lines: // scalar_lines
        value.move< acl::yaml::string_slice_array > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_STRING: // STRING
      case symbol_kind::S_key: // key
      case symbol_kind::S_array_value: // array_value
      case symbol_kind::S_scalar_line: // scalar_line
        value.copy< acl::yaml::string_slice > (that.value);
        break;

      case symbol_kind::S_block_scalar: // block_scalar
      case symbol_kind::S_block_scalar_content: // block_scalar_content
      case symbol_kind::S_scalar_lines: // scalar_lines
        value.copy< acl::yaml::string_slice_array > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    return *this;
  }

  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_STRING: // STRING
      case symbol_kind::S_key: // key
      case symbol_kind::S_array_value: // array_value
      case symbol_kind::S_scalar_line: // scalar_line
        value.move< acl::yaml::string_slice > (that.value);
        break;

      case symbol_kind::S_block_scalar: // block_scalar
      case symbol_kind::S_block_scalar_content: // block_scalar_content
      case symbol_kind::S_scalar_lines: // scalar_lines
        value.move< acl::yaml::string_slice_array > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YAML_DEBUG
  template <typename Base>
  void
  parser::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YY_USE (yyoutput);
    if (yysym.empty ())
      yyo << "empty symbol";
    else
      {
        symbol_kind_type yykind = yysym.kind ();
        yyo << (yykind < YYNTOKENS ? "token" : "nterm")
            << ' ' << yysym.name () << " ("
            << yysym.location << ": ";
        YY_USE (yykind);
        yyo << ')';
      }
  }
#endif

  void
  parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  parser::yypop_ (int n) YY_NOEXCEPT
  {
    yystack_.pop (n);
  }

#if YAML_DEBUG
  std::ostream&
  parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  parser::debug_level_type
  parser::debug_level () const
  {
    return yydebug_;
  }

  void
  parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YAML_DEBUG

  parser::state_type
  parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  parser::yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  parser::yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yytable_ninf_;
  }

  int
  parser::operator() ()
  {
    return parse ();
  }

  int
  parser::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    // User initialization code.
#line 32 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
{
}

#line 462 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';
    YY_STACK_PRINT ();

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token\n";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            symbol_type yylookahead (yylex (cyaml, SCANNER_PARAM));
            yyla.move (yylookahead);
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    if (yyla.kind () == symbol_kind::S_YYerror)
    {
      // The scanner already issued an error message, process directly
      // to error recovery.  But do not keep the error token as
      // lookahead, it is too special and may lead us to an endless
      // loop in error recovery. */
      yyla.kind_ = symbol_kind::S_YYUNDEF;
      goto yyerrlab1;
    }

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.kind ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.kind ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case symbol_kind::S_STRING: // STRING
      case symbol_kind::S_key: // key
      case symbol_kind::S_array_value: // array_value
      case symbol_kind::S_scalar_line: // scalar_line
        yylhs.value.emplace< acl::yaml::string_slice > ();
        break;

      case symbol_kind::S_block_scalar: // block_scalar
      case symbol_kind::S_block_scalar_content: // block_scalar_content
      case symbol_kind::S_scalar_lines: // scalar_lines
        yylhs.value.emplace< acl::yaml::string_slice_array > ();
        break;

      default:
        break;
    }


      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 5: // end: "end of file"
#line 61 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {   cyaml.close_all_mappings(); }
#line 613 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 13: // mapping: key COLON STRING
#line 82 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.start_mapping(yystack_[2].value.as < acl::yaml::string_slice > ());
        cyaml.add_mapping_value(yystack_[0].value.as < acl::yaml::string_slice > ());
        cyaml.end_mapping();
    }
#line 623 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 14: // mapping: key COLON block_scalar
#line 88 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.start_mapping(yystack_[2].value.as < acl::yaml::string_slice > ());
        cyaml.add_block_scalar(std::move(yystack_[0].value.as < acl::yaml::string_slice_array > ()));
        cyaml.end_mapping();
    }
#line 633 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 15: // mapping: key COLON array
#line 94 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.end_mapping();
    }
#line 641 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 16: // mapping: key COLON NEWLINE INDENT nested_sequence optional_dedent
#line 98 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.end_mapping();
    }
#line 649 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 17: // mapping: key COLON NEWLINE INDENT nested_mappings optional_dedent
#line 102 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.end_mapping();
    }
#line 657 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 18: // key: STRING
#line 109 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.start_mapping(yystack_[0].value.as < acl::yaml::string_slice > ());
        yylhs.value.as < acl::yaml::string_slice > () = yystack_[0].value.as < acl::yaml::string_slice > ();
    }
#line 666 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 19: // $@1: %empty
#line 116 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.start_sequence();
    }
#line 674 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 20: // array: $@1 LBRACKET array_values RBRACKET
#line 120 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.end_sequence();
        cyaml.end_mapping();
    }
#line 683 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 21: // array_values: array_value
#line 128 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.start_sequence_item();
        cyaml.add_sequence_item(yystack_[0].value.as < acl::yaml::string_slice > ());
        cyaml.end_sequence_item();
    }
#line 693 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 22: // array_values: array_values COMMA array_value
#line 134 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.start_sequence_item();
        cyaml.add_sequence_item(yystack_[0].value.as < acl::yaml::string_slice > ());
        cyaml.end_sequence_item();
    }
#line 703 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 23: // array_value: STRING
#line 143 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        yylhs.value.as < acl::yaml::string_slice > () = yystack_[0].value.as < acl::yaml::string_slice > ();
    }
#line 711 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 24: // $@2: %empty
#line 149 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.start_mapping(cyaml.get_current_key());
    }
#line 719 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 28: // $@3: %empty
#line 161 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.start_sequence();
    }
#line 727 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 29: // nested_sequence: $@3 sequence
#line 165 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.end_sequence();
    }
#line 735 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 33: // sequence_item: DASH STRING
#line 178 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.start_sequence_item();
        cyaml.add_sequence_item(yystack_[0].value.as < acl::yaml::string_slice > ());
        cyaml.end_sequence_item();
    }
#line 745 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 34: // $@4: %empty
#line 185 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.start_sequence_item();
    }
#line 753 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 35: // nested_mapping_in_sequence: $@4 mappings
#line 189 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        cyaml.end_sequence_item();
    }
#line 761 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 36: // block_scalar: PIPE block_scalar_content
#line 196 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        yylhs.value.as < acl::yaml::string_slice_array > () = std::move(yystack_[0].value.as < acl::yaml::string_slice_array > ());
    }
#line 769 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 37: // block_scalar: GREATER_THAN block_scalar_content
#line 200 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        yylhs.value.as < acl::yaml::string_slice_array > () = std::move(yystack_[0].value.as < acl::yaml::string_slice_array > ());
    }
#line 777 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 38: // block_scalar_content: NEWLINE INDENT scalar_lines DEDENT
#line 207 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
        yylhs.value.as < acl::yaml::string_slice_array > () = std::move(yystack_[1].value.as < acl::yaml::string_slice_array > ());
    }
#line 785 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 39: // scalar_lines: scalar_line
#line 214 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
      yylhs.value.as < acl::yaml::string_slice_array > ().emplace_back(yystack_[0].value.as < acl::yaml::string_slice > ());
    }
#line 793 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 40: // scalar_lines: scalar_lines NEWLINE scalar_line
#line 218 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
    {
      yylhs.value.as < acl::yaml::string_slice_array > ().emplace_back(yystack_[0].value.as < acl::yaml::string_slice > ());
    }
#line 801 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;

  case 41: // scalar_line: STRING
#line 224 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
  {
        yylhs.value.as < acl::yaml::string_slice > () = yystack_[0].value.as < acl::yaml::string_slice > ();
  }
#line 809 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"
    break;


#line 813 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        context yyctx (*this, yyla);
        std::string msg = yysyntax_error_ (yyctx);
        error (yyla.location, YY_MOVE (msg));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.kind () == symbol_kind::S_YYEOF)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    // Pop stack until we find a state that shifts the error token.
    for (;;)
      {
        yyn = yypact_[+yystack_[0].state];
        if (!yy_pact_value_is_default_ (yyn))
          {
            yyn += symbol_kind::S_YYerror;
            if (0 <= yyn && yyn <= yylast_
                && yycheck_[yyn] == symbol_kind::S_YYerror)
              {
                yyn = yytable_[yyn];
                if (0 < yyn)
                  break;
              }
          }

        // Pop the current state because it cannot handle the error token.
        if (yystack_.size () == 1)
          YYABORT;

        yyerror_range[1].location = yystack_[0].location;
        yy_destroy_ ("Error: popping", yystack_[0]);
        yypop_ ();
        YY_STACK_PRINT ();
      }
    {
      stack_symbol_type error_token;

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    YY_STACK_PRINT ();
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  parser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr;
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              else
                goto append;

            append:
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }

  std::string
  parser::symbol_name (symbol_kind_type yysymbol)
  {
    return yytnamerr_ (yytname_[yysymbol]);
  }



  // parser::context.
  parser::context::context (const parser& yyparser, const symbol_type& yyla)
    : yyparser_ (yyparser)
    , yyla_ (yyla)
  {}

  int
  parser::context::expected_tokens (symbol_kind_type yyarg[], int yyargn) const
  {
    // Actual number of expected tokens
    int yycount = 0;

    const int yyn = yypact_[+yyparser_.yystack_[0].state];
    if (!yy_pact_value_is_default_ (yyn))
      {
        /* Start YYX at -YYN if negative to avoid negative indexes in
           YYCHECK.  In other words, skip the first -YYN actions for
           this state because they are default actions.  */
        const int yyxbegin = yyn < 0 ? -yyn : 0;
        // Stay within bounds of both yycheck and yytname.
        const int yychecklim = yylast_ - yyn + 1;
        const int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
        for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
          if (yycheck_[yyx + yyn] == yyx && yyx != symbol_kind::S_YYerror
              && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
            {
              if (!yyarg)
                ++yycount;
              else if (yycount == yyargn)
                return 0;
              else
                yyarg[yycount++] = YY_CAST (symbol_kind_type, yyx);
            }
      }

    if (yyarg && yycount == 0 && 0 < yyargn)
      yyarg[0] = symbol_kind::S_YYEMPTY;
    return yycount;
  }






  int
  parser::yy_syntax_error_arguments_ (const context& yyctx,
                                                 symbol_kind_type yyarg[], int yyargn) const
  {
    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state merging
         (from LALR or IELR) and default reductions corrupt the expected
         token list.  However, the list is correct for canonical LR with
         one exception: it will still contain any token that will not be
         accepted due to an error action in a later state.
    */

    if (!yyctx.lookahead ().empty ())
      {
        if (yyarg)
          yyarg[0] = yyctx.token ();
        int yyn = yyctx.expected_tokens (yyarg ? yyarg + 1 : yyarg, yyargn - 1);
        return yyn + 1;
      }
    return 0;
  }

  // Generate an error message.
  std::string
  parser::yysyntax_error_ (const context& yyctx) const
  {
    // Its maximum.
    enum { YYARGS_MAX = 5 };
    // Arguments of yyformat.
    symbol_kind_type yyarg[YYARGS_MAX];
    int yycount = yy_syntax_error_arguments_ (yyctx, yyarg, YYARGS_MAX);

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
      default: // Avoid compiler warnings.
        YYCASE_ (0, YY_("syntax error"));
        YYCASE_ (1, YY_("syntax error, unexpected %s"));
        YYCASE_ (2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_ (3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_ (4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_ (5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    std::ptrdiff_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += symbol_name (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const signed char parser::yypact_ninf_ = -37;

  const signed char parser::yytable_ninf_ = -29;

  const signed char
  parser::yypact_[] =
  {
       3,   -37,     1,   -37,   -37,   -37,    15,   -37,   -37,    11,
      17,    11,   -37,    16,    19,   -37,   -37,    -1,   -37,   -37,
     -37,    18,    19,   -37,    21,    20,    20,   -37,    22,   -37,
     -37,    24,    25,    23,   -37,   -37,    30,   -37,    16,    19,
      16,    29,    32,   -37,     4,   -37,   -37,    19,   -37,    29,
      28,   -37,     9,   -37,   -37,    30,    31,   -37,   -37,    32,
     -37,   -37,   -37
  };

  const signed char
  parser::yydefact_[] =
  {
       2,     5,     0,     4,     1,    18,     0,    12,     3,     8,
       0,     8,    33,     6,     0,     9,    10,    19,    11,     7,
      32,     0,    35,    13,     0,     0,     0,    15,     0,    14,
      26,     0,    24,     0,    36,    37,     0,    27,     6,     0,
       6,     0,     0,    23,     0,    21,    17,    25,    16,    29,
       0,    41,     0,    39,    20,     0,     0,    30,    38,     0,
      22,    31,    40
  };

  const signed char
  parser::yypgoto_[] =
  {
     -37,   -37,   -37,   -30,    13,   -37,    -2,   -37,   -37,   -37,
     -37,   -18,   -37,   -37,   -14,   -37,   -37,   -37,   -36,   -37,
     -37,   -37,    12,   -37,   -19
  };

  const signed char
  parser::yydefgoto_[] =
  {
       0,     2,     3,    20,    16,     8,    21,    10,    27,    28,
      44,    45,    38,    39,    22,    40,    41,    49,    11,    13,
      14,    29,    34,    52,    53
  };

  const signed char
  parser::yytable_[] =
  {
       9,     4,    23,     1,     5,    50,     6,    24,    46,     7,
      48,    25,    26,    56,    54,    55,    58,    59,    12,    15,
      31,    17,     5,    19,    18,    47,    30,    32,    33,    42,
     -28,    36,    37,    43,     6,    51,    57,    60,    35,    61,
      62,     0,     0,     0,     0,    31
  };

  const signed char
  parser::yycheck_[] =
  {
       2,     0,     3,     0,     3,    41,     5,     8,    38,     8,
      40,    12,    13,    49,    10,    11,     7,     8,     3,     8,
      22,     4,     3,     7,    11,    39,     8,     6,     8,     6,
       5,     9,     8,     3,     5,     3,     8,    55,    26,     8,
      59,    -1,    -1,    -1,    -1,    47
  };

  const signed char
  parser::yystos_[] =
  {
       0,     0,    15,    16,     0,     3,     5,     8,    19,    20,
      21,    32,     3,    33,    34,     8,    18,     4,    18,     7,
      17,    20,    28,     3,     8,    12,    13,    22,    23,    35,
       8,    20,     6,     8,    36,    36,     9,     8,    26,    27,
      29,    30,     6,     3,    24,    25,    17,    28,    17,    31,
      32,     3,    37,    38,    10,    11,    32,     8,     7,     8,
      25,     8,    38
  };

  const signed char
  parser::yyr1_[] =
  {
       0,    14,    15,    15,    15,    16,    17,    17,    18,    18,
      19,    19,    19,    20,    20,    20,    20,    20,    21,    23,
      22,    24,    24,    25,    27,    26,    28,    28,    30,    29,
      31,    31,    32,    32,    34,    33,    35,    35,    36,    37,
      37,    38
  };

  const signed char
  parser::yyr2_[] =
  {
       0,     2,     0,     2,     1,     1,     0,     1,     0,     1,
       2,     2,     1,     3,     3,     3,     6,     6,     1,     0,
       4,     1,     3,     1,     0,     2,     2,     3,     0,     2,
       2,     3,     3,     2,     0,     2,     2,     2,     4,     1,
       3,     1
  };


#if YAML_DEBUG || 1
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const parser::yytname_[] =
  {
  "\"end of file\"", "error", "\"invalid token\"", "STRING", "COLON",
  "DASH", "INDENT", "DEDENT", "NEWLINE", "LBRACKET", "RBRACKET", "COMMA",
  "PIPE", "GREATER_THAN", "$accept", "document", "end", "optional_dedent",
  "optional_newline", "line", "mapping", "key", "array", "$@1",
  "array_values", "array_value", "nested_mappings", "$@2", "mappings",
  "nested_sequence", "$@3", "sequence", "sequence_item",
  "nested_mapping_in_sequence", "$@4", "block_scalar",
  "block_scalar_content", "scalar_lines", "scalar_line", YY_NULLPTR
  };
#endif


#if YAML_DEBUG
  const unsigned char
  parser::yyrline_[] =
  {
       0,    53,    53,    55,    56,    60,    63,    65,    68,    70,
      74,    75,    76,    81,    87,    93,    97,   101,   108,   116,
     116,   127,   133,   142,   149,   149,   156,   157,   161,   161,
     171,   172,   176,   177,   185,   185,   195,   199,   206,   213,
     217,   223
  };

  void
  parser::yy_stack_print_ () const
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  void
  parser::yy_reduce_print_ (int yyrule) const
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YAML_DEBUG


#line 7 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"
} } // acl::yaml
#line 1321 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml_parser_impl.cpp"

#line 228 "C:/repos/lxe/third_party/acl/src/acl/dsl/yaml.yy"


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
