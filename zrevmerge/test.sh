#!/usr/bin/env bash
echo "Starting Redis with ZRevMerge..."

redis-server --loadmodule zrevmerge.so > /dev/null &
SERVER_PID=$!

sleep 2

redis-cli info memory | grep "used_memory:"

echo "ZRevMerge with wrong args..."
for run in {1..1000}
do
  redis-cli zrevmerge > /dev/null
  redis-cli zrevmerge 1 > /dev/null
  redis-cli zrevmerge 1 1 > /dev/null
done

echo "ZRevMerge with absent keys..."
for run in {1..1000}
do
  redis-cli zrevmerge 1 2 key1 key 2 > /dev/null
done

echo "Write some data"
redis-cli zadd key1 1 one 3 three > /dev/null
redis-cli zadd key2 2 two 4 four > /dev/null

redis-cli info memory | grep "used_memory:"

echo "Do real merging stuff"
for run in {1..10}
do
  #                                 limit  min  max  numkeys
  parallel -N0 "redis-cli zrevmerge 2      0    3    2        key1 key2" ::: {1..1000} > /dev/null
  parallel -N0 "redis-cli zrevmerge 2      2    0    2        key1 key2" ::: {1..1000} > /dev/null
  parallel -N0 "redis-cli zrevmerge 2      0    0    2        key1 key2" ::: {1..1000} > /dev/null
done

redis-cli info memory | grep "used_memory:"

kill $SERVER_PID
