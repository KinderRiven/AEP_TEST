block_name=(01_8B 02_16B 03_32B 04_64B 05_128B 06_256B 07_512B 08_1KB 09_4KB 10_16KB 11_64KB 12_256KB 13_1MB 14_4MB 15_16MB)
block_size=(8 16 32 64 128 256 512 1024 4096 16384 65536 262144 1048576 4194304 16777216)
flush_type=(clflush clflushopt clwb)
num_thread=(1 2 4 6 8 10 12 14 16)

DIR=flush

data_amount=$((2*1024))

for ((i=0; i<${#flush_type[*]}; i+=1))
do
for ((j=0; j<${#block_size[*]}; j+=1))
do
OUTPUT=$DIR/${flush_type[$i]}/${block_name[$j]}
mkdir -p $OUTPUT
for ((k=0; k<${#num_thread[*]}; k+=1))
do
echo "running $OUTPUT/${num_thread[$k]}.result"
./../main --numa=0 --sync=1 --ntstore=0 --verify=0 --align_size=256 --benchmark=rw --flush=${flush_type[$i]} --block_size=${block_size[$j]} --num_thread=${num_thread[$k]} --data_amount=$data_amount > $OUTPUT/${num_thread[$k]}.result
done
done
done