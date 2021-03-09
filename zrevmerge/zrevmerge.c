#define REDISMODULE_EXPERIMENTAL_API

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <float.h>
#include "redismodule.h"
#include "common.h"
#include "maxheap.h"
#include "thpool.h"

const int BLOCKED_CLIENT_INDEX = 0;
const int LIMIT_ARG_INDEX = 1;
const int MIN_ARG_INDEX = 2;
const int MAX_ARG_INDEX = 3;
const int KEYS_NUM_ARG_INDEX = 4;
const int FIRST_KEY_ARG_INDEX = 5;

const int BLOCKED_CLIENT_TIMEOUT_MS = 2000;

const int THREAD_POOL_THREADS = 30;

threadpool pool;

/*
 * Fills heap with the contents of sorted set.
 */
static void fillHeap(RedisModuleCtx *ctx, heap_t *heap, void **targ) {
    const int numKeys = (long long) targ[KEYS_NUM_ARG_INDEX];

    double min = *(double *) targ[MIN_ARG_INDEX];

    double max = *(double *) targ[MAX_ARG_INDEX];
    if (max == 0) {
        max = REDISMODULE_POSITIVE_INFINITE;
    }

    for (int j = FIRST_KEY_ARG_INDEX; j < numKeys + FIRST_KEY_ARG_INDEX; j++) {
        RedisModule_ThreadSafeContextLock(ctx);

        RedisModuleKey *key = RedisModule_OpenKey(ctx, (RedisModuleString *) targ[j], REDISMODULE_READ);
        if (key == NULL) {
            RedisModule_ThreadSafeContextUnlock(ctx);
            continue;
        }

        if (RedisModule_KeyType(key) != REDISMODULE_KEYTYPE_ZSET) {
            RedisModule_ThreadSafeContextUnlock(ctx);
            RedisModule_CloseKey(key);
            RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
            return;
        }

        if (min == 0) {
            RedisModule_ZsetLastInScoreRange(key, min, max, 1, 1);
        } else {
            RedisModule_ZsetFirstInScoreRange(key, min, max, 1, 1);
        }

        int pushed = 0;
        while (!RedisModule_ZsetRangeEndReached(key) && pushed < heap->limit) {
            double score;
            RedisModuleString *value = RedisModule_ZsetRangeCurrentElement(key, &score);
            heapPush(heap, score, value);
            if (min == 0) {
                RedisModule_ZsetRangePrev(key);
            } else {
                RedisModule_ZsetRangeNext(key);
            }

            pushed++;
        }

        RedisModule_CloseKey(key);

        RedisModule_ThreadSafeContextUnlock(ctx);
    }
}

/*
 * Replies with values and scores, just like ZREVRANGE WITHSCORES.
 * Also frees nodes popped from heap.
 */
static void sendReply(RedisModuleCtx *ctx, heap_t *heap) {
    RedisModule_ReplyWithArray(ctx, REDISMODULE_POSTPONED_ARRAY_LEN);

    int numReturned = 0;
    while (heap->len > 0 && numReturned < heap->limit) {
        node_t *node = (node_t *) RedisModule_Alloc(sizeof(node_t));
        heapPop(heap, node);

        RedisModule_ReplyWithString(ctx, node->data);
        RedisModule_ReplyWithDouble(ctx, node->priority);

        RedisModule_FreeString(ctx, node->data);
        RedisModule_Free(node);
        numReturned++;
    }

    RedisModule_ReplySetArrayLength(ctx, numReturned * 2);
}

static void freeThreadArg(RedisModuleCtx *ctx, void *arg) {
    void **targ = arg;

    RedisModule_Free(targ[MIN_ARG_INDEX]);
    RedisModule_Free(targ[MAX_ARG_INDEX]);

    // Free keys args as they were retained by thread
    const int numKeys = (long long) targ[KEYS_NUM_ARG_INDEX];
    for (int j = FIRST_KEY_ARG_INDEX; j < numKeys + FIRST_KEY_ARG_INDEX; j++) {
        RedisModule_FreeString(ctx, targ[j]);
    }

    RedisModule_Free(targ);
}

static void *ZRevMerge_ThreadMain(void *arg) {
    void **targ = arg;
    RedisModuleBlockedClient *bc = targ[BLOCKED_CLIENT_INDEX];
    RedisModuleCtx *ctx = RedisModule_GetThreadSafeContext(bc);

    heap_t *heap = allocateHeap((long long) targ[LIMIT_ARG_INDEX]);
    fillHeap(ctx, heap, targ);

    freeThreadArg(ctx, targ);

    sendReply(ctx, heap);
    freeHeap(ctx, heap);

    RedisModule_FreeThreadSafeContext(ctx);
    RedisModule_UnblockClient(bc, NULL);

    return NULL;
}

/*
 * Returns 0 if something's wrong, 1 otherwise.
 */
static int validateInput(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (argc < FIRST_KEY_ARG_INDEX + 1) {
        RedisModule_WrongArity(ctx);
        return 0;
    }

    long long limit;
    if (RedisModule_StringToLongLong(argv[LIMIT_ARG_INDEX], &limit) != REDISMODULE_OK) {
        RedisModule_ReplyWithError(ctx, "ERR invalid limit");
        return 0;
    }

    long long keysNum;
    if (RedisModule_StringToLongLong(argv[KEYS_NUM_ARG_INDEX], &keysNum) != REDISMODULE_OK) {
        RedisModule_ReplyWithError(ctx, "ERR Invalid num of keys");
        return 0;
    }

    double min;
    if (RedisModule_StringToDouble(argv[MIN_ARG_INDEX], &min) != REDISMODULE_OK) {
        RedisModule_ReplyWithError(ctx, "ERR Invalid min");
        return 0;
    }

    double max;
    if (RedisModule_StringToDouble(argv[MAX_ARG_INDEX], &max) != REDISMODULE_OK) {
        RedisModule_ReplyWithError(ctx, "ERR Invalid max");
        return 0;
    }

    if (min != 0 && max != 0) {
        RedisModule_ReplyWithError(ctx, "ERR Either min or max should be 0");
        return 0;
    }

    return 1;
}

/*
 * Input considered to be already validated.
 * Should be freed after usage.
 */
static void **prepareThreadArgs(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    RedisModuleBlockedClient *bc = RedisModule_BlockClient(ctx, NULL, NULL, NULL, BLOCKED_CLIENT_TIMEOUT_MS);

    void **targ = RedisModule_Alloc(sizeof(void *) * argc);
    targ[BLOCKED_CLIENT_INDEX] = bc;

    long long limit;
    RedisModule_StringToLongLong(argv[LIMIT_ARG_INDEX], &limit);
    targ[LIMIT_ARG_INDEX] = (void *) limit;

    double *min = RedisModule_Alloc(sizeof(double));
    RedisModule_StringToDouble(argv[MIN_ARG_INDEX], min);
    targ[MIN_ARG_INDEX] = min;

    double *max = RedisModule_Alloc(sizeof(double));
    RedisModule_StringToDouble(argv[MAX_ARG_INDEX], max);
    targ[MAX_ARG_INDEX] = max;

    long long numKeys = argc - FIRST_KEY_ARG_INDEX;
    targ[KEYS_NUM_ARG_INDEX] = (void *) numKeys;

    // Read all key args to pass to thread
    for (int j = FIRST_KEY_ARG_INDEX; j < argc; j++) {
        targ[j] = (void *) argv[j];
        RedisModule_RetainString(ctx, targ[j]);
    }

    return targ;
}

static int ZRevMerge_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    if (validateInput(ctx, argv, argc) != 1) {
        return REDISMODULE_OK;
    }

    void **targ = prepareThreadArgs(ctx, argv, argc);

    if (thpool_add_work(pool, (void *) ZRevMerge_ThreadMain, targ) != 0) {
        RedisModule_AbortBlock(targ[BLOCKED_CLIENT_INDEX]);
        return RedisModule_ReplyWithError(ctx, "ERR Can't add work to thread pool");
    }

    return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    REDISMODULE_NOT_USED(argv);
    REDISMODULE_NOT_USED(argc);

    pool = thpool_init(THREAD_POOL_THREADS);

    if (RedisModule_Init(ctx, "zrevmerge", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR) return REDISMODULE_ERR;

    if (RedisModule_CreateCommand(ctx, "zrevmerge", ZRevMerge_RedisCommand, "readonly", 0, 0, 0) == REDISMODULE_ERR)
        return REDISMODULE_ERR;

    return REDISMODULE_OK;
}

void *ZRevMerge_Realloc(void *ptr, size_t bytes) {
    return RedisModule_Realloc(ptr, bytes);
}

void ZRevMerge_FreeString(RedisModuleCtx *ctx, RedisModuleString *str) {
    RedisModule_FreeString(ctx, str);
}

void ZRevMerge_Free(void *ptr) {
    RedisModule_Free(ptr);
}

void *ZRevMerge_Alloc(size_t bytes) {
    return RedisModule_Alloc(bytes);
}
