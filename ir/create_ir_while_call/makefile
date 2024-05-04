LLVMCODE = llvm_builder
TEST = test
$(LLVMCODE): $(LLVMCODE).c
	clang -g `llvm-config-15 --cflags` -I /usr/include/llvm-c-15/ -c $(LLVMCODE).c
	clang++ `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ $(LLVMCODE).o -o $@

minic:
	./$(LLVMCODE)
	clang main.c $(TEST).ll -o $(TEST).out

llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).s

clean: 
	rm -rf $(TEST).ll
	rm -rf $(LLVMCODE)
	rm -rf *.o
	rm -rf *.out
