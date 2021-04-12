GCC := g++
TEST_LIB := -lgtest -lgmock -pthread
CC_FLAGS := -std=c++17 -Wno-placement-new
CROSS_COMPILE := aarch64-linux-gnu-
CC := ${CROSS_COMPILE}${GCC}
LIB_FLAGS := 

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
	${CC} ${CC_FLAGS} ${LIB_FLAGS} -c $< -o $@

utest: ${pim-utest-objs}

${pim-utest-objs}: | ${bin-dir}/${utest-dir} ${obj-dir}/${utest-dir}

${obj-dir}/${utest-dir}/%.o: ${utest-dir}/%.cpp ${pim-utest-includes} ${pim-x86-lib-objs} 
	${GCC} ${CC_FLAGS} ${LIB_FLAGS} -c $< -o $@ -I${src-dir} ${TEST_LIB}
	${GCC} ${CC_FLAGS} ${LIB_FLAGS} -o ${bin-dir}/${utest-dir}/$(basename $(notdir $@)) $@ ${pim-x86-lib-objs} ${TEST_LIB}

${pim-x86-lib-objs}: | ${obj-dir}/${x86-src-dir}

${obj-dir}/${x86-src-dir}/%.o: ${src-dir}/%.cpp ${pim-lib-hpps}
	${GCC} ${CC_FLAGS} ${LIB_FLAGS} -c $< -o $@

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