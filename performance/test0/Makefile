ITERS=1
MAX_ITERS=0
INCLUDES_TO_CONSIDER=-I../misc
AA6=-cfl-anders-aa
AA5=-scev-aa
AA4=-tbaa
AA3=-cfl-steens-aa
AA2=-globals-aa
AA1=-basicaa
AA=$(AA1) $(AA2) $(AA3) $(AA4) $(AA5) $(AA6)
LOOPS=-loops -loop-simplify -lcssa -domtree -scalar-evolution -assumption-cache-tracker

all: program_optimized.bc program.bc

program.bc: program.c
	clang ${INCLUDES_TO_CONSIDER} -O0 -Xclang -disable-O0-optnone -fno-inline -emit-llvm -c $< -o $@
	opt -mem2reg $@ -o $@
	llvm-dis $@

program_optimized.bc: program.bc
	../misc/time.sh $< $@ $(ITERS) "$(AA) $(LOOPS)" $(MAX_ITERS)

program: program.bc ../misc/CAT.bc
	clang -O3 -lm $^ -o $@

program_optimized: program_optimized.bc ../misc/CAT.bc
	clang -O3 -lm $^ -o $@

program_output: program
	./$< > $@

program_optimized_output: program_optimized
	./$< > $@
	../misc/remove_last_line.sh $@

check: program_optimized_output
	../misc/check.sh "output/oracle.txt" "program_optimized_output"

clean:
	rm -f program program_optimized program_optimized_output *.bc *.ll a.out diff/* tmpOut time_pass program_output compiler_output.txt;
	rm -f output_code_iter_*.bc ;
	cp bitcode/* . ;

.PHONY: clean check all
