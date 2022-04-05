#ifndef _buffer_h_
#define _buffer_h_

#ifndef __GTK_H__
#include <gtk/gtk.h>
#endif 

#ifndef _buffer_c_
typedef struct _buffer_t_ *pbuffer_t;
#endif

typedef struct {
	int row;
	int col;
} pos_t, *ppos_t;

enum {
	mode_cmd = 0,
	mode_insert,
	mode_ex
};
enum {
	state_saved = 0,
	state_modified
};

size_t 			buffer_size();

pbuffer_t 		buffer_new(GtkWidget *parent);
pbuffer_t 		buffer_file_new(GtkWidget *parent, char *filename);
pbuffer_t 		buffer_destroy(pbuffer_t pb);
int       		buffer_save(pbuffer_t pb);
int       		buffer_save_as(pbuffer_t pb, char *filename);

char *	  		buffer_name_get(pbuffer_t pb);
char *	  		buffer_name_set(pbuffer_t pb, char *filename);

char * 	  		buffer_text_get(pbuffer_t pb);
void 	  		buffer_text_set(pbuffer_t pb, char* text);

const char *	buffer_state_str(int state);
int 	    	buffer_state_get(pbuffer_t pb);
int 	  		buffer_state_set(pbuffer_t pb, int state);

const char *	buffer_mode_str(int mode);
int 	  		buffer_mode_get(pbuffer_t pb);
int 	  		buffer_mode_set(pbuffer_t pb, int mode);

pos_t			buffer_position_get(pbuffer_t pb);
pos_t			buffer_position_set(pbuffer_t pb, pos_t pos);
int				buffer_offset_get(pbuffer_t pb);
int				buffer_offset_set(pbuffer_t pb, int offset);

int				buffer_line_get(pbuffer_t pb);
int				buffer_line_set(pbuffer_t pb, int line);

int 			buffer_line_length(pbuffer_t pb);

int				buffer_col_get(pbuffer_t pb);
int				buffer_col_set(pbuffer_t pb, int col);

/* buffer actions: */

void			buffer_moveto_offset(pbuffer_t pb, unsigned int offset);
void			buffer_moveto_line(pbuffer_t pb, int line);
void			buffer_moveto_sol(pbuffer_t pb);
void			buffer_moveto_eol(pbuffer_t pb);
void			buffer_moveto_col(pbuffer_t pb, int col);
void			buffer_moveto_sow(pbuffer_t pb);
void			buffer_moveto_eow(pbuffer_t pb);
void			buffer_moveto_soe(pbuffer_t pb);
void			buffer_moveto_eoe(pbuffer_t pb);

void 			buffer_move_left(pbuffer_t pb, int count);
void 			buffer_move_right(pbuffer_t pb, int count);
void 			buffer_move_up(pbuffer_t pb, int count);
void 			buffer_move_down(pbuffer_t pb, int count);

void			buffer_mouse_cut_paste_word(pbuffer_t pb, int x, int y);
void			buffer_mouse_cut_paste_expr(pbuffer_t pb, int x, int y);

#endif
