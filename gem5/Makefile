gem5-cli := ./build/ARM/gem5.opt
pim-device := link_list

${gem5-cli}:
	scons -j${build_cores} PIM_DEVICE=${pim-device} build/ARM/gem5.opt

run: ${gem5-cli}
	./build/ARM/gem5.opt configs/example/fs.py \
    	--machine-type=VExpress_EMM64 \
    	-n ${run_cores} \
		--mem-size=2048MB \
    	--disk-image=aarch64-ubuntu-trusty-headless.img \
    	--dtb-file=vexpress.aarch64.20140821.dtb \
		--kernel=vmlinux
	    # --kernel=vmlinux.aarch64.20140821
    	# --disk-type=f    	

clean:
	rm -rf build
	rm -f a.out
	rm -rf m5out
