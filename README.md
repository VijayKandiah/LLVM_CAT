# LLVM_CAT
LLVM Code Analysis and Transformation

## Prerequisites
LLVM 8.0.0

## Build
Copy a version of Catpass from passes/ into CAT-c/catpass/Catpass.cpp, i.e.
```
cp passes/Catpass-1.cpp CAT-c/catpass/Catpass.cpp
```
Compile and install your code by invoking ./CAT-c/run_me.sh
The script run_me.sh compiles and installs an LLVM-based compiler that includes your CAT in the directory ~/CAT

## Run
  1) Add your compiler cat-c in your PATH, i.e.
  ```
  export PATH=~/CAT/bin:$PATH
  ```

  2) Invoke your compiler to compile a C program. For example
	```
    $ cat-c program_to_analyse.c -o mybinary
    $ cat-c -O3 program_to_analyse.c -o mybinary
    $ cat-c -O0 program_to_analyse.bc -o mybinary
	```