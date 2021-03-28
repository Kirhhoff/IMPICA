export driver-dir := $(PWD)/pim_driver

run:
	make -C pim_driver build
	make -C pim_lib test
	make -C gem5 run

connect:
	make -C gem5/util/term connect

clean:
	make -C pim_driver clean
	make -C pim_lib clean
	make -C gem5/util/term clean

clean-all:
	make -C pim_driver clean
	make -C pim_lib clean
	make -C gem5/util/term clean
	make -C gem5 clean