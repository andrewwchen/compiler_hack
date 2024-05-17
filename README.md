# minic_compiler
Andrew W. Chen's miniC compiler for Compilers Spring 2024 

## Usage
```
make clean
make
./minic_compiler.out {.c file} {.ll file}
make clean
```
creates "output_unoptimized.ll" from .c file
creates "output_optimized.ll" from .ll file
You can optimize the c file by setting .ll file to "output_unoptimized.ll"