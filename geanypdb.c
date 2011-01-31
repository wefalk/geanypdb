/*
 *      geanypdb.c - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2007-2010 Enrico Tröger <enrico(dot)troeger(at)uvena(dot)de>
 *      Copyright 2007-2010 Nick Treleaven <nick(dot)treleaven(at)btinternet(dot)com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 *
 * $Id: geanypdb.c 4532 2010-01-18 16:57:37Z Juan Manuel García $
 */

/**
 *   GeanyPDB Plugin - A Integration With WinPdb and Pdb Python Debbugers
 *
 * Note: This is not installed by default, but (on *nix) you can build it as follows:
 * cd plugins
 * make geanypdb.so
 *
 * Then copy or symlink the plugins/geanypdb.so file to ~/.config/geany/plugins
 * - it will be loaded at next startup.
 */

#include "geanyplugin.h"    /* plugin API, always comes first */
#include "Scintilla.h"  /* for the SCNotification struct */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>


/* These items are set by Geany before plugin_init() is called. */
GeanyPlugin     *geany_plugin;
GeanyData       *geany_data;
GeanyFunctions  *geany_functions;


/* Check that the running Geany supports the plugin API version used below, and check
 * for binary compatibility. */
PLUGIN_VERSION_CHECK(147);

/* All plugins must set name, description, version and author. */
PLUGIN_SET_INFO(_("GeanyPDB"), _("Integrate Geany With Pdb"), "0.1" , _("Juan Manuel García"));


/***********************************************************************
*                          GTK Menu Items
***********************************************************************/

GtkWidget* root;
GtkWidget* menu;
GtkWidget* item;

#define MENU_POS 7

#define WIN_PDB_ITEM_TEXT "Run With WinPdb"
#define PDB_ITEM_TEXT "Run With Pdb"
#define DEBUG_ITEM_TEXT "Debug"

/**********************************************************************/

#define PATH_TO_PDB "/usr/bin/pdb"
#define PATH_TO_WINPDB "/usr/bin/winpdb"

char breaks[1000];

static gboolean on_editor_notify(GObject *object, GeanyEditor *editor,
                                 SCNotification *nt, gpointer data)
{
    /* For detailed documentation about the SCNotification struct, please see
     * http://www.scintilla.org/ScintillaDoc.html#Notifications. */
    switch (nt->nmhdr.code)
    {
        case SCN_UPDATEUI:
            /* This notification is sent very often, you should not do time-consuming tasks here */
            break;
        case SCN_CHARADDED:
            /* For demonstrating purposes simply print the typed character in the status bar */
            ui_set_statusbar(FALSE, _("Typed character: %c"), nt->ch);
            break;
        case SCN_URIDROPPED:
        {
            /* Show a message dialog with the dropped URI list when files (i.e. a list of
             * filenames) is dropped to the editor widget) */
            if (nt->text != NULL)
            {
                GtkWidget *dialog;

                dialog = gtk_message_dialog_new(
                    GTK_WINDOW(geany->main_widgets->window),
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_INFO,
                    GTK_BUTTONS_OK,
                    _("The following files were dropped:"));
                gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                    "%s", nt->text);

                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
            }
            /* we return TRUE here which prevents Geany from processing the SCN_URIDROPPED
             * notification, i.e. Geany won't open the passed files */
            return TRUE;
        }
    }

    return FALSE;
}


PluginCallback plugin_callbacks[] =
{
    /* Set 'after' (third field) to TRUE to run the callback @a after the default handler.
     * If 'after' is FALSE, the callback is run @a before the default handler, so the plugin
     * can prevent Geany from processing the notification. Use this with care. */
    { "editor-notify", (GCallback) &on_editor_notify, FALSE, NULL },
    { NULL, NULL, FALSE, NULL }
};


static void show_error(const char* error)
{
    dialogs_show_msgbox(GTK_MESSAGE_ERROR, "%s", _(error));
}
static void show_int(int num)
{
    dialogs_show_msgbox(GTK_MESSAGE_INFO, "%d", num);
}

static void run_debugger(char* file_path, const char* command)
{
    int pid;
    /* fork for winpdb process */
    pid = fork();

    /* if is the child process excev the winpdb */
    if (pid == 0)
    {
        int ret = 0;
        char *cmd[] = {"", file_path , (char *)0 };

        ret = execv(command, cmd);

        if (ret == -1)
            show_error("Error creating the process");
    }
}
static void run_pdb(char* path)
{
    int pid, i;
    /* fork for winpdb process */
    pid = fork();

    /* if is the child process excev the winpdb */
    if (pid == 0)
    {
        int ret = 0;
        char *cmd[] = {"terminal.sh", breaks, path};

        ret = execv("terminal.sh", cmd);

        if (ret == -1)
            show_error("Error creating the process");

    }
    for (i = 0; i<1000; i++)
        breaks[i] = (char)0;
}
static GeanyDocument* save_current_file()
{
    /* get the current active document */
    GeanyDocument* current = document_get_current();

    /* if the file don't exists show a save file dialog
    else save it and run winpdb */
    if (current->file_name == NULL)
    {
        if(!(dialogs_show_save_as()))
        {
            show_error("You Must Save the File Before Debug");
            return;
        }
    }
    else
    {
        document_save_file(current, 0);
    }
    return current;
}
static void on_winpdb_item_activate(GtkMenuItem *menuitem, gpointer gdata)
{
    GeanyDocument* current = save_current_file();
    run_debugger(current->real_path, PATH_TO_WINPDB);
}
void breakpoints_delete_all(ScintillaObject* sci)
{
    scintilla_send_message(sci, SCI_MARKERDELETEALL, -1, 0);
}
int breakpoints_get(ScintillaObject* sci, int line)
{
    return scintilla_send_message(sci, SCI_MARKERGET, line, 0);
}
static void _set_array(char* path, int number)
{
    char* string = malloc(10);
    sprintf(string, "%d", number);

    strcat(breaks, path);
    strcat(breaks, ":");

    strcat(breaks, string);
    strcat(breaks, ",");
    free(string);
}
static void get_breaks(GeanyDocument* doc)
{
    int i;
    int count;
    ScintillaObject* sci = (ScintillaObject*)doc->editor->sci;
    count = scintilla_send_message(sci, SCI_GETLINECOUNT, 0, 0);

    for (i = 0; i < count; i++)
    {
        if(breakpoints_get(sci, i))
        {
            _set_array(doc->real_path, i+1);
            //scintilla_send_message(sci, SCI_MARKERDEFINE, 1, SC_MARK_BACKGROUND);
            //scintilla_send_message(sci, SCI_MARKERSETBACK, 1, 0 | 255 | 0);
        }
    }
}
static void get_documents_breaks()
{
    int i;
    GeanyDocument* doc;
    for (i=0; ((doc = document_get_from_page(i)) != NULL); i++)
    {
        get_breaks(doc);
    }
}
static void on_pdb_item_activate(GtkMenuItem *menuitem, gpointer gdata)
{
    get_documents_breaks();
    GeanyDocument* current = save_current_file();
    run_pdb(current->real_path);
}

static void add_item_menu(GtkWidget* menu, const char* item_text, void* callback)
{
    item = gtk_menu_item_new_with_mnemonic(_(item_text));
    gtk_menu_append(GTK_MENU(menu), item);
    g_signal_connect(item, "activate", G_CALLBACK(callback), NULL);
}

static void make_ui()
{
    root = gtk_menu_item_new_with_mnemonic(_(DEBUG_ITEM_TEXT));
    menu = gtk_menu_new();

    add_item_menu(menu, WIN_PDB_ITEM_TEXT, on_winpdb_item_activate);
    add_item_menu(menu, PDB_ITEM_TEXT, on_pdb_item_activate);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(root), menu);

    /* Insert the Debug Menu in the toolbar at the MENU_POS index*/
    gtk_menu_insert(GTK_CONTAINER(ui_lookup_widget(geany->main_widgets->window, "menubar1")), root, MENU_POS);

    gtk_widget_show_all(root);

    /* make the menu item sensitive only when documents are open */
    ui_add_document_sensitive(root);

    /*gtk_notebook_set_current_page((GtkNotebook*)geany->main_widgets->message_window_notebook, 4);*/
}

/* Called by Geany to initialize the plugin.
 * Note: data is the same as geany_data. */
void plugin_init(GeanyData *data)
{
    make_ui();
}

/* Callback connected in plugin_configure(). */
static void on_configure_response(GtkDialog *dialog, gint response, gpointer user_data)
{
    /* catch OK or Apply clicked */
    if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY)
    {
        /* We only have one pref here, but for more you would use a struct for user_data */
        /* maybe the plugin should write here the settings into a file
         * (e.g. using GLib's GKeyFile API)
         * all plugin specific files should be created in:
         * geany->app->configdir G_DIR_SEPARATOR_S plugins G_DIR_SEPARATOR_S pluginname G_DIR_SEPARATOR_S
         * e.g. this could be: ~/.config/geany/plugins/Demo/, please use geany->app->configdir */
    }
}


/* Called by Geany to show the plugin's configure dialog. This function is always called after
 * plugin_init() was called.
 * You can omit this function if the plugin doesn't need to be configured.
 * Note: parent is the parent window which can be used as the transient window for the created
 *       dialog. */
GtkWidget *plugin_configure(GtkDialog *dialog)
{
    //TODO: Implement a configure including paths to winpdb and pdb

    GtkWidget *label, *entry, *vbox;

    /* example configuration dialog */
    vbox = gtk_vbox_new(FALSE, 6);

    /* add a label and a text entry to the dialog */
    label = gtk_label_new(_("Welcome text to show:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    entry = gtk_entry_new();

    gtk_container_add(GTK_CONTAINER(vbox), label);
    gtk_container_add(GTK_CONTAINER(vbox), entry);

    gtk_widget_show_all(vbox);

    /* Connect a callback for when the user clicks a dialog button */
    g_signal_connect(dialog, "response", G_CALLBACK(on_configure_response), entry);
    return vbox;
}


/* Called by Geany before unloading the plugin.
 * Here any UI changes should be removed, memory freed and any other finalization done.
 * Be sure to leave Geany as it was before plugin_init(). */
void plugin_cleanup(void)
{
    /* remove the menu item added in plugin_init() */
    gtk_widget_destroy(root);
    gtk_widget_destroy(menu);
    gtk_widget_destroy(item);
}
