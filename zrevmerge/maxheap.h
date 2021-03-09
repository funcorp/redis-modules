#ifndef MODULES_MAXHEAP_H
#define MODULES_MAXHEAP_H

typedef struct {
    double priority;
    void *data;
} node_t;

typedef struct {
    node_t *nodes;
    int len;
    int limit;
    int size;
} heap_t;

/* Heap nodes should be freed after usage. */
void heapPush(heap_t *heap, double priority, void *data);

/* Pops to provided result pointer. */
void heapPop(heap_t *heap, node_t *node);

heap_t *allocateHeap(int limit);

void freeHeap(void *ctx, heap_t *heap);

void freeNodeContent(void *ctx, node_t *node);

#endif
