#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include <gtk/gtk.h>

GtkWidget *view;


static GtkWidget *create_list( void )
{

    GtkWidget *scrolled_window;
    GtkWidget *tree_view;
    GtkListStore *model;
    GtkTreeIter iter;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;

    int i;

    /* Create a new scrolled window, with scrollbars only if needed */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);

    model = gtk_list_store_new (1, G_TYPE_STRING);
    tree_view = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (model));
    gtk_widget_show (tree_view);

    /* Add some messages to the window */
    for (i = 0; i < 10; i++)
    {
        gchar *msg = g_strdup_printf ("User #%d", i);
        gtk_list_store_append (GTK_LIST_STORE (model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (model),
                            &iter,
                            0, msg,
                            -1);
        g_free (msg);
    }

    cell = gtk_cell_renderer_text_new ();

    column = gtk_tree_view_column_new_with_attributes ("Users On",
             cell,
             "text", 0,
             NULL);

    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view),
                                 GTK_TREE_VIEW_COLUMN (column));

    return scrolled_window;
}


static void insert_text( GtkTextBuffer *buffer, const gchar *entry_text)
{
    GtkTextIter iter;
    GtkAdjustment *adjustment;

    gtk_text_buffer_get_iter_at_offset (buffer, &iter, gtk_text_buffer_get_char_count(buffer));

    if (gtk_text_buffer_get_char_count(buffer))
        gtk_text_buffer_insert (buffer, &iter, "\n", 1);


    gtk_text_buffer_insert (buffer, &iter,entry_text, -1);

    adjustment = gtk_text_view_get_vadjustment(GTK_TEXT_VIEW(view));

    gtk_adjustment_set_value(adjustment, gtk_adjustment_get_upper(adjustment));



}

static void enter_callback( GtkWidget *widget,
                            GtkWidget *entry )
{
    const gchar *entry_text;
    entry_text = gtk_entry_get_text (GTK_ENTRY (entry));

    //GtkWidget *view;
    GtkTextBuffer *buffer;
    //view = gtk_text_view_new ();
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

    insert_text(buffer, entry_text);

    printf ("Entry contents: %s\n", entry_text);
    gtk_entry_set_text (GTK_ENTRY (entry), "");
}


/* Create a scrolled text area that displays a "message" */
static GtkWidget *create_text( void )
{
    GtkWidget *scrolled_window;

    GtkTextBuffer *buffer;

    view = gtk_text_view_new ();
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);

    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);

    gtk_container_add (GTK_CONTAINER (scrolled_window), view);

    gtk_widget_show_all (scrolled_window);

    return scrolled_window;
}

int main( int   argc, char *argv[] )
{

    GtkWidget *window;
    GtkWidget *vbox, *hbox;
    GtkWidget *entry;
    GtkWidget *label;
    GtkWidget *button;
    GtkWidget *list;
    GtkWidget *vpaned;
    GtkWidget *text;


    gint tmp_pos;

    gtk_init (&argc, &argv);

    /* create a new window */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_size_request (GTK_WIDGET (window), 400, 200);
    gtk_window_set_title (GTK_WINDOW (window), "MyWhatsApp");
    g_signal_connect (window, "destroy",
                      G_CALLBACK (gtk_main_quit), NULL);

    g_signal_connect_swapped (window, "delete-event",
                              G_CALLBACK (gtk_widget_destroy),
                              window);


    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (window), vbox);
    gtk_widget_show (vbox);


    vpaned = gtk_hpaned_new ();
    gtk_container_add (GTK_CONTAINER (vbox), vpaned);
    gtk_widget_show (vpaned);

    list = create_list ();
    gtk_paned_add1 (GTK_PANED (vpaned), list);
    gtk_widget_show (list);

    text = create_text ();
    gtk_paned_add2 (GTK_PANED (vpaned), text);
    gtk_widget_show (text);

    gtk_paned_set_position(GTK_PANED (vpaned),150);


    label = gtk_label_new("Mensagem:");
    gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (entry), 50);
    g_signal_connect (entry, "activate",
                      G_CALLBACK (enter_callback),
                      entry);

    gtk_entry_set_text (GTK_ENTRY (entry), "Escreve aqui");


    gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (entry);

    button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
    g_signal_connect_swapped (button, "clicked",
                              G_CALLBACK (gtk_widget_destroy),
                              window);

    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, TRUE, 0);
    gtk_widget_set_can_default (button, TRUE);
    gtk_widget_grab_default (button);
    gtk_widget_show (button);

    gtk_widget_show (window);

    gtk_main();

    return 0;
}





