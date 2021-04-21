
obj-m += test-driver.o
#obj-m += test_morteza.o
CROSS_COMPILE := aarch64-linux-gnu-

dest-dir := ${mroot}/pim_driver

obj-dir := obj

pim-driver-sources := pimbt_driver.h test-driver.c
_pim-driver-obj := test-driver.ko
pim-driver-obj := $(addprefix ${obj-dir}/, ${_pim-driver-obj})

pim-driver-generated := *.order *.symvers .test-driver* *.o *.mod.c

build: ${pim-driver-obj}

${pim-driver-obj}: ${pim-driver-sources} | ${obj-dir}
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C ../linux-aarch64-gem5-20140821 M=$(driver-dir) modules
	mv ${_pim-driver-obj} ${obj-dir}
	mv ${pim-driver-generated} ${obj-dir}
	rm -rf ${obj-dir}/.tmp_versions
	mv .tmp_versions ${obj-dir}

install:
	rm -rf ${dest-dir}
	cp -r . ${dest-dir}

${obj-dir}:
	mkdir $@

clean: 
	make -C ../linux-aarch64-gem5-20140821 M=$(driver-dir) clean
	rm -rf obj