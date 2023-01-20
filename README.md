# freecell_solver

This program solves freecell solitaire puzzles using four algorithms:
- Depth first search
- Breadth first search
- Best first search
- A*

Max card number per stack is defined as a constant in the code.
<br>
Puzzles are read from an input file, while solution is written to an output file.

# Usage
## Make usage
```
% make
```
To use a different method:
```
% make METHOD={method}
```
To use a different input file:
```
% make FILE={input_file}
```
To configure output file name:
```
% make OUTPUT={output_file}
```

## Direct usage
Compilation:
```
% gcc -o project_freecell project_freecell.c
```
Execution:
```
% ./project_freecell {method} {input_file} {output_file}
```
