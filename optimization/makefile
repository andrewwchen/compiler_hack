LLVMCODE = llvm_parser
TEST = test
TEST_NAME = p1_cfold
TEST1 = p1_cfold
TEST2 = p2_common_subexpr
TEST3 = p3_const_prop

$(LLVMCODE): $(LLVMCODE).c
	gcc -g -I /usr/include/llvm-c-15/ -c $(LLVMCODE).c
	g++ $(LLVMCODE).o `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ -o $@

llvm_file: $(TEST).c
	clang -S -emit-llvm $(TEST).c -o $(TEST).ll

clean: 
	rm -rf $(TEST).ll
	rm -rf $(TEST)_new.ll
	rm -rf $(LLVMCODE)
	rm -rf *.o

test: 
	make
	cp $(TEST_NAME).c $(TEST).c
	make llvm_file
	./llvm_parser $(TEST).ll
	diff $(TEST)_new.ll $(TEST_NAME).ll