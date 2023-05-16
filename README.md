# freecell_solver

This program solves freecell solitaire puzzles using four algorithms:
- Depth first search [1]
- Breadth first search [2]
- Best first search [3]
- A* [4]

Puzzles are read from an input file, while solution is written to an output file.
<br>
Max card number per stack is defined in the first line of the input file.
<br>
Three input files have been provided to play with.
<br>
Note: Breadth first search will result in OOM for max card number > 4.
<br>
This is due to algorithms nature, not implementation.

## Usage
### Make usage
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

### Direct usage
Compilation:
```
% gcc -o project_freecell project_freecell.c
```
Execution:
```
% ./project_freecell {method} {input_file} {output_file}
```

## Execution example
```
❯ make
gcc -o project_freecell project_freecell.c
./project_freecell depth test_file_size_5.txt output.txt
Building puzzle with N: 5
Solving test_file_size_5.txt using depth...
Solution found! (30 steps)
Time spent: 0.001146 secs
```

## References
[1] https://en.wikipedia.org/wiki/Depth-first_search
<br>
[2] https://en.wikipedia.org/wiki/Breadth-first_search
<br>
[2] https://en.wikipedia.org/wiki/Best-first_search
<br>
[2] https://en.wikipedia.org/wiki/A*_search_algorithm
