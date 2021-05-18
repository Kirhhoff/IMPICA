kern-dir := linux-aarch64-gem5-20140821
kern-bin := ${kern-dir}/vmlinux
kern-img := $(M5_PATH)/binaries/vmlinux

disk-img := $(M5_PATH)/disks/aarch64-ubuntu-trusty-headless.img
mnt-dir := /mnt

export mroot := ${mnt-dir}/home
export proj-root := $(PWD)
export driver-dir := ${proj-root}/pim_driver
export build_cores := 17
export run_cores := 6

run: ${kern-img}
	make -C pim_driver build
	make -C pim_lib build
	make -C pim_lib test
	make -C workloads/llubenchmark build
	make -C pim_benchmark

	mount -o loop,offset=32256 ${disk-img} ${mnt-dir}
	make -C pim_driver install 
	make -C pim_lib install
	make -C workloads/llubenchmark install
	make -C pim_benchmark install
	umount ${mnt-dir}

	make -C gem5 run

${kern-img}:
	make -C ${kern-dir} ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- oldconfig
	make -C ${kern-dir} ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- dep
	make -C ${kern-dir} ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j${build_cores}
	cp ${kern-bin} ${kern-img}

connect:
	make -C gem5/util/term connect

clean:
	make -C pim_driver clean
	make -C pim_lib clean
	make -C workloads/llubenchmark clean
	make -C gem5/util/term clean
	make -C pim_benchmark clean

clean-gem5:
	make -C gem5 clean

clean-kernel:
	make -C ${kern-dir} clean
