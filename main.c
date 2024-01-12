#include "header.h"

int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);

    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *latitude_label, *longitude_label, *latitude_entry, *longitude_entry, *details_button;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Weather Plotter");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    latitude_label = gtk_label_new("Latitude:");
    longitude_label = gtk_label_new("Longitude:");
    latitude_entry = gtk_entry_new();
    longitude_entry = gtk_entry_new();
    details_button = gtk_button_new_with_label("Generate Weather Report");

    // Add spacing below each widget
    gtk_widget_set_margin_bottom(latitude_label, 5);
    gtk_widget_set_margin_bottom(latitude_entry, 5);
    gtk_widget_set_margin_bottom(longitude_label, 5);
    gtk_widget_set_margin_bottom(longitude_entry, 5);
    gtk_widget_set_margin_bottom(details_button, 5);

    gtk_grid_attach(GTK_GRID(grid), latitude_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), latitude_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), longitude_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), longitude_entry, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), details_button, 0, 2, 2, 1);

    // Connect signals
    g_signal_connect(G_OBJECT(details_button), "clicked", G_CALLBACK(generatePlot), grid);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Show the window
    gtk_widget_show_all(window);

    // Start the GTK main loop
    gtk_main();
    // Free memory for dates
for (int i = 0; i <= dateIndex; i++) {
    free(dates[i]);
}


    return 0;
}