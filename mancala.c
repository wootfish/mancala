/*
 * mancala.c
 *
 * Framework for playing Mancala.
 *
 * Ruleset: Four stones per house, six houses per side.
 * Capture takes all captured stones & the capturing stone.
 *
 * - Eli Sohl
 *   @elisohl
 *   sohliloquies.blogspot.com
 *
 */

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define NAIVE_MCTS_NUM_PATHS 200000
#define DEBUG 1
#define debug_print(...) \
    do { if (DEBUG) {fprintf(stderr, "[DEBUG] "); fprintf(stderr, __VA_ARGS__);} } while (0)

typedef struct {
    char p1_store;
    char p1[6];
    char p2[6];
    char p2_store;
} board;

typedef enum {TURN_OVER, EXTRA_TURN, ILLEGAL_MOVE, GAME_OVER} move_result;
typedef enum {PLAYER_1, PLAYER_2} player;

typedef char (*move_function)(board,player);

bool is_game_over(board *game_board);
move_result play_move(board *game_board, player which_player, char move);
void draw_board(board game_board, player whose_turn);
char get_move(board game_board, player which_player);
void play_game(board *game_board, move_function p1_player, move_function p2_player);
char pick_random_move(char side[]);
char pick_move_naive_mcts(board game_board, player which_player);

bool is_game_over(board *game_board) {
    int i;
    char p1_stones = 0;
    char p2_stones = 0;

    for (i = 0; i < 6; i++) {
        p1_stones += game_board->p1[i];
        p2_stones += game_board->p2[i];
    }

    if (p1_stones == 0 || p2_stones == 0) {
        for (i = 0; i < 6; i++) {
            game_board->p1[i] = 0;
            game_board->p2[i] = 0;
        }

        game_board->p1_store += p1_stones;
        game_board->p2_store += p2_stones;
        return true;
    }

    return false;
}

move_result play_move(board *game_board, player which_player, char move) {
    if (move < 0 || move > 5)
        return ILLEGAL_MOVE;
    if (which_player == PLAYER_1 && game_board->p1[move] == 0)
        return ILLEGAL_MOVE;
    if (which_player == PLAYER_2 && game_board->p2[move] == 0)
        return ILLEGAL_MOVE;

    player curr_side;
    char curr_house;
    int stones_left;

    curr_house = move;
    curr_side = which_player;
    stones_left = (which_player==PLAYER_1? game_board->p1 : game_board->p2)[move];
    (which_player==PLAYER_1? game_board->p1 : game_board->p2)[move] = 0;

    // distribute stones
    while (stones_left > 0) {
        if (curr_side == PLAYER_1) {
            switch (curr_house) {
                case 5:
                case 4:
                case 3:
                case 2:
                case 1:
                    curr_house--;
                    break;
                case 0:
                    if (which_player == PLAYER_1) {
                        curr_house--;
                    } else {
                        curr_side = PLAYER_2;
                    }
                    break;
                case -1:
                    curr_house++;
                    curr_side = PLAYER_2;
                    break;
            }
        } else {
            switch (curr_house) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                    curr_house++;
                    break;
                case 5:
                    if (which_player == PLAYER_2) {
                        curr_house++;
                    } else {
                        curr_side = PLAYER_1;
                    }
                    break;
                case 6:
                    curr_house--;
                    curr_side = PLAYER_1;
                    break;
            }
        }

        (curr_side==PLAYER_1? game_board->p1 : game_board->p2)[curr_house] += 1;
        stones_left--;
    }

    // did we end in a store?
    if (curr_house == 6 || curr_house == -1) {
        if (is_game_over(game_board)) return GAME_OVER;
        return EXTRA_TURN;
    }

    // did we capture?
    if (curr_side == PLAYER_1 &&
         which_player == PLAYER_1 &&
         game_board->p1[curr_house] == 1 &&
         game_board->p2[curr_house] > 0) {

        game_board->p1_store += game_board->p1[curr_house];
        game_board->p1_store += game_board->p2[curr_house];
        game_board->p1[curr_house] = 0;
        game_board->p2[curr_house] = 0;
        if (is_game_over(game_board)) return GAME_OVER;
        return TURN_OVER;

    } else if (curr_side == PLAYER_2 &&
               which_player == PLAYER_2 &&
               game_board->p1[curr_house] > 0 &&
               game_board->p2[curr_house] == 1) {

        game_board->p2_store += game_board->p1[curr_house];
        game_board->p2_store += game_board->p2[curr_house];
        game_board->p1[curr_house] = 0;
        game_board->p2[curr_house] = 0;
        if (is_game_over(game_board)) return GAME_OVER;
        return TURN_OVER;
    }

    if (is_game_over(game_board)) return GAME_OVER;
    return TURN_OVER;
}

void draw_board(board game_board, player whose_turn) {
    //          0     1     2     3     4     5
    //  /--\  /---\ /---\ /---\ /---\ /---\ /---\ /--\
    //  |  |  |   | |   | |   | |   | |   | |   | |  |
    //  |  |  \---/ \---/ \---/ \---/ \---/ \---/ |  |
    //  |  |                                      |  |
    //  |  |  /---\ /---\ /---\ /---\ /---\ /---\ |  |
    //  |  |  |   | |   | |   | |   | |   | |   | |  |
    //  \--/  \---/ \---/ \---/ \---/ \---/ \---/ \--/

    printf("\n");
    printf("                   0     1     2     3     4     5\n");
    printf("           /--\\  /---\\ /---\\ /---\\ /---\\ /---\\ /---\\ /--\\\n");

    if (whose_turn == PLAYER_1) {
        printf("(PLAYER 1) |  |  |%2d | |%2d | |%2d | |%2d | |%2d | |%2d | |  |\n",
                game_board.p1[0], game_board.p1[1], game_board.p1[2],
                game_board.p1[3], game_board.p1[4], game_board.p1[5]);
    } else {
        printf(" PLAYER 1  |  |  |%2d | |%2d | |%2d | |%2d | |%2d | |%2d | |  |\n",
                game_board.p1[0], game_board.p1[1], game_board.p1[2],
                game_board.p1[3], game_board.p1[4], game_board.p1[5]);
    }

    printf("           |  |  \\---/ \\---/ \\---/ \\---/ \\---/ \\---/ |  |\n");
    printf("           |%2d|                                      |%2d|\n",
            game_board.p1_store, game_board.p2_store);
    printf("           |  |  /---\\ /---\\ /---\\ /---\\ /---\\ /---\\ |  |\n");

    if (whose_turn == PLAYER_1) {
        printf("           |  |  |%2d | |%2d | |%2d | |%2d | |%2d | |%2d | |  |  PLAYER 2\n",
                game_board.p2[0], game_board.p2[1], game_board.p2[2],
                game_board.p2[3], game_board.p2[4], game_board.p2[5]);
    } else {
        printf("           |  |  |%2d | |%2d | |%2d | |%2d | |%2d | |%2d | |  | (PLAYER 2)\n",
                game_board.p2[0], game_board.p2[1], game_board.p2[2],
                game_board.p2[3], game_board.p2[4], game_board.p2[5]);
    }

    printf("           \\--/  \\---/ \\---/ \\---/ \\---/ \\---/ \\---/ \\--/\n");
    printf("\n");
}

char get_move(board game_board, player which_player) {
    int move_choice;

    if (which_player == PLAYER_1)
        printf("Please input move for player 1.\n");
    else
        printf("Please input move for player 2.\n");

    while (true) {
        printf("\n> ");
        fflush(stdout);
        if (scanf("%d", &move_choice) < 0) {
            debug_print("scanf returned error code.\n");
            continue;
        }

        if (move_choice < 0 || move_choice > 5) {
            debug_print("Move outside acceptable range\n");
            continue;
        }

        return (char) move_choice;
    }
}

void play_game(board *game_board, move_function p1_player, move_function p2_player) {
    player curr_player = PLAYER_1;
    char move;
    move_result result;

    while (true) {
        draw_board(*game_board, curr_player);

        if (curr_player == PLAYER_1) {
            debug_print("[play_game] Main loop: Player 1's turn.\n");
            move = (*p1_player)(*game_board, curr_player);
        } else {
            debug_print("[play_game] Main loop: Player 2's turn.\n");
            move = (*p2_player)(*game_board, curr_player);
        }

        result = play_move(game_board, curr_player, move);
        debug_print("[play_game] Player chose %d\n", move);

        switch (result) {
            case TURN_OVER:
                debug_print("[play_game] Turn over.\n");
                curr_player = curr_player^1;
                break;

            case ILLEGAL_MOVE:
                debug_print("[play_game] Illegal move!\n");
                printf("Sorry, you can't do that.\n");
            case EXTRA_TURN:
                debug_print("[play_game] Extra turn!\n");
                break;

            case GAME_OVER:
                printf("Game over! Final board:\n");
                if (game_board->p1_store > game_board->p2_store)
                    draw_board(*game_board, PLAYER_1);
                else
                    draw_board(*game_board, PLAYER_2);
                return;
        }
    }
}

char pick_random_move(char side[]) {
    int divisor;
    int move;

    divisor = RAND_MAX/6;
    do {
        do {
            move = rand() / divisor;
        } while (move > 5);
    } while (side[move] == 0);

    //debug_print("Picked random move %d (house has %d stones)\n", move, side[move]);

    return move;
}

char pick_move_naive_mcts(board game_board, player which_player) {
    // picks moves using naive monte carlo tree search
    // (randomly samples tree paths using random heuristic, attempts to
    // minimize chance of loss)
    board test_board;
    char first_move;
    int i;
    unsigned long int p1_count[6] = {0};
    unsigned long int p2_count[6] = {0};
    char *side;
    player curr_player = which_player;
    move_result result;

    printf("\nAI is thinking... (strategy: naive monte carlo tree search)\n");

    for (i = 0; i < NAIVE_MCTS_NUM_PATHS; i++) {
        memcpy(&test_board, &game_board, 14);
        first_move = pick_random_move(curr_player == PLAYER_1? test_board.p1 : test_board.p2);
        result = play_move(&test_board, curr_player, first_move);

        while (result != GAME_OVER) {
            if (result == ILLEGAL_MOVE) {
                printf("AI chose an illegal move! Debug this!!\n");
                exit(1);
            }

            if (result == TURN_OVER) {
                curr_player ^= 1;
            }

            result = play_move(&test_board, curr_player,
                    pick_random_move(curr_player == PLAYER_1? test_board.p1 : test_board.p2));
        }

        if (test_board.p1_store > test_board.p2_store) {
            p1_count[first_move] += 1;
        } else if (test_board.p1_store < test_board.p2_store) {
            p2_count[first_move] += 1;
        } else {
            // tie
            p1_count[first_move] += 1;
            p2_count[first_move] += 1;
        }
    }

    debug_print("p1_count: %lu %lu %lu %lu %lu %lu\n", p1_count[0], p1_count[1], p1_count[2], p1_count[3], p1_count[4], p1_count[5]);
    debug_print("p2_count: %lu %lu %lu %lu %lu %lu\n", p2_count[0], p1_count[1], p2_count[2], p2_count[3], p2_count[4], p2_count[5]);

    char best_move;
    int score_gap;
    int best_gap = -NAIVE_MCTS_NUM_PATHS;
    for (i = 0; i < 6; i++) {
        if (p1_count[i] == 0 && p2_count[i] == 0) continue;
        if (p1_count[i] == 0 && which_player == PLAYER_1) continue;
        if (p2_count[i] == 0 && which_player == PLAYER_2) continue;

        score_gap = p1_count[i] - p2_count[i];
        if (which_player == PLAYER_2) score_gap *= -1;
        if (score_gap >= best_gap) {
            best_move = i;
            best_gap = score_gap;
        }
    }

    debug_print("best_move: %d   best_gap: %d\n", best_move, best_gap);
    printf("\n    AI chose move #%d.\n\n", best_move);

    return best_move;
}

int main(int argc, char* argv[]) {
    time_t rand_seed = time(NULL);
    debug_print("Seeding RNG with value %ld\n", rand_seed);
    srand(rand_seed);

    board main_board = {0, 4, 4, 4, 4, 4, 4,
                        4, 4, 4, 4, 4, 4, 0};

    debug_print("play_game starting...\n");
    play_game(&main_board, &get_move, &pick_move_naive_mcts);
}

