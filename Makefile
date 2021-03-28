kern-dir := linux-aarch64-gem5-20140821
kern-bin := ${kern-dir}/vmlinux
kern-img := $(M5_PATH)/binaries/vmlinux

export driver-dir := $(PWD)/pim_driver

run: ${kern-img}
	make -C pim_driver build
	make -C pim_lib test
	make -C gem5 run

${kern-img}:
	make -C ${kern-dir} ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- oldconfig
	make -C ${kern-dir} ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- dep
	make -C ${kern-dir} ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j1
	cp ${kern-bin} ${kern-img}

connect:
	make -C gem5/util/term connect

clean:
	make -C pim_driver clean
	make -C pim_lib clean
	make -C gem5/util/term clean

clean-gem5:
	make -C gem5 clean

clean-kernel:
	make -C ${kern-dir} clean