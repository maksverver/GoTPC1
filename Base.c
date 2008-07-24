#include "Base.h"

const int lines_score[6] = { 10, 60, 160, 310, 510, 760 };

void print_form(FILE *fp, const Form *form)
{
    int x, y;
    for(y = form->height - 1; y >= 0; --y)
    {
        for(x = 0; x < form->width; ++x)
            fputc(form->tile[x][y] ? '#' : '.', fp);
        fputc('\n', fp);
    }
}

void print_field(FILE *fp, const Field *field)
{
    int x, y;
    for(y = FIELD_HEIGHT - 1; y >= 0; --y)
    {
        for(x = 0; x < FIELD_WIDTH; ++x)
            fputc(field->tile[x][y] ? '#' : '.', fp);
    }
    for(x = 0; x < FIELD_WIDTH; ++x)
        fprintf(fp, "%d%c", field->top[x], (x == FIELD_WIDTH - 1) ? '\n' : ' ');
}

void print_stats(FILE *fp, const Stats *stats)
{
    fprintf( fp,
        "Pieces processed:          %8d\n"
        "Instructions processed:    %8d\n"
        "Pieces discarded:          %8d\n"
        "Pieces dropped:            %8d\n"
        "   No lines cleared:       %8d\n"
        "   Cleared 1 line(s):      %8d\n"
        "   Cleared 2 line(s):      %8d\n"
        "   Cleared 3 line(s):      %8d\n"
        "   Cleared 4 line(s):      %8d\n"
        "   Cleared 5 line(s):      %8d\n"
        "Score:                     %8d\n",
        stats->pos, stats->instr, stats->discarded, stats->dropped,
        stats->cleared[0], stats->cleared[1], stats->cleared[2],
        stats->cleared[3], stats->cleared[4], stats->cleared[5],
        stats->score + 400*(5 - stats->discarded) );
}


bool form_equivalent(const Form *f, const Form *g)
{
    int x, y;

    if(f->width != g->width || f->height != g->height)
        return false;

    for(x = 0; x < f->width; ++x)
        for(y = 0; y < f->height; ++y)
            if((bool)f->tile[x][y] != (bool)g->tile[x][y])
                return false;

    return true;
}

/* Rotates 'f' clockwise into 'g', setting 'width', 'height', 'rotation'
   and 'tile' fields. */
void rotate_form(const Form *f, Form *g)
{
    int x, y;
    g->width  = f->height;
    g->height = f->width;
    g->rotation = f->rotation + 1;
    g->id = f->id;
    for(x = 0; x < f->width; ++x)
        for(y = 0; y < f->height; ++y)
            g->tile[f->height - y - 1][x] = f->tile[x][y];
}

bool load_piece(Piece *piece, char id, const char *filepath)
{
    Form base[4] = { };
    int x, y, xpivot, ypivot, rot;

    base[0].id = id;

    FILE *fp = fopen(filepath, "rt");
    if(!fp)
        return false;
    else
    {
        /* Read description */
        char line[5][32] = { };
        int max_x = -1, max_y = -1, min_x = 999, min_y = 999;
        for(y = 0; y < 5; ++y)
        {
            if(fscanf(fp, "%31s", line[y]) != 1)
                break;
            if(strlen(line[y]) != strlen(line[0]))
                return false;
        }

        /* Determine boundaries */
        for(y = 0; y < 5; ++y)
            for(x = 0; x < 5; ++x)
                if(line[y][x] == '1' || line[y][x] == 'X')
                {
                    if(x < min_x) min_x = x;
                    if(y < min_y) min_y = y;
                    if(x > max_x) max_x = x;
                    if(y > max_y) max_y = y;
                }
        if(max_x < 0)
            return false;

        /* Initialize base form */
        xpivot = -1;
        for(y = min_y; y <= max_y; ++y)
            for(x = min_x; x <= max_x; ++x)
                if(line[y][x] == '1' || line[y][x] == 'X')
                {
                    if(line[y][x] == 'X')
                    {
                        if(xpivot != -1)
                            return false;
                        xpivot = x - min_x;
                        ypivot = y - min_y;
                    }
                    base[0].tile[x - min_x][y - min_y] = id;
                }
        if(xpivot < 0)
            return false;
        base[0].width  = max_x - min_x + 1;
        base[0].height = max_y - min_y + 1;
    }

    /* Flip vertically */
    for(y = 0; y < base[0].height/2; ++y)
        for(x = 0; x < base[0].width; ++x)
        {
            char tmp = base[0].tile[x][base[0].height - y - 1];
            base[0].tile[x][base[0].height - y - 1] = base[0].tile[x][y];
            base[0].tile[x][y] = tmp;
        }
    ypivot = base[0].height - 1 - ypivot;

    /* Compute rotated forms */
    for(rot = 1; rot < 4; ++rot)
        rotate_form(&base[rot - 1], &base[rot]);

    /* Set translation for rotated forms */
    base[0].translation = xpivot                     - FIELD_WIDTH/2;
    base[1].translation = base[1].width - ypivot - 1 - FIELD_WIDTH/2;
    base[2].translation = base[2].width - xpivot - 1 - FIELD_WIDTH/2;
    base[3].translation = ypivot                     - FIELD_WIDTH/2;

    /* Compute bottom/top profile */
    for(rot = 0; rot < 4; ++rot)
        for(x = 0; x < base[rot].width; ++x)
        {
            base[rot].bottom[x] = base[rot].top[x] = -1;
            for(y = 0; y < base[rot].height; ++y)
            {
                if(base[rot].tile[x][y])
                {
                    base[rot].bottom[x] = y;
                    while(y < base[rot].height)
                        if(base[rot].tile[x][y++])
                            base[rot].top[x] = y;
                    break;
                }
            }
        }

    /* Assign results */
    for(rot = 0; rot < 4; ++rot)
        piece->form[rot] = base[rot];
    for(piece->forms = 1; piece->forms < 4; ++piece->forms)
        if(form_equivalent(&base[0], &base[piece->forms]))
            break;
    return true;
}

int place(Field *field, const Form *form, int xpos)
{
    int n, m, x, y, cleared = 0, ypos = 0;

    /* Determine y-position of block */
    for(n = 0; n < form->width; ++n)
    {
        if(form->bottom[n] >= 0)
        {
            m = field->top[xpos + n] - form->bottom[n];
            if(m > ypos)
                ypos = m;
        }
    }

    if(ypos + form->height > FIELD_HEIGHT)
        return -1;

    for(n = 0; n < form->width; ++n)
    {
        for(m = 0; m < form->height; ++m)
            if(form->tile[n][m])
                field->tile[xpos + n][ypos + m] = form->tile[n][m];
        if(form->top[n] >= 0)
            field->top[xpos + n] = ypos + form->top[n];
    }

    for(y = ypos + form->height - 1; y >= ypos; --y)
    {
        for(x = 0; x < FIELD_WIDTH; ++x)
            if(!field->tile[x][y])
                goto noline;
        ++cleared;
        for(n = 0; n < FIELD_WIDTH; ++n)
        {
            --field->top[n];
            for(m = y; m < field->top[n]; ++m)
                field->tile[n][m] = field->tile[n][m + 1];
            field->tile[n][(int)field->top[n]] = 0;
            while(field->top[n] && !field->tile[n][field->top[n] - 1])
                --field->top[n];
        }
    noline:
        continue;
    }

    return cleared;
}

Game *load_game(const char *dir)
{
    char path[1024];
    Game *game;
    FILE *fp;
    long size, n;

    if(strlen(dir) > sizeof(path) - 32)
    {
        fprintf(stderr, "Directory name too long!\n");
        return NULL;
    }

    /* Read game data */
    sprintf(path, "%s/game.txt", dir);
    fp = fopen(path, "rt");
    if(!fp)
    {
        fprintf(stderr, "Unable to open game data from file \"%s\"!\n", path);
        return NULL;
    }
    if(fseek(fp, 0, SEEK_END) == -1)
        return NULL;
    size = ftell(fp);
    if(size == -1)
        return NULL;
    rewind(fp);
    game = malloc(sizeof(*game) + size - sizeof(game->input));
    game->input_size = size;
    if(fread(game->input, 1, size, fp) != size)
    {
        free(game);
        return NULL;
    }
    for(n = 0; n < size; ++n)
    {
        if(game->input[n] < '0' || game->input[n] > '9')
        {
            fprintf(stderr, "Invalid character in game data (%d)\n", (int)game->input[n]);
            free(game);
            return NULL;
        }
        game->input[n] -= '0';
    }
    fclose(fp);

    /* Read pieces */
    for(n = 0; n < 10; ++n)
    {
        sprintf(path, "%s/%d.txt", dir, (int)n);
        if(!load_piece(&game->piece[n], n + 1, path))
        {
            fprintf( stderr, "Unable to load piece %d from file \"%s\"!\n",
                     (int)n, path );
            free(game);
            return NULL;
        }
    }

    return game;
}

bool update_field(Field *field, const Form *form, int xpos, Stats *stats)
{
    int lines = place(field, form, xpos);
    if(lines < 0)
        return false;
    stats->score += lines_score[lines];
    ++stats->dropped;
    ++stats->cleared[lines];
    return true;
}

void print_move(FILE *fp, const Game *game, Move move, Stats *stats)
{
    if(move.form < 0)
    {
        fputs("DISCARD\n", fp);
        ++stats->instr;
    }
    else
    {
        const Form *form = &game->piece[(int)game->input[stats->pos]].form[move.form];

        fputs("NEW BLOCK\n", fp);
        ++stats->instr;
        while(move.form > 0)
        {
            fputs("ROTATE CCW\n", fp);
            ++stats->instr;
            --move.form;
        }
        while(move.xpos + form->translation < 0)
        {
            fputs("MOVE LEFT\n", fp);
            ++stats->instr;
            ++move.xpos;
        }
        while(move.xpos + form->translation > 0)
        {
            fputs("MOVE RIGHT\n", fp);
            ++stats->instr;
            --move.xpos;
        }
        fputs("DROP\n", fp);
        ++stats->instr;
    }
}
