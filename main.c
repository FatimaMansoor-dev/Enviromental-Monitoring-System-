#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"
#include <gtk/gtk.h>

// Global flag to track whether the plot has been generated
int plotGenerated = 0;

// Global variables to store hottest and coldest days of the week
char hottestDay[20] = "";
char coldestDay[20] = "";

// Function declarations
char *FileToString(FILE *file);
void retrieveAndProcessData(double latitude, double longitude);
void generatePlot(GtkButton *button, gpointer user_data);
gboolean displayTemperatureDetails(gpointer user_data);

// Callback function for the "Temperature Details" button click event
void generatePlot(GtkButton *button, gpointer user_data) {
    // Check if the plot has already been generated
    if (plotGenerated) {
        printf("Temperature details have already been displayed.\n");
        return;
    }

    GtkWidget *latitude_entry = GTK_WIDGET(gtk_grid_get_child_at(GTK_GRID(user_data), 1, 0));
    GtkWidget *longitude_entry = GTK_WIDGET(gtk_grid_get_child_at(GTK_GRID(user_data), 1, 1));

    // Check if entry objects are valid
    if (!GTK_IS_ENTRY(latitude_entry) || !GTK_IS_ENTRY(longitude_entry)) {
        fprintf(stderr, "Error: Invalid entry objects.\n");
        return;
    }

    const gchar *latitude_text = gtk_entry_get_text(GTK_ENTRY(latitude_entry));
    const gchar *longitude_text = gtk_entry_get_text(GTK_ENTRY(longitude_entry));

    // Ensure that latitude and longitude are not empty
    if (strlen(latitude_text) == 0 || strlen(longitude_text) == 0) {
        fprintf(stderr, "Error: Latitude or longitude is empty.\n");
        return;
    }

    double latitude, longitude;

    // Convert latitude and longitude text to double
    sscanf(latitude_text, "%lf", &latitude);
    sscanf(longitude_text, "%lf", &longitude);

    // Call the function to retrieve and process data
    retrieveAndProcessData(latitude, longitude);

    // Set the flag to indicate that the plot has been generated
    plotGenerated = 1;

    // Use g_idle_add to execute displayTemperatureDetails in the next iteration of the GTK main loop
    g_idle_add((GSourceFunc)displayTemperatureDetails, NULL);
}

// Function to retrieve and process data
void retrieveAndProcessData(double latitude, double longitude) {
    CURL *curl;
    CURLcode res;
    FILE *jsonFile;

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Create a curl handle
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error initializing libcurl\n");
        return;
    }

    // Construct the custom URL
    char url[256];
    snprintf(url, sizeof(url), "https://api.open-meteo.com/v1/forecast?latitude=%.6f&longitude=%.6f&hourly=temperature_2m", latitude, longitude);

    curl_easy_setopt(curl, CURLOPT_URL, url);

    // Open a file for writing JSON data
    jsonFile = fopen("weather_data.json", "w");
    if (!jsonFile) {
        fprintf(stderr, "Error opening JSON file for writing\n");
        curl_easy_cleanup(curl);
        return;
    }

    // Set the callback function to handle the response
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, jsonFile);

    // Perform the HTTP GET request
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        fclose(jsonFile);
        curl_easy_cleanup(curl);
        return;
    }

    // Cleanup libcurl
    curl_easy_cleanup(curl);

    // Close the JSON file
    fclose(jsonFile);

    // Open the JSON file for reading
    jsonFile = fopen("weather_data.json", "r");
    if (!jsonFile) {
        fprintf(stderr, "Error opening JSON file for reading\n");
        return;
    }

    // Parse JSON using cJSON
    char *jsonString = FileToString(jsonFile);
    cJSON *root = cJSON_Parse(jsonString);
    free(jsonString);

    if (!root) {
        fprintf(stderr, "Error parsing JSON\n");
        fclose(jsonFile);
        return;
    }

    // Get the hourly array
    cJSON *hourlyArray = cJSON_GetObjectItem(root, "hourly");

    // Check if the array exists and has at least one element
    if (hourlyArray) {
        // Get the temperature_2m array
        cJSON *temperatureArray = cJSON_GetObjectItem(hourlyArray, "temperature_2m");

        // Get the time array
        cJSON *timeArray = cJSON_GetObjectItem(hourlyArray, "time");

        // Check if both arrays exist and have the same number of elements
        if (temperatureArray && timeArray && cJSON_GetArraySize(temperatureArray) == cJSON_GetArraySize(timeArray)) {
            // Arrays for storing data
            char *dates[1000];
            double avgTemps[1000];
            int dateIndex = -1;

            cJSON *temperatureData = temperatureArray->child;
            cJSON *timeData = timeArray->child;

            while (temperatureData != NULL && timeData != NULL) {
                // Extract date and hours
                char *date = strtok(timeData->valuestring, "T");

                // Find or create an entry in the arrays for this date
                int found = 0;
                for (int i = 0; i <= dateIndex; i++) {
                    if (strcmp(dates[i], date) == 0) {
                        // Date found, update average temperature
                        avgTemps[i] = (avgTemps[i] * (double)i + temperatureData->valuedouble) / (double)(i + 1);
                        found = 1;
                        break;
                    }
                }

                if (!found) {
                    // Date not found, create a new entry
                    dateIndex++;
                    dates[dateIndex] = strdup(date);
                    avgTemps[dateIndex] = temperatureData->valuedouble;
                }

                // Move to the next elements in the arrays
                temperatureData = temperatureData->next;
                timeData = timeData->next;
            }

            // Create a processed data file
            FILE *processedDataFile = fopen("processed_data.txt", "w");
            if (!processedDataFile) {
                fprintf(stderr, "Error opening processed data file for writing\n");
                cJSON_Delete(root);
                fclose(jsonFile);
                return;
            }

            // Write processed data to the file
            for (int i = 0; i <= dateIndex; i++) {
                fprintf(processedDataFile, "%s %.2f\n", dates[i], avgTemps[i]);
                free(dates[i]);
            }

            fclose(processedDataFile);

            // Open a file for writing the Gnuplot script
            FILE *gnuplotScript = fopen("plot_script.gnu", "w");
            if (!gnuplotScript) {
                fprintf(stderr, "Error opening Gnuplot script file for writing\n");
                cJSON_Delete(root);
                fclose(jsonFile);
                return;
            }

            // Write Gnuplot commands to the script
            fprintf(gnuplotScript, "set terminal png\n");
            fprintf(gnuplotScript, "set output 'weather_plot.png'\n");
            fprintf(gnuplotScript, "set xdata time\n");
            fprintf(gnuplotScript, "set timefmt '%%Y-%%m-%%d'\n");
            fprintf(gnuplotScript, "set format x '%%m/%%d'\n");
            fprintf(gnuplotScript, "plot 'processed_data.txt' using 1:2 with linespoints title 'Average Temperature'\n");

            fclose(gnuplotScript);

            // Execute the Gnuplot script
            system("gnuplot plot_script.gnu");

            printf("Weather plot generated successfully.\n");

        } else {
            fprintf(stderr, "Error: Invalid JSON structure (missing temperature or time array).\n");
        }
    } else {
        fprintf(stderr, "Error: Invalid JSON structure (missing hourly array).\n");
    }

    // Clean up
    cJSON_Delete(root);
    fclose(jsonFile);
}

// Function to read the entire contents of a file into a string
char *FileToString(FILE *file) {
    char *buffer;
    long length;

    // Get the length of the file
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the buffer
    buffer = (char *)malloc(length + 1);

    // Read the file into the buffer
    fread(buffer, 1, length, file);

    // Add null terminator
    buffer[length] = '\0';

    return buffer;
}

// Function to display temperature details
gboolean displayTemperatureDetails(gpointer user_data) {
    // Open the processed data file for reading
    FILE *processedDataFile = fopen("processed_data.txt", "r");
    if (!processedDataFile) {
        fprintf(stderr, "Error opening processed data file for reading\n");
        return FALSE;
    }

    // Variables to store day-wise temperature data
    char date[20];
    double temperature;

    // Arrays to store temperature data for each day of the week
    double maxTemp[7] = {0};
    double minTemp[7] = {0};

    // Read temperature data from the processed data file
    while (fscanf(processedDataFile, "%s %lf", date, &temperature) == 2) {
        // Update maximum and minimum temperature for the day of the week
        struct tm tm_date = {0};
        sscanf(date, "%d-%d-%d", &tm_date.tm_year, &tm_date.tm_mon, &tm_date.tm_mday);
        tm_date.tm_mon -= 1;  // Adjust month (0-based)
        tm_date.tm_year -= 1900;  // Adjust year
        mktime(&tm_date);  // Normalize the date structure
        int dayOfWeek = tm_date.tm_wday;

        // Update maximum and minimum temperature for the day of the week
        if (temperature > maxTemp[dayOfWeek]) {
            maxTemp[dayOfWeek] = temperature;
            snprintf(hottestDay, sizeof(hottestDay), "%04d-%02d-%02d", tm_date.tm_year + 1900, tm_date.tm_mon + 1, tm_date.tm_mday);
        }
        if (temperature < minTemp[dayOfWeek] || minTemp[dayOfWeek] == 0) {
            minTemp[dayOfWeek] = temperature;
            snprintf(coldestDay, sizeof(coldestDay), "%04d-%02d-%02d", tm_date.tm_year + 1900, tm_date.tm_mon + 1, tm_date.tm_mday);
        }
    }

    // Close the processed data file
    fclose(processedDataFile);

    // Create a new window to display temperature details
    GtkWidget *detailsWindow;
    GtkWidget *detailsGrid;
    GtkWidget *hottestLabel, *coldestLabel, *image;

    detailsWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(detailsWindow), "Temperature Details");
    gtk_window_set_default_size(GTK_WINDOW(detailsWindow), 400, 400);

    detailsGrid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(detailsWindow), detailsGrid);

    hottestLabel = gtk_label_new(NULL);
    coldestLabel = gtk_label_new(NULL);
    image = gtk_image_new_from_file("weather_plot.png");  // Assuming the plot is saved as weather_plot.png

    // Decrease the size of the image
    GdkPixbuf *pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(image));
    GdkPixbuf *scaled_pixbuf = gdk_pixbuf_scale_simple(pixbuf, 400, 300, GDK_INTERP_BILINEAR);
    gtk_image_set_from_pixbuf(GTK_IMAGE(image), scaled_pixbuf);

    gtk_label_set_markup(GTK_LABEL(hottestLabel), g_strdup_printf("<b>Hottest Date:</b> %s (%.2f °C)", hottestDay, maxTemp[0]));
    gtk_label_set_markup(GTK_LABEL(coldestLabel), g_strdup_printf("<b>Coldest Date:</b> %s (%.2f °C)", coldestDay, minTemp[0]));

    // Add spacing below each widget
    gtk_widget_set_margin_bottom(image, 10);
    gtk_widget_set_margin_bottom(hottestLabel, 10);
    gtk_widget_set_margin_bottom(coldestLabel, 10);

    gtk_grid_attach(GTK_GRID(detailsGrid), image, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(detailsGrid), hottestLabel, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(detailsGrid), coldestLabel, 0, 2, 1, 1);

    // Show the window
    gtk_widget_show_all(detailsWindow);

    // Return FALSE to remove the idle handler
    return FALSE;
}

// Main function
int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *latitude_label, *longitude_label, *latitude_entry, *longitude_entry, *details_button;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Weather Plotter");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    latitude_label = gtk_label_new("Latitude:");
    longitude_label = gtk_label_new("Longitude:");
    latitude_entry = gtk_entry_new();
    longitude_entry = gtk_entry_new();
    details_button = gtk_button_new_with_label("Temperature Details");

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

    return 0;
}