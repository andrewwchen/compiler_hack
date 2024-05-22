# compiler_hack
Andrew Chen and Charlie Childress's miniC program similarity scorer. Scores two miniC programs' similarity from 0 (low similarity) to 100 (high similarity).

## Formula
We use abstract syntax trees to calculate structure and logic similarity between the two miniC programs.
```
maximum_structure_difference = tree_size_1 + tree_size_2
structure_similarity = 1 - (tree_edit_distance / maximum_structure_difference)

logic_difference = number of nodes present in one tree but not the other
maximum_logic_difference = max(tree_num_nodes_1, tree_num_nodes_2)
logic_similarity =  1 - (logic_difference / maximum_logic_difference)

similarity = 80 * structure_similarity + 20 * logic_similarity 
```
We first convert each miniC program into AST trees.
We count and compare the frequencies of each node in each tree to clculate logic similarity.

We then convert each AST tree into string bracket format.
We then used Tree Similarity Library (https://github.com/DatabaseGroup/tree-similarity) to calculate the tree edit distance between the two trees in string bracket format.
We use the tree edit distance to calculate structure similarity.


## Usage
```
make clean
make
./main.out {.c file 1} {.c file 2}
make clean
```
