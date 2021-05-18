CC := aarch64-linux-gnu-g++
CC_FLAGS := -Wall -g -std=c++11

asm-file := m5op_arm_A64
benchmark-src := llubenchmark.cpp
benchmark-pim := llubenchmark_pim
benchmark-normal := llubenchmark_normal

dest-dir := ${mroot}/pim_benchmark
obj-dir := obj
bin-dir := bin

binaries := ${benchmark-pim} ${benchmark-normal}
targets := $(addprefix ${bin-dir}/, ${binaries})

dep-dir :=  ${proj-root}/pim_lib
dep-src-dir := ${dep-dir}/src
dep-archive := ${dep-dir}/libpim.a

.PHONY: build
build: ${targets}

${bin-dir}/${benchmark-pim}: ${obj-dir}/${benchmark-pim}.o ${obj-dir}/${asm-file}.o ${dep-archive} | ${bin-dir}
	${CC} $^ -o $@

${bin-dir}/${benchmark-normal}: ${obj-dir}/${benchmark-normal}.o ${obj-dir}/${asm-file}.o ${dep-archive} | ${bin-dir}
	${CC} $^ -o $@

${obj-dir}/${benchmark-pim}.o: ${benchmark-src} | ${obj-dir}
	${CC} -I${dep-src-dir} ${CC_FLAGS} -DPIM -c $^ -o $@

${obj-dir}/${benchmark-normal}.o: ${benchmark-src} | ${obj-dir}
	${CC} -I${dep-src-dir} ${CC_FLAGS} -c $^ -o $@

${obj-dir}/%.o: %.S | ${obj-dir}
	${CC} ${CC_FLAGS} -c $^ -o $@

${bin-dir}:
	mkdir $@

${obj-dir}:
	mkdir $@

.PHONY: install
install:
	rm -rf ${dest-dir}
	cp -r . ${dest-dir}	

.PHONY: clean
clean:
	rm -rf ${bin-dir} ${obj-dir}