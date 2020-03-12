block_name=(04_64B 05_128B 06_256B 07_512B 08_1KB 09_4KB 10_16KB 11_64KB 12_256KB 13_1MB 14_4MB 15_16MB)
block_size=(64 128 256 512 1024 4096 16384 65536 262144 1048576 4194304 16777216)
test_type=(rw sw)
num_thread=(1) # 2 4 6 8 10 12 14 16)

DIR=basic

data_amount=$((32*1024))

for ((i=0; i<${#test_type[*]}; i+=1))
do
for ((j=0; j<${#block_size[*]}; j+=1))
do
OUTPUT=$DIR/${test_type[$i]}/${block_name[$j]}
mkdir -p $OUTPUT
for ((k=0; k<${#num_thread[*]}; k+=1))
do
echo "running $OUTPUT/${num_thread[$k]}.result"
./../main --numa=1 --sync=1 --ntstore=1 --verify=0 --align_size=256 --benchmark=${test_type[$i]} --block_size=${block_size[$j]} --num_thread=${num_thread[$k]} --data_amount=$data_amount > $OUTPUT/${num_thread[$k]}.result
done
done
done