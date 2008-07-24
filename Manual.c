#include "Base.h"
#include "Gui.h"

bool process(Game *game, FILE *input, GUI *gui)
{
    Field field = { };
    Stats stats = { };
    Move move;

    while(stats.pos < game->input_size)
    {
        Piece *piece;

        gui_update(gui, &field, &stats, NULL, 0);
        if(!gui_manual_move(gui, &move))
            return false;

        piece = &game->piece[(int)game->input[stats.pos]];

        if(move.form < 0)
        {
            if(stats.discarded >= 5)
            {
                fprintf(stderr, "No discard available at piece %d!\n", stats.pos);
                break;
            }
            ++stats.discarded;
        }
        else
        {
            if(move.xpos < 0 || move.xpos + piece->form[move.form].width > FIELD_WIDTH)
            {
                fprintf( stderr, "Piece with translation %d and rotation %d "
                    "is outside field at piece %d!\n",
                    move.xpos, move.form, stats.pos );
                break;
            }

            if(!update_field(&field, &piece->form[move.form], move.xpos, &stats))
            {
                fprintf( stderr, "Piece does not fit with translation %d "
                    "and rotation %d at piece %d!\n",
                    move.xpos, move.form, stats.pos );
                break;
            }
        }

        print_move(stdout, game, move, &stats);
        fflush(stdout);

        ++stats.pos;
    }

    print_stats(stderr, &stats);
    fflush(stderr);

    gui_wait(gui);

    return true;
}

int main(int argc, char *argv[])
{
    Game *game;
    GUI *gui;
    bool success;

    game = load_game((argc < 2) ? "." : argv[1]);
    if(!game)
    {
        fprintf(stderr, "Could not load game.\n");
        return 1;
    }

    gui = gui_create(game, "Manual");
    if(!gui)
    {
        fprintf(stderr, "Could not create graphical user interface!\n");
        return 1;
    }

    success = process(game, stdin, gui);
    gui_destroy(gui);

    return success ? 0 : 1;
}
