#ifndef GUI_H
#define GUI_H

#include "Base.h"

typedef struct GUI GUI;

GUI *gui_create(Game *game, const char *window_name);
void gui_destroy(GUI *gui);
void gui_update(GUI *gui, Field *field, Stats *stats, Form *form, int xpos);
void gui_redraw(GUI *gui);
bool gui_manual_move(GUI *gui, Move *move);
void gui_wait(GUI *gui);

#endif /* ndef GUI_H */
