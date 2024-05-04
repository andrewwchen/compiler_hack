# minic_compiler
Andrew W. Chen's miniC compiler for Compilers Spring 2024 

# Part 1: Frontend

## Usage
```
make
./frontend.out < parser_tests/p4.c
make clean
```

## Files
```
/parser_tests - provided tests
ast.c - provided
ast.h - provided
frontend.c - contains main function
frontend.l - lex file
frontend.y - yacc file
semantic_analysis.c - contains the function for semantic analysis called by main function
semantic_analysis.h - header for semantic_analysis.c
```


# Part 3: Optimization

## Usage
first write a mini-c program to optimize into opt_tests/test.ll
```
make
make llvm_file
./llvm_parser opt_tests/test.ll
```
results in optimized test file test_new.ll
```
make clean
```

## Files
```
/opt_tests - provided tests
Core.h - provided
llvm_parser.c - main c file for optimizer
```
