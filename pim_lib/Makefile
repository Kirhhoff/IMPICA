GCC := g++
TEST_LIB := -lgtest -lgmock -pthread
CC_FLAGS := -std=c++11 -Wno-placement-new
CROSS_COMPILE := aarch64-linux-gnu-
CC := ${CROSS_COMPILE}${GCC}
LIB_FLAGS := 

dest-dir := ${mroot}/pim_lib

obj-dir := obj
bin-dir := bin
src-dir := src
x86-src-dir := x86-src
utest-dir := utest
test-dir := test

pim-lib-hpps := $(shell ls ${src-dir}/*.hpp)
pim-lib-targets := $(basename $(notdir $(shell ls ${src-dir}/*.cpp)))
pim-lib-objs := $(addprefix ${obj-dir}/${src-dir}/, $(addsuffix .o, ${pim-lib-targets}))
pim-lib-archive := libpim.a

pim-x86-lib-objs := $(addprefix ${obj-dir}/${x86-src-dir}/, $(addsuffix .o, ${pim-lib-targets}))
pim-utest-includes := $(shell ls ${utest-dir}/*.h)
pim-utest-targets := $(basename $(notdir $(shell ls ${utest-dir}/*.cpp)))
pim-utest-bin := $(addprefix ${bin-dir}/${utest-dir}/, ${pim-utest-targets}) 
pim-utest-objs := $(addprefix ${obj-dir}/${utest-dir}/, $(addsuffix .o, ${pim-utest-targets}))

pim-test-includes := $(shell ls ${test-dir}/*.h)
pim-test-targets := $(basename $(notdir $(shell ls ${test-dir}/*.cpp)))
pim-test-bin := $(addprefix ${bin-dir}/${test-dir}/, ${pim-test-targets}) 
pim-test-objs := $(addprefix ${obj-dir}/${test-dir}/, $(addsuffix .o, ${pim-test-targets}))

build: $(pim-lib-archive)

install:
	rm -rf ${dest-dir} 
	cp -r . ${dest-dir}

${pim-lib-archive}: ${pim-lib-objs}
	ar -crv $@ $^

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

test: ${pim-test-objs}

${pim-test-objs}: | ${bin-dir}/${test-dir} ${obj-dir}/${test-dir}

${obj-dir}/${test-dir}/%.o: ${test-dir}/%.cpp ${pim-test-includes} ${pim-lib-archive} 
	${CC} ${CC_FLAGS} ${LIB_FLAGS} -c $< -o $@ -I${src-dir}
	${CC} ${CC_FLAGS} ${LIB_FLAGS} -o ${bin-dir}/${test-dir}/$(basename $(notdir $@)) $@ -static ${pim-lib-archive}

# ------------------------------------------------------------
${obj-dir}/%: | ${obj-dir}
	mkdir $@

${bin-dir}/%: | ${bin-dir}
	mkdir $@ 

${obj-dir}:
	mkdir $@

${bin-dir}:
	mkdir $@

clean: 
	rm -rf ${obj-dir} ${bin-dir} ${pim-lib-archive}