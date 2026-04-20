#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOB_IMPLEMENTATION
#include "../thirdparty/nob.h"
#define NOB_UNSTRIP_PREFIX

#define HT_IMPLEMENTATION
#include "../thirdparty/ht.h"

// dom -----------------------------------------

typedef Ht(String_View, String_View) ElementAttributes;

typedef struct Node Node;
typedef struct Element Element;

typedef enum NodeKind { NODE_TEXT, NODE_ELEMENT } NodeKind;

typedef struct {
  Node **items;
  size_t count;
  size_t capacity;
} Nodes;

struct Element {
  char *name;
  ElementAttributes attrs;
};

struct Node {
  Node *parent;
  Nodes *children;

  NodeKind kind;
  union {
    char *text;
    Element *element;
  } as;
};

Node *node_create_text(char *data) {
  Node *node = malloc(sizeof(Node));

  *node = (Node){.parent = NULL,
                 .children = NULL,
                 .kind = NODE_TEXT,
                 .as.text = strdup(data)};
  assert(node);

  return node;
}

Node *node_create_element(char *name) {
  Element *elem = malloc(sizeof(Element));
  elem->name = strdup(name);
  elem->attrs = (ElementAttributes){.hasheq = ht_sv_hasheq};
  assert(elem);

  Node *node = malloc(sizeof(Node));
  *node = (Node){.parent = NULL,
                 .children = NULL,
                 .kind = NODE_ELEMENT,
                 .as.element = elem};
  assert(node);

  return node;
}

void node_set_attribute(Node *node, const char *key, const char *value) {
  if (node->kind != NODE_ELEMENT)
    return;

  *ht_put(&node->as.element->attrs, sv_from_cstr(key)) = sv_from_cstr(value);
}

void node_append_child(Node *parent, Node *child) {
  if (parent->kind != NODE_ELEMENT)
    return;

  if (!parent->children) {
    parent->children = malloc(sizeof(Nodes));
    *parent->children = (Nodes){0};
  }

  da_append(parent->children, child);

  child->parent = parent;
}

void node_free(Node *node) {
  if (!node)
    return;

  if (node->children) {
    for (size_t i = 0; i < node->children->count; i++) {
      node_free(node->children->items[i]);
    }
    free(node->children->items);
    free(node->children);
  }

  switch (node->kind) {
  case NODE_TEXT:
    free(node->as.text);
    break;
  case NODE_ELEMENT:
    if (node->as.element) {
      free(node->as.element->name);
      ht_free(&node->as.element->attrs);
      free(node->as.element);
    }
  }

  free(node);
}

#define dump_node(node)                                                        \
  dump_node_((node), 0);                                                       \
  printf("\n");

void dump_node_(Node *node, size_t level) {
  for (size_t i = 0; i <= level; i++) {
    printf("  ");
  }

  switch (node->kind) {
  case NODE_TEXT:
    printf("%s", node->as.text);
    break;
  case NODE_ELEMENT:
    printf("<%s", node->as.element->name);
    ht_foreach(attr, &node->as.element->attrs) {
      printf(" " SV_Fmt "=\"" SV_Fmt "\"",
             SV_Arg(ht_key(&node->as.element->attrs, attr)), SV_Arg(*attr));
    }
    printf(">");

    if (node->children) {
      level += 1;
      for (size_t i = 0; i < node->children->count; i++) {
        printf("\n");
        dump_node_(node->children->items[i], level);
      }
    }
    printf("\n");
    for (size_t i = 1; i <= level; i++) {
      printf("  ");
    }

    printf("</%s>", node->as.element->name);

    break;
  }
}

// lexer ---------------------------------------

typedef enum {
  TOKEN_DOCTYPE,
  TOKEN_TAG_OPEN,       // <
  TOKEN_TAG_CLOSE,      // >
  TOKEN_TAG_SELF_CLOSE, // />
  TOKEN_END_TAG_OPEN,   // </

  TOKEN_TEXT,
  TOKEN_IDENT,
  TOKEN_EQUAL,
  TOKEN_STRING,

  TOKEN_WHITESPACE,
  TOKEN_EOF,
  TOKEN_INVALID,
} TokenKind;

const char *token_kind_display(TokenKind kind) {
  switch (kind) {
  case TOKEN_DOCTYPE:
    return "TOKEN_DOCTYPE";
  case TOKEN_TAG_OPEN:
    return "TOKEN_TAG_OPEN";
  case TOKEN_TAG_CLOSE:
    return "TOKEN_TAG_CLOSE";
  case TOKEN_TAG_SELF_CLOSE:
    return "TOKEN_TAG_SELF_CLOSE";
  case TOKEN_END_TAG_OPEN:
    return "TOKEN_END_TAG_OPEN";
  case TOKEN_TEXT:
    return "TOKEN_TEXT";
  case TOKEN_IDENT:
    return "TOKEN_IDENT";
  case TOKEN_EQUAL:
    return "TOKEN_EQUAL";
  case TOKEN_STRING:
    return "TOKEN_STRING";
  case TOKEN_WHITESPACE:
    return "TOKEN_WHITESPACE";
  case TOKEN_EOF:
    return "TOKEN_EOF";
  case TOKEN_INVALID:
    return "TOKEN_INVALID";
  default:
    UNREACHABLE("TokenKind");
  }
}

typedef struct {
  size_t pos, bol, row;
} Location;

typedef struct {
  const char *content;
  size_t count;
  const char *file_path;

  bool inside_tag;

  Location cur;

  TokenKind token;
  String_Builder string;
  size_t row, col;
} Lexer;

void lexer_init(Lexer *l, const char *content, size_t count,
                const char *file_path) {
  l->content = content;
  l->count = count;
  l->file_path = file_path;
  memset(&l->cur, 0, sizeof(l->cur));
}

static const char *VOID_TAGS[] = {"area",  "base",   "br",    "col",  "embed",
                                  "hr",    "img",    "input", "link", "meta",
                                  "param", "source", "track", "wbr"};

#define NUM_VOID_TAGS (sizeof(VOID_TAGS) / sizeof(VOID_TAGS[0]))

int compare_tag(const void *a, const void *b) {
  return strcmp((const char *)a, *(const char **)b);
}

bool is_void_tag(const char *tag) {
  return bsearch(tag, VOID_TAGS, NUM_VOID_TAGS, sizeof(char *), compare_tag) !=
         NULL;
}

void lexer_print_loc(Lexer *l, FILE *stream) {
  if (l->file_path)
    fprintf(stream, "%s:", l->file_path);
  fprintf(stream, "%zu:%zu", l->row, l->col);
}

char lexer_curr_char(Lexer *l) {
  if (l->cur.pos >= l->count)
    return 0;
  return l->content[l->cur.pos];
}

char lexer_peek_char(Lexer *l) {
  if (l->cur.pos >= l->count)
    return 0;

  char x = l->content[l->cur.pos + 1];

  return x;
}

char lexer_next_char(Lexer *l) {

  if (l->cur.pos >= l->count)
    return 0;

  char x = l->content[l->cur.pos++];
  if (x == '\n') {
    l->cur.row += 1;
    l->cur.bol = l->cur.pos;
  }

  return x;
}

void lexer_trim_left(Lexer *l) {
  while (isspace(lexer_curr_char(l))) {
    lexer_next_char(l);
  }
}

bool lexer_starts_with(Lexer *l, const char *prefix) {
  size_t pos = l->cur.pos;
  while (pos < l->count && *prefix != '\0' && *prefix == l->content[pos]) {
    pos++;
    prefix++;
  }
  return *prefix == '\0';
}

void lexer_drop_line(Lexer *l) {
  while (l->cur.pos < l->count && lexer_next_char(l) != '\n') {
  }
}

bool issymbol(int x) { return isalnum(x) || x == '-' || x == '_' || x == ':'; }

bool lexer_next(Lexer *l) {
  lexer_trim_left(l);

  l->row = l->cur.row + 1;
  l->col = l->cur.pos - l->cur.bol + 1;

  char x = lexer_curr_char(l);

  if (x == 0) {
    l->token = TOKEN_EOF;

    return false;
  }

  if (x == ' ' || x == '\n' || x == '\r') {
    l->token = TOKEN_WHITESPACE;
    lexer_next_char(l);

    return true;
  }

  if (x == '<') {
    lexer_next_char(l);
    if (lexer_curr_char(l) == '/') {
      l->token = TOKEN_END_TAG_OPEN;
      lexer_next_char(l);
    } else {
      l->token = TOKEN_TAG_OPEN;
    }

    l->inside_tag = true;
    return true;
  }

  if (x == '=') {
    l->token = TOKEN_EQUAL;
    lexer_next_char(l);

    return true;
  }

  if (x == '>') {
    l->token = TOKEN_TAG_CLOSE;
    lexer_next_char(l);

    l->inside_tag = false;

    return true;
  }

  if (x == '/' && lexer_peek_char(l) == '>') {
    l->token = TOKEN_TAG_SELF_CLOSE;
    lexer_next_char(l); // consume >
    lexer_next_char(l); // consume /

    l->inside_tag = false;
    return true;
  }

  if (x == '"' || x == '\'') {
    l->string.count = 0;

    l->token = TOKEN_STRING;
    char quote = x;
    lexer_next_char(l);

    while (lexer_curr_char(l) != quote && lexer_curr_char(l) != 0) {
      sb_append(&l->string, lexer_next_char(l));
    }

    if (lexer_curr_char(l) == quote) {
      lexer_next_char(l);
    }
    sb_append_null(&l->string);

    return true;
  }

  if (issymbol(x) && l->inside_tag) {
    l->string.count = 0;
    l->token = TOKEN_IDENT;

    while (issymbol(lexer_curr_char(l))) {
      da_append(&l->string, lexer_next_char(l));
    }
    sb_append_null(&l->string);

    return true;
  }

  l->token = TOKEN_TEXT;
  l->string.count = 0;
  while (lexer_curr_char(l) != '<' && lexer_curr_char(l) != 0) {
    da_append(&l->string, lexer_next_char(l));
  }
  sb_append_null(&l->string);

  return true;
}

bool lexer_peek(Lexer *l) {
  Location cur = l->cur;
  bool result = lexer_next_char(l);
  l->cur = cur;
  return result;
}

void report_unexpected(Lexer *l, TokenKind expected) {
  lexer_print_loc(l, stderr);
  fprintf(stderr, "Error: Unexpected token. Got %s, expected %s.\n\n",
          token_kind_display(l->token), token_kind_display(expected));
}

bool lexer_expect(Lexer *l, TokenKind expected) {
  if (!lexer_next(l))
    return false;
  if (l->token != expected) {
    report_unexpected(l, expected);
    return false;
  }

  return true;
}

// parser --------------------------------------

Node *parse_node(Lexer *l);

Node *parse_tag(Lexer *l) {
  if (!lexer_expect(l, TOKEN_IDENT)) {
    lexer_print_loc(l, stderr);
    printf("hell nah\n");
    return NULL;
  }

  Node *el = node_create_element(l->string.items);

  if (!lexer_expect(l, TOKEN_TAG_CLOSE)) {
    lexer_print_loc(l, stderr);
    printf("hell nah\n");

    return NULL;
  }

  while (lexer_next(l)) {
    if (l->token == TOKEN_EOF) {
      lexer_print_loc(l, stderr);
      fprintf(stderr, "ERROR: Unexpected EOF while parsing content of <%s>\n",
              el->as.element->name);
      return NULL;
    }

    if (l->token == TOKEN_END_TAG_OPEN) {
      break;
    }

    Node *child = parse_node(l);
    if (!child) {
      return NULL;
    }

    node_append_child(el, child);
  }

  if (!lexer_expect(l, TOKEN_IDENT)) {
    lexer_print_loc(l, stderr);
    printf("hell nah\n");
    return NULL;
  }

  if (strcmp(el->as.element->name, l->string.items)) {
    lexer_print_loc(l, stderr);
    printf("hell nah\n");
    return NULL;
  }

  if (!lexer_expect(l, TOKEN_TAG_CLOSE)) {
    return NULL;
  }

  return el;
}

Node *parse_text(Lexer *l) { return node_create_text(l->string.items); }

Node *parse_node(Lexer *l) {
  switch ((int)l->token) {
  case TOKEN_TEXT:
    return parse_text(l);
  case TOKEN_TAG_OPEN:
    return parse_tag(l);
  default:
    lexer_print_loc(l, stderr);
    fprintf(stderr,
            "ERROR: Unexpected token %s. Expected a tag open instead.\n",
            token_kind_display(l->token));
    return NULL;
  }
}

// ---------------------------------------------

int main() {
  const char *input = "<html>"
                      "<body>"
                      "<h1>"
                      "Hello World"
                      "</h1>"
                      "<h2>"
                      "Yay!"
                      "</h2>"
                      "</body>"
                      "</html>";

  Lexer lexer = {0};
  lexer_init(&lexer, input, strlen(input), "main.html");

  if (!lexer_next(&lexer)) {
    fprintf(stderr, "Empty input\n");
    return 1;
  }

  Node *aboba = parse_node(&lexer);

  dump_node(aboba);

  node_free(aboba);
  sb_free(lexer.string);

  return 0;
}
