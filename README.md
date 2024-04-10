# minic_compiler
Andrew W. Chen's miniC compiler for Compilers Spring 2024 

## Usage
```
make
./minic_compiler.out < parser_tests/p4.c
make clean
```

## Files
```
/parser_tests - provided tests
ast.c - provided
ast.h - provided
minic_compiler.c - contains main function
minic_compiler.l - lex file
minic_compiler.y - yacc file
semantic_analysis.c - contains the function for semantic analysis called by main function
semantic_analysis.h - header for semantic_analysis.c
```
