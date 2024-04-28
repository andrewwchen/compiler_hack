LLVMCODE = llvm_parser
TEST = opt_tests/test

$(LLVMCODE): $(LLVMCODE).c
	g++ -g -I /usr/include/llvm-c-15/ -c $(LLVMCODE).c
	g++ $(LLVMCODE).o `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ -o $@

llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).ll

clean: 
	rm -rf $(TEST).ll
	rm -rf test_new.ll
	rm -rf $(LLVMCODE)
	rm -rf *.o