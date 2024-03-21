#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "cc.h"

set_t* set_init(int (*comparator)(void*, void*))
{
    set_t* s = calloc(1, sizeof *s);
    s->root = NULL;
    s->size = 0;
    s->comparator = comparator;
    return s;
}

static binary_node_t* binary_node_init(void* value)
{
    binary_node_t* node = calloc(1, sizeof *node);
    node->left = NULL;
    node->right = NULL;
    node->value = value;
    return node;
}

static void set_add_node(set_t* s, binary_node_t* node, void* value)
{
    int c = s->comparator(value, node->value);
    if (c < 0)
    {
        if (node->left)
            set_add_node(s, node->left, value);
        else
            node->left = binary_node_init(value);
    }
    else
    {
        if (node->right)
            set_add_node(s, node->right, value);
        else
            node->right = binary_node_init(value);
    }
}

set_t* set_add(set_t* s, void* value)
{
    ++(s->size);
    if (!s->root)
    {
        s->root = binary_node_init(value);
        return s;
    }
    set_add_node(s, s->root, value);
    return s;
}

static bool set_contains_node(set_t* s, binary_node_t* node, void* value)
{
    if (!node) return false;
    int c = s->comparator(value, node->value);
    return !c || set_contains_node(s, node->left, value) || set_contains_node(s, node->right, value);
}

bool set_contains(set_t* s, void* value)
{
    return set_contains_node(s, s->root, value);
}

static void binary_node_delete(binary_node_t* node)
{
    if (!node) return;
    binary_node_delete(node->left);
    binary_node_delete(node->right);
    free(node);
}

static void set_print_node(binary_node_t* node, void (*printer)(void*))
{
    printer(node->value);
    printf(" (left: ");
    if (node->left)
        printer(node->left->value);
    else
        printf("null");
    printf(", right: ");
    if (node->right)
        printer(node->right->value);
    else
        printf("null");
    printf(")\n");
}

static void set_print_recur(binary_node_t* node, unsigned level, void (*printer)(void*))
{
    if (!node) return;
    printf("level %d: ", level);
    set_print_node(node, printer);
    set_print_recur(node->left, level + 1, printer);
    set_print_recur(node->right, level + 1, printer);
}

void set_print(set_t* s, void (*printer)(void*))
{
    printf("[set]\n");
    if (!s->root)
    {
        printf("empty\n");
        return;
    }
    printf("root: ");
    set_print_node(s->root, printer);
    set_print_recur(s->root->left, 1, printer);
    set_print_recur(s->root->right, 1, printer);
}

void set_delete(set_t* s)
{
    binary_node_delete(s->root);
    free(s);
}