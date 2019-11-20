block_name=(8B 16B 32B 64B 128B 256B 512B 1KB 4KB 16KB 64KB 256KB 1MB 4MB 16MB)
block_size=(8 16 32 64 128 256 512 1024 4096 16384 65536 262144 1048576 4194304 16777216)
test_type=(rr rw sr sw)
num_thread=(1) # 2 4 6 8 10 12 14 16)

DIR=basic

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
./main --sync=1 --ntstore=1 --verify=0 --pmem_file_path=/home/pmem1/pm --align_size=256 --benchmark=${test_type[$i]} --block_size=${block_size[$j]} --num_thread=${num_thread[$k]} --data_amount=$data_amount > $OUTPUT/${num_thread[$k]}.result
done
done
done