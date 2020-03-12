RESULT_PATH=dd_result
BLOCK_SIZE=$((1*4096))
TOTAL_SIZE=$((4*1024))

mkdir $RESULT_PATH
for ((i=1; i<=3; i++))
do
rm -rf 0.io
for ((j=1; j<=10; j++))
do
# perf record ./tester $i 1 $BLOCK_SIZE $TOTAL_SIZE
./tester $i 1 $BLOCK_SIZE $TOTAL_SIZE
# mv perf.data $RESULT_PATH/$i_$j
sync && echo 3 > /proc/sys/vm/drop_caches
done
done
