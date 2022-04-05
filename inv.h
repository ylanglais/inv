#ifndef _inv_h_
#define _inv_h_

#include <gtk/gtk.h>
#include "buffer.h"

#ifndef _inv_c_
typedef void *pinv_t;
#endif


pinv_t      inv_new();
pinv_t      inv_destroy(pinv_t inv);
int         inv_buffer_add(pinv_t inv, char *filename);
void        inv_buffer_remove(pinv_t inv);
void        inv_buffer_destroyed(pinv_t inv, int num);

#ifdef _inv_c_
static void on_menu_new(GtkApplication *app, pinv_t inv);
static void on_menu_open(GtkApplication *app, pinv_t inv);
static void on_menu_save(GtkApplication *app, pinv_t inv);
static void on_menu_save_as(GtkApplication *app, pinv_t inv);
static void on_menu_save(GtkApplication *app, pinv_t inv);
static void on_menu_quit(GtkApplication *app, pinv_t inv);
static void on_menu_close(GtkApplication *app, pinv_t inv);
static void on_page_removed(GtkWidget *widget, GtkNotebookTab *page, gint page_num, pinv_t inv);
#endif
#endif
