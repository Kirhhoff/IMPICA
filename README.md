# IMPICA (In-Memory PoInter Chasing Accelerator)

[![License](https://img.shields.io/badge/license-BSD-blue.svg)](LICENSE)

This repo consists of the simulator and the workloads for IMPICA (In-Memory PoInter Chasing Accelerator), an ICCD 2016 paper.

## Folder structure

gem5: The main gem5-based simulator.

linux-aarch64-gem5-20140821: The Linux kernel code.

pim_driver: The Linux kernel driver for IMPICA.

workloads: The evaluated workloads.

## Run lab with docker in a few commands
Suppose you have cloned this repository to your host, you can simply run the following command under root directory:
```
sudo docker-compose run gem5-lab
```
and docker will:
1. pull image from docker hub
2. create and run a container from the image
3. spawn a bash

and you may find yourself under container /home directory:
```
root@98ce37a6bcd0:/home# ls
IMPICA  aarch-system-2014-10
```
where IMPICA is mapped onto your host repository.

Check into IMPICA and run:
```
cd IMPICA
make run
``` 
in a word _make_ will:
1. cross-compile linux kernel if it has not been compiled
2. build pim driver if it has not been built
3. build pim lib if it has not been built
4. compile gem5 if it has not been compiled
5. run pim-equipped kernel on gem5

As cross-compiler and gem5 disk image(under /home/aarch-system-2014-10/disks) has been built into docker image, you don't have to worry about them. Compiling kernel and gem5 may spends an hour or less for the first time.

After gem5 has run, you may want to attach a terminal to it, this may be achieved in two steps:
1. attach another bash to running docker container
2. attach a terminal to gem5 within the container

you may run:
```
# step 1: 
#   Before attaching another bash to the container, you have to
#   first find out its id. Use "docker container ls".
sudo docker container ls
sudo docker exec -it [container-id] bash

# step 2:
#   Now we are under /home, check into IMPICA and use "make"
#   to connect to gem5.
cd IMPICA
make connect
```
All control scripts are listed here:
- **Dockerfile**: you may change it if you want to DIY docker image
- **docker-compose.yml**: you may change it if you want to DIY container
- **Makefile**: automate build/run of the whole project
- **Makefiles under subdirectories**: control builds of different modules
## System prerequisites

The system needs to be able to build gem5 and the Linux kernel for ARM. Please refer to:

```
http://www.gem5.org/Dependencies
https://wiki.linaro.org/Resources/HowTo/KernelDeploy#A3_-_Build_the_Kernel
```

If using Ubuntu, the dependecies are

```
sudo apt-get install libncurses5-dev gcc make git exuberant-ctags bc libssl-dev python-dev scons m4 build-essential g++ swig zlib-dev
```

## Build the simulator and driver

```
cd gem5
scons -j8 PIM_DEVICE=btree build/ARM.gem5.opt

cd linux-aarch64-gem5-20140821
make menuconfig
make dep
make -j

cd pim_driver
make
```

The PIM_DEVICE can be btree, hash_table, or link_list. It needs to match the workload.

## Prepare the disk image

Get the full-system files for 64-bit ARM at

```
http://www.gem5.org/dist/current/arm/aarch-system-2014-10.tar.xz
```

Follow the tutorial to prepare the disk image.

```
http://gem5.org/Ubuntu_Disk_Image_for_ARM_Full_System
```

This is an example to put the driver (pim_driver) and workloads into the disk image. 

```
mount -oloop,offset=32256 aarch64-ubuntu-trusty-headless.img /mnt
cp -r pim_driver /mnt/home/
cp -r workloads /mnt/home/
mount -o bind /proc /mnt/proc
mount -o bind /dev /mnt/dev
mount -o bind /sys /mnt/sys
cp /etc/resolv.conf /mnt/etc/
chroot .
```

Then go to the subdirectory of the workload and make it. Please remember to umount the image before running the simulator.


## Run the simulator

```
cd gem5
M5_PATH=/path/to/full/system/ ./build/ARM/gem5.opt configs/example/fs.py --machine-type=VExpress_EMM64 -n 4 --mem-size=2048MB --disk-image=aarch64-ubuntu-trusty-headless.img --dtb-file=/path/to/full/system/binaries/vexpress.aarch64.20140821.dtb --kernel=/path/to/full/system/binaries/vmlinux.aarch64.20140821 
```

Open another terminal

```
cd gem5/util/term
make
m5 localhost 3456
```

Wait until the booting finishes, and login as root

You can make a checkpoint so you don't have to run the boot-up everytime

```
m5 checkpoint 
```

Install the driver in the simulator

```
cd /path/to/pim_driver
./go_test.sh
```

Now you can run the workload. All workloads make a checkpoint after initialization so you can resume the detailed (cycle accurate) simulation from that checkpoint. Assuming the second checkpoint is the one after workload initialization. For example, a detailed simulation can be executed as:

```
cd gem5
M5_PATH=/path/to/full/system/ --redirect-stdout --redirect-stderr --outdir=/path/to/output ./build/ARM/gem5.opt configs/example/fs.py --machine-type=VExpress_EMM64 -n 4 --mem-size=2048MB --disk-image=aarch64-ubuntu-trusty-headless.img --dtb-file=/path/to/full/system/binaries/vexpress.aarch64.20140821.dtb --kernel=/path/to/full/system/binaries/vmlinux.aarch64.20140821 -r 2 --checkpoint-dir=/path/to/checkpoints --caches --l2cache --cpu-type=detailed --ioc_size=32kB --ioc_lat=1 --pim_tlb_num=32 --l2_size=1MB --l1d_size=32kB
```


## Reference Paper

Kevin Hsieh, Samira Khan, Nandita Vijaykumar, Kevin K. Chang, Amirali Boroumand, Saugata Ghose, and Onur Mutlu, 
[Accelerating Pointer Chasing in 3D-Stacked Memory: Challenges, Mechanisms, Evaluation](https://users.ece.cmu.edu/~omutlu/pub/in-memory-pointer-chasing-accelerator_iccd16.pdf).
In Proceedings of the 34th IEEE International Conference on Computer Design (ICCD), Phoenix, AZ, USA, October 2016.
