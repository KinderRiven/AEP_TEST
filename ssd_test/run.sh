block_name=(09_4KB 10_16KB 11_64KB 12_256KB 13_1MB 14_4MB 15_16MB)
block_size=(4096 16384 65536 262144 1048576 4194304 16777216)
TOTAL_SIZE=$((32*1024))
RESULT_PATH=result
mkdir $RESULT_PATH

for ((i=0; i<${#block_size[*]}; i+=1))
do
rm -rf /home/hanshukai/optane/*.io
for ((j=1; j<=2; j++))
do
# perf record ./tester $i 1 $BLOCK_SIZE $TOTAL_SIZE
./tester $j 2 1 ${block_size[$i]} $TOTAL_SIZE > $RESULT_PATH/${block_name[$i]}
# mv perf.data $RESULT_PATH/$i_$j
sync && echo 3 > /proc/sys/vm/drop_caches
done
done
