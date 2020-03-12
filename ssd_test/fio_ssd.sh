#!/bin/bash

threads=("1" "2" "4" "6" "8" "10" "12" "14" "16")
threads_s=0
threads_len=9
nr_files=1
all_size=322122547200

cpus=('0-9,20-29' '0-9,20-29'  '0-9,20-29'  '0-9,20-29' '0-9,20-29')

bs_size=("8B" "16B" "32B" "64B" "128B" "256B" "512B" "1KB" "4KB" "16KB" "64KB" "256KB" "1MB" "4MB" "16MB")
size_s=0
size_len=15

type=('read' 'write' 'randread' 'randwrite')
type_s=0
type_len=2
ioengine=('sync' 'mmap' 'libaio')
io_s=2
io_len=3

work_path=`pwd`
test_times=1
branch="-b 1"
depth="-z 1"
dev_name=/dev/nvme0n1
fallocate="--fallocate=1"
runtime=30

rm -rf *.results
pre_size=397284474880
# pre_size=$((1024*1024))
fio --filename=$dev_name -rw=trim -ioengine=libaio --direct=1 -bs=128KB -name=test -iodepth=1 -size=$pre_size --nrfiles=1 --numjobs=1 
fio --filename=$dev_name -rw=write -ioengine=libaio --direct=1 -bs=128KB -name=test -iodepth=1 -size=$pre_size --nrfiles=1 --numjobs=1 
fio --filename=$dev_name -rw=randwrite -ioengine=libaio --direct=1 -bs=4KB -name=test -iodepth=1 -size=$pre_size  --nrfiles=1  --numjobs=1 

for((io=$io_s; io<$io_len; ++io)); # libio
do
    for((t=$type_s; t<$type_len; ++t)); # randomread 
    do
        for((i=$threads_s; i<$threads_len; ++i)); # thread
        do
            for((size=$size_s; size<$size_len; ++size)); 
            do
                for((times=0; times<$test_times; ++times)); 
                do
                fio --filename=$dev_name -rw=${type[$t]} -ioengine=${ioengine[$io]} -runtime=$runtime -time_based --direct=0 -bs=${bs_size[$size]} -name=test -iodepth=1 -size=$all_size $fallocate  --nrfiles=${nr_files} --cpus_allowed=${cpus[$f]} --numjobs=${threads[$i]} > ${type[$t]}_${ioengine[$io]}_${bs_size[$size]}_${threads[$i]}.results
                done
            done
        done
    done
done

mkdir test_ssd
mv *.results test_ssd


#mpirun -np 2 /mdtest/mdtest -s 10 -d /mdtest/wang
#./mdtest/mdtest -I 5 -z 3 -b 5 -u -d /mdtest/wang
#./mdtest/mdtest -I 5 -z 3 -b 5 
#./mdtest/mdtest -n 10000 -F -C  -z 3 -l 2 -I 10 -b 1 -u -d /mdtest/wang

