block_name=(1_8B 2_16B 3_32B 4_64B 5_128B 6_256B 7_512B 8_1KB 9_4KB 10_16KB 11_64KB 12_256KB 13_1MB 14_4MB 15_16MB)
block_size=(8 16 32 64 128 256 512 1024 4096 16384 65536 262144 1048576 4194304 16777216)
test_type=(rr rw)
num_thread=(1 2 4 6 8 10 12 14 16)

DIR=align

data_amount=$((2*1024))

for ((i=0; i<${#test_type[*]}; i+=1))
do
for ((j=0; j<${#block_size[*]}; j+=1))
do
OUTPUT=$DIR/${test_type[$i]}/${block_name[$j]}
mkdir -p $OUTPUT
for ((k=0; k<${#num_thread[*]}; k+=1))
do
echo "running $OUTPUT/${num_thread[$k]}.result"
./main --numa=0 --sync=1 --ntstore=0 --verify=0 --align_size=123 --benchmark=${test_type[$i]} --block_size=${block_size[$j]} --num_thread=${num_thread[$k]} --data_amount=$data_amount > $OUTPUT/${num_thread[$k]}.result
done
done
done