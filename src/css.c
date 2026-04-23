#include <stdio.h>
#include <stdlib.h>

#include "css.h"

const char *css_token_display(CSSTokenKind kind) {
  switch (kind) {
  case CSS_TOKEN_RCURLY:
    return "CSS_TOKEN_RCURLY"; // {
  case CSS_TOKEN_LCURLY:
    return "CSS_TOKEN_LCURLY"; // }
  case CSS_TOKEN_LPAREN:
    return "CSS_TOKEN_LPAREN"; // (
  case CSS_TOKEN_RPAREN:
    return "CSS_TOKEN_RPAREN"; // )
  case CSS_TOKEN_MINUS:
    return "CSS_TOKEN_MINUS"; // -
  case CSS_TOKEN_PLUS:
    return "CSS_TOKEN_PLUS"; // +
  case CSS_TOKEN_COMMA:
    return "CSS_TOKEN_COMMA"; // ,
  case CSS_TOKEN_COLON:
    return "CSS_TOKEN_COLON"; // :
                              //
  case CSS_TOKEN_SEMICOLON:
    return "CSS_TOKEN_SEMICOLON"; // :

  case CSS_TOKEN_NUMBER:
    return "CSS_TOKEN_NUMBER"; // e.g. 42
  case CSS_TOKEN_PX_NUMBER:
    return "CSS_TOKEN_PX_NUMBER"; // e.g. 42px
  case CSS_TOKEN_EM_NUMBER:
    return "CSS_TOKEN_EM_NUMBER"; // e.g. 42em
  case CSS_TOKEN_REM_NUMBER:
    return "CSS_TOKEN_REM_NUMBER"; // e.g. 42rem

  case CSS_TOKEN_IDENT:
    return "CSS_TOKEN_IDENT";
  case CSS_TOKEN_CLASS_SELECTOR:
    return "CSS_TOKEN_CLASS_SELECTOR"; // e.g. .foo
  case CSS_TOKEN_ID_SELECTOR:
    return "CSS_TOKEN_ID_SELECTOR"; // e.g. #foo

  case CSS_TOKEN_WHITESPACE:
    return "CSS_TOKEN_WHITESPACE";
  case CSS_TOKEN_EOF:
    return "CSS_TOKEN_EOF";
  case CSS_TOKEN_INVALID:
    return "CSS_TOKEN_INVALID";
  }
}

void css_lexer_init(CSSLexer *l, const char *content, size_t count,
                    const char *file_path) {
  l->content = content;
  l->count = count;
  l->file_path = file_path;
  memset(&l->cur, 0, sizeof(l->cur));
}

void css_lexer_print_loc(CSSLexer *l, FILE *stream) {
  if (l->file_path)
    fprintf(stream, "%s:", l->file_path);
  fprintf(stream, "%zu:%zu", l->row, l->col);
}

char css_lexer_curr_char(CSSLexer *l) {
  if (l->cur.pos >= l->count)
    return 0;

  return l->content[l->cur.pos];
}

char css_lexer_peek_char(CSSLexer *l) {
  if (l->cur.pos >= l->count)
    return 0;

  char x = l->content[l->cur.pos + 1];

  return x;
}

char css_lexer_next_char(CSSLexer *l) {

  if (l->cur.pos >= l->count)
    return 0;

  char x = l->content[l->cur.pos++];
  if (x == '\n') {
    l->cur.row += 1;
    l->cur.bol = l->cur.pos;
  }

  return x;
}

void lexer_trim_left(CSSLexer *l) {
  while (isspace(css_lexer_curr_char(l))) {
    css_lexer_next_char(l);
  }
}

bool css_lexer_starts_with(CSSLexer *l, const char *prefix) {
  size_t pos = l->cur.pos;
  while (pos < l->count && *prefix != '\0' && *prefix == l->content[pos]) {
    pos++;
    prefix++;
  }

  return *prefix == '\0';
}

void lexer_drop_line(CSSLexer *l) {
  while (l->cur.pos < l->count && css_lexer_next_char(l) != '\n') {
  }
}

bool issymbol(int x) { return isalpha(x) || x == '-' || x == '_'; }

bool iswhitespace(int x) { return x == ' ' || x == '\n' || x == '\r'; }

bool css_lexer_next(CSSLexer *l) {
  lexer_trim_left(l);

  l->row = l->cur.row + 1;
  l->col = l->cur.pos - l->cur.bol + 1;

  char x = css_lexer_curr_char(l);

  if (x == 0) {
    l->token = CSS_TOKEN_EOF;

    return false;
  }
  if (iswhitespace(x)) {
    l->token = CSS_TOKEN_WHITESPACE;
    css_lexer_next_char(l);

    return true;
  }

  if (x == ',') {
    l->token = CSS_TOKEN_COMMA;
    css_lexer_next_char(l);

    return true;
  }

  if (x == '+') {
    l->token = CSS_TOKEN_PLUS;
    css_lexer_next_char(l);

    return true;
  }

  if (x == '-') {
    l->token = CSS_TOKEN_MINUS;
    css_lexer_next_char(l);

    return true;
  }

  if (x == ':') {
    l->token = CSS_TOKEN_COLON;
    css_lexer_next_char(l);

    return true;
  }

  if (x == ';') {
    l->token = CSS_TOKEN_SEMICOLON;
    css_lexer_next_char(l);

    return true;
  }

  if (x == '{') {
    l->token = CSS_TOKEN_RCURLY;
    css_lexer_next_char(l);

    return true;
  }

  if (x == '{') {
    l->token = CSS_TOKEN_LCURLY;
    css_lexer_next_char(l);

    return true;
  }

  if (x == '.') {
    l->string.count = 0;
    l->token = CSS_TOKEN_CLASS_SELECTOR;
    css_lexer_next_char(l); // skip dot

    while (css_lexer_curr_char(l) != ',' && css_lexer_curr_char(l) != '{' &&
           css_lexer_curr_char(l) != 0) {
      sb_append(&l->string, css_lexer_next_char(l));
    }
    sb_append_null(&l->string);

    return true;
  }

  if (x == '#') {
    l->string.count = 0;
    l->token = CSS_TOKEN_ID_SELECTOR;
    css_lexer_next_char(l); // skip sharp

    while (!iswhitespace(css_lexer_curr_char(l)) &&
           css_lexer_curr_char(l) != 0) {
      sb_append(&l->string, css_lexer_next_char(l));
    }
    sb_append_null(&l->string);

    return true;
  }

  if (isnumber(x)) {
    l->string.count = 0;
    l->token = CSS_TOKEN_NUMBER;
    while (isnumber(css_lexer_curr_char(l))) {
      sb_append(&l->string, css_lexer_next_char(l));
    }
    sb_append_null(&l->string);

    return true;
  }

  if (issymbol(x)) {
    l->string.count = 0;
    l->token = CSS_TOKEN_IDENT;
    while (!iswhitespace(css_lexer_curr_char(l)) &&
           css_lexer_curr_char(l) != 0) {
      sb_append(&l->string, css_lexer_next_char(l));
    }
    sb_append_null(&l->string);

    return true;
  }

  l->token = CSS_TOKEN_INVALID;
  return false;
}

int main() {
  char *input = ".aboba {yay: 123;}";

  CSSLexer lexer = {0};
  css_lexer_init(&lexer, input, strlen(input), "css.c");

  while (css_lexer_next(&lexer)) {
    printf("%s(%s)\n", css_token_display(lexer.token), lexer.string.items);
  }

  return 0;
}
