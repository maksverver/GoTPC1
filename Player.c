#include "Base.h"
#include "Gui.h"

#define INF             999999999
#define SEARCH_DEPTH            3

Game *game;

int evaluate(const Field *field, int score)
{
    int x, y, boundaries = 0, h;

    for(y = 0; y < FIELD_HEIGHT; ++y)
    {
        if(!field->tile[0][y])
            ++boundaries;
        if(!field->tile[FIELD_WIDTH - 1][y])
            ++boundaries;
        for(x = 1; x < FIELD_WIDTH; ++x)
            if(field->tile[x - 1][y] != field->tile[x][y])
                ++boundaries;
    }

    for(x = 0; x < FIELD_WIDTH; ++x)
    {
        if(!field->tile[x][0])
            ++boundaries;
        for(y = 1; y < FIELD_HEIGHT; ++y)
            if(field->tile[x][y - 1] != field->tile[x][y])
                ++boundaries;
    }

    h = 0;
    for(x = 0; x < FIELD_WIDTH; ++x)
        h += field->top[x]*field->top[x];

    return 1*score - boundaries - h;
/*
    for(x = 1; x < FIELD_WIDTH; ++x)
    {
        int n = field->top[x - 1] - field->top[x];
        if(n < 0)
            n = -n;
        if(n > 2)
            val -= (2 + n)*(2 + n);
    }

    for(x = 0; x < FIELD_WIDTH; ++x)
    {
        val -= (field->top[x])*(field->top[x]);
        for(y = 0; y < field->top[x]; ++y)
            if(!field->tile[x][y])
                val -= 50;
    }

    return val;
*/
}

int search(const Field *field, int pos, int score, int depth, Move *best_move)
{
    Piece *piece = &game->piece[(int)game->input[pos]];
    int best = -INF, val, rot, x, lines;

    if(pos >= game->input_size)
        return 0;

    if(depth <= 0)
        return evaluate(field, score);

    for(rot = 0; rot < piece->forms; ++rot)
    {
        Field new_field;
        for(x = 0; x + piece->form[rot].width <= FIELD_WIDTH; ++x)
        {
            new_field = *field;
            lines = place(&new_field, &piece->form[rot], x);
            if(lines >= 0)
            {
                val = search( &new_field, pos + 1, score + lines_score[lines],
                              depth - 1, NULL);
                if(val > best)
                {
                    best = val;
                    if(best_move)
                    {
                        best_move->form = rot;
                        best_move->xpos = x;
                    }
                }
            }
        }
    }
    return best;
}

int main(int argc, char *argv[])
{
    Field field = { };
    Stats stats = { };
    GUI *gui;

    game = load_game((argc < 2) ? "." : argv[1]);
    if(!game)
    {
        fprintf(stderr, "Could not load game.\n");
        return 1;
    }

    gui = gui_create(game, "Player");

    if(gui)
        gui_update(gui, &field, &stats, NULL, 0);

    for(stats.pos = 0; stats.pos < game->input_size; ++stats.pos)
    {
        Move best_move;
        Form *form;

        if(search(&field, stats.pos, 0, SEARCH_DEPTH, &best_move) < -INF/2)
        {
            fprintf(stderr, "No suitable move found.\n");
            break;
        }

        form = &game->piece[(int)game->input[stats.pos]].form[best_move.form];

        if(gui)
            gui_update(gui, &field, &stats, form, best_move.xpos);

        if(!update_field(&field, form, best_move.xpos, &stats))
        {
            fprintf(stderr, "INTERNAL ERROR: invalid move selected!\n");
            break;
        }

        print_move(stdout, game, best_move, &stats);
        fflush(stdout);
    }

    print_stats(stderr, &stats);
    fflush(stderr);

    if(gui)
    {
        gui_wait(gui);
        gui_destroy(gui);
    }

    return 0;
}
