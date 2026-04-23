#include <stdlib.h>

#define NOB_IMPLEMENTATION
#include "../thirdparty/nob.h"
#define NOB_UNSTRIP_PREFIX

typedef struct {
  size_t pos, bol, row;
} Location;

typedef enum {
  CSS_TOKEN_RCURLY,    // {
  CSS_TOKEN_LCURLY,    // }
  CSS_TOKEN_LPAREN,    // (
  CSS_TOKEN_RPAREN,    // )
  CSS_TOKEN_MINUS,     // -
  CSS_TOKEN_PLUS,      // +
  CSS_TOKEN_COMMA,     // ,
  CSS_TOKEN_COLON,     // :
  CSS_TOKEN_SEMICOLON, // :

  CSS_TOKEN_NUMBER,     // e.g. 42
  CSS_TOKEN_PX_NUMBER,  // e.g. 42px
  CSS_TOKEN_EM_NUMBER,  // e.g. 42em
  CSS_TOKEN_REM_NUMBER, // e.g. 42rem
                        //
  CSS_TOKEN_IDENT,
  CSS_TOKEN_CLASS_SELECTOR, // e.g. .foo
  CSS_TOKEN_ID_SELECTOR,    // e.g. #foo

  CSS_TOKEN_WHITESPACE,
  CSS_TOKEN_EOF,
  CSS_TOKEN_INVALID,
} CSSTokenKind;

typedef struct {
  const char *content;
  size_t count;
  const char *file_path;

  Location cur;

  CSSTokenKind token;
  String_Builder string;
  size_t row, col;
} CSSLexer;

typedef enum {
  VALUE_KEYWORD,
  VALUE_LENGTH,
  VALUE_COLOR,
} ValueKind;

typedef enum { PX, EM, REM } Unit;

typedef struct {
  float value;
  Unit unit;
} ValueLength;

typedef struct {
  int r;
  int g;
  int b;
  int a;
} Color;

typedef struct {
  ValueKind kind;
  union {
    char *keyword;
    ValueLength length;
    Color color;
  } as;
} Value;

typedef struct {
  char *name;
  Value value;
} Declaration;

typedef struct {
  Declaration *items;
  size_t count;
  size_t capacity;
} Declarations;

typedef enum {
  SELECTOR_SIMPLE,
} SelectorKind;

typedef struct {
  char **items;
  size_t count;
  size_t capacity;
} ClassList;

typedef struct {
  char *tag_name;
  char *id;
  ClassList classes;
} SimpleSelector;

typedef struct {
  SelectorKind kind;
  union {
    SimpleSelector simple;
  } as;
} Selector;

typedef struct {
  Selector *items;
  size_t count;
  size_t capacity;
} Selectors;

typedef struct {
  Selectors selectors;
  Declarations declarations;
} Rule;

typedef struct {
  Rule *items;
  size_t count;
  size_t capacity;
} Rules;

typedef struct {
  Rules rule;
} StyleSheet;
