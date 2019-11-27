block_name=(64B 256B 1KB 4KB 64KB 256KB 1MB 4MB)
block_size=(64 256 1024 4096 65536 262144 1048576 4194304)
test_type=(sm rm)
read_thread=(1 2 3 4 5 6 7 8)
write_thread=(1 2 3 4 5 6 7 8)

DIR=mixed
data_amount=$((2*1024))

for ((i=0; i<${#test_type[*]}; i+=1))
do
for ((j=0; j<${#block_size[*]}; j+=1))
do
OUTPUT=$DIR/${test_type[$i]}/${block_name[$j]}
mkdir -p $OUTPUT
for ((k=0; k<${#read_thread[*]}; k+=1))
do
for ((l=0; l<${#write_thread[*]}; l+=1))
do
echo "running $OUTPUT/${read_thread[$k]}_${write_thread[$l]}.result"
./../main --numa=1 --sync=1 --ntstore=1 --verify=0 --align_size=256 --benchmark=${test_type[$i]} --block_size=${block_size[$j]} --read_thread=${read_thread[$k]} --write_thread=${write_thread[$l]} --data_amount=$data_amount > $OUTPUT/${read_thread[$k]}_${write_thread[$l]}.result
done
done
done
done