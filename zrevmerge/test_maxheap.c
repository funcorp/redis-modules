#include "common.h"
#include "maxheap.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define UNUSED(x) (void) (x)

char *names[10] = {"zero", "one", "two", "free", "four", "five", "six", "seven", "eight", "nine"};

static void pushBundle(heap_t *heap, int cnt) {
    for (int i = 0; i < cnt; i++) {
        char *name = malloc(strlen(names[i]));
        strcpy(name, names[i]);
        heapPush(heap, (double) rand(), name);
    }
}

static int popBundle(heap_t *heap, int cnt) {
    node_t *first = (node_t *) malloc(sizeof(node_t));
    node_t *second = (node_t *) malloc(sizeof(node_t));
    heapPop(heap, first);
    for (int i = 1; i < cnt; i++) {
        heapPop(heap, second);
        if (first->priority < second->priority) {
            return 0;
        }
        first = second;
    }
    return 1;
}

int main(int argc, char **argv) {
    UNUSED(argc);
    UNUSED(argv);

    heap_t *heap = allocateHeap(2);

    for (int i = 0; i < 1000000; i++) {
        int cnt = rand() % 10;
        if (cnt < 2) continue;
        pushBundle(heap, cnt);
        assert(popBundle(heap, cnt));
    }

    pushBundle(heap, 9);
    freeHeap(NULL, heap);

    printf("PASS!\n");
    return 0;
}

void *ZRevMerge_Realloc(void *ptr, size_t bytes) {
    return realloc(ptr, bytes);
}

void ZRevMerge_FreeString(void *ctx, void *str) {
    UNUSED(ctx);
    free(str);
}

void ZRevMerge_Free(void *ptr) {
    free(ptr);
}

void *ZRevMerge_Alloc(size_t bytes) {
    return malloc(bytes);
}