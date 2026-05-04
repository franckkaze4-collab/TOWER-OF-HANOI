/* ============================================================
 *  Chapter 5 — C Functions
 *  Exercise 5.39: Towers of Hanoi
 *
 *  MODIFIED: Step-by-step output with disk size, move counter,
 *            and a visual peg state after every move.
 *
 *  Compile: gcc -Wall -o hanoi hanoi.c
 *  Run:     ./hanoi
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>

/* ============================================================
 *  Global variables for step tracking
 * ============================================================ */
int step        = 0;   /* current move number          */
int total_moves = 0;   /* total moves expected (2^n-1) */
int num_disks   = 0;   /* total number of disks        */

/* ============================================================
 *  Peg state: pegs[peg][0] = number of disks on that peg
 *             pegs[peg][1..n] = disk sizes (bottom to top)
 *  Peg index: 1, 2, 3  (we use index 0 unused for clarity)
 * ============================================================ */
#define MAX_DISKS 10
int pegs[4][MAX_DISKS + 1];  /* pegs[peg][0] = count */

/* ============================================================
 *  init_pegs()
 *  Places all disks on from_peg at the start.
 *  Disk sizes: largest = n, smallest = 1 (bottom to top).
 * ============================================================ */
void init_pegs(int n, int from_peg) {
    int i, p;

    /* Clear all pegs */
    for (p = 1; p <= 3; p++)
        for (i = 0; i <= MAX_DISKS; i++)
            pegs[p][i] = 0;

    /* Stack disks on from_peg: largest at bottom */
    pegs[from_peg][0] = n;
    for (i = 0; i < n; i++)
        pegs[from_peg][i + 1] = n - i;  /* bottom = n, top = 1 */
}

/* ============================================================
 *  move_disk()
 *  Moves the top disk from src peg to dst peg in the state.
 * ============================================================ */
void move_disk(int src, int dst) {
    int top_index = pegs[src][0];          /* count = index of top disk */
    int disk_size = pegs[src][top_index];  /* size of that disk         */

    /* Remove from source */
    pegs[src][top_index] = 0;
    pegs[src][0]--;

    /* Place on destination */
    pegs[dst][0]++;
    pegs[dst][ pegs[dst][0] ] = disk_size;
}

/* ============================================================
 *  print_state()
 *  Draws a simple ASCII picture of all three pegs after a move.
 *
 *  Example for 3 disks:
 *
 *    Peg 1       Peg 2       Peg 3
 *    |           |           |
 *    |          [1]          |
 *    |         [===]         |
 *   [=====]     |            |
 *   -------   -------   -------
 *
 * ============================================================ */
void print_state(void) {
    int row, p;
    int height = num_disks;   /* rows to draw (one per disk slot) */

    printf("\n");
    printf("    Peg 1         Peg 2         Peg 3\n");

    /* Print from top row down to bottom row */
    for (row = height; row >= 1; row--) {
        printf("  ");
        for (p = 1; p <= 3; p++) {
            int count     = pegs[p][0];
            int disk_size = (row <= count) ? pegs[p][row] : 0;

            if (disk_size == 0) {
                /* Empty slot — just the pole */
                printf("    |         ");
            } else {
                /* Draw disk as '=' characters, centred in 9 chars */
                int width  = disk_size * 2 - 1;  /* 1->1, 2->3, 3->5, ... */
                int pad    = (9 - width) / 2;
                int i;
                printf("  ");
                for (i = 0; i < pad; i++)    printf(" ");
                printf("[");
                for (i = 0; i < width; i++)  printf("=");
                printf("]");
                for (i = 0; i < pad; i++)    printf(" ");
                printf("  ");
            }
        }
        printf("\n");
    }

    /* Base line */
    printf("  ");
    for (p = 1; p <= 3; p++)
        printf("  =========   ");
    printf("\n\n");
}

/* ============================================================
 *  hanoi()
 *
 *  Recursive Towers of Hanoi — now prints a full step block:
 *    Step X / Y  |  Move disk [size] : peg A --> peg B
 *    [ASCII peg diagram]
 * ============================================================ */
void hanoi(int n, int from_peg, int to_peg, int temp_peg) {

    if (n == 1) {
        /* --- Base case: move the single disk --- */
        step++;
        int disk_size = pegs[from_peg][ pegs[from_peg][0] ];
        move_disk(from_peg, to_peg);

        printf("  Step %2d / %d  |  Move disk [size %d] :  Peg %d  -->  Peg %d\n",
               step, total_moves, disk_size, from_peg, to_peg);
        print_state();
        return;
    }

    /* Step 1: Move top n-1 disks from source to temp */
    hanoi(n - 1, from_peg, temp_peg, to_peg);

    /* Step 2: Move the largest disk from source to destination */
    step++;
    int disk_size = pegs[from_peg][ pegs[from_peg][0] ];
    move_disk(from_peg, to_peg);

    printf("  Step %2d / %d  |  Move disk [size %d] :  Peg %d  -->  Peg %d\n",
           step, total_moves, disk_size, from_peg, to_peg);
    print_state();

    /* Step 3: Move the n-1 disks from temp to destination */
    hanoi(n - 1, temp_peg, to_peg, from_peg);
}

/* ============================================================
 *  main()
 * ============================================================ */
int main(void) {

    int from_peg, to_peg, temp_peg;

    printf("====================================\n");
    printf("      TOWERS OF HANOI SOLVER        \n");
    printf("====================================\n");

    /* --- Input: number of disks --- */
    printf("Enter number of disks (1-%d)   : ", MAX_DISKS);
    while (scanf("%d", &num_disks) != 1 || num_disks < 1 || num_disks > MAX_DISKS) {
        printf("[!] Please enter a number between 1 and %d: ", MAX_DISKS);
        while (getchar() != '\n');
    }

    /* --- Input: starting peg --- */
    printf("Enter starting peg    (1/2/3) : ");
    while (scanf("%d", &from_peg) != 1 || from_peg < 1 || from_peg > 3) {
        printf("[!] Peg must be 1, 2 or 3: ");
        while (getchar() != '\n');
    }

    /* --- Input: destination peg --- */
    printf("Enter destination peg (1/2/3) : ");
    while (scanf("%d", &to_peg) != 1 ||
           to_peg < 1 || to_peg > 3 ||
           to_peg == from_peg) {
        printf("[!] Peg must be 1, 2 or 3 and different from start: ");
        while (getchar() != '\n');
    }

    /* --- Calculate temporary peg (1+2+3=6, so temp = 6-from-to) --- */
    temp_peg    = 6 - from_peg - to_peg;
    total_moves = (1 << num_disks) - 1;   /* 2^n - 1 */

    /* --- Initialise peg state --- */
    init_pegs(num_disks, from_peg);

    /* --- Summary --- */
    printf("\nMoving %d disk(s) from peg %d to peg %d "
           "(using peg %d as temp):\n",
           num_disks, from_peg, to_peg, temp_peg);
    printf("Total moves expected: %d  (= 2^%d - 1)\n", total_moves, num_disks);
    printf("------------------------------------\n");

    /* --- Show initial state --- */
    printf("\n  Initial state:\n");
    print_state();
    printf("------------------------------------\n\n");

    /* --- Solve step by step --- */
    hanoi(num_disks, from_peg, to_peg, temp_peg);

    /* --- Done --- */
    printf("====================================\n");
    printf("  DONE! All %d disk(s) moved in %d step(s).\n",
           num_disks, total_moves);
    printf("====================================\n");

    return 0;
}
