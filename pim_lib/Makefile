GCC := g++
GCCLIB := -lgtest -lgmock -pthread
GCCFLAGS := -std=c++17
CROSS_COMPILE := aarch64-linux-gnu-
CC := ${CROSS_COMPILE}${GCC}

img := $(M5_PATH)/disks/aarch64-ubuntu-trusty-headless.img
mroot := /mnt
dest-dir := ${mroot}/home/pim_lib

obj-dir := obj
bin-dir := bin
src-dir := src
x86-src-dir := x86-src
utest-dir := utest

pim-lib-hpps := $(shell ls ${src-dir}/*.hpp)
pim-lib-targets := $(basename $(notdir $(shell ls ${src-dir}/*.cpp)))
pim-lib-objs := $(addprefix ${obj-dir}/${src-dir}/, $(addsuffix .o, ${pim-lib-targets}))

pim-x86-lib-objs := $(addprefix ${obj-dir}/${x86-src-dir}/, $(addsuffix .o, ${pim-lib-targets}))
pim-utest-includes := $(shell ls ${utest-dir}/*.h)
pim-utest-targets := $(basename $(notdir $(shell ls ${utest-dir}/*.cpp)))
pim-utest-bin := $(addprefix ${bin-dir}/${utest-dir}/, ${pim-utest-targets}) 
pim-utest-objs := $(addprefix ${obj-dir}/${utest-dir}/, $(addsuffix .o, ${pim-utest-targets}))

build: $(pim-lib-objs)

${pim-lib-objs}: | ${obj-dir}/${src-dir}

${obj-dir}/${src-dir}/%.o: ${src-dir}/%.cpp ${pim-lib-hpps}
	${CC} -c $< -o $@

utest: ${pim-utest-objs}

${pim-utest-objs}: | ${bin-dir}/${utest-dir} ${obj-dir}/${utest-dir}

${obj-dir}/${utest-dir}/%.o: ${utest-dir}/%.cpp ${pim-utest-includes} ${pim-x86-lib-objs} 
	${GCC} ${GCCFLAGS} -c $< -o $@ -I${src-dir} ${GCCLIB}
	${GCC} ${GCCFLAGS} -o ${bin-dir}/${utest-dir}/$(basename $(notdir $@)) $@ ${pim-x86-lib-objs} ${GCCLIB}

${pim-x86-lib-objs}: | ${obj-dir}/${x86-src-dir}

${obj-dir}/${x86-src-dir}/%.o: ${src-dir}/%.cpp ${pim-lib-hpps}
	${GCC} ${GCCFLAGS} -c $< -o $@

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