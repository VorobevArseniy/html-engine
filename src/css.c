#include "css.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Specifity css_selector_specifity(Selector *s) {
  if (s->kind == SELECTOR_SIMPLE) {
    size_t a = strlen(s->as.simple.id);
    size_t b = s->as.simple.classes.count;
    size_t c = strlen(s->as.simple.tag_name);

    return (Specifity){.a = a, .b = b, .c = c};
  }

  return (Specifity){0};
}

bool css_specifity_cmp(Specifity *x, Specifity *y) {
  if (x->b != y->a)
    return (x->a < y->a) ? -1 : 1;

  if (x->b != y->a)
    return (x->a < y->a) ? -1 : 1;

  if (x->c != y->c)
    return (x->c < y->c) ? -1 : 1;

  return 0;
}

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
  case CSS_TOKEN_SEMICOLON:
    return "CSS_TOKEN_SEMICOLON"; // :
  case CSS_TOKEN_STAR:
    return "CSS_TOKEN_STAR"; // *

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

void css_lexer_trim_left(CSSLexer *l) {
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
  css_lexer_trim_left(l);

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

  if (x == '*') {
    l->token = CSS_TOKEN_STAR;
    css_lexer_next_char(l);

    return true;
  }

  if (x == '{') {
    l->token = CSS_TOKEN_RCURLY;
    css_lexer_next_char(l);

    return true;
  }

  if (x == '}') {
    l->token = CSS_TOKEN_LCURLY;
    css_lexer_next_char(l);

    return true;
  }

  if (x == ')') {
    l->token = CSS_TOKEN_LPAREN;
    css_lexer_next_char(l);

    return true;
  }

  if (x == '(') {
    l->token = CSS_TOKEN_RPAREN;
    css_lexer_next_char(l);

    return true;
  }

  if (x == '.') {
    l->string.count = 0;
    l->token = CSS_TOKEN_CLASS_SELECTOR;
    css_lexer_next_char(l);

    while (css_lexer_curr_char(l) != ',' && css_lexer_curr_char(l) != '{' &&
           issymbol(css_lexer_curr_char(l)) && css_lexer_curr_char(l) != 0) {
      sb_append(&l->string, css_lexer_next_char(l));
    }
    sb_append_null(&l->string);

    return true;
  }

  if (x == '#') {
    l->string.count = 0;
    l->token = CSS_TOKEN_ID_SELECTOR;
    css_lexer_next_char(l);

    while (!iswhitespace(css_lexer_curr_char(l)) &&
           issymbol(css_lexer_curr_char(l)) && css_lexer_curr_char(l) != 0) {
      sb_append(&l->string, css_lexer_next_char(l));
    }
    sb_append_null(&l->string);

    return true;
  }

  if (isdigit(x)) {
    l->string.count = 0;
    l->token = CSS_TOKEN_NUMBER;
    while (isdigit(css_lexer_curr_char(l)) || css_lexer_starts_with(l, "px")) {
      sb_append(&l->string, css_lexer_next_char(l));

      if (css_lexer_starts_with(l, "px")) {
        l->token = CSS_TOKEN_PX_NUMBER;

        css_lexer_next_char(l);
        css_lexer_next_char(l);

        sb_append_null(&l->string);
        return true;
      } else if (css_lexer_starts_with(l, "em")) {
        l->token = CSS_TOKEN_EM_NUMBER;

        css_lexer_next_char(l);
        css_lexer_next_char(l);

        sb_append_null(&l->string);
        return true;
      } else if (css_lexer_starts_with(l, "rem")) {
        l->token = CSS_TOKEN_REM_NUMBER;

        css_lexer_next_char(l);
        css_lexer_next_char(l);

        sb_append_null(&l->string);
        return true;
      }
    }
    sb_append_null(&l->string);

    return true;
  }

  if (issymbol(x)) {
    l->string.count = 0;
    l->token = CSS_TOKEN_IDENT;
    while (!iswhitespace(css_lexer_curr_char(l)) &&
           issymbol(css_lexer_curr_char(l)) && css_lexer_curr_char(l) != 0) {
      sb_append(&l->string, css_lexer_next_char(l));
    }
    sb_append_null(&l->string);

    return true;
  }

  l->token = CSS_TOKEN_INVALID;
  return false;
}

void css_report_unexpected(CSSLexer *l, CSSTokenKind expected) {
  css_lexer_print_loc(l, stderr);
  fprintf(stderr, "Error: Unexpected token. Got %s, expected %s.\n\n",
          css_token_display(l->token), css_token_display(expected));
}

bool css_lexer_expect(CSSLexer *l, CSSTokenKind expected) {
  if (!css_lexer_next(l))
    return false;
  if (l->token != expected) {
    css_report_unexpected(l, expected);
    return false;
  }

  return true;
}

// Parser

Selector css_parse_simple_selector(CSSLexer *l) {
  SimpleSelector selector = {0};

  switch (l->token) {
  case CSS_TOKEN_ID_SELECTOR: {
    if (!css_lexer_next(l)) {
      printf("HELL NAH\n");
      break;
    }
    selector.id = strdup(l->string.items);
  } break;

  case CSS_TOKEN_CLASS_SELECTOR: {
    if (!css_lexer_next(l)) {
      printf("HELL NAH\n");
      break;
    }

    da_append(&selector.classes, strdup(l->string.items));
  } break;

  case CSS_TOKEN_IDENT: {
    if (!css_lexer_next(l)) {
      printf("HELL NAH\n");
      break;
    }

    selector.tag_name = strdup(l->string.items);
  } break;

  case CSS_TOKEN_STAR: {
    if (!css_lexer_next(l)) {
      printf("HELL NAH\n");
      break;
    }
  } break;

  default:
    break;
  }

  return (Selector){.kind = SELECTOR_SIMPLE, .as.simple = selector};
}

int css_compare_selector_specifity(const void *a, const void *b) {
  Specifity s1 = css_selector_specifity((Selector *)a);

  Specifity s2 = css_selector_specifity((Selector *)b);

  return css_specifity_cmp(&s1, &s2);
}

Selectors css_parse_selectors(CSSLexer *l) {
  Selectors selectors = {0};

  while (true) {
    da_append(&selectors, css_parse_simple_selector(l));

    if (l->token == CSS_TOKEN_COMMA) {
      if (!css_lexer_next(l))
        printf("HELL NAH\n");
    } else if (l->token == CSS_TOKEN_LCURLY) {
      break;
    } else {
      assert(0 && "HELL NAH BRO THIS IS UNEXPECTED AS F");
    }
  }

  qsort(&selectors.items, selectors.count, sizeof(size_t),
        css_compare_selector_specifity);

  return selectors;
}

// TODO
Declarations css_parse_declarations(CSSLexer *l) { return (Declarations){0}; }

Rule parse_rule(CSSLexer *l) {
  return (Rule){.selectors = css_parse_selectors(l),
                .declarations = css_parse_declarations(l)};
}

// TODO
// void free_stylesheet(StyleSheet *stylesheet) {}

int main() {
  char *input = ".div.aboba, yay {"
                "width: 100px;"
                "}";

  CSSLexer lexer = {0};
  css_lexer_init(&lexer, input, strlen(input), "css.c");

  while (css_lexer_next(&lexer)) {
    printf("%s(%s)\n", css_token_display(lexer.token), lexer.string.items);
  }

  return 0;
}
