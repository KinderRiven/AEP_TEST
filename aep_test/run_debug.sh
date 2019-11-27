block_name=(256B)
block_size=(256)
test_type=(rr rw sr sw)
num_thread=(1 8)

DIR=debug

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
./../main --numa=0 --sync=1 --ntstore=1 --verify=1 --align_size=256 --benchmark=${test_type[$i]} --block_size=${block_size[$j]} --num_thread=${num_thread[$k]} --data_amount=$data_amount > $OUTPUT/${num_thread[$k]}.result
done
done
done