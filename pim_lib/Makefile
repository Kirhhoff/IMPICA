obj-m += test-driver.o
#obj-m += test_morteza.o
CROSS_COMPILE := aarch64-linux-gnu-
CC := ${CROSS_COMPILE}gcc

img := $(M5_PATH)/disks/aarch64-ubuntu-trusty-headless.img
mroot := /mnt
dest-dir := ${mroot}/home/pim_lib

obj-dir := obj
src-dir := src
test-dir := test

pim-driver-sources := pimbt_driver.h test-driver.c
pim-driver-objs := *.ko *.order *.symvers .test-driver* *.o *.mod.c

pim-lib-targets := user_code
pim-lib-objs := $(addprefix ${obj-dir}/${src-dir}/, $(addsuffix .o, ${pim-lib-targets}))
pim-lib := libpim.a

pim-test-targets := user_code
pim-test-objs := $(addprefix ${obj-dir}/${test-dir}/, $(addsuffix .o, ${pim-test-targets}))
pim-test-bin := test_code

build: $(pim-lib)

${pim-lib}: $(pim-lib-objs) 
	touch libpim.a

${pim-lib-objs}: | ${obj-dir}/${src-dir}

${obj-dir}/${src-dir}/%.o : ${src-dir}/%.c 
	${CC} -c $< -o $@

test: ${pim-test-bin}

${pim-test-bin} : ${pim-test-objs} ${pim-lib}
	${CC} -o $@ $<
	mount -o loop,offset=32256 ${img} ${mroot}
	rm -rf ${dest-dir}
	cp -r . ${dest-dir}
	umount ${mroot}

${pim-test-objs}: | ${obj-dir}/${test-dir}

${obj-dir}/${test-dir}/%.o : ${test-dir}/%.c
	${CC} -c $< -o $@

${obj-dir}/%: | ${obj-dir}
	mkdir $@

${obj-dir}:
	mkdir $@

clean: 
	rm -rf ${obj-dir}
	rm -f ${pim-lib} ${pim-test-bin}