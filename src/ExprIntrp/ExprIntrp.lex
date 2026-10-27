/*

 Copyright (c) 1997-1999 Matra Datavision
 Copyright (c) 1999-2014 OPEN CASCADE SAS

 This file is part of Open CASCADE Technology software library.

 This library is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License version 2.1 as published
 by the Free Software Foundation, with special exception defined in the file
 OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
 distribution for complete text of the license and disclaimer of any warranty.

 Alternatively, this file may be used under the terms of Open CASCADE
 commercial license or contractual agreement.
*/ 

/*
    c++                 generate C++ parser class
    8bit                don't fail on 8-bit input characters
    warn                warn about inconsistencies
    nodefault           don't create default echo-all rule
    noinput             disables the generation of code for reading input from standard input
    noyywrap            don't use yywrap() function
    yyclass             define name of the scanner class
*/
%option c++
%option 8bit warn nodefault
%option noyywrap
%option noinput
%option yyclass="ExprIntrp::scanner"

%top{
// This file is part of Open CASCADE Technology software library.
// This file is generated, do not modify it directly; edit source file ExprIntrp.lex instead.

// Pre-include stdlib.h to avoid redefinition of integer type macros (INT8_MIN and similar in generated code)
#if !defined(_MSC_VER) || (_MSC_VER >= 1600) // Visual Studio 2010+
  #include "stdint.h"
#endif
}

%{
#include <ExprIntrp.tab.hxx>
#include "stdio.h"

// Tell flex which function to define
#ifdef  YY_DECL
  #undef YY_DECL
#endif
#define YY_DECL int ExprIntrp::scanner::lex(ExprIntrp::parser::semantic_type* /*yylval*/)

// Disable checking for eof
#ifdef  YY_INTERACTIVE
  #undef YY_INTERACTIVE
#endif

typedef ExprIntrp::parser::token token;

#include <Standard_Failure.hxx>
#define YY_FATAL_ERROR(theMsg) throw Standard_Failure(theMsg);

#if defined(_MSC_VER)
  // add includes for flex 2.91 (Linux version)
  #include <stdlib.h>
  #include <io.h>

  // Avoid includion of unistd.h if parser is generated on Linux (flex 2.5.35)
  #ifndef YY_NO_UNISTD_H
    #define YY_NO_UNISTD_H
  #endif

  // disable MSVC warnings in flex 2.89 and 2.5.35 code
  // Note that Intel compiler also defines _MSC_VER but has different warning ids
  #if defined(__INTEL_COMPILER)
    #pragma warning(disable:177 1786 1736)
  #elif defined(__clang__)
    #pragma GCC diagnostic ignored "-Wunused-function"
    #pragma GCC diagnostic ignored "-Winconsistent-dllimport"
    #pragma GCC diagnostic ignored "-Wunneeded-internal-declaration"
  #else
    #pragma warning(disable:4131 4244 4273 4127 4267)
  #endif
#elif defined(__clang__)
  // disable Clang warnings in flex code
  #pragma clang diagnostic ignored "-Wshorten-64-to-32"
  #if __has_warning("-Wmisleading-indentation")
    #pragma clang diagnostic ignored "-Wmisleading-indentation"
  #endif
#elif defined(__GNUC__)
  // disable GCC warnings in flex code
  #pragma GCC diagnostic ignored "-Wunused-function"
  #if (__GNUC__ >= 6)
    #pragma GCC diagnostic ignored "-Wmisleading-indentation"
  #endif
#endif
%}
%%
" "             {;}
"+"             { return(token::SUMOP); }
"-"             { return(token::MINUSOP); }
"/"             { return(token::DIVIDEOP); }
"^"             { return(token::EXPOP); }
"**"            { return(token::EXPOP); }
"*"             { return(token::MULTOP); }
"("             { return(token::PARENTHESIS); }
"["             { return(token::BRACKET); }
")"             { return(token::ENDPARENTHESIS); }
"]"             { return(token::ENDBRACKET); }
","             { return(token::COMMA); }
"@"             { return(token::DIFFERENTIAL); }
"<-"            { return(token::ASSIGNOP); }
"="             { return(token::EQUALOP); }
"Deassign"      { return(token::DEASSIGNKEY); }
"Deriv"         { return(token::DERIVKEY); }
"Const"         { return(token::CONSTKEY); }
"Sum"           { return(token::SUMKEY); }
"Prod"          { return(token::PRODKEY); }

[0-9.]+e[+|-]?[0-9]+   { myCtx.ExprIntrp_SetResult(YYText(), YYLeng()); return(token::VALUE); }
[0-9.]+	               { myCtx.ExprIntrp_SetResult(YYText(), YYLeng()); return(token::VALUE); }
[a-zA-Z][a-zA-Z0-9_]*  { myCtx.ExprIntrp_SetResult(YYText(), YYLeng()); return(token::IDENTIFIER); }
";"                    { return(token::RELSEPARATOR); }
"'"+                   { myCtx.ExprIntrp_SetDegree(YYText(), YYLeng()); return(token::DERIVATE); }

%%

ExprIntrp::scanner::scanner(ExprIntrp_yaccintrf& theCtx, std::istream& theIn)
: ExprIntrpFlexLexer(&theIn, static_cast<std::ostream*>(nullptr)), myCtx(theCtx)
{
  //
}
