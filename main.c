#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#define UTILS_IMPLEMENTATION

typedef struct Node Node;
typedef struct NodeVec NodeVec;
typedef struct ElementData ElementData;
typedef Map AttrMap;
typedef struct ElementData ElementData;
typedef enum NodeKind { NODE_TEXT, NODE_ELEMENT } NodeKind;
typedef struct NodeType NodeType;

struct NodeType {
  NodeKind kind;
  union {
    char *text;
    ElementData *element;
  } as;
};

struct Node {
  NodeVec *children;
  NodeType node_type;
};

struct NodeVec {
  Node *items;

  size_t count;
  size_t capacity;
};

struct ElementData {
  char *tag_name;
  AttrMap attrs;
};

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
} Cur;

typedef struct {
  const char *content;
  size_t count;
  const char *file_path;

  bool inside_tag;

  Cur cur;

  TokenKind token;
  StringBuilder string;
  size_t row, col;
} Lexer;

char *copy_string_sized(const char *s, size_t n) {
  char *ds = malloc(n + 1);
  memcpy(ds, s, n);
  ds[n] = '\0';
  return ds;
}

char *copy_string(const char *s) { return copy_string_sized(s, strlen(s)); }

void lexer_init(Lexer *l, const char *content, size_t count,
                const char *file_path) {
  l->content = content;
  l->count = count;
  l->file_path = file_path;
  memset(&l->cur, 0, sizeof(l->cur));
  l->string.items = NULL;
  l->string.count = 0;
  l->string.capacity = 0;
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

// sorted list of void tags
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
      da_append(&l->string, lexer_next_char(l));
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
  Cur cur = l->cur;
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
Node elem(char *tag_name, AttrMap attrs, NodeVec *children);
Node text(char *data);

bool parse_attr(Lexer *l, AttrMap *attrs) {
  if (!lexer_expect(l, TOKEN_IDENT))
    return false;

  char *attr_name = l->string.items;

  if (!lexer_expect(l, TOKEN_EQUAL))
    return false;

  if (!lexer_expect(l, TOKEN_STRING))
    return false;

  char *attr_value = l->string.items;

  ht_put(attrs, attr_name, attr_value);

  return true;
};

bool parse_tag(Lexer *l, Node *node) {
  if (!lexer_expect(l, TOKEN_TAG_OPEN))
    return false;

  AttrMap attrs = {0};
  ht_init(&attrs, 64);

  NodeVec tag_body = {0};

  if (!lexer_expect(l, TOKEN_IDENT)) // advance identifier
    return false;

  char *tag_name = l->string.items;

  if (is_void_tag(tag_name)) {
    while (l->token != TOKEN_TAG_CLOSE && l->token != TOKEN_EOF) {
      if (!parse_attr(l, &attrs))
        return false;
    }

    *node = elem(tag_name, attrs, &tag_body);

    if (!lexer_next(l)) // advance tag close (>)
      return false;

    return true;
  }

  while (l->token != TOKEN_TAG_CLOSE && l->token != TOKEN_EOF &&
         l->token != TOKEN_TAG_SELF_CLOSE) {
    if (!parse_attr(l, &attrs))
      return false;
  }

  if (!lexer_next(l)) // advance tag close (>)
    return false;

  while (l->token != TOKEN_TAG_OPEN && l->token != TOKEN_EOF) {
    if (l->token == TOKEN_TEXT) {
      da_append(&tag_body, text(l->string.items));
      if (!lexer_next(l))
        return false;
    } else if (l->token == TOKEN_TAG_OPEN) {
      Node b = {0};
      if (!parse_tag(l, &b))
        return false;
      da_append(&tag_body, b);
    } else {
      report_unexpected(l, TOKEN_TEXT);
      return false;
    }
  }

  if (!lexer_expect(l, TOKEN_TAG_OPEN))
    return false; // advance tag open (<)

  if (!lexer_expect(l, TOKEN_IDENT))
    return false;

  char *close_tag_name = l->string.items;

  if (strcmp(tag_name, close_tag_name) != 0) {
    printf("Unexpected tag close name");
    return false;
  }

  if (!lexer_expect(l, TOKEN_TAG_CLOSE))
    return false;

  return true;
}

Node text(char *data) {
  NodeVec node_vec = {0};
  NodeType node_type = {.kind = NODE_TEXT, .as.text = data};

  Node node = {.children = &node_vec, .node_type = node_type};

  return node;
}

Node elem(char *tag_name, AttrMap attrs, NodeVec *children) {
  ElementData *element = malloc(sizeof(ElementData));
  *element = (ElementData){.tag_name = tag_name, .attrs = attrs};

  NodeType node_type = {.kind = NODE_ELEMENT, .as.element = element};

  Node node = {.children = children, .node_type = node_type};

  return node;
}

void node_free(Node *node) {
  if (node->node_type.kind == NODE_ELEMENT) {
    ht_free(&node->node_type.as.element->attrs);
    free(node->node_type.as.element);
  }
}

void node_display(Node *node, StringBuilder *sb) {
  switch (node->node_type.kind) {
  case NODE_TEXT:
    sb_appendf(sb, "%s ", node->node_type.as.text);
    break;
  case NODE_ELEMENT:
    sb_appendf(sb, "<%s", node->node_type.as.element->tag_name);

    ht_foreach(&node->node_type.as.element->attrs, entry) {
      sb_appendf(sb, "%s=\"%s\"", entry->key, (char *)entry->value);
    }
    sb_appendf(sb, ">");

    if (node->children) {
      da_foreach(Node, x, node->children) {
        size_t i = x - node->children->items;
        node_display(&node->children->items[i], sb);
      }
    }

    sb_appendf(sb, "</%s>", node->node_type.as.element->tag_name);
    break;
  }
}

int main() {
  Lexer lexer;
  const char *test = "<div>Aboba</div>";
  lexer_init(&lexer, test, strlen(test), "foo.html");

  StringBuilder sb = {0};

  Node foo = {0};
  if (!parse_tag(&lexer, &foo)) {
    printf("%s: '%.*s' (row: %zu, col: %zu)\n", token_kind_display(lexer.token),
           (int)lexer.string.count,
           lexer.string.count > 0 && lexer.string.items != NULL
               ? lexer.string.items
               : "",
           lexer.row, lexer.col);

    if (lexer.token == TOKEN_INVALID) {
      fprintf(stderr, "Invalid token at %zu:%zu\n", lexer.row, lexer.col);
    }
  } else {
    node_display(&foo, &sb);
  }
  return 0;
}
