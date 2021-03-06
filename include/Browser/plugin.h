/* $Id$ */
/* Copyright (c) 2012 Pierre Pronchery <khorben@defora.org> */
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



#ifndef DESKTOP_BROWSER_PLUGIN_H
# define DESKTOP_BROWSER_PLUGIN_H

# include <sys/stat.h>
# include <gtk/gtk.h>
# include <Desktop.h>


/* Browser */
/* public */
/* types */
typedef struct _Browser Browser;

typedef struct _BrowserPlugin BrowserPlugin;

typedef struct _BrowserPluginHelper
{
	Browser * browser;
	int (*error)(Browser * browser, char const * message, int ret);
	GdkPixbuf * (*get_icon)(Browser * browser, char const * filename,
			char const * type, struct stat * lst, struct stat * st,
			int size);
	Mime * (*get_mime)(Browser * browser);
	char const * (*get_type)(Browser * browser, char const * filename,
			mode_t mode);
	int (*set_location)(Browser * browser, char const * path);
} BrowserPluginHelper;

typedef const struct _BrowserPluginDefinition
{
	char const * name;
	char const * icon;
	char const * description;
	BrowserPlugin * (*init)(BrowserPluginHelper * helper);
	void (*destroy)(BrowserPlugin * plugin);
	GtkWidget * (*get_widget)(BrowserPlugin * plugin);
	void (*refresh)(BrowserPlugin * plugin, GList * selection);
} BrowserPluginDefinition;

#endif /* !DESKTOP_BROWSER_PLUGIN_H */
