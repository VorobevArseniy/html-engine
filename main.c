#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#define UTILS_IMPLEMENTATION

typedef struct Map {
  size_t count;
  size_t capacity;

  void **buckets;
} Map;

typedef struct Node Node;
typedef struct NodeVec NodeVec;
typedef struct ElementData ElementData;
typedef Map AttrMap;
typedef struct ElementData ElementData;

typedef enum NodeKind { TEXT, ELEMENT } NodeKind;
typedef struct NodeType {

  NodeKind kind;
  union {
    char *text;
    ElementData *element;
  } as;
} NodeType;

struct Node {
  NodeVec *children;
  NodeType node_type;
  int x;
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

Node text(char *data) {
  NodeVec node_vec = {0};
  NodeType node_type = {.kind = TEXT, .as.text = data};

  Node node = {.children = &node_vec, .node_type = node_type};

  return node;
}

Node elem(char *tag_name, AttrMap attrs, NodeVec *children) {
  ElementData *element = malloc(sizeof(ElementData));
  *element = (ElementData){.tag_name = tag_name, .attrs = attrs};

  NodeType node_type = {.kind = ELEMENT, .as.element = element};

  Node node = {.children = children, .node_type = node_type};

  return node;
}

void node_free(Node *node) {
  if (node->node_type.kind == ELEMENT) {
    free(node->node_type.as.element);
  }
}

int main() {
  Node text_node = text("foo");

  AttrMap attrs = {0};
  ht_init(&attrs, 64);
  ht_put(&attrs, "class", "main");

  NodeVec children = {0};
  da_append(&children, text_node);

  Node elem_node = elem("div", attrs, &children);

  node_free(&text_node);
  node_free(&elem_node);
  return 0;
}
