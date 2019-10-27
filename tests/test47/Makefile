LLVM_SUFFIX= 
OPTIONS=-I../misc

AA6=-cfl-anders-aa
AA5=-scev-aa
AA4=-tbaa
AA3=-cfl-steens-aa
AA2=-globals-aa
AA1=-basicaa
AA=$(AA1) $(AA2) $(AA3) $(AA4) $(AA5) $(AA6)
LOOPS=-loops -loop-simplify -lcssa -domtree -scalar-evolution -assumption-cache-tracker

all: program_optimized.bc

program_optimized.bc: program.bc
	../misc/run_test.sh "$(AA) $(LOOPS)" "$<" "$@" 0
	llvm-dis$(LLVM_SUFFIX) $@

program.bc: program.c
	clang$(LLVM_SUFFIX) $(OPTIONS) -O1 -emit-llvm -c $<

program.ll: program.bc
	llvm-dis$(LLVM_SUFFIX) $^

program: program.bc ../misc/CAT.bc
	clang$(LLVM_SUFFIX) -O3 -lm $^ -o $@

program_optimized: program_optimized.bc ../misc/CAT.bc
	clang$(LLVM_SUFFIX) -O3 -lm $^ -o $@

program_optimized.ll: program_optimized.bc
	llvm-dis$(LLVM_SUFFIX) $^

program_output: program
	./$< > $@

program_optimized_output: program_optimized
	./$< > $@

check: program_optimized_output
	../misc/check.sh "output/oracle.txt" "program_optimized_output"

install: program.bc program.ll
	cp *.bc *.ll bitcode/

oracle: program_optimized_output
	mkdir -p output
	mv $^ output/oracle.txt

clean:
	rm -rf program program_optimized program_optimized_output *.bc *.ll a.out diff core* program_output compiler_output *.txt;
	cp bitcode/* . ;
