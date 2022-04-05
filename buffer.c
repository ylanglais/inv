#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <gtksourceview/gtksource.h>
#include <gdk/gdkkeysyms.h>
#include <tbx/err.h>
#include <tbx/futl.h>
#include <tbx/str.h>

//static char _expr[] ="\n\r\t ?,.;/:§!%*^$£+=@\\`|-'#\"~&><";
static char _expr[] ="\n\r\t ?,;/:§!%^£+=@\\`|'#\"~<";

typedef struct {
	GtkWidget 		*parent;
	GtkTextTagTable *tagtab;
	GtkSourceBuffer	*srcbuf;
	GtkWidget		*sclwin;	
 	GtkWidget		*srcvie;
	GtkWidget		*modebr;
	GtkWidget		*statbr;
	GtkWidget		*acctxt;
	GtkWidget		*cnttxt;
	GtkWidget		*rowtxt;
	GtkWidget		*coltxt;
	GtkWidget		*colcnt;
	char 			*name;
	char 			*lang;
	int				mode;
	int				eol;
	int				state;
	char 			acc[1001];
	char 			cnt[11];
	pstr_t 			rec;
} buffer_t, *pbuffer_t;

#define _buffer_c_
#include "buffer.h"
#undef  _buffer_c_

gboolean cb_on_kb(GtkWidget *widget, GdkEventKey *event, gpointer vpb);
gboolean cb_on_ex_kb(GtkWidget *widget, GdkEventKey *event, gpointer vpb);
gboolean cb_on_mb(GtkWidget *widget, GdkEventButton *event, gpointer vpb);
gboolean cb_on_pos_changed(GtkTextBuffer *buffer, GParamSpec *pspec G_GNUC_UNUSED, void *data);

int buffer_line_length(pbuffer_t pb);

size_t buffer_size() { return sizeof(buffer_t); }

#define pbuf(x) ((pbuffer_t) x)
#define vp(x)   ((void *)    x)

char *
buffer_name_get(pbuffer_t pb) {
	if (!pb) return NULL;
	return pb->name;
}

char *
buffer_name_set(pbuffer_t pb, char *name) {
	if (!pb) return NULL;
	if (pb->name) free(pb->name);
	return pb->name = strdup(name);
}

char *
buffer_text_get(pbuffer_t pb) {
	if (!pb) return NULL;
	GtkTextIter sta, end;
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(pb->srcbuf), &sta);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(pb->srcbuf),   &end);
	pb->state = state_saved;
	return gtk_text_buffer_get_text(GTK_TEXT_BUFFER(pb->srcbuf), &sta, &end, FALSE);
}

void
buffer_text_set(pbuffer_t pb, char *text) {
	if (!pb) return;
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(pb->srcbuf), text, strlen(text));
}

int 
buffer_state_set(pbuffer_t pb, int state) {
	if (!pb) return 55;
	return pb->state = state;
}

int 
buffer_state_get(pbuffer_t pb) {
	if (!pb) return 55;
	return pb->state;
}

const char *
buffer_state_str(int state) {
	if (state == state_modified) return "modified";
	if (state == state_saved) 	 return "saved";
	return "ERR state";
}

const char *
buffer_mode_str(int mode) {
	if (mode == mode_cmd) 	 return "command";
	if (mode == mode_insert) return "insert";
	if (mode == mode_ex) 	 return "ex";
	return "ERR mode";
}

GtkTextIter *
buffer_cursor_get(pbuffer_t pb, GtkTextIter *iter) {
	GtkTextMark	*mark;
	mark = gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(pb->srcbuf));
	gtk_text_buffer_get_iter_at_mark(GTK_TEXT_BUFFER(pb->srcbuf), iter, mark);
	return iter;
}

pos_t iter_to_pos(GtkTextIter iter) {
	pos_t p = { 0, 0 };
	p.row = gtk_text_iter_get_line(&iter);
	p.col = gtk_text_iter_get_line_offset(&iter);
	return p;
}

pos_t buffer_pos_get(pbuffer_t pb) {
	pos_t p = { 0, 0 };
	if (pb == NULL) return p;

	GtkTextIter iter;
	buffer_cursor_get(pb, &iter);
	p.row = gtk_text_iter_get_line(&iter);
	p.col = gtk_text_iter_get_line_offset(&iter);
	return p;
}

pos_t buffer_pos_set(pbuffer_t pb, pos_t pos) {
	GtkTextIter iter;
	buffer_cursor_get(pb, &iter);
	gtk_text_iter_set_line(&iter, pos.row);
	gtk_text_iter_set_line_offset(&iter, pos.col);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(pb->srcbuf), &iter);
	return buffer_pos_get(pb);		
}

char *
buffer_int_to_str(char *b, int i) {
	snprintf(b, 9, "%d", i);
	return b;
}

void buffer_status_pos(pbuffer_t pb) {
	char b[12];

	pos_t pos = buffer_pos_get(pb);
	gtk_entry_set_text(GTK_ENTRY(pb->rowtxt), buffer_int_to_str(b, pos.row + 1));
	gtk_entry_set_text(GTK_ENTRY(pb->coltxt), buffer_int_to_str(b, pos.col + 1));
	gtk_entry_set_text(GTK_ENTRY(pb->colcnt), buffer_int_to_str(b, buffer_line_length(pb)));
}

void buffer_status_acccnt(pbuffer_t pb) {
	gtk_entry_set_text(GTK_ENTRY(pb->acctxt), pb->acc);
	gtk_entry_set_text(GTK_ENTRY(pb->cnttxt), pb->cnt);
}

pbuffer_t
buffer_new(GtkWidget *parent) {
	pbuffer_t pb = NULL;
	if (!(pb = (pbuffer_t) malloc(buffer_size()))) {
		err_error("cannot allocate mem for buffer");
		return NULL;
	}

	pb->name = 
	pb->lang = NULL;
	
	GtkWidget *grid;

	pb->eol = FALSE;

	pb->parent = parent;
	//pb->tagtab = gtk_text_tag_table_new();
	//pb->srcvie = gtk_text_buffer_new(pb->tagtab);
	grid 	   = gtk_grid_new();
 
	gtk_widget_set_hexpand(GTK_WIDGET(grid), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(grid), TRUE);

//	pb->txtvie = gtk_text_view_new();
//	pb->txtbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(pb->txtvie)); 
	//pb->srcvie = gtk_source_view_new();

    pb->sclwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pb->sclwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC); 
	gtk_widget_set_hexpand(GTK_WIDGET(pb->sclwin), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(pb->sclwin), TRUE);

	pb->tagtab = gtk_text_tag_table_new();
	pb->srcbuf = gtk_source_buffer_new(pb->tagtab);
	pb->srcvie = gtk_source_view_new_with_buffer(pb->srcbuf);

	gtk_widget_set_hexpand(GTK_WIDGET(pb->srcvie), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(pb->srcvie), TRUE);

	//gtk_source_buffer_set_language(pb->srcbuf, int) ;
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(pb->srcvie), TRUE);
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(pb->srcvie), 4);
	gtk_source_view_set_indent_width(GTK_SOURCE_VIEW(pb->srcvie), 4);
	//gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(pb->srcvie), TRUE);
	gtk_source_view_set_show_line_marks(GTK_SOURCE_VIEW(pb->srcvie), TRUE);
	gtk_source_buffer_set_highlight_matching_brackets(GTK_SOURCE_BUFFER(pb->srcbuf), TRUE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(pb->srcvie), GTK_WRAP_CHAR);
	
	gtk_widget_set_events(pb->srcvie, GDK_KEY_PRESS_MASK|GDK_BUTTON_PRESS_MASK);
	g_signal_connect(G_OBJECT(pb->srcvie), "key_press_event", G_CALLBACK(cb_on_kb), vp(pb));
	g_signal_connect(G_OBJECT(pb->srcvie), "button_press_event", G_CALLBACK(cb_on_mb), vp(pb));
	// Doesn't work:
	//g_signal_connect(G_OBJECT(pb->srcvie), "notify::cursor-position", G_CALLBACK(cb_on_pos_changed), vp(pb));

	buffer_mode_set(pb, mode_cmd);
	pb->modebr = gtk_entry_new();	
	gtk_widget_set_can_focus(GTK_WIDGET(pb->modebr), FALSE);
	gtk_widget_set_hexpand(GTK_WIDGET(pb->modebr), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(pb->modebr), FALSE);
	g_signal_connect(G_OBJECT(pb->modebr), "key_press_event", G_CALLBACK(cb_on_ex_kb), vp(pb));

	pb->statbr = gtk_entry_new();	
	gtk_widget_set_can_focus(GTK_WIDGET(pb->statbr), FALSE);
	gtk_widget_set_hexpand(GTK_WIDGET(pb->statbr), FALSE);
	gtk_widget_set_vexpand(GTK_WIDGET(pb->statbr), FALSE);

	pb->rowtxt = gtk_entry_new();	
	gtk_widget_set_can_focus(GTK_WIDGET(pb->rowtxt), FALSE);
	gtk_widget_set_hexpand(GTK_WIDGET(pb->rowtxt), FALSE);
	gtk_widget_set_vexpand(GTK_WIDGET(pb->rowtxt), FALSE);

	pb->coltxt = gtk_entry_new();	
	gtk_widget_set_can_focus(GTK_WIDGET(pb->coltxt), FALSE);
	gtk_widget_set_hexpand(GTK_WIDGET(pb->coltxt), FALSE);
	gtk_widget_set_vexpand(GTK_WIDGET(pb->coltxt), FALSE);

	pb->colcnt = gtk_entry_new();	
	gtk_widget_set_can_focus(GTK_WIDGET(pb->colcnt), FALSE);
	gtk_widget_set_hexpand(GTK_WIDGET(pb->colcnt), FALSE);
	gtk_widget_set_vexpand(GTK_WIDGET(pb->colcnt), FALSE);

	pb->acctxt = gtk_entry_new();	
	gtk_widget_set_can_focus(GTK_WIDGET(pb->acctxt), FALSE);
	gtk_widget_set_hexpand(GTK_WIDGET(pb->acctxt), FALSE);
	gtk_widget_set_vexpand(GTK_WIDGET(pb->acctxt), FALSE);

	pb->cnttxt = gtk_entry_new();	
	gtk_widget_set_can_focus(GTK_WIDGET(pb->cnttxt), FALSE);
	gtk_widget_set_hexpand(GTK_WIDGET(pb->cnttxt), FALSE);
	gtk_widget_set_vexpand(GTK_WIDGET(pb->cnttxt), FALSE);


	gtk_container_add(GTK_CONTAINER(pb->parent), grid);
	gtk_container_add(GTK_CONTAINER(pb->sclwin), pb->srcvie);

	gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(pb->sclwin), 0, 0, 6, 1);
	gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(pb->modebr), 0, 1, 6, 1);
	gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(pb->statbr), 0, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(pb->rowtxt), 1, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(pb->coltxt), 2, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(pb->colcnt), 3, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(pb->acctxt), 4, 2, 1, 1);
	gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(pb->cnttxt), 5, 2, 1, 1);

	gtk_widget_show_all(pb->sclwin);
	gtk_widget_show_all(grid);
	gtk_widget_show_all(pb->parent);

	gtk_entry_set_text(GTK_ENTRY(pb->statbr), buffer_mode_str(pb->mode));

	buffer_status_pos(pb);
	buffer_status_acccnt(pb);

	gtk_widget_grab_focus(GTK_WIDGET(pb->srcvie));
	
	return pb;
}

pbuffer_t
buffer_file_new(GtkWidget *parent, char *filename) {
	static char _def[] = "*scartch*";
	pbuffer_t pb = buffer_new(parent);
	if (filename == NULL) {
		filename = _def;
	} else  if (futl_exists(filename)) {
		size_t size;
		char   *text;
		text = futl_load(filename, &size);
		gtk_text_buffer_set_text(GTK_TEXT_BUFFER(pb->srcbuf), text, -1);

		GtkSourceLanguage *lang;
		if ((lang = gtk_source_language_manager_guess_language(gtk_source_language_manager_get_default(), filename, NULL))) {
			gtk_source_buffer_set_language(pb->srcbuf, lang);
			pb->lang = strdup(gtk_source_language_get_name(lang));
		}
	}
	pb->name = strdup(filename);
	return pb;
}

int
buffer_save(pbuffer_t pb) {
	if (!pb) return 1;
	char *text = buffer_text_get(pb);
	futl_write(pb->name, text, strlen(text));
	g_free(text);
	return 0;
}
int 
buffer_save_as(pbuffer_t pb, char *filename) {
	if (!pb) return 1;
	char *text = buffer_text_get(pb);
	futl_write(filename, text, strlen(text));
	GtkSourceLanguage *lang;
	if ((lang = gtk_source_language_manager_guess_language(gtk_source_language_manager_get_default(), filename, NULL))) {
		gtk_source_buffer_set_language(pb->srcbuf, lang);
		pb->lang = strdup(gtk_source_language_get_name(lang));
	}
	g_free(text);
	return 0;
}

pbuffer_t
buffer_destroy(pbuffer_t pb) {
	if (pb) {
		if (pb->name) free(pb->name);
		if (pb->lang) free(pb->lang);
		free(pb);
	}
	return NULL;
}

void
buffer_close(pbuffer_t pb) {
	gtk_widget_destroy(GTK_WIDGET(pb->parent));
}

int 
buffer_line_length(pbuffer_t pb) {
	GtkTextIter c;
	buffer_cursor_get(pb, &c);
	gtk_text_iter_forward_to_line_end(&c);
	return gtk_text_iter_get_line_offset(&c);
}

void
buffer_join(pbuffer_t pb) {
	GtkTextIter i1, i2;
	buffer_cursor_get(pb, &i1);
	if (gtk_text_iter_get_char(&i1) != '\n') buffer_moveto_eol(pb);
	buffer_cursor_get(pb, &i1);
	i2 = i1;
	gtk_text_iter_forward_char(&i2);
	gtk_text_buffer_delete(GTK_TEXT_BUFFER(pb->srcbuf), &i1, &i2);
}

void
buffer_line_insert(pbuffer_t pb) {
	GtkTextIter i1;
	buffer_moveto_sol(pb);
	gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(pb->srcbuf), "\n", 1);
	buffer_cursor_get(pb, &i1);
	gtk_text_iter_backward_chars(&i1, 1);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(pb->srcbuf), &i1);
}

void
buffer_line_append(pbuffer_t pb) {
	GtkTextIter i1;
	buffer_cursor_get(pb, &i1);
	if (gtk_text_iter_get_char(&i1) != '\n') buffer_moveto_eol(pb);

	gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(pb->srcbuf), "\n", 1);
}

void
buffer_moveto_sol(pbuffer_t pb) {
	GtkTextIter iter;
	buffer_cursor_get(pb, &iter);
	gtk_text_iter_set_line_offset(&iter, 0);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(pb->srcbuf), &iter);
}
void
buffer_moveto_eol(pbuffer_t pb) {
	GtkTextIter iter;
	buffer_cursor_get(pb, &iter);
	gtk_text_iter_forward_to_line_end(&iter);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(pb->srcbuf), &iter);
	pb->eol = TRUE;
}

void buffer_move_left(pbuffer_t pb, int count) {
	GtkTextIter iter;
	pb->eol = FALSE;
	buffer_cursor_get(pb, &iter);
	if (gtk_text_iter_get_line_offset(&iter) == 0) return;
	gtk_text_iter_backward_chars(&iter, count);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(pb->srcbuf), &iter);
}

void buffer_move_right(pbuffer_t pb, int count) {
	GtkTextIter iter;
	buffer_cursor_get(pb, &iter);

	int len = buffer_line_length(pb);
	int off = gtk_text_iter_get_line_offset(&iter);
	
	
	if (off == len - 1) return;
	if (count > len - off) count = len - off - 1;

	gtk_text_iter_forward_chars(&iter, count);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(pb->srcbuf), &iter);
	
	if (gtk_text_iter_get_line_offset(&iter) == buffer_line_length(pb)) pb->eol = TRUE; 
}

void buffer_move_up(pbuffer_t pb, int count) {
	int lof;
	GtkTextIter iter;
	buffer_cursor_get(pb, &iter);
	lof = gtk_text_iter_get_line_offset(&iter);
	gtk_text_iter_backward_lines(&iter, count);
	if (pb->eol) {
		gtk_text_iter_forward_to_line_end(&iter);
	} else {
		gtk_text_iter_set_line_offset(&iter, lof);
		if (gtk_text_iter_get_line_offset(&iter) == buffer_line_length(pb)) pb->eol = TRUE; 
	}
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(pb->srcbuf), &iter);
}

void buffer_move_down(pbuffer_t pb, int count) {
	int lof;
	GtkTextIter iter;
	buffer_cursor_get(pb, &iter);
	lof = gtk_text_iter_get_line_offset(&iter);
	gtk_text_iter_forward_lines(&iter, count);
	if (pb->eol) {
		gtk_text_iter_forward_to_line_end(&iter);
	} else {
		gtk_text_iter_set_line_offset(&iter, lof);
		if (gtk_text_iter_get_line_offset(&iter) == buffer_line_length(pb)) pb->eol = TRUE; 
	}
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(pb->srcbuf), &iter);
}

void buffer_move_bw_word(pbuffer_t pb) {
}

void acc_append(pbuffer_t pb, char c) { if (!pb) return; char *p = pb->acc; while (*p && p - pb->acc < 1000) p++; *p = c;   }
void cnt_append(pbuffer_t pb, char c) { if (!pb) return; char *p = pb->cnt; while (*p && p - pb->cnt <   10) p++; *p = c;   }
void acc_rst(pbuffer_t pb)            { if (!pb) return; memset(pb->acc, 0, 1001);                                          }
void cnt_rst(pbuffer_t pb)            { if (!pb) return; memset(pb->cnt, 0, 11);                                            }
int  cnt_get(pbuffer_t pb)            { if (!pb) return 0; int c = atoi(pb->cnt); cnt_rst(pb); if (c == 0) c = 1; return c; }
void _rst(pbuffer_t pb)               { acc_rst(pb); cnt_rst(pb);                                                           }

int
buffer_mode_set(pbuffer_t pb, int mode) {
	if (!pb || pb->mode == mode) return 55;
	if (pb->mode == mode_insert) {
		gtk_text_buffer_end_user_action(GTK_TEXT_BUFFER(pb->srcbuf));
	} else if (pb->mode == mode_ex) {
		gtk_widget_grab_focus(GTK_WIDGET(pb->srcvie));
		gtk_widget_set_can_focus(GTK_WIDGET(pb->modebr), FALSE);
		gtk_entry_set_text(GTK_ENTRY(pb->modebr), "");
	} 
	if (mode == mode_insert) {
		gtk_text_buffer_begin_user_action(GTK_TEXT_BUFFER(pb->srcbuf));
		gtk_text_view_set_overwrite(GTK_TEXT_VIEW(pb->srcvie), FALSE);
	} else if (mode == mode_cmd) {
		gtk_text_view_set_overwrite(GTK_TEXT_VIEW(pb->srcvie), TRUE);
		_rst(pb);
	} else if (mode == mode_ex) {
		gtk_text_view_set_overwrite(GTK_TEXT_VIEW(pb->srcvie), FALSE);
		gtk_widget_set_can_focus(GTK_WIDGET(pb->modebr), TRUE);
		gtk_widget_grab_focus(GTK_WIDGET(pb->modebr));
	} 
	return pb->mode = mode;
}

gboolean
cb_on_ex_kb(GtkWidget *widget, GdkEventKey *event, gpointer vpb) {
	int stop = FALSE;
	pbuffer_t pb = pbuf(vpb);

	if (event->keyval == GDK_KEY_Escape) { 
		buffer_mode_set(pb, mode_cmd); 
		stop = TRUE;
	} else if (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) {
		char *text;
		text = strdup(gtk_entry_get_text(GTK_ENTRY(pb->modebr)));
		if (!strcmp(text, ":w")) {
		} else if (!strcmp(text, ":q")) {
			buffer_close(pb);
			return TRUE;
		} else {
			err_log("ex command: %s", text);
		} 
		free(text);	
		buffer_mode_set(pb, mode_cmd); 
		stop = TRUE;
	}
	return stop;
}
gboolean
cb_on_kb(GtkWidget *widget, GdkEventKey *event, gpointer vpb) {
	gboolean stop = FALSE;
	pbuffer_t pb = pbuf(vpb);
	if (pb == NULL)  {
		err_error("no buffer sent");
		return stop; 
	}

	//buffer_events(pbuffer_t pb, 


	if (pb->mode == mode_insert) {
		if (event->keyval == GDK_KEY_Escape) { 
			pb->mode = mode_cmd; 
			gtk_text_buffer_begin_user_action(GTK_TEXT_BUFFER(pb->srcbuf));
			if (pb->cnt[0] != 0) {
				// Redo		
			}
		} else switch(event->keyval) {
			// push key + modifiers 
		
		case GDK_KEY_Insert:
		case GDK_KEY_KP_Insert:
			stop = TRUE;
			break;
		case GDK_KEY_KP_Home:
		case GDK_KEY_Home:
			buffer_moveto_sol(pb);
			stop = TRUE;
			break;
		case GDK_KEY_KP_End:
		case GDK_KEY_End:
			buffer_moveto_eol(pb);
			stop = TRUE;
			break;
		case GDK_KEY_Left:
		case GDK_KEY_KP_Left:
			buffer_move_left(pb, 1);
			stop = TRUE;
			break;
		case GDK_KEY_Down:
		case GDK_KEY_KP_Down:
			buffer_move_down(pb, 1);
			stop = TRUE;
			break;
		case GDK_KEY_Up:
		case GDK_KEY_KP_Up:
			buffer_move_up(pb, 1);
			stop = TRUE;
			break;
		case GDK_KEY_Right:
		case GDK_KEY_KP_Right:
			buffer_move_right(pb, 1);
			stop = TRUE;
			break;
		default:
		}
	}  else {
		if (event->keyval == GDK_KEY_Escape) { 
			_rst(pb); 
		} else if (event->keyval == ':' || event->keyval == '!' || event->keyval == '/') { 
			buffer_mode_set(pb, mode_ex);  
			_rst(pb); 
			char b[2];
			b[0] = event->keyval; b[1] = 0;
			gtk_entry_set_text(GTK_ENTRY(pb->modebr), b); 
			gtk_editable_set_position(GTK_EDITABLE(pb->modebr), -1);
		}
		switch (event->keyval) {

		/* Move: */
		case GDK_KEY_h:
		case GDK_KEY_Left:
		case GDK_KEY_KP_Left:
			buffer_move_left(pb, cnt_get(pb));
			break;
		case GDK_KEY_j:
		case GDK_KEY_Down:
		case GDK_KEY_KP_Down:
			buffer_move_down(pb, cnt_get(pb));
			break;
		case GDK_KEY_k:
		case GDK_KEY_Up:
		case GDK_KEY_KP_Up:
			buffer_move_up(pb, cnt_get(pb));
			break;
		case GDK_KEY_l:
		case GDK_KEY_Right:
		case GDK_KEY_KP_Right:
			buffer_move_right(pb, cnt_get(pb));
			break;

		case GDK_KEY_KP_Home:
		case GDK_KEY_Home:
		case '0': 
			buffer_moveto_sol(pb);
			break;

		case GDK_KEY_KP_End:
		case GDK_KEY_End:
		case '$':
			buffer_moveto_eol(pb);
			break;

		case 'w':
			if (pb->acc[0] != 0) {
				/* we have a command */
			}
		case 'W':
		case 'b':
		case 'B':
		case 'e':
		case 'E':
			break;
		case '\'' :
			break;
		case  '`':
			break;

		/* Insert: */
		case 'i':
			buffer_mode_set(pb, mode_insert);
			break;
		case 'I':
			buffer_moveto_sol(pb);
			buffer_mode_set(pb, mode_insert);
			break;

		case 'a': 
			buffer_move_right(pb, 1);
			buffer_mode_set(pb, mode_insert);
			break;

		case 'A':
			buffer_moveto_eol(pb);
			buffer_mode_set(pb, mode_insert);
			break;

		case 'J':
			buffer_join(pb);
			break;	

		case 'o':
			buffer_line_append(pb);
			buffer_mode_set(pb, mode_insert);
			break;

		case 'O':
			buffer_line_insert(pb);
			buffer_mode_set(pb, mode_insert);
			break;
		

		/* Other */
		default:
			//err_log("default");
			if (event->keyval >= GDK_KEY_0 && event->keyval <= GDK_KEY_9) {
				cnt_append(pb, (event->keyval - GDK_KEY_0) + '0');  
			} else {
				acc_append(pb, (char) *event->string);
			}
		}
		stop = TRUE;	
	}
	gtk_entry_set_text(GTK_ENTRY(pb->statbr), buffer_mode_str(pb->mode));
	buffer_status_pos(pb);
	buffer_status_acccnt(pb);
	return stop;
}

void
buffer_mouse_cut_paste_word(pbuffer_t pb, int x, int y) {
	GtkTextIter pos, sow;

	gtk_text_view_get_iter_at_position(GTK_TEXT_VIEW(pb->srcvie), &pos, NULL, x, y); 
	sow = pos;
	gtk_text_iter_backward_word_start(&sow);
	gtk_text_iter_forward_word_end(&pos);
	char *txt = gtk_text_iter_get_text(&sow, &pos);
	gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(pb->srcbuf), txt, strlen(txt));
}

int char_in_charlist(char c, char * charlist) {
	char *p;
	for (p = charlist; *p && *p != c; p++);
	return *p - c;
}

void
buffer_mouse_cut_paste_expr(pbuffer_t pb, int x, int y) {
	GtkTextIter pos, soe, eoe;
	gtk_text_view_get_iter_at_position(GTK_TEXT_VIEW(pb->srcvie), &pos, NULL, x, y); 

	for (soe = pos; !gtk_text_iter_starts_line(&soe) && char_in_charlist(gtk_text_iter_get_char(&soe), _expr); gtk_text_iter_backward_char(&soe));
	if (!char_in_charlist(gtk_text_iter_get_char(&soe), _expr)) gtk_text_iter_forward_char(&soe);
	for (eoe = pos; !gtk_text_iter_ends_line(&eoe)   && char_in_charlist(gtk_text_iter_get_char(&eoe), _expr); gtk_text_iter_forward_char(&eoe));

	char * txt = gtk_text_iter_get_text(&soe, &eoe);
	err_log("txt = '%s'", txt);

	gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(pb->srcbuf), txt, strlen(txt));
}

gboolean
cb_on_mb(GtkWidget *widget, GdkEventButton *event, gpointer vpb) {
	gboolean stop = FALSE;

	//err_error("on_mb with button: %d, state: %d at (%f, %f)", event->button, event->state, event->x, event->y);
	pbuffer_t pb = pbuf(vpb);
	if (pb == NULL)  {
		err_error("no buffer sent");
		return stop; 
	}
	if (pb->mode == mode_insert) {
		if (event->button == 1 && event->state & GDK_CONTROL_MASK) {
			int x, y;
			gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT, event->x, event->y, &x, &y);
			buffer_mouse_cut_paste_word(pb, x, y);
			stop = TRUE;
		} else if (event->button == 1 && event->state & GDK_SHIFT_MASK) {
			int x, y;
			gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(widget), GTK_TEXT_WINDOW_TEXT, event->x, event->y, &x, &y);
			buffer_mouse_cut_paste_expr(pb, x, y);
			stop = TRUE;
		}
	}
	return stop;
}

gboolean 
cb_on_pos_changed(GtkTextBuffer *w, GParamSpec *pspec G_GNUC_UNUSED, void *vpb) {
	err_error("cb");
	pbuffer_t pb = pbuf(vpb);
	if (pb == NULL)  {
		err_error("no buffer sent");
		return FALSE; 
	}
	buffer_status_pos(pb);
	return FALSE;
}

