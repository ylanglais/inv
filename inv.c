#include <stdlib.h>
#include <unistd.h>
#include <tbx/err.h>
#include <tbx/futl.h>

#ifndef __GTK_H__
#include <gtk/gtk.h>
#endif 

#include "buffer.h"

typedef struct {
	GtkWidget *top;
	GtkWidget *notebook;
	int nb;
	int cur;
	pbuffer_t *bfs;
} inv_t, *pinv_t;

#define _inv_c_
#include "inv.h"
#undef  _inv_c_ 

static char _def[] = "*scratch*"; 

pinv_t
inv_new() {
	pinv_t inv;
	if (!(inv = malloc(sizeof(inv_t)))) return NULL;
	inv->nb  = 0;
	inv->bfs = NULL;
	return inv;
}

pinv_t
inv_destroy(pinv_t inv) {
	if (inv) {
		int i;	
		for (i = 0; i < inv->nb; i++) { inv_buffer_remove(inv); }
		inv->nb = 0;
	} 
	return NULL;
}

int
inv_buffer_add(pinv_t inv, char *filename) {
	if (!inv) return 1;

	inv->cur = inv->nb++;
	if (inv->nb == 0) {
		inv->bfs = malloc(sizeof(pbuffer_t));
	} else {
		inv->bfs = realloc(inv->bfs, sizeof(pbuffer_t) * inv->nb);
	}
	GtkWidget *label;
	if (!filename) filename = _def;

    label = gtk_label_new(filename);
    gtk_widget_show(label);

	GtkWidget *w = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(inv->notebook), w, label);

	inv->bfs[inv->nb - 1] = buffer_file_new(w, filename);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(inv->notebook), inv->cur);
	return 0;	
}

void inv_buffer_remove(pinv_t inv) {
	if (!inv) return;
	gtk_notebook_remove_page(GTK_NOTEBOOK(inv->notebook), inv->cur); 
}

void inv_buffer_destroyed(pinv_t inv, int num) {
	pbuffer_t bs;
	if (!inv) return;
	bs = inv->bfs[num];

	for (int i = num + 1; i < inv->nb; i++) inv->bfs[i - 1] = inv->bfs[i];

	if (inv->cur == inv->nb - 1) inv->cur--;
	if (inv->cur >= 0) gtk_notebook_set_current_page(GTK_NOTEBOOK(inv->notebook), inv->cur);
	
	buffer_destroy(bs);
}

static void
on_menu_new(GtkApplication *app, pinv_t inv) {
	inv_buffer_add(inv, NULL);
}
static void
on_menu_open(GtkApplication *app, pinv_t inv) {
	GtkWidget *fsel;

	fsel = gtk_file_chooser_dialog_new("Open file", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);
	//gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(fsel), TRUE);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fsel), get_current_dir_name());
	if (gtk_dialog_run(GTK_DIALOG(fsel)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fsel));
		gtk_widget_destroy(fsel);	
		
		//wf_new(sana_userbox(), filename);
		inv_buffer_add(inv, filename);
		g_free(filename);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(inv->notebook), inv->cur); 
	}
}

static void on_menu_save(GtkApplication *app, pinv_t inv);

static void
on_menu_save_as(GtkApplication *app, pinv_t inv) {
	GtkWidget *fsel;
	if (!inv || inv->nb < 0 || inv->cur >= inv->nb) {
		err_error("no buffer");
		return;
	}
	fsel = gtk_file_chooser_dialog_new("Save file as", NULL, GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fsel), get_current_dir_name());
	if (gtk_dialog_run(GTK_DIALOG(fsel)) == GTK_RESPONSE_ACCEPT) {
		char *filename;
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fsel));
		gtk_widget_destroy(fsel);	
		gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(inv->notebook), gtk_notebook_get_nth_page(GTK_NOTEBOOK(inv->notebook), inv->cur), filename);

		g_free(filename);
		buffer_save_as(inv->bfs[inv->cur], filename);
	}
}
static void
on_menu_save(GtkApplication *app, pinv_t inv) {
	char *name;
	if (!inv || inv->nb < 0 || inv->cur >= inv->nb) {
		err_error("no buffer");
		return;
	}
	name = buffer_name_get(inv->bfs[inv->cur]);
	if (!strcmp(name, _def)) { 
		on_menu_save_as(app, inv);
	}
	buffer_save(inv->bfs[inv->cur]);
}
static void
on_menu_quit(GtkApplication *app, pinv_t inv) {
	GtkWidget *top;
	top = inv->top;
	inv_destroy(inv);
	gtk_widget_destroyed(top, &top);
	gtk_main_quit();
	//exit(0);
}

static void
on_menu_close(GtkApplication *app, pinv_t inv) {
	if (inv->nb < 1) return;
	inv_buffer_remove(inv);
}

static void
on_page_switched(GtkWidget *widget, GtkNotebookTab *page, gint page_num, pinv_t inv) {
	inv->cur = page_num;	
}

static void
on_page_removed(GtkWidget *widget, GtkNotebookTab *page, gint page_num, pinv_t inv) {
	if (!inv) return;
	inv_buffer_destroyed(inv, page_num);	
}
int
main(int n, char *a[]) {
	GtkWidget *box;
	GtkWidget *mb;
	GtkWidget *sm;
	GtkWidget *mi;

	pinv_t inv;
	inv = malloc(sizeof(inv_t));
	inv->nb = 0;
	inv->bfs = NULL;


   // GtkAccelGroup *accel;
    gtk_init(&n, &a);     /* gtk initializer can take filename as an argument */
	//GtkApplication *app = gtk_application_new("org.ilay.inv", G_APPLICATION_FLAGS_NONE);

    inv->top = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    GdkDisplay 		*display;
    GdkScreen 		*screen;
    GtkCssProvider 	*provider;
    GError 			*error;

    display  = gdk_display_get_default();
    screen   = gdk_display_get_default_screen(display);
    provider = gtk_css_provider_new();
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    error = NULL;
    gtk_css_provider_load_from_file(provider, g_file_new_for_path("inv.css"), &error);

	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(inv->top), box);
	mb = gtk_menu_bar_new();

	gtk_box_pack_start(GTK_BOX(box), mb, FALSE, FALSE, 0);
	mi = gtk_menu_item_new_with_mnemonic("_File");
	gtk_container_add(GTK_CONTAINER(mb), mi);

	sm = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), sm);

	mi = gtk_menu_item_new_with_mnemonic ("_New");
	g_signal_connect((void *) mi, "activate", G_CALLBACK(on_menu_new), inv);
	gtk_container_add(GTK_CONTAINER(sm), mi);
	

	mi = gtk_menu_item_new_with_mnemonic ("_Open");
	g_signal_connect((void *) mi, "activate", G_CALLBACK(on_menu_open), inv);
	gtk_container_add(GTK_CONTAINER(sm), mi);

	mi = gtk_menu_item_new_with_mnemonic ("_Save");
	g_signal_connect((void *) mi, "activate", G_CALLBACK(on_menu_save), inv);
	gtk_container_add(GTK_CONTAINER(sm), mi);

	mi = gtk_menu_item_new_with_mnemonic ("Save _as");
	g_signal_connect((void *) mi, "activate", G_CALLBACK(on_menu_save_as), inv);
	gtk_container_add(GTK_CONTAINER(sm), mi);

	mi = gtk_menu_item_new_with_mnemonic ("_Close");
	g_signal_connect((void *) mi, "activate", G_CALLBACK(on_menu_close), inv);
	gtk_container_add(GTK_CONTAINER(sm), mi);

	mi = gtk_menu_item_new_with_mnemonic ("_Quit");
	g_signal_connect((void *) mi, "activate", G_CALLBACK(on_menu_quit), inv);
	gtk_container_add(GTK_CONTAINER(sm), mi);

    g_signal_connect(G_OBJECT(inv->top), "destroy", G_CALLBACK(on_menu_quit), inv);

	inv->notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(inv->notebook), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(box), inv->notebook, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(inv->notebook), "switch_page", G_CALLBACK(on_page_switched), inv);
	g_signal_connect(G_OBJECT(inv->notebook), "page_removed", G_CALLBACK(on_page_removed), inv);

	gtk_widget_show_all(inv->top);

	if (n > 1) for (int i = 1; i < n; i++) inv_buffer_add(inv, a[i]);
	else inv_buffer_add(inv, NULL);
		
    gtk_main();

	return 0;
}	
