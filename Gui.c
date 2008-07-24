#include "Gui.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/time.h>
void usleep(unsigned long usec);

#define SCALE           10
#define STATS_WIDTH    175

struct GUI
{
    Game    *game;
    Stats   *stats;
    Field   *field;
    Form    *form;
    int     xpos;

    Display *display;
    Window  window;
    GC      gc;

    XColor block_color[22][3];

    long long last_update;
    int     last_game_pos;
    int     update_delay;
    int     rotation;
    bool    discard, drop, abort;
};

int block_color[22][3] = {
    {  32,  32,  32 }, {   0, 255, 204 }, { 255, 128, 178 }, { 255, 102,   0 },
    { 255, 212,  42 }, {  44, 160,  90 }, { 215, 238, 244 }, { 160,  44,  44 },
    { 205, 135, 222 }, {  42, 212, 255 }, { 204, 255,   0 }, {  64,  64,  64 } };
const double block_shade[3] = { 0.7, 0.9, 1.0 };

static long long utime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 1000000ll*tv.tv_sec + tv.tv_usec;

}

GUI *gui_create(Game *game, const char *window_title)
{
    char *display_name;
    Display *display;
    GUI *gui;
    XTextProperty window_title_property;

    /* Create window properties */
    if(!XStringListToTextProperty((char**)&window_title, 1, &window_title_property))
        return NULL;

    /* Open X display */
    display_name = getenv("DISPLAY");
    if(!display_name || !*display_name)
        display_name = ":0";
    display = XOpenDisplay(display_name);
    if(!display)
        return NULL;

    /* Create GUI structure */
    gui = malloc(sizeof(*gui));
    memset(gui, 0, sizeof(*gui));
    if(!gui)
    {
        XCloseDisplay(gui->display);
        return NULL;
    }
    gui->game           = game;
    gui->display        = display;
    gui->update_delay   = 6;
    gui->last_update    = 0;
    gui->last_game_pos  = -1;

    /* Create window */
    {
        int screen_width, screen_height, width, height;
        XSizeHints* size_hints;

        screen_width  = DisplayWidth(display, 0);
        screen_height = DisplayWidth(display, 0);
        width  = STATS_WIDTH + 21*SCALE;
        height = 45*SCALE;
        gui->window = XCreateSimpleWindow( display, RootWindow(display, 0),
            (screen_width + width)/2, (screen_height + height)/2, width, height,
            2, BlackPixel(display, 0), BlackPixel(display, 0) );
        XSetWMName(display, gui->window, &window_title_property);
        XMapWindow(display, gui->window);

        size_hints = XAllocSizeHints();
        size_hints->flags = PSize | PMinSize | PMaxSize;
        size_hints->min_width  = size_hints->max_width =
            size_hints->base_width  = width;
        size_hints->min_height = size_hints->base_height =
             size_hints->max_height = height;
        XSetWMNormalHints(display, gui->window, size_hints);
        XFree(size_hints);

        XSelectInput(display, gui->window, ExposureMask | KeyPressMask);
    }

    /* Create graphics context and load fonts */
    {
        gui->gc = XCreateGC(display, gui->window, 0, 0);
    }

    /* Allocate colors */
    {
        Visual* default_visual = DefaultVisual(display, DefaultScreen(display));
        Colormap colormap = XCreateColormap(display, gui->window, default_visual, AllocNone);
        int n, m;

        for(n = 12; n < 22; ++n)
            for(m = 0; m < 3; ++m)
                block_color[n][m] = block_color[n - 11][m]/3;

        for(n = 0; n < 22; ++n)
        {
            for(m = 0; m < 3; ++m)
            {
                gui->block_color[n][m].red =
                    (int)(block_shade[m]*257*block_color[n][0] + 0.5);
                gui->block_color[n][m].green =
                    (int)(block_shade[m]*257*block_color[n][1] + 0.5);
                gui->block_color[n][m].blue =
                    (int)(block_shade[m]*257*block_color[n][2] + 0.5);
                XAllocColor(display, colormap, &gui->block_color[n][m]);
            }
        }
    }
    XSync(display, false);

    return gui;
}

void gui_destroy(GUI *gui)
{
    XFreeGC(gui->display, gui->gc);
    XCloseDisplay(gui->display);
    free(gui);
}

void gui_process(GUI *gui, bool blocking)
{
    XEvent event;
    while( blocking ? XNextEvent(gui->display, &event), true
                    : XCheckMaskEvent(gui->display, -1, &event) )
    {
        blocking = false;

        switch(event.type)
        {
        case KeyPress:
            {
                switch(XKeycodeToKeysym(gui->display, event.xkey.keycode, 0))
                {
                case XK_equal:
                    if(gui->update_delay > 0)
                        --gui->update_delay;
                    break;
                case XK_minus:
                    ++(gui->update_delay);
                    break;
                case XK_Left:
                    --gui->xpos;
                    break;
                case XK_Right:
                    ++gui->xpos;
                    break;
                case XK_Up:
                    ++gui->rotation;
                    break;
                case XK_Down:
                    gui->drop = true;
                    break;
                case XK_Delete:
                    gui->discard = true;
                    break;
                case XK_Escape:
                    gui->abort = true;
                    break;
                }
            } break;
        }
    }
}

static void draw_block(GUI *gui, int x, int y, int color)
{
    XPoint pts[6] = {
        { x + SCALE - 1, y }, { x, y }, { x, y + SCALE - 1 },
        { x + SCALE - 1, y }, { x + SCALE - 1, y + SCALE - 1 }, { x, y + SCALE - 1 } };

    XSetForeground(gui->display, gui->gc, gui->block_color[color][1].pixel);
    XFillRectangle(gui->display, gui->window, gui->gc, x + 1, y + 1, SCALE - 2, SCALE - 2);

    XSetForeground(gui->display, gui->gc, gui->block_color[color][2].pixel);
    XDrawLines(gui->display, gui->window, gui->gc, pts, 3, CoordModeOrigin);

    XSetForeground(gui->display, gui->gc, gui->block_color[color][0].pixel);
    XDrawLines(gui->display, gui->window, gui->gc, pts + 3, 3, CoordModeOrigin);
}

void gui_redraw(GUI *gui)
{
    int x, y, ypos = 0;

    if(gui->field && gui->form)
    {
        for(x = gui->xpos; x < gui->xpos + gui->form->width; ++x)
        {
            if(gui->form->bottom[x - gui->xpos] >= 0)
            {
                y = gui->field->top[x] - gui->form->bottom[x - gui->xpos];
                if(y > ypos)
                    ypos = y;
            }
        }
    }

    for(x = 0; x < FIELD_WIDTH; ++x)
        for(y = 0; y < FIELD_HEIGHT; ++y)
        {
            int sx = STATS_WIDTH + SCALE*x,
                sy = SCALE*(FIELD_HEIGHT + PIECE_SIZE - 1 - y);
            if( gui->form && x >= gui->xpos && x < gui->xpos + gui->form->width
                          && y >= ypos + gui->form->top[x - gui->xpos]
                          && gui->form->bottom[x - gui->xpos] >= 0 )
            {
                draw_block(gui, sx, sy, 11 + gui->form->id);
            }
            else
            if( gui->form && x >= gui->xpos && x < gui->xpos + gui->form->width
                          && y >= ypos && y < ypos + gui->form->height
                          && gui->form->tile[x - gui->xpos][y - ypos] )
            {
                draw_block(gui, sx, sy, gui->form->id);
            }
            else
            {
                draw_block( gui, sx, sy,
                            gui->field ? gui->field->tile[x][y] : 0 );
            }
        }
    for(x = 0; x < FIELD_WIDTH; ++x)
        for(y = 0; y < PIECE_SIZE; ++y)
        {
            int sx = STATS_WIDTH + SCALE*x,
                sy = SCALE*(PIECE_SIZE - 1 - y);
            if( gui->form && x >= gui->xpos && x < gui->xpos + gui->form->width
                          && y < gui->form->bottom[x - gui->xpos] )
            {
                draw_block(gui, sx, sy, 11 + gui->form->id);
            }
            else
            if( gui->form && x >= gui->xpos && x < gui->xpos + gui->form->width
                          && gui->form->tile[x - gui->xpos][y] )
            {
                draw_block(gui, sx, sy, gui->form->id);
            }
            else
            {
                draw_block(gui, sx, sy, 11);
            }
        }

    /* Draw upcoming pieces */
    if(!gui->stats || gui->last_game_pos != gui->stats->pos)
    {
        XSetForeground(gui->display, gui->gc, BlackPixel(gui->display, 0));
        XFillRectangle(gui->display, gui->window, gui->gc,
            STATS_WIDTH + 15*SCALE + 5, 0, 6*SCALE - 5, 45*SCALE );
        gui->last_game_pos = gui->stats->pos;
    }

    if(gui->stats)
    {
        int n, sy = 0;

        for(n = gui->stats->pos + 1; n < gui->game->input_size; ++n)
        {
            Form *form = gui->game->piece[(int)gui->game->input[n]].form;
            if(form[0].height > form[1].height)
                ++form;
            if(sy + SCALE*(1 + form->height) > 45*SCALE)
                break;
            for(x = 0; x < form->width; ++x)
                for(y = 0; y < form->height; ++y)
                    if(form->tile[x][y])
                    {
                        draw_block( gui, STATS_WIDTH + (15 + x)*SCALE + (6 - form->width)*SCALE/2,
                            sy + (form->height - y - 1)*SCALE + SCALE/2,
                            form->tile[x][y] );
                    }
            sy += (form->height + 1)*SCALE;
        }

        /* Draw stats */
        static const char * const headings[12] = {
            "Source revision:",
            "Instruction:",
            "Pieces:",
            "   Discarded:",
            "   Dropped:",
            "Lines cleared:",
            "   1 line: ",
            "   2 lines: ",
            "   3 lines:",
            "   4 lines:",
            "   5 lines:",
            "Score:" };
        const int values[12] = {
            REVISION,
            gui->stats->instr, gui->stats->pos,
            gui->stats->discarded, gui->stats->dropped,
            ( gui->stats->cleared[1] + gui->stats->cleared[2] +
              gui->stats->cleared[3] + gui->stats->cleared[4] +
              gui->stats->cleared[5] ),
            gui->stats->cleared[1], gui->stats->cleared[2],
            gui->stats->cleared[3], gui->stats->cleared[4],
            gui->stats->cleared[5],
            gui->stats->score + 400*(5 - gui->stats->discarded) };
        const int valuesx = (STATS_WIDTH*2)/3 - 10;

        XSetForeground(gui->display, gui->gc, BlackPixel(gui->display, 0));
        XFillRectangle(gui->display, gui->window, gui->gc,
            valuesx, 0, STATS_WIDTH - valuesx, 15*12);

        XSetForeground(gui->display, gui->gc, WhitePixel(gui->display, 0));
        for(n = 0; n < 12; ++n)
        {
            char buf[64];
            XDrawString( gui->display, gui->window, gui->gc,
                10, 15*(n + 1), headings[n], strlen(headings[n]) );
            sprintf(buf, "%8d", values[n]);
            XDrawString( gui->display, gui->window, gui->gc,
                (STATS_WIDTH*2)/3 - 10, 15*(n + 1), buf, strlen(buf) );
        }
    }
}

void gui_update(GUI *gui, Field *field, Stats *stats, Form *form, int xpos)
{
    long long t, ut;

    gui->field = field;
    gui->stats = stats;
    gui->form  = form;
    gui->xpos  = xpos;

    gui_redraw(gui);
    gui_process(gui, false);

    if(gui->update_delay)
    {
        t  = utime();
        ut = gui->last_update + 50000*gui->update_delay;
        if(ut > t)
            usleep(ut - t);
        gui->last_update = utime();
    }
}

void gui_wait(GUI *gui)
{
    while(!gui->abort)
    {
        gui_process(gui, true);
        gui_redraw(gui);
    }
}

bool gui_manual_move(GUI *gui, Move *move)
{
    Piece *piece;
    if(!(gui->field && gui->stats))
        return false;

    piece = &gui->game->piece[(int)gui->game->input[gui->stats->pos]];
    gui->xpos      = (FIELD_WIDTH - piece->form[0].width)/2;
    gui->rotation  = 0;
    gui->drop = gui->discard = false;
    while(!(gui->drop || gui->discard || gui->abort))
    {
        gui->form = &piece->form[gui->rotation];
        gui_redraw(gui);
        gui_process(gui, true);
        gui->rotation = gui->rotation%4;
        if(gui->xpos < 0)
            gui->xpos = 0;
        if(gui->xpos > FIELD_WIDTH - piece->form[gui->rotation].width)
            gui->xpos = FIELD_WIDTH - piece->form[gui->rotation].width;
    }
    if(gui->abort)
        return false;
    if(gui->discard)
        move->form = -1;
    else
    {
        move->form = gui->rotation;
        move->xpos = gui->xpos;
    }
    return true;
}
