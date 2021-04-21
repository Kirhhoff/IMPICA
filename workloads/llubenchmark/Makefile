CC=aarch64-linux-gnu-g++
CFLAGS=-Wall -g -std=c++11

.SUFFIXES: .o .c .h

SRC_DIRS = ./ 
DEPS = -I. 

CFLAGS += $(DEPS) #-D  -Werror
LDFLAGS = -Wall -L. -pthread -g -lrt -std=c++0x 
LDFLAGS += $(CFLAGS)

CPPS = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)*.c)) 
ASMS = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)*.S)) 

obj-dir := obj
OBJS = $(addprefix ${obj-dir}/, $(notdir $(CPPS:.c=.o))) $(addprefix ${obj-dir}/, $(notdir $(ASMS:.S=.o))) 
workload-bin := llubenchmark

dest-dir := ${mroot}/workloads

build: ${workload-bin}

install:
	rm -rf ${dest-dir}
	cp -r . ${dest-dir}	

.PHONY: deps
deps:$(CPPS) $(ASMS) 
	$(CC) $(CFLAGS) -MM $^ > obj/deps
	sed '/^[^ ]/s/^/obj\//g' obj/deps > obj/deps.tmp
	mv obj/deps.tmp obj/deps
-include obj/deps

${workload-bin}: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	
${obj-dir}:
	mkdir ${obj-dir}

${obj-dir}/%.o: %.c | ${obj-dir}
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<

${obj-dir}/%.o: %.S | ${obj-dir}
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<


.PHONY: clean
clean:
	rm -rf ${obj-dir} ${workload-bin}