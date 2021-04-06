obj-m += test-driver.o
#obj-m += test_morteza.o
CROSS_COMPILE := aarch64-linux-gnu-
CC := ${CROSS_COMPILE}gcc
CCFLAGS := -std=gnu99
GCC := gcc
GCCLIB := -lcunit

img := $(M5_PATH)/disks/aarch64-ubuntu-trusty-headless.img
mroot := /mnt
dest-dir := ${mroot}/home/pim_lib

obj-dir := obj
bin-dir := bin
src-dir := src
test-dir := test
x86-src-dir := x86-src
utest-dir := utest

pim-driver-sources := pimbt_driver.h test-driver.c
pim-driver-objs := *.ko *.order *.symvers .test-driver* *.o *.mod.c

pim-lib-targets := $(basename $(notdir $(shell ls ${src-dir}/*.c)))
pim-lib-objs := $(addprefix ${obj-dir}/${src-dir}/, $(addsuffix .o, ${pim-lib-targets}))
pim-lib := libpim.a

pim-test-targets := user_code
pim-test-objs := $(addprefix ${obj-dir}/${test-dir}/, $(addsuffix .o, ${pim-test-targets}))
pim-test-bin := test_code

pim-x86-lib-objs := $(addprefix ${obj-dir}/${x86-src-dir}/, $(addsuffix .o, ${pim-lib-targets}))
pim-utest-targets := $(basename $(notdir $(shell ls ${utest-dir}/*.c)))
pim-utest-bin := $(addprefix ${bin-dir}/${utest-dir}/, ${pim-utest-targets}) 
pim-utest-objs := $(addprefix ${obj-dir}/${utest-dir}/, $(addsuffix .o, ${pim-utest-targets}))

build: $(pim-lib)

${pim-lib}: $(pim-lib-objs) 
	touch libpim.a

${pim-lib-objs}: | ${obj-dir}/${src-dir}

${obj-dir}/${src-dir}/%.o: ${src-dir}/%.c 
	${CC} -c $< -o $@

test: ${pim-test-bin}

${pim-test-bin}: ${pim-test-objs} ${pim-lib}
	${CC} -o $@ $<
	mount -o loop,offset=32256 ${img} ${mroot}
	rm -rf ${dest-dir}
	cp -r . ${dest-dir}
	umount ${mroot}

${pim-test-objs}: | ${obj-dir}/${test-dir}

${obj-dir}/${test-dir}/%.o : ${test-dir}/%.c
	${CC} -c $< -o $@

utest: ${pim-utest-objs}

${pim-utest-objs}: | ${bin-dir}/${utest-dir} ${obj-dir}/${utest-dir}

${obj-dir}/${utest-dir}/%.o: ${utest-dir}/%.c  ${pim-x86-lib-objs}
	${GCC} -c $< -o $@ -I${src-dir} ${GCCLIB}
	${GCC} -o ${bin-dir}/${utest-dir}/$(basename $(notdir $@)) $@ ${pim-x86-lib-objs} ${GCCLIB}

${pim-x86-lib-objs}: | ${obj-dir}/${x86-src-dir}

${obj-dir}/${x86-src-dir}/%.o: ${src-dir}/%.c 
	${GCC} -c $< -o $@

${obj-dir}/%: | ${obj-dir}
	mkdir $@

${bin-dir}/%: | ${bin-dir}
	mkdir $@ 

${obj-dir}:
	mkdir $@

${bin-dir}:
	mkdir $@

clean: 
	rm -rf ${obj-dir}
	rm -rf ${bin-dir}
	rm -f ${pim-lib} ${pim-test-bin}