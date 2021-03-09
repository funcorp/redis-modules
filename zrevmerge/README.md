# ZRevMerge

Command example: `ZREVMERGE limit min max num_keys key1...keyN`, where:
* `limit` — max items returned in merged set;
* `min` — minimum item score, exclusive. Either min or max should be 0, 0 is a null value;
* `max` — maximum item score, exclusive. Either min or max should be 0, 0 is a null value;
* `num_keys` is number of sorted sets being merged;
* `key1 key2` keys holding sorted sets being merged.

*Limitations*: scores of items in sorted set should be positive (> 0).

## Build

```
make
```

## Test

Note: Redis is required to run `test.sh`.

```
make test
./test.sh
```

## Run

```
redis-server --loadmodule ../../zrevmerge.so
```