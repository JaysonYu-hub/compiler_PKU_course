/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 8 "/root/compiler/src/sysy.y"


#include <iostream>
#include <memory>
#include <string>
#include "ast.hpp"
#include <vector>
#include <assert.h>

int yylex();
extern int yylineno;
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;


#line 88 "/root/compiler/src/sysy.tab.cpp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "sysy.tab.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INT = 3,                        /* INT  */
  YYSYMBOL_RETURN = 4,                     /* RETURN  */
  YYSYMBOL_VOID = 5,                       /* VOID  */
  YYSYMBOL_FLOAT = 6,                      /* FLOAT  */
  YYSYMBOL_CONST = 7,                      /* CONST  */
  YYSYMBOL_WHILE = 8,                      /* WHILE  */
  YYSYMBOL_BREAK = 9,                      /* BREAK  */
  YYSYMBOL_CONTINUE = 10,                  /* CONTINUE  */
  YYSYMBOL_IDENT = 11,                     /* IDENT  */
  YYSYMBOL_INT_CONST = 12,                 /* INT_CONST  */
  YYSYMBOL_ADD = 13,                       /* ADD  */
  YYSYMBOL_SUB = 14,                       /* SUB  */
  YYSYMBOL_NOT = 15,                       /* NOT  */
  YYSYMBOL_MUL = 16,                       /* MUL  */
  YYSYMBOL_MOD = 17,                       /* MOD  */
  YYSYMBOL_DIV = 18,                       /* DIV  */
  YYSYMBOL_ASSIGN = 19,                    /* ASSIGN  */
  YYSYMBOL_LT = 20,                        /* LT  */
  YYSYMBOL_LE = 21,                        /* LE  */
  YYSYMBOL_GT = 22,                        /* GT  */
  YYSYMBOL_GE = 23,                        /* GE  */
  YYSYMBOL_EQ = 24,                        /* EQ  */
  YYSYMBOL_NE = 25,                        /* NE  */
  YYSYMBOL_L_AND = 26,                     /* L_AND  */
  YYSYMBOL_L_OR = 27,                      /* L_OR  */
  YYSYMBOL_IF = 28,                        /* IF  */
  YYSYMBOL_ELSE = 29,                      /* ELSE  */
  YYSYMBOL_30_ = 30,                       /* '('  */
  YYSYMBOL_31_ = 31,                       /* ')'  */
  YYSYMBOL_32_ = 32,                       /* ','  */
  YYSYMBOL_33_ = 33,                       /* '['  */
  YYSYMBOL_34_ = 34,                       /* ']'  */
  YYSYMBOL_35_ = 35,                       /* '{'  */
  YYSYMBOL_36_ = 36,                       /* '}'  */
  YYSYMBOL_37_ = 37,                       /* ';'  */
  YYSYMBOL_YYACCEPT = 38,                  /* $accept  */
  YYSYMBOL_CompUnit = 39,                  /* CompUnit  */
  YYSYMBOL_end_comp_unit = 40,             /* end_comp_unit  */
  YYSYMBOL_comp_unit = 41,                 /* comp_unit  */
  YYSYMBOL_FuncDef = 42,                   /* FuncDef  */
  YYSYMBOL_FuncFParams = 43,               /* FuncFParams  */
  YYSYMBOL_FuncFParam = 44,                /* FuncFParam  */
  YYSYMBOL_Block = 45,                     /* Block  */
  YYSYMBOL_block_item_list = 46,           /* block_item_list  */
  YYSYMBOL_BlockItem = 47,                 /* BlockItem  */
  YYSYMBOL_Stmt = 48,                      /* Stmt  */
  YYSYMBOL_MatchedStmt = 49,               /* MatchedStmt  */
  YYSYMBOL_UnmatchedStmt = 50,             /* UnmatchedStmt  */
  YYSYMBOL_Number = 51,                    /* Number  */
  YYSYMBOL_UnaryExp = 52,                  /* UnaryExp  */
  YYSYMBOL_FuncRParams = 53,               /* FuncRParams  */
  YYSYMBOL_Exp = 54,                       /* Exp  */
  YYSYMBOL_PrimaryExp = 55,                /* PrimaryExp  */
  YYSYMBOL_MulExp = 56,                    /* MulExp  */
  YYSYMBOL_AddExp = 57,                    /* AddExp  */
  YYSYMBOL_RelExp = 58,                    /* RelExp  */
  YYSYMBOL_EqExp = 59,                     /* EqExp  */
  YYSYMBOL_LAndExp = 60,                   /* LAndExp  */
  YYSYMBOL_LOrExp = 61,                    /* LOrExp  */
  YYSYMBOL_Decl = 62,                      /* Decl  */
  YYSYMBOL_ConstDecl = 63,                 /* ConstDecl  */
  YYSYMBOL_const_def_list = 64,            /* const_def_list  */
  YYSYMBOL_Type = 65,                      /* Type  */
  YYSYMBOL_FuncType = 66,                  /* FuncType  */
  YYSYMBOL_ConstDef = 67,                  /* ConstDef  */
  YYSYMBOL_arr_dims = 68,                  /* arr_dims  */
  YYSYMBOL_lval_indexes = 69,              /* lval_indexes  */
  YYSYMBOL_ConstInitVal = 70,              /* ConstInitVal  */
  YYSYMBOL_const_init_val_list = 71,       /* const_init_val_list  */
  YYSYMBOL_VarDecl = 72,                   /* VarDecl  */
  YYSYMBOL_var_def_list = 73,              /* var_def_list  */
  YYSYMBOL_VarDef = 74,                    /* VarDef  */
  YYSYMBOL_InitVal = 75,                   /* InitVal  */
  YYSYMBOL_init_val_list = 76,             /* init_val_list  */
  YYSYMBOL_LVal = 77,                      /* LVal  */
  YYSYMBOL_ConstExp = 78,                  /* ConstExp  */
  YYSYMBOL_UnaryOp = 79,                   /* UnaryOp  */
  YYSYMBOL_AddOp = 80,                     /* AddOp  */
  YYSYMBOL_MulOp = 81,                     /* MulOp  */
  YYSYMBOL_ComOp = 82,                     /* ComOp  */
  YYSYMBOL_EquOp = 83,                     /* EquOp  */
  YYSYMBOL_LOrOp = 84,                     /* LOrOp  */
  YYSYMBOL_LAndOp = 85                     /* LAndOp  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  14
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   209

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  38
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  48
/* YYNRULES -- Number of rules.  */
#define YYNRULES  105
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  174

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   284


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      30,    31,     2,     2,    32,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    37,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    33,     2,    34,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    35,     2,    36,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    56,    56,    64,    69,    78,    83,    91,    99,   110,
     115,   123,   130,   137,   149,   155,   160,   168,   173,   181,
     186,   195,   200,   205,   211,   216,   220,   225,   232,   238,
     242,   249,   255,   262,   271,   275,   280,   286,   292,   301,
     306,   314,   322,   327,   332,   340,   345,   355,   360,   370,
     375,   385,   390,   400,   405,   415,   420,   430,   431,   435,
     445,   451,   459,   463,   466,   472,   478,   489,   494,   502,
     507,   515,   520,   525,   533,   538,   546,   556,   562,   570,
     575,   582,   588,   599,   604,   608,   614,   619,   627,   632,
     642,   646,   647,   651,   652,   656,   657,   658,   662,   663,
     664,   665,   669,   670,   674,   678
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "INT", "RETURN",
  "VOID", "FLOAT", "CONST", "WHILE", "BREAK", "CONTINUE", "IDENT",
  "INT_CONST", "ADD", "SUB", "NOT", "MUL", "MOD", "DIV", "ASSIGN", "LT",
  "LE", "GT", "GE", "EQ", "NE", "L_AND", "L_OR", "IF", "ELSE", "'('",
  "')'", "','", "'['", "']'", "'{'", "'}'", "';'", "$accept", "CompUnit",
  "end_comp_unit", "comp_unit", "FuncDef", "FuncFParams", "FuncFParam",
  "Block", "block_item_list", "BlockItem", "Stmt", "MatchedStmt",
  "UnmatchedStmt", "Number", "UnaryExp", "FuncRParams", "Exp",
  "PrimaryExp", "MulExp", "AddExp", "RelExp", "EqExp", "LAndExp", "LOrExp",
  "Decl", "ConstDecl", "const_def_list", "Type", "FuncType", "ConstDef",
  "arr_dims", "lval_indexes", "ConstInitVal", "const_init_val_list",
  "VarDecl", "var_def_list", "VarDef", "InitVal", "init_val_list", "LVal",
  "ConstExp", "UnaryOp", "AddOp", "MulOp", "ComOp", "EquOp", "LOrOp",
  "LAndOp", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-114)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      87,  -114,  -114,    52,    11,    87,  -114,  -114,  -114,  -114,
      24,  -114,  -114,    33,  -114,  -114,    -5,     2,  -114,     8,
      40,  -114,    56,     7,   172,    23,    71,  -114,    89,    31,
      33,  -114,    -7,  -114,  -114,  -114,  -114,   172,    18,  -114,
    -114,  -114,  -114,   158,    85,   147,    83,    59,    66,  -114,
    -114,   172,  -114,    61,   109,  -114,   110,  -114,    86,    56,
     172,    46,  -114,    48,  -114,  -114,    89,  -114,   151,   172,
      92,    97,  -114,  -114,    90,  -114,  -114,  -114,   172,   172,
    -114,  -114,  -114,  -114,   172,  -114,  -114,   172,  -114,   172,
    -114,   172,  -114,  -114,  -114,    61,    52,   100,  -114,  -114,
     101,  -114,  -114,    91,  -114,  -114,   113,  -114,   104,   172,
    -114,    56,  -114,  -114,   158,    85,   147,    83,    59,   143,
    -114,  -114,   125,  -114,    89,  -114,  -114,   172,  -114,   126,
    -114,     6,   131,   135,   140,   159,  -114,  -114,  -114,  -114,
    -114,  -114,  -114,   153,  -114,    71,   169,   160,  -114,  -114,
    -114,  -114,   154,   172,  -114,  -114,   172,  -114,   172,   161,
    -114,   164,   165,   155,   102,   102,  -114,  -114,  -114,  -114,
     168,   102,  -114,  -114
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    63,    64,     0,     0,     2,     3,     5,     6,    57,
       0,    62,    58,     0,     1,     4,    79,     0,    77,     0,
       0,    60,     0,     0,     0,    80,     0,    76,     0,     0,
       0,    59,    88,    34,    93,    94,    92,     0,     0,    43,
      45,    83,    35,    47,    49,    51,    53,    55,    41,    81,
      44,     0,    91,     0,     0,     9,     0,    90,     0,     0,
       0,    79,    78,     0,    65,    71,     0,    61,     0,     0,
      89,     0,    84,    86,     0,    95,    97,    96,     0,     0,
      99,    98,   101,   100,     0,   102,   103,     0,   105,     0,
     104,     0,    36,    15,     8,     0,     0,    11,    67,    82,
       0,    72,    74,     0,    66,    37,     0,    39,     0,     0,
      42,     0,    85,    46,    48,    50,    52,    54,    56,     0,
       7,    10,     0,    68,     0,    73,    38,     0,    69,     0,
      87,     0,     0,     0,     0,     0,    14,    25,    26,    16,
      17,    19,    20,     0,    18,     0,    44,    12,    75,    40,
      70,    22,     0,     0,    29,    30,     0,    24,     0,    13,
      21,     0,     0,     0,     0,     0,    23,    28,    33,    31,
      19,     0,    27,    32
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -114,  -114,  -114,   193,  -114,  -114,   103,   -46,  -114,  -114,
      35,   -91,   -83,  -114,   -38,  -114,   -22,  -114,   122,   119,
     117,   116,   115,  -114,    88,  -114,  -114,    -1,  -114,   178,
     -18,  -114,   -58,  -114,  -114,  -114,   183,   -35,  -114,  -113,
     -15,  -114,   -40,  -114,  -114,  -114,  -114,  -114
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     4,     5,     6,     7,    54,    55,   138,   119,   139,
     140,   141,   142,    39,    40,   106,    57,    42,    43,    44,
      45,    46,    47,    48,     8,     9,    20,    10,    11,    21,
      25,    70,    64,   103,    12,    17,    18,    49,    74,    50,
      65,    51,    52,    78,    84,    87,    91,    89
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      41,    29,    13,    73,    79,   102,   146,    94,   104,    58,
       1,    14,     2,    92,    22,    71,    41,    32,    33,    34,
      35,    36,    56,    68,    99,    23,    69,    28,    24,    32,
      33,    34,    35,    36,    26,    16,    37,    41,    53,    27,
     113,    24,    59,   151,    19,   100,   107,   108,    37,   120,
      66,   146,   146,    38,    72,     1,    60,     2,   146,    32,
      33,    34,    35,    36,    60,    22,   148,    32,    33,    34,
      35,    36,    30,   167,   170,    79,   130,    31,    37,    24,
     172,   168,    61,    63,   101,    88,    37,   129,   173,    41,
       1,    38,     2,    90,     3,    56,    93,   143,    34,    35,
      32,    33,    34,    35,    36,   149,   131,    85,    86,   152,
     132,   133,   134,    32,    33,    34,    35,    36,   145,    37,
      98,    97,   111,   124,    63,   109,   112,   125,   110,   159,
     135,   161,    37,   122,   162,   123,   163,    93,   128,   137,
      95,    96,   143,   143,   126,   127,     1,   131,     2,   143,
       3,   132,   133,   134,    32,    33,    34,    35,    36,   147,
     150,   153,    32,    33,    34,    35,    36,    80,    81,    82,
      83,   135,   154,    37,    75,    76,    77,   155,    93,   136,
     137,    37,   105,    32,    33,    34,    35,    36,   158,   156,
     157,   160,   166,    24,    60,   164,   165,   171,    15,   121,
     169,   114,    37,   115,   116,   117,   118,   144,    67,    62
};

static const yytype_uint8 yycheck[] =
{
      22,    19,     3,    38,    44,    63,   119,    53,    66,    24,
       3,     0,     5,    51,    19,    37,    38,    11,    12,    13,
      14,    15,    23,    30,    59,    30,    33,    19,    33,    11,
      12,    13,    14,    15,    32,    11,    30,    59,    31,    37,
      78,    33,    19,    37,    11,    60,    68,    69,    30,    95,
      19,   164,   165,    35,    36,     3,    33,     5,   171,    11,
      12,    13,    14,    15,    33,    19,   124,    11,    12,    13,
      14,    15,    32,   164,   165,   115,   111,    37,    30,    33,
     171,   164,    11,    35,    36,    26,    30,   109,   171,   111,
       3,    35,     5,    27,     7,    96,    35,   119,    13,    14,
      11,    12,    13,    14,    15,   127,     4,    24,    25,   131,
       8,     9,    10,    11,    12,    13,    14,    15,   119,    30,
      34,    11,    32,    32,    35,    33,    36,    36,    31,   147,
      28,   153,    30,    33,   156,    34,   158,    35,    34,    37,
      31,    32,   164,   165,    31,    32,     3,     4,     5,   171,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    34,
      34,    30,    11,    12,    13,    14,    15,    20,    21,    22,
      23,    28,    37,    30,    16,    17,    18,    37,    35,    36,
      37,    30,    31,    11,    12,    13,    14,    15,    19,    30,
      37,    37,    37,    33,    33,    31,    31,    29,     5,    96,
     165,    79,    30,    84,    87,    89,    91,   119,    30,    26
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     5,     7,    39,    40,    41,    42,    62,    63,
      65,    66,    72,    65,     0,    41,    11,    73,    74,    11,
      64,    67,    19,    30,    33,    68,    32,    37,    19,    68,
      32,    37,    11,    12,    13,    14,    15,    30,    35,    51,
      52,    54,    55,    56,    57,    58,    59,    60,    61,    75,
      77,    79,    80,    31,    43,    44,    65,    54,    78,    19,
      33,    11,    74,    35,    70,    78,    19,    67,    30,    33,
      69,    54,    36,    75,    76,    16,    17,    18,    81,    80,
      20,    21,    22,    23,    82,    24,    25,    83,    26,    85,
      27,    84,    52,    35,    45,    31,    32,    11,    34,    75,
      78,    36,    70,    71,    70,    31,    53,    54,    54,    33,
      31,    32,    36,    52,    56,    57,    58,    59,    60,    46,
      45,    44,    33,    34,    32,    36,    31,    32,    34,    54,
      75,     4,     8,     9,    10,    28,    36,    37,    45,    47,
      48,    49,    50,    54,    62,    65,    77,    34,    70,    54,
      34,    37,    54,    30,    37,    37,    30,    37,    19,    68,
      37,    54,    54,    54,    31,    31,    37,    49,    50,    48,
      49,    29,    49,    50
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    38,    39,    40,    40,    41,    41,    42,    42,    43,
      43,    44,    44,    44,    45,    46,    46,    47,    47,    48,
      48,    49,    49,    49,    49,    49,    49,    49,    49,    49,
      49,    50,    50,    50,    51,    52,    52,    52,    52,    53,
      53,    54,    55,    55,    55,    56,    56,    57,    57,    58,
      58,    59,    59,    60,    60,    61,    61,    62,    62,    63,
      64,    64,    65,    66,    66,    67,    67,    68,    68,    69,
      69,    70,    70,    70,    71,    71,    72,    73,    73,    74,
      74,    74,    74,    75,    75,    75,    76,    76,    77,    77,
      78,    79,    79,    80,    80,    81,    81,    81,    82,    82,
      82,    82,    83,    83,    84,    85
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     2,     1,     1,     6,     5,     1,
       3,     2,     4,     5,     3,     0,     2,     1,     1,     1,
       1,     3,     2,     4,     2,     1,     1,     7,     5,     2,
       2,     5,     7,     5,     1,     1,     2,     3,     4,     1,
       3,     1,     3,     1,     1,     1,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     1,     4,
       1,     3,     1,     1,     1,     3,     4,     3,     4,     3,
       4,     1,     2,     3,     1,     3,     3,     1,     3,     1,
       2,     3,     4,     1,     2,     3,     1,     3,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (ast, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, ast); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, std::unique_ptr<BaseAST> &ast)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (ast);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, std::unique_ptr<BaseAST> &ast)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, ast);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, std::unique_ptr<BaseAST> &ast)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)], ast);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, ast); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
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
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, std::unique_ptr<BaseAST> &ast)
{
  YY_USE (yyvaluep);
  YY_USE (ast);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (std::unique_ptr<BaseAST> &ast)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* CompUnit: end_comp_unit  */
#line 56 "/root/compiler/src/sysy.y"
                  {
    auto comp_unit = new CompUnitAST();
    comp_unit->end_comp_unit = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    ast = unique_ptr<BaseAST>(comp_unit);
  }
#line 1566 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 3: /* end_comp_unit: comp_unit  */
#line 64 "/root/compiler/src/sysy.y"
              {
    auto start = new CompUnitSequenceAST();
    start->comp_unit = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = start;
  }
#line 1576 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 4: /* end_comp_unit: end_comp_unit comp_unit  */
#line 69 "/root/compiler/src/sysy.y"
                            { 
    auto start = new CompUnitListAST();
    start->end_comp_unit = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    start->comp_unit = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = start;
  }
#line 1587 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 5: /* comp_unit: FuncDef  */
#line 78 "/root/compiler/src/sysy.y"
            { 
    auto comp_unit = new FunctionDefinitionUnitAST();
    comp_unit->func_def = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = comp_unit;
  }
#line 1597 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 6: /* comp_unit: Decl  */
#line 83 "/root/compiler/src/sysy.y"
         { 
    auto comp_unit = new DeclarationUnitAST();
    comp_unit->decl = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = comp_unit;
  }
#line 1607 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 7: /* FuncDef: Type IDENT '(' FuncFParams ')' Block  */
#line 91 "/root/compiler/src/sysy.y"
                                         {
    auto func_def = new FuncDefAST();
    func_def->type = unique_ptr<string>((yyvsp[-5].str_val));
    func_def->ident = unique_ptr<string>((yyvsp[-4].str_val));
    func_def->func_f_params = unique_ptr<FuncFParamsAST>(static_cast<FuncFParamsAST*>((yyvsp[-2].ast_val)));
    func_def->block = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = func_def;
  }
#line 1620 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 8: /* FuncDef: Type IDENT '(' ')' Block  */
#line 99 "/root/compiler/src/sysy.y"
                             { 
    auto func_def = new FuncDefAST();
    func_def->type = unique_ptr<string>((yyvsp[-4].str_val));
    func_def->ident = unique_ptr<string>((yyvsp[-3].str_val));
    func_def->func_f_params = nullptr;
    func_def->block = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = func_def;
  }
#line 1633 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 9: /* FuncFParams: FuncFParam  */
#line 110 "/root/compiler/src/sysy.y"
               { 
    auto func_f_params = new FuncFParamsAST();
    func_f_params->func_f_params.push_back(unique_ptr<FuncFParamAST>(static_cast<FuncFParamAST*>((yyvsp[0].ast_val))));
    (yyval.ast_val) = func_f_params;
  }
#line 1643 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 10: /* FuncFParams: FuncFParams ',' FuncFParam  */
#line 115 "/root/compiler/src/sysy.y"
                               { 
    auto func_f_params = (static_cast<FuncFParamsAST*>((yyvsp[-2].ast_val)));
    func_f_params->func_f_params.push_back(unique_ptr<FuncFParamAST>(static_cast<FuncFParamAST*>((yyvsp[0].ast_val))));
    (yyval.ast_val) = func_f_params;
  }
#line 1653 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 11: /* FuncFParam: Type IDENT  */
#line 123 "/root/compiler/src/sysy.y"
               { 
    auto func_f_param = new FuncFParamAST();
    func_f_param->type = unique_ptr<string>((yyvsp[-1].str_val));
    func_f_param->ident = unique_ptr<string>((yyvsp[0].str_val));
    func_f_param->is_array = false;
    (yyval.ast_val) = func_f_param;
  }
#line 1665 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 12: /* FuncFParam: Type IDENT '[' ']'  */
#line 130 "/root/compiler/src/sysy.y"
                       {
    auto func_f_param = new FuncFParamAST();
    func_f_param->type = unique_ptr<string>((yyvsp[-3].str_val));
    func_f_param->ident = unique_ptr<string>((yyvsp[-2].str_val));
    func_f_param->is_array = true;
    (yyval.ast_val) = func_f_param;
  }
#line 1677 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 13: /* FuncFParam: Type IDENT '[' ']' arr_dims  */
#line 137 "/root/compiler/src/sysy.y"
                                {
    auto func_f_param = new FuncFParamAST();
    func_f_param->type = unique_ptr<string>((yyvsp[-4].str_val));
    func_f_param->ident = unique_ptr<string>((yyvsp[-3].str_val));
    func_f_param->is_array = true;
    auto dims = unique_ptr<ArrayDimensionsAST>(static_cast<ArrayDimensionsAST*>((yyvsp[0].ast_val)));
    func_f_param->array_dims = std::move(dims->dims);
    (yyval.ast_val) = func_f_param;
  }
#line 1691 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 14: /* Block: '{' block_item_list '}'  */
#line 149 "/root/compiler/src/sysy.y"
                            { 
    (yyval.ast_val) = (yyvsp[-1].ast_val);
  }
#line 1699 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 15: /* block_item_list: %empty  */
#line 155 "/root/compiler/src/sysy.y"
    { 
    auto block_ast = new BlockAST(); 
    block_ast->block_items.clear(); 
    (yyval.ast_val) = block_ast; 
  }
#line 1709 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 16: /* block_item_list: block_item_list BlockItem  */
#line 160 "/root/compiler/src/sysy.y"
                              { 
    auto block_ast = (static_cast<BlockAST*>((yyvsp[-1].ast_val)));
    block_ast->block_items.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val))); 
    (yyval.ast_val) = block_ast; 
  }
#line 1719 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 17: /* BlockItem: Stmt  */
#line 168 "/root/compiler/src/sysy.y"
         {
    auto stmt_block = new StmtBlockAST();
    stmt_block->stmt = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = stmt_block;
  }
#line 1729 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 18: /* BlockItem: Decl  */
#line 173 "/root/compiler/src/sysy.y"
         { 
    auto decl_block = new DeclBlockAST();
    decl_block->decl = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = decl_block;
   }
#line 1739 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 19: /* Stmt: MatchedStmt  */
#line 181 "/root/compiler/src/sysy.y"
                { 
    auto stmt = new MatchedStmtWrapperAST();
    stmt->matched_stmt = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = stmt;
  }
#line 1749 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 20: /* Stmt: UnmatchedStmt  */
#line 186 "/root/compiler/src/sysy.y"
                  { 
    auto stmt = new UnmatchedStmtWrapperAST();
    stmt->unmatched_stmt = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = stmt;
  }
#line 1759 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 21: /* MatchedStmt: RETURN Exp ';'  */
#line 195 "/root/compiler/src/sysy.y"
                   {
    auto stmt = new ReturnStmtAST();
    stmt->exp = unique_ptr<ValueAST>((yyvsp[-1].value_ast));
    (yyval.ast_val) = stmt;
  }
#line 1769 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 22: /* MatchedStmt: RETURN ';'  */
#line 200 "/root/compiler/src/sysy.y"
               { 
    auto stmt = new ReturnStmtAST();
    stmt->exp = nullptr;
    (yyval.ast_val) = stmt;
  }
#line 1779 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 23: /* MatchedStmt: LVal ASSIGN Exp ';'  */
#line 205 "/root/compiler/src/sysy.y"
                        { 
    auto assign_stmt = new AssignStmtAST();
    assign_stmt->lval = unique_ptr<LValAST>(static_cast<LValAST*>((yyvsp[-3].value_ast)));
    assign_stmt->exp = unique_ptr<ValueAST>((yyvsp[-1].value_ast));
    (yyval.ast_val) = assign_stmt;
  }
#line 1790 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 24: /* MatchedStmt: Exp ';'  */
#line 211 "/root/compiler/src/sysy.y"
            { 
    auto exp_stmt = new ExpStmtAST(); 
    exp_stmt->exp = unique_ptr<ValueAST>((yyvsp[-1].value_ast)); 
    (yyval.ast_val) = exp_stmt;
  }
#line 1800 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 25: /* MatchedStmt: ';'  */
#line 216 "/root/compiler/src/sysy.y"
        {
    auto empty_stmt = new EmptyStmtAST();
    (yyval.ast_val) = empty_stmt; 
  }
#line 1809 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 26: /* MatchedStmt: Block  */
#line 220 "/root/compiler/src/sysy.y"
          {
    auto block_stmt = new BlockStmtAST(); 
    block_stmt->block = unique_ptr<BaseAST>((yyvsp[0].ast_val)); 
    (yyval.ast_val) = block_stmt; 
  }
#line 1819 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 27: /* MatchedStmt: IF '(' Exp ')' MatchedStmt ELSE MatchedStmt  */
#line 225 "/root/compiler/src/sysy.y"
                                                { 
     auto if_stmt = new IfMatchedStmtAST();
     if_stmt->if_exp = unique_ptr<ValueAST>((yyvsp[-4].value_ast));
     if_stmt->if_stmt = unique_ptr<BaseAST>((yyvsp[-2].ast_val));
     if_stmt->else_stmt = unique_ptr<BaseAST>((yyvsp[0].ast_val));
     (yyval.ast_val) = if_stmt;
  }
#line 1831 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 28: /* MatchedStmt: WHILE '(' Exp ')' MatchedStmt  */
#line 232 "/root/compiler/src/sysy.y"
                                  { 
    auto while_stmt = new WhileStmtAST();
    while_stmt->while_exp = unique_ptr<ValueAST>((yyvsp[-2].value_ast));
    while_stmt->while_stmt = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = while_stmt;
  }
#line 1842 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 29: /* MatchedStmt: BREAK ';'  */
#line 238 "/root/compiler/src/sysy.y"
              { 
    auto break_stmt = new BreakStmtAST();
    (yyval.ast_val) = break_stmt;
  }
#line 1851 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 30: /* MatchedStmt: CONTINUE ';'  */
#line 242 "/root/compiler/src/sysy.y"
                 { 
    auto continue_stmt = new ContinueStmtAST();
    (yyval.ast_val) = continue_stmt;
  }
#line 1860 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 31: /* UnmatchedStmt: IF '(' Exp ')' Stmt  */
#line 249 "/root/compiler/src/sysy.y"
                        { 
    auto if_stmt = new IfUnmatchedStmtAST();
    if_stmt->if_exp = unique_ptr<ValueAST>((yyvsp[-2].value_ast));
    if_stmt->if_stmt = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = if_stmt;
  }
#line 1871 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 32: /* UnmatchedStmt: IF '(' Exp ')' MatchedStmt ELSE UnmatchedStmt  */
#line 255 "/root/compiler/src/sysy.y"
                                                  { 
     auto if_else_stmt = new IfElseUnmatchedStmtAST();
     if_else_stmt->if_exp = unique_ptr<ValueAST>((yyvsp[-4].value_ast));
     if_else_stmt->if_stmt = unique_ptr<BaseAST>((yyvsp[-2].ast_val));
     if_else_stmt->else_stmt = unique_ptr<BaseAST>((yyvsp[0].ast_val));
     (yyval.ast_val) = if_else_stmt;
  }
#line 1883 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 33: /* UnmatchedStmt: WHILE '(' Exp ')' UnmatchedStmt  */
#line 262 "/root/compiler/src/sysy.y"
                                    { 
    auto while_stmt = new WhileStmtAST();
    while_stmt->while_exp = unique_ptr<ValueAST>((yyvsp[-2].value_ast));
    while_stmt->while_stmt = unique_ptr<BaseAST>((yyvsp[0].ast_val));
    (yyval.ast_val) = while_stmt;
  }
#line 1894 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 34: /* Number: INT_CONST  */
#line 271 "/root/compiler/src/sysy.y"
                { (yyval.int_val) = (yyvsp[0].int_val); }
#line 1900 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 35: /* UnaryExp: PrimaryExp  */
#line 275 "/root/compiler/src/sysy.y"
               { 
    auto primary_unary_exp = new PrimaryUnaryExpAST();
    primary_unary_exp->primary_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = primary_unary_exp;
  }
#line 1910 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 36: /* UnaryExp: UnaryOp UnaryExp  */
#line 280 "/root/compiler/src/sysy.y"
                     { 
    auto unary_op_exp = new UnaryOPExpAST();
    unary_op_exp->op = unique_ptr<OpAST>((yyvsp[-1].op_ast));
    unary_op_exp->unary_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = unary_op_exp; 
  }
#line 1921 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 37: /* UnaryExp: IDENT '(' ')'  */
#line 286 "/root/compiler/src/sysy.y"
                  { 
    auto func_call_exp = new FuncCallExpAST();
    func_call_exp->ident = unique_ptr<string>((yyvsp[-2].str_val));
    func_call_exp->func_r_params = nullptr;
    (yyval.value_ast) = func_call_exp;
  }
#line 1932 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 38: /* UnaryExp: IDENT '(' FuncRParams ')'  */
#line 292 "/root/compiler/src/sysy.y"
                              { 
    auto func_call_exp = new FuncCallExpAST();
    func_call_exp->ident = unique_ptr<string>((yyvsp[-3].str_val));
    func_call_exp->func_r_params = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    (yyval.value_ast) = func_call_exp;
  }
#line 1943 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 39: /* FuncRParams: Exp  */
#line 301 "/root/compiler/src/sysy.y"
        { 
    auto func_r_params = new FuncRParamsAST();
    func_r_params->func_r_params.push_back(unique_ptr<ValueAST>((yyvsp[0].value_ast)));
    (yyval.ast_val) = func_r_params;
  }
#line 1953 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 40: /* FuncRParams: FuncRParams ',' Exp  */
#line 306 "/root/compiler/src/sysy.y"
                        { 
    auto func_r_params = (static_cast<FuncRParamsAST*>((yyvsp[-2].ast_val)));
    func_r_params->func_r_params.push_back(unique_ptr<ValueAST>((yyvsp[0].value_ast)));
    (yyval.ast_val) = func_r_params;
  }
#line 1963 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 41: /* Exp: LOrExp  */
#line 314 "/root/compiler/src/sysy.y"
           {
    auto exp = new ExpAST();
    exp->lor_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = exp;
  }
#line 1973 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 42: /* PrimaryExp: '(' Exp ')'  */
#line 322 "/root/compiler/src/sysy.y"
                {
    auto paren_expr = new ParenthesizedExprAST();
    paren_expr->exp = unique_ptr<ValueAST>((yyvsp[-1].value_ast));
    (yyval.value_ast) = paren_expr;
  }
#line 1983 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 43: /* PrimaryExp: Number  */
#line 327 "/root/compiler/src/sysy.y"
           {
    auto number_exp = new NumberPrimaryExprAST();
    number_exp->value = (yyvsp[0].int_val);
    (yyval.value_ast) = number_exp;  
  }
#line 1993 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 44: /* PrimaryExp: LVal  */
#line 332 "/root/compiler/src/sysy.y"
         {
    auto lval_exp = new LValuePrimaryExprAST();
    lval_exp->lval = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = lval_exp;
  }
#line 2003 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 45: /* MulExp: UnaryExp  */
#line 340 "/root/compiler/src/sysy.y"
             {
    auto unary_mul = new UnaryMulExpAST();
    unary_mul->unary_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = unary_mul;
  }
#line 2013 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 46: /* MulExp: MulExp MulOp UnaryExp  */
#line 345 "/root/compiler/src/sysy.y"
                          {
    auto three_mul_exp = new ThreeMulExpAST();
    three_mul_exp->mul_exp = unique_ptr<ValueAST>((yyvsp[-2].value_ast));
    three_mul_exp->op = unique_ptr<OpAST>((yyvsp[-1].op_ast));
    three_mul_exp->unary_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = three_mul_exp;
  }
#line 2025 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 47: /* AddExp: MulExp  */
#line 355 "/root/compiler/src/sysy.y"
           {
    auto mul_add_exp = new MulAddExpAST();
    mul_add_exp->mul_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = mul_add_exp;
  }
#line 2035 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 48: /* AddExp: AddExp AddOp MulExp  */
#line 360 "/root/compiler/src/sysy.y"
                        {
    auto binary_add_exp = new BinaryAddExprAST();
    binary_add_exp->add_exp = unique_ptr<ValueAST>((yyvsp[-2].value_ast));
    binary_add_exp->op = unique_ptr<OpAST>((yyvsp[-1].op_ast));
    binary_add_exp->mul_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = binary_add_exp;
  }
#line 2047 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 49: /* RelExp: AddExp  */
#line 370 "/root/compiler/src/sysy.y"
           {
    auto add_rel_exp = new AddRelExpAST();
    add_rel_exp->add_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = add_rel_exp;
  }
#line 2057 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 50: /* RelExp: RelExp ComOp AddExp  */
#line 375 "/root/compiler/src/sysy.y"
                        {
    auto binary_rel_exp = new BinaryRelExprAST();
    binary_rel_exp->rel_exp = unique_ptr<ValueAST>((yyvsp[-2].value_ast));
    binary_rel_exp->op = unique_ptr<OpAST>((yyvsp[-1].op_ast));
    binary_rel_exp->add_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = binary_rel_exp;
  }
#line 2069 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 51: /* EqExp: RelExp  */
#line 385 "/root/compiler/src/sysy.y"
           {
    auto rel_eq_exp = new RelEqExpAST();
    rel_eq_exp->rel_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = rel_eq_exp;
  }
#line 2079 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 52: /* EqExp: EqExp EquOp RelExp  */
#line 390 "/root/compiler/src/sysy.y"
                       {
    auto binary_eq_exp = new BinaryEqExprAST();
    binary_eq_exp->eq_exp = unique_ptr<ValueAST>((yyvsp[-2].value_ast));
    binary_eq_exp->op = unique_ptr<OpAST>((yyvsp[-1].op_ast));
    binary_eq_exp->rel_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = binary_eq_exp;
  }
#line 2091 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 53: /* LAndExp: EqExp  */
#line 400 "/root/compiler/src/sysy.y"
          {
    auto eq_land_exp = new EqLAndExpAST();
    eq_land_exp->eq_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = eq_land_exp;
  }
#line 2101 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 54: /* LAndExp: LAndExp LAndOp EqExp  */
#line 405 "/root/compiler/src/sysy.y"
                         {
    auto binary_land_exp = new BinaryLogicalAndExprAST();
    binary_land_exp->land_exp = unique_ptr<ValueAST>((yyvsp[-2].value_ast));
    binary_land_exp->op = unique_ptr<OpAST>((yyvsp[-1].op_ast));
    binary_land_exp->eq_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = binary_land_exp;
  }
#line 2113 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 55: /* LOrExp: LAndExp  */
#line 415 "/root/compiler/src/sysy.y"
            {
    auto land_lor_exp = new LAndLOrExpAST();
    land_lor_exp->land_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = land_lor_exp;
  }
#line 2123 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 56: /* LOrExp: LOrExp LOrOp LAndExp  */
#line 420 "/root/compiler/src/sysy.y"
                         {
    auto binary_lor_exp = new BinaryLogicalOrExprAST();
    binary_lor_exp->lor_exp = unique_ptr<ValueAST>((yyvsp[-2].value_ast));
    binary_lor_exp->op = unique_ptr<OpAST>((yyvsp[-1].op_ast));
    binary_lor_exp->land_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = binary_lor_exp;
  }
#line 2135 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 57: /* Decl: ConstDecl  */
#line 430 "/root/compiler/src/sysy.y"
              { auto decl = new ConstDeclWrapperAST(); decl->const_decl = unique_ptr<BaseAST>((yyvsp[0].ast_val)); (yyval.ast_val) = decl; }
#line 2141 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 58: /* Decl: VarDecl  */
#line 431 "/root/compiler/src/sysy.y"
              { auto decl = new VarDeclWrapperAST(); decl->var_decl = unique_ptr<BaseAST>((yyvsp[0].ast_val)); (yyval.ast_val) = decl; }
#line 2147 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 59: /* ConstDecl: CONST Type const_def_list ';'  */
#line 435 "/root/compiler/src/sysy.y"
                                  {
    assert(*(yyvsp[-2].str_val) != "void");
    auto decl_ast = new ConstDeclAST();
    decl_ast->type = unique_ptr<string>((yyvsp[-2].str_val));
    decl_ast->const_def_list = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    (yyval.ast_val) = decl_ast;
  }
#line 2159 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 60: /* const_def_list: ConstDef  */
#line 445 "/root/compiler/src/sysy.y"
             {
    auto const_def_list_ast = new ConstDefListAST();
    const_def_list_ast->const_defs = vector<unique_ptr<BaseAST>>();
    const_def_list_ast->const_defs.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = const_def_list_ast;
  }
#line 2170 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 61: /* const_def_list: const_def_list ',' ConstDef  */
#line 451 "/root/compiler/src/sysy.y"
                                {
    auto const_def_list_ast = (static_cast<ConstDefListAST*>((yyvsp[-2].ast_val)));
    const_def_list_ast->const_defs.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = const_def_list_ast;
  }
#line 2180 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 62: /* Type: FuncType  */
#line 459 "/root/compiler/src/sysy.y"
             { (yyval.str_val) = (yyvsp[0].str_val); }
#line 2186 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 63: /* FuncType: INT  */
#line 463 "/root/compiler/src/sysy.y"
        {
    (yyval.str_val) = new string("i32");
  }
#line 2194 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 64: /* FuncType: VOID  */
#line 466 "/root/compiler/src/sysy.y"
         { 
    (yyval.str_val) = new string("void");
  }
#line 2202 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 65: /* ConstDef: IDENT ASSIGN ConstInitVal  */
#line 472 "/root/compiler/src/sysy.y"
                              { 
    auto const_def = new ConstDefAST();
    const_def->ident = unique_ptr<string>((yyvsp[-2].str_val));
    const_def->const_init_val = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.ast_val) = const_def;
  }
#line 2213 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 66: /* ConstDef: IDENT arr_dims ASSIGN ConstInitVal  */
#line 478 "/root/compiler/src/sysy.y"
                                       {
    auto const_def = new ArrayConstDefAST();
    const_def->ident = unique_ptr<string>((yyvsp[-3].str_val));
    auto dims = unique_ptr<ArrayDimensionsAST>(static_cast<ArrayDimensionsAST*>((yyvsp[-2].ast_val)));
    const_def->const_exps = std::move(dims->dims);
    const_def->const_init_val = unique_ptr<CompoundConstInitValueAST>(static_cast<CompoundConstInitValueAST*>((yyvsp[0].value_ast)));
    (yyval.ast_val) = const_def;
  }
#line 2226 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 67: /* arr_dims: '[' ConstExp ']'  */
#line 489 "/root/compiler/src/sysy.y"
                     {
    auto dims = new ArrayDimensionsAST();
    dims->dims.push_back(unique_ptr<ValueAST>((yyvsp[-1].value_ast)));
    (yyval.ast_val) = dims;
  }
#line 2236 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 68: /* arr_dims: arr_dims '[' ConstExp ']'  */
#line 494 "/root/compiler/src/sysy.y"
                              {
    auto dims = static_cast<ArrayDimensionsAST*>((yyvsp[-3].ast_val));
    dims->dims.push_back(unique_ptr<ValueAST>((yyvsp[-1].value_ast)));
    (yyval.ast_val) = dims;
  }
#line 2246 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 69: /* lval_indexes: '[' Exp ']'  */
#line 502 "/root/compiler/src/sysy.y"
                {
    auto dims = new ArrayDimensionsAST();
    dims->dims.push_back(unique_ptr<ValueAST>((yyvsp[-1].value_ast)));
    (yyval.ast_val) = dims;
  }
#line 2256 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 70: /* lval_indexes: lval_indexes '[' Exp ']'  */
#line 507 "/root/compiler/src/sysy.y"
                             {
    auto dims = static_cast<ArrayDimensionsAST*>((yyvsp[-3].ast_val));
    dims->dims.push_back(unique_ptr<ValueAST>((yyvsp[-1].value_ast)));
    (yyval.ast_val) = dims;
  }
#line 2266 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 71: /* ConstInitVal: ConstExp  */
#line 515 "/root/compiler/src/sysy.y"
             { 
    auto constinit_val = new ConstExpConstInitValAST(); 
    constinit_val->const_exp = unique_ptr<ValueAST>((yyvsp[0].value_ast)); 
    (yyval.value_ast) = constinit_val;
  }
#line 2276 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 72: /* ConstInitVal: '{' '}'  */
#line 520 "/root/compiler/src/sysy.y"
            { 
    auto constinit_val = new CompoundConstInitValueAST(); 
    constinit_val->const_init_val_list = nullptr;
    (yyval.value_ast) = constinit_val; 
  }
#line 2286 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 73: /* ConstInitVal: '{' const_init_val_list '}'  */
#line 525 "/root/compiler/src/sysy.y"
                                {
    auto constinit_val = new CompoundConstInitValueAST();
    constinit_val->const_init_val_list = unique_ptr<ConstInitValListAST>(static_cast<ConstInitValListAST*>((yyvsp[-1].ast_val)));
    (yyval.value_ast) = constinit_val;
  }
#line 2296 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 74: /* const_init_val_list: ConstInitVal  */
#line 533 "/root/compiler/src/sysy.y"
                 {
    auto list = new ConstInitValListAST();
    list->const_init_vals.push_back(unique_ptr<ValueAST>(static_cast<ValueAST*>((yyvsp[0].value_ast))));
    (yyval.ast_val) = list;
  }
#line 2306 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 75: /* const_init_val_list: const_init_val_list ',' ConstInitVal  */
#line 538 "/root/compiler/src/sysy.y"
                                         {
    auto list = static_cast<ConstInitValListAST*>((yyvsp[-2].ast_val));
    list->const_init_vals.push_back(unique_ptr<ValueAST>(static_cast<ValueAST*>((yyvsp[0].value_ast))));
    (yyval.ast_val) = list;
  }
#line 2316 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 76: /* VarDecl: Type var_def_list ';'  */
#line 546 "/root/compiler/src/sysy.y"
                          { 
    assert(*(yyvsp[-2].str_val) != "void");
    auto var_decl = new VarDeclAST();
    var_decl->type = unique_ptr<string>((yyvsp[-2].str_val));
    var_decl->var_def_list = unique_ptr<BaseAST>((yyvsp[-1].ast_val));
    (yyval.ast_val) = var_decl;
  }
#line 2328 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 77: /* var_def_list: VarDef  */
#line 556 "/root/compiler/src/sysy.y"
           {
    auto var_def_list_ast = new VarDefListAST();
    var_def_list_ast->var_defs = vector<unique_ptr<BaseAST>>();
    var_def_list_ast->var_defs.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = var_def_list_ast;
  }
#line 2339 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 78: /* var_def_list: var_def_list ',' VarDef  */
#line 562 "/root/compiler/src/sysy.y"
                            {
    auto var_def_list_ast = (static_cast<VarDefListAST*>((yyvsp[-2].ast_val)));
    var_def_list_ast->var_defs.push_back(unique_ptr<BaseAST>((yyvsp[0].ast_val)));
    (yyval.ast_val) = var_def_list_ast;
  }
#line 2349 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 79: /* VarDef: IDENT  */
#line 570 "/root/compiler/src/sysy.y"
          { 
    auto var_def = new SimpleVarDefAST();
    var_def->ident = unique_ptr<string>((yyvsp[0].str_val));
    (yyval.ast_val) = var_def;
  }
#line 2359 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 80: /* VarDef: IDENT arr_dims  */
#line 575 "/root/compiler/src/sysy.y"
                   { 
    auto var_def = new ArrayVarDefAST();
    var_def->ident = unique_ptr<string>((yyvsp[-1].str_val));
    auto dims = unique_ptr<ArrayDimensionsAST>(static_cast<ArrayDimensionsAST*>((yyvsp[0].ast_val)));
    var_def->const_exps = std::move(dims->dims);
    (yyval.ast_val) = var_def;
  }
#line 2371 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 81: /* VarDef: IDENT ASSIGN InitVal  */
#line 582 "/root/compiler/src/sysy.y"
                         { 
    auto var_def = new InitializedVarDefAST();
    var_def->ident = unique_ptr<string>((yyvsp[-2].str_val));
    var_def->init_val = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.ast_val) = var_def;
  }
#line 2382 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 82: /* VarDef: IDENT arr_dims ASSIGN InitVal  */
#line 588 "/root/compiler/src/sysy.y"
                                  { 
    auto var_def = new ArrayInitializedVarDefAST();
    var_def->ident = unique_ptr<string>((yyvsp[-3].str_val));
    auto dims = unique_ptr<ArrayDimensionsAST>(static_cast<ArrayDimensionsAST*>((yyvsp[-2].ast_val)));
    var_def->const_exps = std::move(dims->dims);
    var_def->init_val_list = unique_ptr<InitValListAST>(static_cast<InitValListAST*>((yyvsp[0].value_ast)));
    (yyval.ast_val) = var_def;
  }
#line 2395 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 83: /* InitVal: Exp  */
#line 599 "/root/compiler/src/sysy.y"
        { 
    auto init_val = new InitValAST();
    init_val->exp = unique_ptr<ValueAST>((yyvsp[0].value_ast));
    (yyval.value_ast) = init_val;
  }
#line 2405 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 84: /* InitVal: '{' '}'  */
#line 604 "/root/compiler/src/sysy.y"
            { 
    auto init_val = new InitValListAST();
    (yyval.value_ast) = init_val;
  }
#line 2414 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 85: /* InitVal: '{' init_val_list '}'  */
#line 608 "/root/compiler/src/sysy.y"
                          { 
    (yyval.value_ast) = static_cast<InitValListAST*>((yyvsp[-1].ast_val));
  }
#line 2422 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 86: /* init_val_list: InitVal  */
#line 614 "/root/compiler/src/sysy.y"
            {
    auto list = new InitValListAST();
    list->init_vals.push_back(unique_ptr<ValueAST>(static_cast<ValueAST*>((yyvsp[0].value_ast))));
    (yyval.ast_val) = list;
  }
#line 2432 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 87: /* init_val_list: init_val_list ',' InitVal  */
#line 619 "/root/compiler/src/sysy.y"
                              {
    auto list = static_cast<InitValListAST*>((yyvsp[-2].ast_val));
    list->init_vals.push_back(unique_ptr<ValueAST>(static_cast<ValueAST*>((yyvsp[0].value_ast))));
    (yyval.ast_val) = list;
  }
#line 2442 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 88: /* LVal: IDENT  */
#line 627 "/root/compiler/src/sysy.y"
          { 
    auto lval_ast = new LValAST(); 
    lval_ast->ident = unique_ptr<string>((yyvsp[0].str_val));
    (yyval.value_ast) = lval_ast; 
  }
#line 2452 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 89: /* LVal: IDENT lval_indexes  */
#line 632 "/root/compiler/src/sysy.y"
                       { 
    auto lval_ast = new LValAST(); 
    lval_ast->ident = unique_ptr<string>((yyvsp[-1].str_val));
    auto dims = unique_ptr<ArrayDimensionsAST>(static_cast<ArrayDimensionsAST*>((yyvsp[0].ast_val)));
    lval_ast->index_exp = std::move(dims->dims);
    (yyval.value_ast) = lval_ast; 
  }
#line 2464 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 90: /* ConstExp: Exp  */
#line 642 "/root/compiler/src/sysy.y"
        { auto const_exp = new ConstExpAST(); const_exp->exp = unique_ptr<ValueAST>((yyvsp[0].value_ast)); (yyval.value_ast) = const_exp; }
#line 2470 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 91: /* UnaryOp: AddOp  */
#line 646 "/root/compiler/src/sysy.y"
          { (yyval.op_ast) = (yyvsp[0].op_ast); }
#line 2476 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 92: /* UnaryOp: NOT  */
#line 647 "/root/compiler/src/sysy.y"
          { auto op_ast = new OpAST(); op_ast->op = "!"; (yyval.op_ast) = op_ast; }
#line 2482 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 93: /* AddOp: ADD  */
#line 651 "/root/compiler/src/sysy.y"
        { auto op_ast = new OpAST(); op_ast->op = "+"; (yyval.op_ast) = op_ast; }
#line 2488 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 94: /* AddOp: SUB  */
#line 652 "/root/compiler/src/sysy.y"
        { auto op_ast = new OpAST(); op_ast->op = "-"; (yyval.op_ast) = op_ast; }
#line 2494 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 95: /* MulOp: MUL  */
#line 656 "/root/compiler/src/sysy.y"
        { auto op_ast = new OpAST(); op_ast->op = "*"; (yyval.op_ast) = op_ast; }
#line 2500 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 96: /* MulOp: DIV  */
#line 657 "/root/compiler/src/sysy.y"
        { auto op_ast = new OpAST(); op_ast->op = "/"; (yyval.op_ast) = op_ast; }
#line 2506 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 97: /* MulOp: MOD  */
#line 658 "/root/compiler/src/sysy.y"
        { auto op_ast = new OpAST(); op_ast->op = "%"; (yyval.op_ast) = op_ast; }
#line 2512 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 98: /* ComOp: LE  */
#line 662 "/root/compiler/src/sysy.y"
       { auto op_ast = new OpAST(); op_ast->op = "<="; (yyval.op_ast) = op_ast; }
#line 2518 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 99: /* ComOp: LT  */
#line 663 "/root/compiler/src/sysy.y"
       { auto op_ast = new OpAST(); op_ast->op = "<"; (yyval.op_ast) = op_ast; }
#line 2524 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 100: /* ComOp: GE  */
#line 664 "/root/compiler/src/sysy.y"
       { auto op_ast = new OpAST(); op_ast->op = ">="; (yyval.op_ast) = op_ast; }
#line 2530 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 101: /* ComOp: GT  */
#line 665 "/root/compiler/src/sysy.y"
       { auto op_ast = new OpAST(); op_ast->op = ">"; (yyval.op_ast) = op_ast; }
#line 2536 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 102: /* EquOp: EQ  */
#line 669 "/root/compiler/src/sysy.y"
       { auto op_ast = new OpAST(); op_ast->op = "=="; (yyval.op_ast) = op_ast; }
#line 2542 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 103: /* EquOp: NE  */
#line 670 "/root/compiler/src/sysy.y"
       { auto op_ast = new OpAST(); op_ast->op = "!="; (yyval.op_ast) = op_ast; }
#line 2548 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 104: /* LOrOp: L_OR  */
#line 674 "/root/compiler/src/sysy.y"
          { auto op_ast = new OpAST(); op_ast->op = "||"; (yyval.op_ast) = op_ast; }
#line 2554 "/root/compiler/src/sysy.tab.cpp"
    break;

  case 105: /* LAndOp: L_AND  */
#line 678 "/root/compiler/src/sysy.y"
          { auto op_ast = new OpAST(); op_ast->op = "&&"; (yyval.op_ast) = op_ast; }
#line 2560 "/root/compiler/src/sysy.tab.cpp"
    break;


#line 2564 "/root/compiler/src/sysy.tab.cpp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (ast, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, ast);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, ast);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (ast, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, ast);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, ast);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 680 "/root/compiler/src/sysy.y"


void yyerror(std::unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "Parser Error ad line: " << yylineno << ":" << s << endl;
}
