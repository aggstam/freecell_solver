// -------------------------------------------------------------
//
// This program solves freecell solitaire puzzles using four algorithms:
// - Depth first search
// - Breadth first search
// - Best first search
// - A*
// N is defined as a constant.
// Puzzles are read from an input file, while solution is written
// to an output file.
//
// Author: Aggelos Stamatiou, April 2016
//
// --------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Enable DEBUG mode
//#define DEBUG
// Constants denoting the four algorithms.
#define breadth     1
#define depth       2
#define best        3
#define astar       4
// Constants denoting the four moves.
#define foundation  0
#define newstack    1        
#define stack       2
#define freecell    3
// Constants denoting the four suits.
#define HEARTS      0
#define SPADES      1
#define DIAMONDS    2
#define CLUBS       3

// Max card number, provided by the first line of input file
int N;

// Cart structure.
struct card {
    int suit;
    int value;
};

// Tree's node structure.
struct tree_node {
    struct card board[16][52];
    int tops[16];
    int h;                          // The value of the heuristic function for this node.
    int g;                          // The depth of this node .
    int f;                          // f=0 or f=h or f=h+g, depending on the search algorithm used.
    struct tree_node* parent;       // Pointer to the parrent node (NULL for the root).
    int move;                       // The last move.
    struct card moved0, moved1;     // The card moved and the one it landed on if used stack.
    struct tree_node* children[20]; // Pointer to a table with 20 pointers to the childerns (NULL if no child).
};

// Frontier's node structure.
struct frontier_node{
    struct tree_node* n;            // Pointer to a search-tree node.
    struct frontier_node* previous; // Pointer to the previous frontier node.
    struct frontier_node* next;     // Pointer to the next frontier node.
};

struct frontier_node* frontier_head = NULL; // The one end of the frontier.
struct frontier_node* frontier_tail = NULL; // The other end of the frontier.

clock_t t1;         // Start time of the search algorithm.
clock_t t2;         // End time of the search algorithm.
#define TIMEOUT 300 // Program terminates after TIMOUT secs.

int solution_length;     // The lenght of the solution table.
int* solution;           // Pointer to a dynamic table with the moves of the solution.
struct card* sol_moved0; // Pointer to a dynamic table with the moved cards of the solution.
struct card* sol_moved1; // Pointer to a dynamic table with the cards the moved card landed if used stack.

int mem_error; // Constant for errors while allocating memory. If mem_error -1 programm exhausted all available memory and terminates. 

// Auxiliary function that displays a message in case of wrong input parameters.
void syntax_message() {
    printf("project_freecell <method> <input-file> <output-file>\n\n");
    printf("where: ");
    printf("<method> = breadth|depth|best|astar\n");
    printf("<input-file> is a file containing a %dx%d puzzle description.\n", N, N);
    printf("<output-file> is the file where the solution will be written.\n");
}

// Reading run-time parameters.
int get_method(char* s) {
    if (strcmp(s, "breadth") == 0) {
        return  breadth;
    } else if (strcmp(s, "depth") == 0) {
        return depth;
    } else if (strcmp(s, "best") == 0) {
        return best;
    } else if (strcmp(s, "astar") == 0) {
        return astar;
    }
    
    return -1;
}

// Function that displays the board on the screen.
// Inputs:
//        struct card board[16][52]: Board to display.
//          int tops[16]: Board tops array.
void display_board(struct card board[16][52], int tops[16]) {
    for (int i = 0; i < 16; i++) {
        printf("top: %d\n", tops[i]);
        for (int j = 0; j < 52; j++) {
            if (board[i][j].value == -1) {
                continue;
            }
            
            if (board[i][j].suit == 0) {
                printf("H");
            } else if (board[i][j].suit == 1) {
                printf("S");
            } else if (board[i][j].suit == 2) {
                printf("D");
            } else if (board[i][j].suit == 3) {
                printf("C");
            }
            
            printf("%d ", board[i][j].value);
        }
        
        printf("\n");
    }
}

// This function adds a pointer to a new leaf search-tree node at the front of the frontier.
// This function is called by the depth-first search algorithm.
// Inputs:
//        struct tree_node* node: A (leaf) search-tree node.
// Output:
//        0 --> The new frontier node has been added successfully.
//        -1 --> Memory problem when inserting the new frontier node.
int add_frontier_front(struct tree_node* node) {
    #ifdef DEBUG
        printf("Adding to the front:\n");
        display_board(node->board, node->tops);
    #endif

    // Creating the new frontier node.
    struct frontier_node* new_frontier_node = (struct frontier_node*) malloc(sizeof(struct frontier_node));
    if (new_frontier_node == NULL) {
        return -1;
    }

    new_frontier_node->n = node;
    new_frontier_node->previous = NULL;
    new_frontier_node->next = frontier_head;

    if (frontier_head == NULL) {
        frontier_head = new_frontier_node;
        frontier_tail = new_frontier_node;
        return 0;
    }
    
    frontier_head->previous = new_frontier_node;
    frontier_head = new_frontier_node;

    return 0;
}

// This function adds a pointer to a new leaf search-tree node at the back of the frontier.
// This function is called by the breadth-first search algorithm.
// Inputs:
//        struct tree_node* node: A (leaf) search-tree node.
// Output:
//        0 --> The new frontier node has been added successfully.
//        -1 --> Memory problem when inserting the new frontier node .
int add_frontier_back(struct tree_node* node) {
    #ifdef DEBUG
        printf("Adding to the back...\n");
        display_board(node->board, node->tops);
    #endif

    // Creating the new frontier node.
    struct frontier_node* new_frontier_node = (struct frontier_node*) malloc(sizeof(struct frontier_node));
    if (new_frontier_node == NULL) {
        return -1;
    }

    new_frontier_node->n = node;
    new_frontier_node->next = NULL;
    new_frontier_node->previous = frontier_tail;

    if (frontier_tail == NULL) {
        frontier_head = new_frontier_node;
        frontier_tail = new_frontier_node;
        return 0;
    }
    
    frontier_tail->next = new_frontier_node;
    frontier_tail = new_frontier_node;

    return 0;
}

// This function adds a pointer to a new leaf search-tree node within the frontier.
// The frontier is always kept in decreasing order with the f values of the corresponding
// search-tree nodes. The new frontier node is inserted in order.
// This function is called by the heuristic search algorithm.
// Inputs:
//        struct tree_node* node: A (leaf) search-tree node.
// Output:
//        0 --> The new frontier node has been added successfully.
//        -1 --> Memory problem when inserting the new frontier node .
int add_frontier_in_order(struct tree_node* node) {
    #ifdef DEBUG
        printf("Adding in order (f=%d)...\n", node->f);
        display_board(node->board, node->tops);
    #endif

    // Creating the new frontier node.
    struct frontier_node* new_frontier_node = (struct frontier_node*) malloc(sizeof(struct frontier_node));
    if (new_frontier_node == NULL) {
        return -1;
    }

    new_frontier_node->n = node;
    new_frontier_node->previous = NULL;
    new_frontier_node->next = NULL;

    if (frontier_head == NULL) {
        frontier_head = new_frontier_node;
        frontier_tail = new_frontier_node;
        return 0;
    }
    
    struct frontier_node* pt = frontier_head;
    // Search in the frontier for the first node that corresponds to either a smaller f value
    // or to an equal f value but smaller h value.
    // Note that for the best first search algorithm, f and h values coincide.
    while (pt != NULL && (pt->n->f > node->f || (pt->n->f == node->f && pt->n->h > node->h))) {
        pt = pt->next;
    }

    if (pt == NULL) {
        // New_frontier_node is inserted at the back of the frontier.
        frontier_tail->next = new_frontier_node;
        new_frontier_node->previous = frontier_tail;
        frontier_tail = new_frontier_node;
        return 0;
    }

    // new_frontier_node is inserted before pt .
    if (pt->previous != NULL) {
        pt->previous->next = new_frontier_node;
        new_frontier_node->next = pt;
        new_frontier_node->previous = pt->previous;
        pt->previous = new_frontier_node;
        return 0;
    }
    
    // In this case, new_frontier_node becomes the first node of the frontier.
    new_frontier_node->next = pt;
    pt->previous = new_frontier_node;
    frontier_head = new_frontier_node;

    return 0;
}

// This function generates a new puzzle board.
// Inputs:
//        struct card[16][52] board: The board to initialize.
//        int[16] tops: Board tops array.
void generate_board(struct card board[16][52], int tops[16]) {
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 52; j++) {
            board[i][j].suit = -1;
            board[i][j].value = -1;
        }
        tops[i] = -1;
    }
}

// This function reads a file containing a puzzle.
// Inputs:
//        char* filename: The name of the file containing a freecell solitaire puzzle.
//        struct card[16][52] puzzle: The puzzle.
//        int[16] tops: Board tops array.
// Output:
//        0 --> Successful read.
//        1 --> Unsuccessful read
int read_puzzle(char* filename, struct card puzzle[16][52], int tops[16]) {
    FILE *fin;
    int i, j;
    char c;
    struct card filecard;

    fin = fopen(filename, "r");
    if (fin == NULL) {
        printf("Cannot open file %s. Program terminates.\n", filename);
        return -1;
    }

    // Extracting N value
    fscanf(fin, "%d ", &N);

    // Initializing the puzzle board.
    printf("Building puzzle with N: %d\n", N);
    generate_board(puzzle, tops);

    // Reading lines
    char* buffer = NULL;
    size_t bufsize = 0;
    size_t characters;
    i = 0;
    while ((characters = getline(&buffer, &bufsize, fin)) != -1) {
        j = 0;
        for (int jj = 0; jj < (int)characters; jj++) {
            if ((buffer[jj] == ' ') || buffer[jj] == '\n') {
                continue;
            }
            char suit = buffer[jj];
            puzzle[i][j].suit = 0;
            if (suit == 'S') {
                puzzle[i][j].suit = 1;
            } else if (suit == 'D') {
                puzzle[i][j].suit = 2;
            } else if (suit == 'C') {
                puzzle[i][j].suit = 3;
            }
            jj++;
            puzzle[i][j].value = buffer[jj] - '0';
            j++;
            tops[i]++;
        }
        i++;
    }

    fclose(fin);

    return 0;
}

// This function checks whether a board of a node is a solution board.
// Inputs:
//        struct tree_node* current: A node
// Outputs:
//        1 --> The puzzle is a solution puzzle
//        0 --> The puzzle is NOT a solution puzzle
int is_solution(struct tree_node* current) {
    if ((current->tops[12] == (N - 1))
        && (current->tops[13] == (N - 1))
        && (current->tops[14] == (N - 1))
        && (current->tops[15] == (N - 1))) {
        return 1;
    }
    return 0;
}

// This function moves a card to the foundation with the same suit.
void move_to_foundation(struct tree_node* child, int from, int to) {
    child->tops[to]++;
    child->board[to][child->tops[to]].suit = child->board[from][child->tops[from]].suit;
    child->board[to][child->tops[to]].value = child->board[from][child->tops[from]].value;
    child->board[from][child->tops[from]].suit = -1;
    child->board[from][child->tops[from]].value = -1;
    child->tops[from]--;
}

// This function moves an ACE to a free foundation.
void move_to_empty_foundation(struct tree_node* child, int from) {
    for (int i = 12; i < 16; i++) {
        if (child->tops[i] != -1) {
            continue;
        }
        child->tops[i]++;
        child->board[i][child->tops[i]].suit = child->board[from][child->tops[from]].suit;
        child->board[i][child->tops[i]].value = child->board[from][child->tops[from]].value;
        child->board[from][child->tops[from]].suit = -1;
        child->board[from][child->tops[from]].value = -1;
        child->tops[from]--;
        break;
    }
}

// This function moves a card to free stack.
void move_to_new_stack(struct tree_node* child, int from) {
    for (int i = 0; i < 8; i++) {
        if (child->tops[i] != -1) {
            continue;
        }
        child->tops[i]++;
        child->board[i][child->tops[i]].suit = child->board[from][child->tops[from]].suit;
        child->board[i][child->tops[i]].value = child->board[from][child->tops[from]].value;
        break;
    }
    child->board[from][child->tops[from]].suit = -1;
    child->board[from][child->tops[from]].value = -1;
    child->tops[from]--;
}

// This function moves a card to another stack.
void move_to_stack(struct tree_node* child, int from, int to) {
    child->tops[to]++;
    child->board[to][child->tops[to]].suit = child->board[from][child->tops[from]].suit;
    child->board[to][child->tops[to]].value = child->board[from][child->tops[from]].value;
    child->board[from][child->tops[from]].suit = -1;
    child->board[from][child->tops[from]].value = -1;
    child->tops[from]--;
}

// This function moves a card to a freecell.
void move_to_a_freecell(struct tree_node* child, int from) {
    if (child->tops[8] == -1) {
        child->tops[8]++;
        child->board[8][0].suit = child->board[from][child->tops[from]].suit;
        child->board[8][0].value = child->board[from][child->tops[from]].value;
    } else if (child->tops[9] == -1) {
        child->tops[9]++;
        child->board[9][0].suit = child->board[from][child->tops[from]].suit;
        child->board[9][0].value = child->board[from][child->tops[from]].value;
    } else if (child->tops[10] == -1) {
        child->tops[10]++;
        child->board[10][0].suit = child->board[from][child->tops[from]].suit;
        child->board[10][0].value = child->board[from][child->tops[from]].value;
    } else if (child->tops[11] == -1) {
        child->tops[11]++;
        child->board[11][0].suit = child->board[from][child->tops[from]].suit;
        child->board[11][0].value = child->board[from][child->tops[from]].value;
    }

    child->board[from][child->tops[from]].suit = -1;
    child->board[from][child->tops[from]].value = -1;
    child->tops[from]--;
}

// This function checks whether two boards are qual.
int equal_nodes(struct tree_node* n, struct tree_node* p) {
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 52; j++) {
            if ((n->board[i][j].suit != p->board[i][j].suit) || (n->board[i][j].value != p->board[i][j].value)) {
                return 0;
            }
        }
    }

    return 1;
}

// This function checks whether a node in the search tree
// holds exactly the same board with at least one of its
// predecessors. This function is used when creating the childs
// of an existing search tree node, in order to check for each one of the childs
// whether this appears in the path from the root to its parent.
// This is a moderate way to detect loops in the search.
// Inputs:
//        struct tree_node* new_node: A search tree node (usually a new one)
// Output:
//        1 --> No coincidence with any predecessor
//        0 --> Loop detection
int check_with_parents(struct tree_node* new_node) {
    struct tree_node* parent = new_node->parent;
    while (parent != NULL) {
        if (equal_nodes(new_node, parent)) {
            return 0;
        }
        parent = parent->parent;
    }
    return 1;
}

// Computes the sum of the freecells of the board.
int freecells_count(struct tree_node* node) {
    int score = 0;
    if (node->tops[8] == -1) {
        score++;
    }
    if (node->tops[9] == -1) {
        score++;
    }
    if (node->tops[10] == -1) {
        score++;
    }
    if (node->tops[11] == -1) {
        score++;
    }

    return score;
}

// Computes the sum of the cards at foundations of the board.
int num_cards_at_foundations(struct tree_node* node) {
    int score = 0;
    if (node->tops[12] != -1) {
        score += node->tops[12];
        score++;
    }
    if (node->tops[13] != -1) {
        score += node->tops[13];
        score++;
    }
    if (node->tops[14] != -1) {
        score += node->tops[14];
        score++;
    }
    if (node->tops[15] != -1) {
        score += node->tops[15];
        score++;
    }

    return score * 10;
}

// Computes the sum of the freestacks of the board.
int freestacks_count(struct tree_node* node) {
    int score = 0;
    for (int i = 0; i < 8; i++) {
        if (node->board[i][node->tops[i]].value == -1) {
            score++;
        }
    }

    return score * 5;
}

// This function returns the score of the current board, based on:
// -Num of cards at foundations * 10
// -Freestacks count
// -Freecells count
// The score is ncaf - fs - fc. This was generated from the idea that the more
// spread the cards are, the bigger the chance to get a card to foundations. So
// the board should not have empty freecells or stacks.
int heuristic(struct tree_node* node) {
    return num_cards_at_foundations(node) - freestacks_count(node) - freecells_count(node);
}

// Evaluates the child node generated by
// computing the evaluation function value based on the search method used.
void evaluate_child(struct tree_node* child_node, int method) {
    if (method == best) {
        child_node->f = heuristic(child_node);
    } else if (method == astar) {
        child_node->f = child_node->g + heuristic(child_node);
    } else {
        child_node->f = 0;
    }
}

// Create Child Node.
void create_child(struct tree_node* current_node, int move, int p, int method, int from, int to) {
    int i, j;
    struct tree_node* child_node = (struct tree_node*) malloc(sizeof(struct tree_node));
    if (child_node == NULL) {
        mem_error = -1;
        return;
    }
    current_node->children[p] = child_node;

    for (i = 0; i < 20; i++) {
        child_node->children[i] = NULL;
    }

    child_node->parent = current_node;
    child_node->move = move;
    child_node->g = current_node->g + 1; // The depth of the new child.

    // Computing the puzzle for the new child.
    // Copy all positions.
    for (i = 0; i < 16; i++) {
        for (j = 0; j < 52; j++) {
            child_node->board[i][j].suit = current_node->board[i][j].suit;
            child_node->board[i][j].value = current_node->board[i][j].value;
        }
        child_node->tops[i] = current_node->tops[i];
    }

    // Change those that are different.
    child_node->moved0 = current_node->board[from][current_node->tops[from]];

    if (move == foundation) {
        if (to == 0) {
            move_to_empty_foundation(child_node, from);
        } else {
            move_to_foundation(child_node, from, to);
        }
    } else if (move == newstack) {
        move_to_new_stack(child_node, from);
    } else if (move == stack) {
        child_node->moved1 = current_node->board[to][current_node->tops[to]];
        move_to_stack(child_node, from, to);
    } else {
        move_to_a_freecell(child_node, from);
    }

    // Check for loops.
    if (!check_with_parents(child_node)) {
        // In case of loop detection, the child is deleted.
        free(child_node);
        current_node->children[p] = NULL;
        return;
    }

    // Computing the heuristic value
    evaluate_child(child_node, method);
}

// This function expands a leaf-node of the search tree.
// Inputs:
//        struct tree_node* current_node: A leaf-node of the search tree.
// Output:
//        The same leaf-node expanded with pointers to its children (if any).
void find_children(struct tree_node* current_node, int method) {
    int i, j, jj;
    j = 0;
    for (i = 0; i < 12; i++) {
		if ((current_node->tops[i] == -1)) {
			continue;
		}
		// Check for foundation.
		if (current_node->board[i][current_node->tops[i]].value == 0) {
		    // Move to an empty foudnation.
			create_child(current_node, foundation, j, method, i, 0);
			j++;
			continue;
		}

		for (jj = 12; jj < 16; jj++) {
			if (current_node->board[i][current_node->tops[i]].suit == current_node->board[jj][current_node->tops[jj]].suit) {
				if (current_node->board[i][current_node->tops[i]].value == current_node->board[jj][current_node->tops[jj]].value + 1) {
				    // Move to a foundation with cards.
					create_child(current_node, foundation, j, method, i, jj);
					j++;
					break;
				}
			}
		}

		// Check for another stack.
		for (jj = 0; jj < 8; jj++) {
		    if ((current_node->tops[jj] == -1)) {
			    continue;
		    }
			if (((current_node->board[i][current_node->tops[i]].suit == HEARTS) || (current_node->board[i][current_node->tops[i]].suit == DIAMONDS))
			    && ((current_node->board[jj][current_node->tops[jj]].suit == SPADES) || (current_node->board[jj][current_node->tops[jj]].suit == CLUBS))) {
				if (current_node->board[i][current_node->tops[i]].value == current_node->board[jj][current_node->tops[jj]].value - 1) {
					create_child(current_node, stack, j, method, i, jj);
					j++;
				}
			} else if (((current_node->board[i][current_node->tops[i]].suit == SPADES) || (current_node->board[i][current_node->tops[i]].suit == CLUBS))
			            && ((current_node->board[jj][current_node->tops[jj]].suit == HEARTS) || (current_node->board[jj][current_node->tops[jj]].suit == DIAMONDS))) {
				if (current_node->board[i][current_node->tops[i]].value == current_node->board[jj][current_node->tops[jj]].value - 1) {
					create_child(current_node, stack, j, method, i, jj);
					j++;
				}
			} else if (current_node->tops[jj] == -1) {
				create_child(current_node, newstack, j, method, i, jj);
				j++;
				break;
			}
		}

		if (i != 8 && i != 9 && i != 10 && i != 11) {
			// Check for a freecell.
			for (jj = 8; jj < 12; jj++) {
				if (current_node->tops[jj] == -1) {
					create_child(current_node, freecell, j, method, i, jj);
					j++;
					break;
				}
			}
		}
	}
}

// This function initializes the search, i.e. it creates the root node of the search tree
// and the first node of the frontier.
// Inputs:
//        struct card[16][52] puzzle: The puzzle.
//        int[16] tops: Board tops array.
//        int method: Search method to execute.
void initialize_search(struct card puzzle[16][52], int tops[16], int method) {
    int i, j, jj;
    // Initialize search tree.
    struct tree_node* root = (struct tree_node*) malloc(sizeof(struct tree_node));
    if (root == NULL) {
        printf("Root creation failed.\n");
        mem_error = -1;
        return;
    }

    generate_board(root->board, root->tops);
    root->parent = NULL;
    root->move = -1;

    for (jj = 0; jj<20; jj++) {
        root->children[jj] = NULL;
    }

    for (i = 0; i < 8; i++) {
        for (j = 0; j <= tops[i]; j++) {
            root->board[i][j].suit = puzzle[i][j].suit;
            root->board[i][j].value = puzzle[i][j].value;
        }
        root->tops[i] = tops[i];
    }

    root->g = 0;
    root->h = heuristic(root);
    if (method == best) {
        root->f = root->h;
    } else if (method == astar) {
        root->f = root->g + root->h;
    } else {
        root->f = 0;
    }

    #ifdef DEBUG
        printf("Root puzzle:\n");
        display_board(root->board, root->tops);
    #endif

    add_frontier_front(root);
}

// This function fill the last stack of the board with the remaining cards.
struct tree_node* complete_solution(struct tree_node* node, int method) {
    for (int i = 0; i < 12; i++) {
        if (node->tops[i] == -1) {
            continue;
        }
        if (node->board[i][node->tops[i]].value == 0) {
            create_child(node, foundation, 0, method, i, 0);
            return complete_solution(node->children[0], method);
        }
        for (int j = 12; j < 16; j++) {
            if ((node->board[i][node->tops[i]].suit != node->board[j][node->tops[j]].suit)
                || (node->board[i][node->tops[i]].value != node->board[j][node->tops[j]].value + 1)) {
               continue; 
            }
            create_child(node, foundation, 0, method, i, j);
            return complete_solution(node->children[0], method);
        }
    }

    return node;
}

// This function implements at the higest level the search algorithms.
// The various search algorithms differ only in the way the insert
// new nodes into the frontier, so most of the code is commmon for all algorithms.
// Inputs:
//        Nothing, except for the global variables root, frontier_head and frontier_tail.
// Output:
//        NULL --> The problem cannot be solved
//        struct tree_node*: A pointer to a search-tree leaf node that corresponds to a solution.
struct tree_node* search(int method) {
    clock_t t;
    int i, err;
    struct frontier_node *current_node;

    while (frontier_head != NULL) {
        t = clock();
        if ((t - t1) > (CLOCKS_PER_SEC * TIMEOUT)) {
            printf("Timeout\n");
            return NULL;
        }

        // Extract the first node from the frontier.
        current_node = frontier_head;

        // Check if its a solution
        int count = 0;
        for (i = 12; i < 16; i++) {
            if (current_node->n->tops[i] == -1) {
                continue;
            }
            if (current_node->n->board[i][current_node->n->tops[i]].value == N - 1) {
                count++;
            }
        }
        if (count == 3) {
            return complete_solution(current_node->n, method);
        }        

        // Find the children of the frontier node.
        find_children(current_node->n, method);
        if (mem_error == -1) {
            printf("Memory exhausted while creating new child node. Search is terminated...\n");
            return NULL;
        }

        // Add children to frontier.
        for (i = 0; i < 20; i++) {
            if (current_node->n->children[i] == NULL) {
                break;
            }
            if (method == depth) {
                err = add_frontier_front(current_node->n->children[i]);
            } else if (method == breadth) {
                err = add_frontier_back(current_node->n->children[i]);
            } else {
                err = add_frontier_in_order(current_node->n->children[i]);
            }
            if (err < 0) {
                printf("Memory exhausted while creating new frontier node. Search is terminated...\n");
                return NULL;
            }
        }

        // Shift frontier head
        if (current_node->previous != NULL) {
            // Current node is no longer frontier head, so we link
            // its previous with its next node.
            current_node->previous->next = current_node->next;
        } else {
            // Node is still frontier head, so we move to next one.
            frontier_head = current_node->next;
            if (frontier_head != NULL) {
                frontier_head->previous = NULL;
            }
        }

        // Free node.
        free(current_node);
    }

    return NULL;
}

// Giving a (solution) leaf-node of the search tree, this function computes
// the moves that have to be done, starting from the root puzzle, in order to
// go to the leaf node's puzzle.
// Inputs:
//        struct tree_node* solution_node: A leaf-node
// Output:
//        The sequence of moves that have to be done, starting from the root puzzle, in order
//        to receive the leaf-node's puzzle, is stored into the global variable solution.
void extract_solution(struct tree_node* solution_node) {
    struct tree_node* temp_node = solution_node;
    solution_length = solution_node->g;
    
    solution = (int*)malloc(solution_length*sizeof(int));
    sol_moved0 = (struct card*)malloc(solution_length*sizeof(struct card));
    sol_moved1 = (struct card*)malloc(solution_length*sizeof(struct card));
    if ((solution == NULL) || (sol_moved0 == NULL) || (sol_moved1 == NULL)) {
        mem_error = -1;
        return;
    }
    temp_node = solution_node;
    int i = solution_length;
    while (temp_node->parent != NULL) {
        i--;
        solution[i] = temp_node->move;
        sol_moved0[i] = temp_node->moved0;
        if (temp_node->move == 2) {
            sol_moved1[i] = temp_node->moved1;
        }
        temp_node = temp_node->parent;
    }
}

// This function writes the solution into a file
// Inputs:
//        char* filename: The name of the file where the solution will be written.
// Outputs:
//        Nothing (apart from the new file)
void write_solution_to_file(char* filename, int solution_length, int *solution, struct card* m0, struct card* m1) {
    FILE *fout = fopen(filename, "w");
    if (fout == NULL) {
        printf("Cannot open output file to write solution.\n");
        printf("Now exiting...");
        return;
    }
    fprintf(fout, "K = %d\n", solution_length);
    for (int i = 0; i < solution_length; i++) {
        if (solution[i] == foundation) {
            fprintf(fout, "foundation ");
        } else if (solution[i] == newstack) {
            fprintf(fout, "newstack ");
        } else if (solution[i] == stack) {
            fprintf(fout, "stack ");
        } else {
            fprintf(fout, "freecell ");
        }

        if (m0[i].suit == 0) {
            fprintf(fout, "H");
        } else if (m0[i].suit == 1) {
            fprintf(fout, "S");
        } else if (m0[i].suit == 2) {
            fprintf(fout, "D");
        } else {
            fprintf(fout, "C");
        }
        fprintf(fout, "%d ", m0[i].value);
        
        if (solution[i] != stack) {
            fprintf(fout, "\n");
            continue;
        }

        if (m1[i].suit == 0) {
            fprintf(fout, "H");
        } else if (m1[i].suit == 1) {
            fprintf(fout, "S");
        } else if (m1[i].suit == 2) {
            fprintf(fout, "D");
        } else {
            fprintf(fout, "C");
        }
        fprintf(fout, "%d\n", m1[i].value);
            
    }
}

int main(int argc, char** argv) {
    int err;
    struct tree_node* solution_node;
    struct card puzzle[16][52]; // The initial puzzle read from a file.
    int method; // The search algorithm that will be used to solve the puzzle.
    int tops[16];

    method = get_method(argv[1]);
    if (method<0) {
        printf("Wrong method. Use correct syntax:\n");
        syntax_message();
        return -1;
    }

    // Parsing puzzle
    read_puzzle(argv[2], puzzle, tops);

    printf("Solving %s using %s...\n", argv[2], argv[1]);
    t1 = clock();

    initialize_search(puzzle, tops, method);
    // TODO: fix algo
    solution_node = search(method); // The main call.

    t2 = clock();
    
    if (solution_node == NULL) {
        printf("No solution found.\n");
        return 0;
    }

    extract_solution(solution_node);
    if (mem_error == -1) {
        printf("Memory exhausted while creating solution path...\n");
        return 0;
    }
    
    if (solution_length == 0) {
        printf("No solution found.\n");
        return 0;
    }
    
    printf("Solution found! (%d steps)\n", solution_length);
    printf("Time spent: %f secs\n", ((float)t2 - t1) / CLOCKS_PER_SEC);
    write_solution_to_file(argv[3], solution_length, solution, sol_moved0, sol_moved1);

    return 0;
}
