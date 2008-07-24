#ifndef BASE_H
#define BASE_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PIECE_SIZE       5
#define NUM_PIECES      10
#define FIELD_WIDTH     15
#define FIELD_HEIGHT    40

typedef struct Form
{
    int width, height;
    char tile[PIECE_SIZE][PIECE_SIZE];
    int bottom[PIECE_SIZE], top[PIECE_SIZE];
    int rotation, translation;
    char id;
} Form;

typedef struct Piece
{
    int forms;
    Form form[4];
} Piece;

typedef struct Field
{
    char tile[FIELD_WIDTH][FIELD_HEIGHT];
    int top[FIELD_WIDTH];
} Field;

typedef struct Game
{
    Piece piece[NUM_PIECES];

    int input_size;
    char input[32];
} Game;

typedef struct Move
{
    int form, xpos;
} Move;

typedef struct Stats
{
    int pos, instr, score, discarded, dropped, cleared[6];
} Stats;

const int lines_score[6];

void print_form(FILE *fp, const Form *form);
void print_field(FILE *fp, const Field *field);
void print_stats(FILE *fp, const Stats *stats);
void print_move(FILE *fp, const Game *game, Move move, Stats *stats);

Game *load_game(const char *dir);
bool load_piece(Piece *piece, char id, const char *filepath);
int place(Field *field, const Form *form, int xpos);
bool update_field(Field *field, const Form *form, int xpos, Stats *stats);

#endif /* ndef BASE_H */
