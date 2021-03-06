/* $Id$ */
/* Copyright (c) 2006-2012 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Browser */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */



#include <sys/param.h>
#ifndef __GNU__ /* XXX hurd portability */
# include <sys/mount.h>
# if defined(__linux__) || defined(__CYGWIN__)
#  define unmount(a, b) umount(a)
# endif
# ifndef unmount
#  define unmount unmount
# endif
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include "callbacks.h"
#include "browser.h"
#include "../config.h"
#define _(string) gettext(string)

/* constants */
#define PROGNAME "browser"
#define COMMON_DND
#define COMMON_EXEC
#define COMMON_SYMLINK
#include "common.c"


/* private */
/* prototypes */
static GList * _copy_selection(Browser * browser);
static void _paste_selection(Browser * browser);


/* public */
/* functions */
/* callbacks */
/* window */
/* on_closex */
gboolean on_closex(gpointer data)
{
	Browser * browser = data;

	browser_delete(browser);
	if(browser_cnt == 0)
		gtk_main_quit();
	return FALSE;
}


/* accelerators */
/* on_close */
gboolean on_close(gpointer data)
{
	on_closex(data);
	return FALSE;
}


/* on_location */
gboolean on_location(gpointer data)
{
	Browser * browser = data;

	browser_focus_location(browser);
	return FALSE;
}


/* on_new_window */
gboolean on_new_window(gpointer data)
{
	Browser * browser = data;

	browser_new_copy(browser);
	return FALSE;
}


/* on_open_file */
gboolean on_open_file(gpointer data)
{
	Browser * browser = data;

	browser_open(browser, NULL);
	return FALSE;
}


/* file menu */
/* on_file_new_window */
void on_file_new_window(gpointer data)
{
	on_new_window(data);
}


/* on_file_new_folder */
void on_file_new_folder(gpointer data)
{
	Browser * browser = data;
	char const * newfolder = _("New folder");
	char const * location;
	size_t len;
	char * path;

	if((location = browser_get_location(browser)) == NULL)
		return;
	len = strlen(location) + strlen(newfolder) + 2;
	if((path = malloc(len)) == NULL)
	{
		browser_error(browser, strerror(errno), 1);
		return;
	}
	snprintf(path, len, "%s/%s", location, newfolder);
	if(mkdir(path, 0777) != 0)
		browser_error(browser, strerror(errno), 1);
	free(path);
}


/* on_file_new_symlink */
void on_file_new_symlink(gpointer data)
{
	Browser * browser = data;
	char const * location;

	if((location = browser_get_location(browser)) == NULL)
		return;
	if(_common_symlink(browser->window, location) != 0)
		browser_error(browser, strerror(errno), 1);
}


/* on_file_close */
void on_file_close(gpointer data)
{
	on_closex(data);
}


/* on_file_open_file */
void on_file_open_file(gpointer data)
{
	on_open_file(data);
}


/* edit menu */
/* on_edit_copy */
void on_edit_copy(gpointer data)
{
	on_copy(data);
}


/* on_edit_cut */
void on_edit_cut(gpointer data)
{
	on_cut(data);
}


/* on_edit_delete */
void on_edit_delete(gpointer data)
{
	Browser * browser = data;
	GtkWidget * dialog;
	unsigned long cnt = 0;
	int res = GTK_RESPONSE_YES;
	GList * selection;
	GList * p;

	if((selection = _copy_selection(browser)) == NULL)
		return;
	for(p = selection; p != NULL; p = p->next)
		if(p->data != NULL)
			cnt++;
	if(cnt == 0)
		return;
	if(browser->prefs.confirm_before_delete == TRUE)
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(browser->window),
				GTK_DIALOG_MODAL
				| GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
#if GTK_CHECK_VERSION(2, 6, 0)
				"%s", _("Warning"));
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(
					dialog),
#endif
				_("Are you sure you want to delete %lu"
					" file(s)?"), cnt);
		gtk_window_set_title(GTK_WINDOW(dialog), _("Warning"));
		res = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
	if(res == GTK_RESPONSE_YES
			&& _common_exec("delete", "-ir", selection) != 0)
		browser_error(browser, strerror(errno), 1);
	g_list_foreach(selection, (GFunc)free, NULL);
	g_list_free(selection);
}


/* on_edit_paste */
void on_edit_paste(gpointer data)
{
	on_paste(data);
}


/* on_edit_select_all */
void on_edit_select_all(gpointer data)
{
	Browser * browser = data;

	browser_select_all(browser);
}


/* on_edit_preferences */
void on_edit_preferences(gpointer data)
{
	Browser * browser = data;

	browser_show_preferences(browser);
}


/* on_edit_unselect_all */
void on_edit_unselect_all(gpointer data)
{
	Browser * browser = data;

	browser_unselect_all(browser);
}


/* view menu */
/* on_view_home */
void on_view_home(gpointer data)
{
	on_home(data);
}


#if GTK_CHECK_VERSION(2, 6, 0)
/* on_view_details */
void on_view_details(gpointer data)
{
	Browser * browser = data;

	browser_set_view(browser, BV_DETAILS);
}


/* on_view_icons */
void on_view_icons(gpointer data)
{
	Browser * browser = data;

	browser_set_view(browser, BV_ICONS);
}


/* on_view_list */
void on_view_list(gpointer data)
{
	Browser * browser = data;

	browser_set_view(browser, BV_LIST);
}


/* on_view_thumbnails */
void on_view_thumbnails(gpointer data)
{
	Browser * browser = data;

	browser_set_view(browser, BV_THUMBNAILS);
}
#endif /* GTK_CHECK_VERSION(2, 6, 0) */


/* help menu */
/* on_help_about */
void on_help_about(gpointer data)
{
	Browser * browser = data;

	browser_about(browser);
}


/* on_help_contents */
void on_help_contents(gpointer data)
{
	desktop_help_contents(PACKAGE, PROGNAME);
}


/* toolbar */
/* on_back */
void on_back(gpointer data)
{
	Browser * browser = data;

	browser_go_back(browser);
}


/* on_copy */
void on_copy(gpointer data)
{
	Browser * browser = data;
	GtkWidget * entry;

	entry = gtk_bin_get_child(GTK_BIN(browser->tb_path));
	if(gtk_window_get_focus(GTK_WINDOW(browser->window)) == entry)
	{
		gtk_editable_copy_clipboard(GTK_EDITABLE(entry));
		return;
	}
	g_list_foreach(browser->selection, (GFunc)free, NULL);
	g_list_free(browser->selection);
	browser->selection = _copy_selection(browser);
	browser->selection_cut = 0;
}


/* on_cut */
void on_cut(gpointer data)
{
	Browser * browser = data;
	GtkWidget * entry;

	entry = gtk_bin_get_child(GTK_BIN(browser->tb_path));
	if(gtk_window_get_focus(GTK_WINDOW(browser->window)) == entry)
	{
		gtk_editable_cut_clipboard(GTK_EDITABLE(entry));
		return;
	}
	g_list_foreach(browser->selection, (GFunc)free, NULL);
	g_list_free(browser->selection);
	browser->selection = _copy_selection(browser);
	browser->selection_cut = 1;
}


/* on_forward */
void on_forward(gpointer data)
{
	Browser * browser = data;

	browser_go_forward(browser);
}


/* on_home */
void on_home(gpointer data)
{
	Browser * browser = data;

	browser_go_home(browser);
}


/* on_paste */
void on_paste(gpointer data)
{
	Browser * browser = data;
	GtkWidget * entry;

	entry = gtk_bin_get_child(GTK_BIN(browser->tb_path));
	if(gtk_window_get_focus(GTK_WINDOW(browser->window)) == entry)
	{
		gtk_editable_paste_clipboard(GTK_EDITABLE(entry));
		return;
	}
	_paste_selection(browser);
}


/* properties */
/* on_properties */
void on_properties(gpointer data)
{
	Browser * browser = data;
	char const * location;
	char * p;
	GList * selection;

	if((location = browser_get_location(browser)) == NULL)
		return;
	if((selection = _copy_selection(browser)) == NULL)
	{
		if((p = strdup(location)) == NULL)
		{
			browser_error(browser, strerror(errno), 1);
			return;
		}
		selection = g_list_append(NULL, p);
	}
	if(_common_exec("properties", NULL, selection) != 0)
		browser_error(browser, strerror(errno), 1);
	g_list_foreach(selection, (GFunc)free, NULL);
	g_list_free(selection);
}


/* on_refresh */
void on_refresh(gpointer data)
{
	Browser * browser = data;

	browser_refresh(browser);
}


/* on_updir */
void on_updir(gpointer data)
{
	Browser * browser = data;
	char const * location;
	char * dir;

	if((location = browser_get_location(browser)) == NULL)
		return;
	dir = g_path_get_dirname(location);
	browser_set_location(browser, dir);
	g_free(dir);
}


#if GTK_CHECK_VERSION(2, 6, 0)
/* on_view_as */
void on_view_as(gpointer data)
{
	Browser * browser = data;
	BrowserView view;

	view = browser_get_view(browser);
	switch(view)
	{
		case BV_DETAILS:
			browser_set_view(browser, BV_ICONS);
			break;
		case BV_LIST:
			browser_set_view(browser, BV_THUMBNAILS);
			break;
		case BV_ICONS:
			browser_set_view(browser, BV_LIST);
			break;
		case BV_THUMBNAILS:
			browser_set_view(browser, BV_DETAILS);
			break;
	}
}
#endif


/* address bar */
/* on_path_activate */
void on_path_activate(gpointer data)
{
	Browser * browser = data;
	GtkWidget * widget;
	gchar const * p;

	widget = gtk_bin_get_child(GTK_BIN(browser->tb_path));
	p = gtk_entry_get_text(GTK_ENTRY(widget));
	browser_set_location(browser, p);
}


/* view */
/* on_detail_default */
static void _default_do(Browser * browser, GtkTreePath * path);

void on_detail_default(GtkTreeView * view, GtkTreePath * path,
		GtkTreeViewColumn * column, gpointer data)
{
	Browser * browser = data;

	if(GTK_TREE_VIEW(browser->detailview) != view)
		return;
	_default_do(browser, path);
}

static void _default_do(Browser * browser, GtkTreePath * path)
{
	char * location;
	GtkTreeIter iter;
	gboolean is_dir;

	gtk_tree_model_get_iter(GTK_TREE_MODEL(browser->store), &iter, path);
	gtk_tree_model_get(GTK_TREE_MODEL(browser->store), &iter, BC_PATH,
			&location, BC_IS_DIRECTORY, &is_dir, -1);
	if(is_dir)
		browser_set_location(browser, location);
	else if(browser->mime == NULL
			|| mime_action(browser->mime, "open", location) != 0)
		browser_open_with(browser, location);
	g_free(location);
}


#if GTK_CHECK_VERSION(2, 6, 0)
/* on_icon_default */
void on_icon_default(GtkIconView * view, GtkTreePath * path, gpointer data)
{
	Browser * browser = data;

	if(GTK_ICON_VIEW(browser->iconview) != view)
		return;
	_default_do(browser, path);
}
#endif


/* on_filename_edited */
void on_filename_edited(GtkCellRendererText * renderer, gchar * path,
		gchar * filename, gpointer data)
{
	Browser * browser = data;
	GtkTreeModel * model = GTK_TREE_MODEL(browser->store);
	GtkTreeIter iter;
	int isdir = 0;
	char * to;
	ssize_t len;
	char * q;
	char * f = filename;
	GError * error = NULL;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", \"%s\") \"%s\"\n", __func__, path,
			filename);
#endif
	if(gtk_tree_model_get_iter_from_string(model, &iter, path) != TRUE)
		return; /* XXX report error */
	path = NULL;
	gtk_tree_model_get(model, &iter, BC_IS_DIRECTORY, &isdir, BC_PATH,
			&path, -1);
	if(path == NULL)
		return; /* XXX report error */
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() \"%s\"\n", __func__, path);
#endif
	if((q = strrchr(path, '/')) == NULL)
	{
		free(path);
		return; /* XXX report error */
	}
	len = q - path;
	/* obtain the real new filename */
	if((q = g_filename_from_utf8(filename, -1, NULL, NULL, &error)) == NULL)
	{
		browser_error(NULL, error->message, 1);
		g_error_free(error);
	}
	else
		f = q;
	/* generate the complete new path */
	if((to = malloc(len + strlen(f) + 2)) == NULL)
	{
		browser_error(NULL, strerror(errno), 1);
		free(path);
		return;
	}
	strncpy(to, path, len);
	sprintf(&to[len], "/%s", f);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() \"%s\"\n", __func__, to);
#endif
	/* rename */
	if(rename(path, to) != 0)
		browser_error(browser, strerror(errno), 1);
	else if(strchr(filename, '/') == NULL)
		gtk_list_store_set(browser->store, &iter, BC_PATH, to,
				BC_DISPLAY_NAME, filename, -1);
	free(to);
	free(q);
	free(path);
}


#if GTK_CHECK_VERSION(2, 8, 0)
/* on_view_drag_data_get */
void on_view_drag_data_get(GtkWidget * widget, GdkDragContext * context,
		GtkSelectionData * seldata, guint info, guint time,
		gpointer data)
	/* XXX could be more optimal */
{
	Browser * browser = data;
	GList * selection;
	GList * s;
	size_t len = 0;
	size_t l;
	char * p = NULL;
	char * q;

	selection = _copy_selection(browser);
	for(s = selection; s != NULL; s = s->next)
	{
		l = strlen(s->data) + 1;
		if((q = realloc(p, len + l)) == NULL)
			continue; /* XXX report error */
		p = q;
		memcpy(&p[len], s->data, l);
		len += l;
	}
	gtk_selection_data_set_text(seldata, p, len);
	free(p);
	g_list_foreach(selection, (GFunc)free, NULL);
	g_list_free(selection);
}


/* on_view_drag_data_received */
void on_view_drag_data_received(GtkWidget * widget, GdkDragContext * context,
		gint x, gint y, GtkSelectionData * seldata, guint info,
		guint time, gpointer data)
	/* FIXME - may not be an icon view
	 *       - not fully checking if the source matches */
{
	Browser * browser = data;
	GtkTreePath * path;
	GtkTreeIter iter;
	char const * location;
	gchar * p = NULL;

	path = gtk_icon_view_get_path_at_pos(GTK_ICON_VIEW(browser->iconview),
			x, y);
	if(path == NULL)
		location = browser_get_location(browser);
	else
	{
		gtk_tree_model_get_iter(GTK_TREE_MODEL(browser->store), &iter,
				path);
		gtk_tree_model_get(GTK_TREE_MODEL(browser->store), &iter,
				BC_PATH, &p, -1);
		location = p;
	}
	if(_common_drag_data_received(context, seldata, location) != 0)
		browser_error(browser, strerror(errno), 1);
	g_free(p);
}
#endif /* GTK_CHECK_VERSION(2, 8, 0) */


/* on_view_popup */
gboolean on_view_popup(GtkWidget * widget, gpointer data)
{
	GdkEventButton event;

	memset(&event, 0, sizeof(event));
	event.type = GDK_BUTTON_PRESS;
	event.button = 0;
	event.time = gtk_get_current_event_time();
	return on_view_press(widget, &event, data);
}


/* on_view_press */
/* types */
typedef struct _IconCallback
{
	Browser * browser;
	int isdir;
	int isexec;
	int ismnt;
	char * path;
} IconCallback;

static gboolean _press_context(Browser * browser, GdkEventButton * event,
		GtkWidget * menu, IconCallback * ic);
static void _press_directory(GtkWidget * menu, IconCallback * ic);
static void _press_file(Browser * browser, GtkWidget * menu, char * mimetype,
		IconCallback * ic);
static void _press_mime(Mime * mime, char const * mimetype, char const * action,
		char const * label, GCallback callback, IconCallback * ic,
		GtkWidget * menu);
static gboolean _press_show(Browser * browser, GdkEventButton * event,
		GtkWidget * menu);

/* callbacks */
static void _on_icon_delete(gpointer data);
static void _on_icon_open(gpointer data);
static void _on_icon_open_new_window(gpointer data);
static void _on_icon_edit(gpointer data);
static void _on_icon_open_with(gpointer data);
static void _on_icon_run(gpointer data);
static void _on_icon_paste(gpointer data);
static void _on_icon_unmount(gpointer data);

gboolean on_view_press(GtkWidget * widget, GdkEventButton * event,
		gpointer data)
{
	static IconCallback ic;
	Browser * browser = data;
	GtkTreePath * path = NULL;
	GtkTreeIter iter;
	GtkTreeSelection * sel;
	GtkWidget * menuitem;
	char * mimetype = NULL;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %d\n", __func__, event->button);
#endif
	if(event->type != GDK_BUTTON_PRESS
			|| (event->button != 3 && event->button != 0))
		return FALSE;
	widget = gtk_menu_new();
	/* FIXME prevents actions to be called but probably leaks memory
	g_signal_connect(G_OBJECT(widget), "deactivate", G_CALLBACK(
				gtk_widget_destroy), NULL); */
#if GTK_CHECK_VERSION(2, 6, 0)
	if(browser_get_view(browser) != BV_DETAILS)
		path = gtk_icon_view_get_path_at_pos(GTK_ICON_VIEW(
					browser->iconview), (int)event->x,
				(int)event->y);
	else
#endif
		gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(
					browser->detailview), (int)event->x,
				(int)event->y, &path, NULL, NULL, NULL);
	ic.browser = browser;
	ic.isdir = 0;
	ic.isexec = 0;
	ic.path = NULL;
	if(path == NULL)
		return _press_context(browser, event, widget, &ic);
	/* FIXME error checking + sub-functions */
	gtk_tree_model_get_iter(GTK_TREE_MODEL(browser->store), &iter, path);
#if GTK_CHECK_VERSION(2, 6, 0)
	if(browser_get_view(browser) != BV_DETAILS)
	{
		if(gtk_icon_view_path_is_selected(GTK_ICON_VIEW(
						browser->iconview), path)
				== FALSE)
		{
			browser_unselect_all(browser);
			gtk_icon_view_select_path(GTK_ICON_VIEW(
						browser->iconview), path);
		}
	}
	else
#endif
	{
		sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(
					browser->detailview));
		if(!gtk_tree_selection_iter_is_selected(sel, &iter))
		{
			browser_unselect_all(browser);
			gtk_tree_selection_select_iter(sel, &iter);
		}
	}
	gtk_tree_model_get(GTK_TREE_MODEL(browser->store), &iter,
			BC_PATH, &ic.path, BC_IS_DIRECTORY, &ic.isdir,
			BC_IS_EXECUTABLE, &ic.isexec,
			BC_IS_MOUNT_POINT, &ic.ismnt, BC_MIME_TYPE, &mimetype,
			-1);
	if(ic.isdir == TRUE)
		_press_directory(widget, &ic);
	else
		_press_file(browser, widget, mimetype, &ic);
	g_free(mimetype);
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(widget), menuitem);
	menuitem = gtk_image_menu_item_new_from_stock(
			GTK_STOCK_PROPERTIES, NULL);
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				on_properties), browser);
	gtk_menu_shell_append(GTK_MENU_SHELL(widget), menuitem);
#if !GTK_CHECK_VERSION(2, 6, 0)
	gtk_tree_path_free(path);
#endif
	return _press_show(browser, event, widget);
}

static void _on_popup_new_text_file(gpointer data);
static void _on_popup_new_folder(gpointer data);
static void _on_popup_new_symlink(gpointer data);
static gboolean _press_context(Browser * browser, GdkEventButton * event,
		GtkWidget * menu, IconCallback * ic)
{
	GtkWidget * menuitem;
	GtkWidget * submenu;
#if GTK_CHECK_VERSION(2, 8, 0)
	GtkWidget * image;
#endif

	browser_unselect_all(browser);
	/* new submenu */
	menuitem = gtk_menu_item_new_with_label(_("New"));
	submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), submenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
#if GTK_CHECK_VERSION(2, 8, 0) /* XXX actually depends on the icon theme */
	menuitem = gtk_image_menu_item_new_with_label(_("Folder"));
	image = gtk_image_new_from_icon_name("folder-new", GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
#else
	menuitem = gtk_menu_item_new_with_label(_("Folder"));
#endif
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				_on_popup_new_folder), ic);
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menuitem);
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menuitem);
	menuitem = gtk_menu_item_new_with_label(_("Symbolic link..."));
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				_on_popup_new_symlink), ic);
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menuitem);
	menuitem = gtk_image_menu_item_new_with_label(_("Text file"));
	image = gtk_image_new_from_icon_name("stock_new-text",
			GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				_on_popup_new_text_file), ic);
	gtk_menu_shell_append(GTK_MENU_SHELL(submenu), menuitem);
	/* cut/copy/paste */
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_CUT, NULL);
	gtk_widget_set_sensitive(menuitem, FALSE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY, NULL);
	gtk_widget_set_sensitive(menuitem, FALSE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE, NULL);
	if(browser->selection != NULL)
		g_signal_connect_swapped(G_OBJECT(menuitem), "activate",
				G_CALLBACK(_on_icon_paste), ic);
	else
		gtk_widget_set_sensitive(menuitem, FALSE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_PROPERTIES,
			NULL);
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				on_properties), browser);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	return _press_show(browser, event, menu);
}

static void _on_popup_new_text_file(gpointer data)
{
	char const * newtext = _("New text file.txt");
	IconCallback * ic = data;
	Browser * browser = ic->browser;
	char const * location;
	size_t len;
	char * path;
	int fd;

	if((location = browser_get_location(browser)) == NULL)
		return;
	len = strlen(location) + strlen(newtext) + 2;
	if((path = malloc(len)) == NULL)
	{
		browser_error(browser, strerror(errno), 1);
		return;
	}
	snprintf(path, len, "%s/%s", location, newtext);
	if((fd = creat(path, 0666)) < 0)
		browser_error(browser, strerror(errno), 1);
	else
		close(fd);
	free(path);
}

static void _on_popup_new_folder(gpointer data)
{
	IconCallback * ic = data;
	Browser * browser = ic->browser;

	on_file_new_folder(browser);
}

static void _on_popup_new_symlink(gpointer data)
{
	IconCallback * ic = data;
	Browser * browser = ic->browser;

	on_file_new_symlink(browser);
}

static void _press_directory(GtkWidget * menu, IconCallback * ic)
{
	GtkWidget * menuitem;

	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				_on_icon_open), ic);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_with_mnemonic(
			_("Open in new _window"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),
			gtk_image_new_from_icon_name("window-new",
				GTK_ICON_SIZE_MENU));
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				_on_icon_open_new_window), ic);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_CUT, NULL);
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				on_edit_cut), ic->browser);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY, NULL);
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				on_edit_copy), ic->browser);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE, NULL);
	if(ic->browser->selection == NULL)
		gtk_widget_set_sensitive(menuitem, FALSE);
	else /* FIXME only if just this one is selected */
		g_signal_connect_swapped(G_OBJECT(menuitem), "activate",
				G_CALLBACK(_on_icon_paste), ic);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if(ic->ismnt)
	{
		menuitem = gtk_menu_item_new_with_mnemonic(_("_Unmount"));
		g_signal_connect_swapped(G_OBJECT(menuitem), "activate",
				G_CALLBACK(_on_icon_unmount), ic);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		menuitem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				_on_icon_delete), ic);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
}

static void _press_file(Browser * browser, GtkWidget * menu, char * mimetype,
		IconCallback * ic)
{
	GtkWidget * menuitem;

	_press_mime(browser->mime, mimetype, "open", GTK_STOCK_OPEN, G_CALLBACK(
				_on_icon_open), ic, menu);
	_press_mime(browser->mime, mimetype, "edit",
#if GTK_CHECK_VERSION(2, 6, 0)
			GTK_STOCK_EDIT,
#else
			"_Edit",
#endif
			G_CALLBACK(_on_icon_edit), ic, menu);
	if(ic->isexec)
	{
		menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_EXECUTE,
				NULL);
		g_signal_connect_swapped(G_OBJECT(menuitem), "activate",
				G_CALLBACK(_on_icon_run), ic);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}
	menuitem = gtk_menu_item_new_with_mnemonic(_("Open _with..."));
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				_on_icon_open_with), ic);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_CUT, NULL);
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				on_edit_cut), browser);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY, NULL);
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				on_edit_copy), browser);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE, NULL);
	gtk_widget_set_sensitive(menuitem, FALSE);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", G_CALLBACK(
				_on_icon_delete), ic);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
}

static void _press_mime(Mime * mime, char const * mimetype, char const * action,
		char const * label, GCallback callback, IconCallback * ic,
		GtkWidget * menu)
{
	GtkWidget * menuitem;

	if(mime == NULL || mime_get_handler(mime, mimetype, action) == NULL)
		return;
	if(strncmp(label, "gtk-", 4) == 0)
		menuitem = gtk_image_menu_item_new_from_stock(label, NULL);
	else
		menuitem = gtk_menu_item_new_with_mnemonic(label);
	g_signal_connect_swapped(G_OBJECT(menuitem), "activate", callback, ic);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
}

static gboolean _press_show(Browser * browser, GdkEventButton * event,
		GtkWidget * menu)
{
#if GTK_CHECK_VERSION(2, 6, 0)
	if(browser_get_view(browser) != BV_DETAILS)
		gtk_menu_attach_to_widget(GTK_MENU(menu), browser->iconview,
				NULL);
	else
#endif
		gtk_menu_attach_to_widget(GTK_MENU(menu), browser->detailview,
				NULL);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button,
			event->time);
	return TRUE;
}

static void _on_icon_delete(gpointer data)
{
	IconCallback * cb = data;

	/* FIXME not selected => cursor */
	on_edit_delete(cb->browser);
}

static void _on_icon_open(gpointer data)
{
	IconCallback * cb = data;

	if(cb->isdir)
		browser_set_location(cb->browser, cb->path);
	else if(cb->browser->mime != NULL)
		mime_action(cb->browser->mime, "open", cb->path);
}

static void _on_icon_open_new_window(gpointer data)
{
	IconCallback * cb = data;

	if(!cb->isdir)
		return;
	browser_new(cb->path);
}

static void _on_icon_edit(gpointer data)
{
	IconCallback * cb = data;

	if(cb->browser->mime != NULL)
		mime_action(cb->browser->mime, "edit", cb->path);
}

static void _on_icon_run(gpointer data)
	/* FIXME does not work with scripts */
{
	IconCallback * cb = data;
	GtkWidget * dialog;
	int res;
	GError * error = NULL;
	char * argv[2];

	dialog = gtk_message_dialog_new(GTK_WINDOW(cb->browser->window),
			GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
			GTK_BUTTONS_YES_NO,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Warning"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s", _("Are you sure you want to execute this file?"));
	gtk_window_set_title(GTK_WINDOW(dialog), _("Warning"));
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if(res != GTK_RESPONSE_YES)
		return;
	argv[0] = cb->path;
	if(g_spawn_async(NULL, argv, NULL, 0, NULL, NULL, NULL, &error) != TRUE)
	{
		browser_error(cb->browser, error->message, 1);
		g_error_free(error);
	}
}

static void _on_icon_open_with(gpointer data)
{
	IconCallback * cb = data;

	browser_open_with(cb->browser, cb->path);
}

static void _on_icon_paste(gpointer data)
{
	IconCallback * cb = data;
	char const * location;

	if((location = browser_get_location(cb->browser)) == NULL)
		return;
	/* XXX the following assignments are totally ugly */
	if(cb->path != NULL)
		cb->browser->current->data = cb->path;
	_paste_selection(cb->browser);
	cb->browser->current->data = location;
}

static void _on_icon_unmount(gpointer data)
{
	IconCallback * cb = data;

#ifndef unmount
	errno = ENOSYS;
#else
	if(unmount(cb->path, 0) != 0)
#endif
		browser_error(cb->browser, strerror(errno), 1);
}


/* private */
/* functions */
/* copy_selection */
static GList * _copy_selection(Browser * browser)
{
	GtkTreeSelection * treesel;
	GList * sel;
	GList * p;
	GtkTreeIter iter;
	char * q;

#if GTK_CHECK_VERSION(2, 6, 0)
	if(browser_get_view(browser) != BV_DETAILS)
		sel = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(
					browser->iconview));
	else
#endif
	if((treesel = gtk_tree_view_get_selection(GTK_TREE_VIEW(
						browser->detailview))) == NULL)
		return NULL;
	else
		sel = gtk_tree_selection_get_selected_rows(treesel, NULL);
	for(p = NULL; sel != NULL; sel = sel->next)
	{
		if(!gtk_tree_model_get_iter(GTK_TREE_MODEL(browser->store),
					&iter, sel->data))
			continue;
		gtk_tree_model_get(GTK_TREE_MODEL(browser->store), &iter,
				BC_PATH, &q, -1);
		p = g_list_append(p, q);
	}
	g_list_foreach(sel, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(sel); /* XXX can probably be optimized for re-use */
	return p;
}


/* paste_selection */
static void _paste_selection(Browser * browser)
{
	char const * location;
	char * p;

	if(browser->selection == NULL
			|| (location = browser_get_location(browser)) == NULL)
		return;
	if((p = strdup(location)) == NULL)
	{
		browser_error(browser, strerror(errno), 1);
		return;
	}
	browser->selection = g_list_append(browser->selection, p);
	if(browser->selection_cut != 1)
	{
		/* copy the selection */
		if(_common_exec("copy", "-ir", browser->selection) != 0)
			browser_error(browser, strerror(errno), 1);
		browser->selection = g_list_remove(browser->selection, p);
		free(p);
	}
	else
	{
		/* move the selection */
		if(_common_exec("move", "-i", browser->selection) != 0)
			browser_error(browser, strerror(errno), 1);
		else
		{
			g_list_foreach(browser->selection, (GFunc)free, NULL);
			g_list_free(browser->selection);
			browser->selection = NULL;
		}
	}
}
