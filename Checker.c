#include "Base.h"
#include "Gui.h"

bool process(Game *game, FILE *input, GUI *gui)
{
    char line[64];
    Field field = { };
    Stats stats = { };
    Piece *cur = NULL;
    int rotation = 0, translation = 0;

    if(gui)
        gui_update(gui, &field, &stats, NULL, 0);

    while(!feof(input) && (cur || stats.pos < game->input_size))
    {
        fgets(line, sizeof(line), input);
        ++stats.instr;

        if(cur && strcmp(line, "MOVE LEFT\n") == 0)
            translation -= 1;
        else
        if(cur && strcmp(line, "MOVE RIGHT\n") == 0)
            translation += 1;
        else
        if(cur && strcmp(line, "ROTATE CCW\n") == 0)
            rotation = (rotation + 1)%4;
        else
        if(cur && strcmp(line, "ROTATE CW\n") == 0)
            rotation = (rotation + 3)%4;
        else
        if(!cur && strcmp(line, "NEW BLOCK\n") == 0)
        {
            rotation = translation = 0;
            cur = &game->piece[(int)game->input[stats.pos]];
        }
        else
        if(cur && strcmp(line, "DROP\n") == 0)
        {
            int xpos = translation - cur->form[rotation].translation;
            if(xpos < 0 || xpos + cur->form[rotation].width > FIELD_WIDTH)
            {
                fprintf( stderr, "Piece with translation %d and rotation %d "
                    "is outside field at instruction %d (piece %d)!\n",
                    translation, rotation, stats.instr, stats.pos );
                break;
            }

            if(gui)
                gui_update(gui, &field, &stats, &cur->form[rotation], xpos);

            if(!update_field(&field, &cur->form[rotation], xpos, &stats))
            {
                fprintf( stderr, "Piece does not fit with translation %d "
                    "and rotation %d at instruction %d (piece %d)!\n",
                    translation, rotation, stats.instr, stats.pos );
                break;
            }
            cur = NULL;
            ++stats.pos;
        }
        else
        if(strcmp(line, "DEBUG\n") == 0)
        {
            /*
            fprintf( stderr, "Debug at instruction %d (piece %d)\n",
                     stats.instr, stats.pos );
            */
        }
        else
        if(cur && strcmp(line, "DISCARD\n") == 0)
        {
            if(stats.discarded >= 5)
            {
                fprintf( stderr, "May not discard piece "
                    "at instruction %d (piece %d)\n", stats.instr, stats.pos );
                break;
            }
            ++stats.discarded;
            cur = NULL;
            ++stats.pos;
        }
        else
        {
            fprintf( stderr, "Unexpected input at "
                "instruction %d (piece %d)\n", stats.instr, stats.pos );
            break;
        }
    }

    print_stats(stdout, &stats);
    fflush(stdout);

    if(gui)
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
    gui = gui_create(game, "Checker");
    success = process(game, stdin, gui);

    if(gui)
        gui_destroy(gui);

    return success ? 0 : 1;
}
