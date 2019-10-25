# 1. create a mount dir
# mkdir /home/pmem0

# 2. create ext4 fs
# mkfs.ext4 /dev/pmem0

# 3. use dax mode to mount pmem device
# mount -o dax /dev/pmem0 /home/pmem0

# 4. create a empty file as memory pool (64GB)
# dd if=/dev/zero of=/home/pmem0/pm bs=$((64*1024)) count=$((1024*1024))

# 5. do make
# make use_pmdk

# 6. run basic test
# chmod 777 run_basic.sh
# ./run_basic

# 6. more parameters see [main.cc]~
