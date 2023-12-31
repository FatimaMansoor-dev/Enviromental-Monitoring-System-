#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include "cJSON.h"
#include <cairo-pdf.h>


// Global flag to track whether the plot has been generated
int plotGenerated = 0;

// Global variables to store hottest and coldest days of the week
char hottestDay[20] = "";
char coldestDay[20] = "";
char *dates[1000];
double avgTemps[1000];
int dateIndex = -1;
int maxIndex = 0;  // Make maxIndex and minIndex global
int minIndex = 0;

// Function declarations
char *FileToString(FILE *file);
void retrieveAndProcessData(double latitude, double longitude);
void generateAndSaveLineChartToPDF(const char *filename);
void generatePlot(GtkButton *button, gpointer user_data);
void displayTemperatureDetailsToPDF(GtkWidget *grid);

char *FileToString(FILE *file) {
    char *buffer = NULL;
    long length;

    if (file) {
        // Seek to the end of the file to get its length
        fseek(file, 0, SEEK_END);
        length = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Allocate memory for the buffer
        buffer = (char *)malloc((length + 1) * sizeof(char));
        if (buffer) {
            // Read the file content into the buffer
            fread(buffer, 1, length, file);

            // Null-terminate the buffer
            buffer[length] = '\0';
        }
    }

    return buffer;
}

void generateAndSaveLineChartToPDF(const char *filename) {
    FILE *gnuplotPipe = popen("gnuplot -persistent", "w");
    
    if (!gnuplotPipe) {
        fprintf(stderr, "Error opening gnuplot pipe\n");
        return;
    }

    // Create a gnuplot script to plot the data
    fprintf(gnuplotPipe, "set terminal pdfcairo\n");
    fprintf(gnuplotPipe, "set output '%s'\n", filename);
    fprintf(gnuplotPipe, "set xdata time\n");
    fprintf(gnuplotPipe, "set timefmt \"%%Y-%%m-%%d\"\n");
    fprintf(gnuplotPipe, "set format x \"%%m/%%d\"\n");
    fprintf(gnuplotPipe, "set xlabel 'Date'\n");  // Add X label
    fprintf(gnuplotPipe, "set ylabel 'Temperature (°C)'\n");  // Add Y label
    fprintf(gnuplotPipe, "set lmargin at screen 0.15\n");  // Adjust left margin

    // Plot the data
    fprintf(gnuplotPipe, "plot '-' using 1:2 with lines title 'Average Temperature'\n");

    for (int i = 0; i <= dateIndex; i++) {
        fprintf(gnuplotPipe, "%s %.2f\n", dates[i], avgTemps[i]);
    }

    fprintf(gnuplotPipe, "e\n");

    // Close the gnuplot pipe
    fclose(gnuplotPipe);
}

void generatePlot(GtkButton *button, gpointer user_data) {
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

    // Schedule the function to display temperature details in the next iteration of the GTK main loop
    g_idle_add((GSourceFunc)displayTemperatureDetailsToPDF, user_data);
}

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

            // Find the index of maximum and minimum average temperatures
            for (int i = 1; i <= dateIndex; i++) {
                if (avgTemps[i] > avgTemps[maxIndex]) {
                    maxIndex = i;
                }
                if (avgTemps[i] < avgTemps[minIndex]) {
                    minIndex = i;
                }
            }

            // Print the maximum and minimum average temperatures along with their dates
            printf("Maximum Temperature: %s (%.2f °C)\n", dates[maxIndex], avgTemps[maxIndex]);
            printf("Minimum Temperature: %s (%.2f °C)\n", dates[minIndex], avgTemps[minIndex]);

            // Save the processed data to a file
            FILE *processedDataFile = fopen("processed_data.txt", "w");
            if (!processedDataFile) {
                fprintf(stderr, "Error opening processed data file for writing\n");
                cJSON_Delete(root);
                fclose(jsonFile);
                return;
            }

            for (int i = 0; i <= dateIndex; i++) {
                fprintf(processedDataFile, "%s %.2f\n", dates[i], avgTemps[i]);
            }

            fclose(processedDataFile);
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

void displayTemperatureDetailsToPDF(GtkWidget *grid) {
    const char *pdfFilename = "temperature_details.pdf";

    // Generate and save the line chart to PDF
    generateAndSaveLineChartToPDF(pdfFilename);

    // Print the hottest and coldest days along with the PDF filename
    printf("Hottest Day of Week on Average: %s (%.2f °C)\n", dates[maxIndex], avgTemps[maxIndex]);
    printf("Coldest Day of Week on Average: %s (%.2f °C)\n", dates[minIndex], avgTemps[minIndex]);
    printf("PDF saved to: %s\n", pdfFilename);

    // Create a PDF surface
    cairo_surface_t *surface = cairo_pdf_surface_create(pdfFilename, 500, 700); // Increased height to allow space for moving the plot
    cairo_t *cr = cairo_create(surface);

    // Set the heading
    cairo_set_font_size(cr, 18);
    cairo_move_to(cr, 150, 50);
    cairo_show_text(cr, "Temperature Report");

    // Draw the plot
    cairo_scale(cr, 0.5, 0.5);  // Scale the image to half its size
    cairo_set_source_surface(cr, cairo_image_surface_create_from_png("temperature_chart.png"), 50, 250); // Adjust the y-coordinate here
    cairo_paint(cr);

    // Set the labels
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_scale(cr, 2.0, 2.0);  // Restore the scale for labels
    cairo_set_font_size(cr, 12);
    cairo_move_to(cr, 50, 80); // Adjusted y-coordinate for labels
    cairo_show_text(cr, "Hottest Day of Week on Average:");
    cairo_move_to(cr, 50, 100);
    cairo_show_text(cr, "Coldest Day of Week on Average:");

    // Display the temperatures for the hottest and coldest days
    char tempString[20]; // Buffer to store the temperature as a string

    // Convert temperatures to strings with the degree Celsius symbol
    sprintf(tempString, "%.2f °C", avgTemps[maxIndex]);
    cairo_move_to(cr, 390, 80);
    cairo_show_text(cr, tempString);

    sprintf(tempString, "%.2f °C", avgTemps[minIndex]);
    cairo_move_to(cr, 390, 100);
    cairo_show_text(cr, tempString);

    // Set the dates
    cairo_move_to(cr, 290, 80); // Adjusted x-coordinate for dates (right beside the labels)
    cairo_show_text(cr, dates[maxIndex]);
    cairo_move_to(cr, 290, 100);
    cairo_show_text(cr, dates[minIndex]);

    // Clean up
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}


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

