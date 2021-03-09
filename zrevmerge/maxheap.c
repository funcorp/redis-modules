#include "maxheap.h"
#include "zrevmerge.h"
#include "common.h"

static void resizeHeap(heap_t *h) {
    h->size = h->size * 2;
    h->nodes = (node_t *) ZRevMerge_Realloc(h->nodes, h->size * sizeof(node_t));
}

void freeNodeContent(void *ctx, node_t *node) {
    if (node->data == NULL) {
        return;
    }
    ZRevMerge_FreeString(ctx, node->data);
}

void heapPush(heap_t *heap, double priority, void *data) {
    if (UNEXPECTED(heap->len == heap->size)) {
        resizeHeap(heap);
    }

    heap->len++;
    int i = heap->len;
    int j = i / 2;
    while (i > 1 && heap->nodes[j - 1].priority < priority) {
        heap->nodes[i - 1] = heap->nodes[j - 1];
        i = j;
        j = j / 2;
    }
    heap->nodes[i - 1].priority = priority;
    heap->nodes[i - 1].data = data;
}

void heapPop(heap_t *heap, node_t *node) {
    int i, j, k;

    node->priority = heap->nodes[0].priority;
    node->data = heap->nodes[0].data;

    heap->nodes[0] = heap->nodes[heap->len - 1];
    heap->len--;

    i = 1;
    while (i != heap->len + 1) {
        k = heap->len + 1;
        j = 2 * i;
        if (j <= heap->len && heap->nodes[j - 1].priority > heap->nodes[k - 1].priority) {
            k = j;
        }
        if (j + 1 <= heap->len && heap->nodes[j].priority > heap->nodes[k - 1].priority) {
            k = j + 1;
        }
        heap->nodes[i - 1] = heap->nodes[k - 1];
        i = k;
    }

    heap->nodes[heap->len].data = NULL;
}

heap_t *allocateHeap(int limit) {
    heap_t *heap = (heap_t *) ZRevMerge_Alloc(sizeof(heap_t));
    heap->len = 0;
    heap->limit = limit;
    if (heap->limit == 0) {
        heap->limit = 1;
    }

    heap->size = heap->limit;
    heap->nodes = (node_t *) ZRevMerge_Alloc(heap->size * sizeof(node_t));
    return heap;
}

void freeHeap(void *ctx, heap_t *heap) {
    for (int i = 0; i < heap->len; i++) {
        freeNodeContent(ctx, &heap->nodes[i]);
    }

    ZRevMerge_Free(heap->nodes);

    ZRevMerge_Free(heap);
}
