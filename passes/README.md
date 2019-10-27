# Passes

## CatPass-0.cpp 
Prints function name and body for every bitcode function in a program.

## CatPass-1.cpp 
Prints CAT-API function name and number of instructions that invoke it for each bitcode function in a program.

## CatPass-2.cpp 
Computes and prints GEN and KILL sets (analyzed for only CAT variables) for every bitcode instruction of a program under the following assumptions:
### Assumptions
1. A C variable used to store the return value of CAT_new (i.e., reference to a CAT variable) is defined statically not more than once in the C function it has been declared.
2. A C variable that includes a reference to a CAT variable cannot be copied to other C variables (no aliasing).
3. A C variable that includes a reference to a CAT variable cannot be copied into a data structure.
4. A C variable that includes a reference to a CAT variable does not escape the C function where it has been declared.

## CatPass-3.cpp 
Computes and prints IN and OUT sets (analyzed for only CAT variables) for every bitcode instruction of a program under previously stated assumptions.

## CatPass-4.cpp 
Performs intra-procedural Constant Propagation, Constant Folding, and Dead Code Elimination for CAT variables under previously stated assumptions.

## CatPass-5.cpp 
Performs intra-procedural Constant Propagation, Constant Folding, and Dead Code Elimination for CAT variables while removing all assumptions stated above.

## CatPass-6.cpp 
Reduces the conservativeness of the previous pass by using AliasAnalyis to compute reaching definitions.

## CatPass-7.cpp 
Uses Function-Inlining to perform inter-procedural Constant Propagation, Constant Folding, and Dead Code Elimination for CAT variables. Also added a naive constant folding pass for non-CAT variables.

## CatPass-8.cpp 
Made the reaching definition analysis inter-procedural to enable constant propagation to function input parameters and function return value propagation to call site. Added Function Cloning, Loop Unrolling and Loop Peeling.

## CatPass-9.cpp 
Various optimizations to considerably improve compilation time of the pass. Replaces unnecessary conditional branches with unconditional branches to further enable Constant Propagation and Constant Folding passes.

## CatPass-final.cpp 
Added Loop-Rotation to enable/unblock further loop optimizations such as Loop Peeling which in-turn unblocks Constant Propagation and Constant Folding passes. 
