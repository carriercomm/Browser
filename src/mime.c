/* mime.c */



#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fnmatch.h>
#include "browser.h"
#include "mime.h"


/* Mime */
static void _new_config(Mime * mime);
Mime * mime_new(void)
{
	Mime * mime;
	char * globs[] = { /* ideally taken from Gtk+ but seems impossible */
	       	"/usr/pkg/share/mime/globs",
	       	"/usr/local/share/mime/globs",
	       	"/usr/share/mime/globs",
		NULL };
	char ** g = globs;
	FILE * fp = NULL;
	char buf[256];
	size_t len;
	char * glob;
	MimeType * p;

	if((mime = malloc(sizeof(*mime))) == NULL)
		return NULL;
	for(g = globs; *g != NULL; g++)
		if((fp = fopen(*g, "r")) != NULL)
			break;
	if(fp == NULL)
	{
		perror("Error while loading MIME globs");
		free(mime);
		return NULL;
	}
	mime->types = NULL;
	mime->types_cnt = 0;
	_new_config(mime);
	while(fgets(buf, sizeof(buf), fp) != NULL)
	{
		errno = EINVAL;
		len = strlen(buf);
		if(buf[--len] != '\n')
			break;
		if(buf[0] == '#')
			continue;
		buf[len] = '\0';
		glob = strchr(buf, ':');
		*(glob++) = '\0';
		if((p = realloc(mime->types, sizeof(*(mime->types))
						* (mime->types_cnt+1))) == NULL)
			break;
		mime->types = p;
		p[mime->types_cnt].type = strdup(buf);
		p[mime->types_cnt].glob = strdup(glob);
		p[mime->types_cnt].icon = NULL;
		p[mime->types_cnt++].open = mime->config != NULL
			? config_get(mime->config, buf, "open") : NULL;
		if(p[mime->types_cnt-1].type == NULL
				|| p[mime->types_cnt-1].glob == NULL)
			break;
	}
	if(!feof(fp))
	{
		perror("/usr/share/mime/globs");
		mime_delete(mime);
		mime = NULL;
	}
	fclose(fp);
	return mime;
}

static void _new_config(Mime * mime)
{
	char * homedir;
	char * filename;

	if((homedir = getenv("HOME")) == NULL)
		return;
	if((mime->config = config_new()) == NULL)
		return;
	if((filename = malloc(strlen(homedir) + 1 + strlen(MIME_CONFIG_FILE)
					+ 1)) == NULL)
		return;
	sprintf(filename, "%s/%s", homedir, MIME_CONFIG_FILE);
	config_load(mime->config, filename);
	free(filename);
}


void mime_delete(Mime * mime)
{
	unsigned int i;

	for(i = 0; i < mime->types_cnt; i++)
	{
		free(mime->types[i].type);
		free(mime->types[i].glob);
		free(mime->types[i].icon);
		free(mime->types[i].open);
	}
	free(mime->types);
	if(mime->config != NULL)
		config_delete(mime->config);
	free(mime);
}


/* useful */
char const * mime_type(Mime * mime, char const * path)
{
	unsigned int i;

	for(i = 0; i < mime->types_cnt; i++)
		if(fnmatch(mime->types[i].glob, path, FNM_NOESCAPE) == 0)
			break;
	return i < mime->types_cnt ? mime->types[i].type : NULL;
}


void mime_open(Mime * mime, char const * path)
	/* FIXME report errors */
{
	char const * type;
	unsigned int i;
	pid_t pid;

	if((type = mime_type(mime, path)) == NULL)
		return;
	for(i = 0; i < mime->types_cnt; i++)
		if(strcmp(type, mime->types[i].type) == 0)
			break;
	if(i == mime->types_cnt || mime->types[i].open == NULL)
		return;
	if((pid = fork()) == -1)
	{
		perror("fork");
		return;
	}
	if(pid != 0)
		return;
	execlp(mime->types[i].open, mime->types[i].open, path, NULL);
	fprintf(stderr, "%s%s%s%s", "browser: ", mime->types[i].open, ": ",
			strerror(errno));
	exit(2);
}


GdkPixbuf * mime_icon(Mime * mime, GtkIconTheme * theme, char const * type)
{
	unsigned int i;
	static char buf[256] = "gnome-mime-";
	char * p;

	for(i = 0; i < mime->types_cnt; i++)
		if(strcmp(type, mime->types[i].type) == 0)
			break;
	if(i == mime->types_cnt)
		return NULL;
	if(mime->types[i].icon != NULL)
		return mime->types[i].icon;
	strncpy(&buf[11], type, sizeof(buf)-11);
	for(; (p = strchr(&buf[11], '/')) != NULL; *p = '-');
#if GTK_CHECK_VERSION(2, 6, 0)
	if((mime->types[i].icon = gtk_icon_theme_load_icon(theme, buf, 48, 0,
#else
	if((mime->types[i].icon = gtk_icon_theme_load_icon(theme, buf, 24, 0,
#endif
					NULL)) == NULL)
	{
		if((p = strchr(&buf[11], '-')) != NULL)
		{
			*p = '\0';
#if GTK_CHECK_VERSION(2, 6, 0)
			mime->types[i].icon = gtk_icon_theme_load_icon(theme,
					buf, 48, 0, NULL);
#else
			mime->types[i].icon = gtk_icon_theme_load_icon(theme,
					buf, 24, 0, NULL);
#endif
		}
	}
	return mime->types[i].icon;
}