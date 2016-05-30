all:
	@echo Making all programs.  If this fails, try "'make no_root'".
	if [ ! -d bin ]; then mkdir bin; fi
	cd add_mca && make
	cd add_tree && make
	cd analyze_mca && make
	cd chn2mca && make
	cd contract_mca && make
	cd mca2txt && make
	cd spx2mca && make
	cd sum_mca && make
	cd tree2binnedavgtxt && make
	cd tree2mca_gated && make
	cd tree2tree && make
	cd tree_gate_intensity && make
	cd txt2binnedavgtxt && make
	cd txt2mca && make
no_root:
	@echo Making programs which don"'"t depend on ROOT.
	if [ ! -d bin ]; then mkdir bin; fi
	cd add_mca && make
	cd analyze_mca && make
	cd chn2mca && make
	cd contract_mca && make
	cd mca2txt && make
	cd spx2mca && make
	cd sum_mca && make
	cd txt2binnedavgtxt && make
	cd txt2mca && make
	

clean:
	rm bin/*
