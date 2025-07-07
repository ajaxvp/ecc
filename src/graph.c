#include <stdlib.h>
#include <string.h>

#include "ecc.h"

void graph_delete(graph_t* graph)
{
    if (!graph) return;
    set_delete(graph->lists);
    free(graph);
}

graph_t* graph_init(int (*comparator)(void*, void*), unsigned long (*hash)(void*), void (*vertex_deleter)(void*))
{
    graph_t* graph = calloc(1, sizeof *graph);
    graph->lists = map_init(comparator, hash);
    map_set_deleters(graph->lists, vertex_deleter, (void (*)(void*)) set_delete);
    graph->comparator = comparator;
    graph->hash = hash;
    graph->vertex_deleter = vertex_deleter;
    return graph;
}

bool graph_add_vertex(graph_t* graph, void* item)
{
    if (map_contains_key(graph->lists, item))
        return false;
    map_t* set = set_init(graph->comparator, graph->hash);
    set_set_deleter(set, graph->vertex_deleter);
    map_add(graph->lists, item, set);
    return true;
}

bool graph_add_edge(graph_t* graph, void* from, void* to)
{
    map_t* from_set = map_get(graph->lists, from);
    if (!from_set)
        return false;
    map_t* to_set = map_get(graph->lists, to);
    if (!to_set)
        return false;
    set_add(from_set, to);
    set_add(to_set, from);
    return true;
}

bool graph_remove_vertex(graph_t* graph, void* item)
{
    if (!map_contains_key(graph->lists, item))
        return false;
    map_t* vset = map_get(graph->lists, item);
    MAP_FOR(void*, void*, vset)
    {
        (void) v;
        map_t* set = map_get(graph->lists, k);
        set_remove(set, item);
    }
    map_remove(graph->lists, item);
    return true;
}

bool graph_remove_edge(graph_t* graph, void* from, void* to)
{
    map_t* from_set = map_get(graph->lists, from);
    if (!from_set)
        return false;
    map_t* to_set = map_get(graph->lists, to);
    if (!to_set)
        return false;
    set_remove(from_set, to);
    set_remove(to_set, from);
    return true;
}

bool graph_has_edge(graph_t* graph, void* from, void* to)
{
    map_t* from_set = map_get(graph->lists, from);
    if (!from_set)
        return false;
    map_t* to_set = map_get(graph->lists, to);
    if (!to_set)
        return false;
    return set_contains(from_set, to) && set_contains(to_set, from);
}

size_t graph_size(graph_t* graph)
{
    return graph->lists->size;
}
