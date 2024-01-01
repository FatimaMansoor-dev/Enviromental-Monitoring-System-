#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include "cJSON.h"
#include <cairo-pdf.h>
#include <math.h>


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
double std_dev = 0.0;
// Function declarations
char *FileToString(FILE *file);
void retrieveAndProcessData(double latitude, double longitude);
void generateAndSaveLineChartToPNG(const char *filename);
void generatePlot(GtkButton *button, gpointer user_data);
void analyzeTemperatureOutliers(double *avgTemps, int dateIndex, cJSON *temperatureArray, cJSON *timeArray);
void displayTemperatureDetailsToPDF(GtkWidget *grid, gpointer user_data);


void analyzeTemperatureOutliers(double *avgTemps, int dateIndex, cJSON *temperatureArray, cJSON *timeArray) {
    // Initialize weekly average temperature and standard deviation variables for maximum temperature
    double maxWeeklyAvgTemp = 0.0;
    double maxSumSquaredDifferences = 0.0;
    double maxOutlierTemp = -DBL_MAX;
    char maxOutlierDate[11] = "";  // Assuming date format "yyyy-mm-dd"
    char maxOutlierTime[9] = "";   // Assuming time format "hh:mm:ss"

    // Initialize weekly average temperature and standard deviation variables for minimum temperature
    double minWeeklyAvgTemp = 0.0;
    double minSumSquaredDifferences = 0.0;
    double minOutlierTemp = DBL_MAX;
    char minOutlierDate[11] = "";  // Assuming date format "yyyy-mm-dd"
    char minOutlierTime[9] = "";   // Assuming time format "hh:mm:ss"

    // Initialize date variable
    char *date = NULL;

    // Iterate through avgTemps array and calculate the average for maximum temperature
    for (int i = 0; i <= dateIndex; i++) {
        maxWeeklyAvgTemp += avgTemps[i];
    }

    if (dateIndex > 0) {
        maxWeeklyAvgTemp /= (double)(dateIndex + 1);
    }

    // Calculate standard deviation for maximum temperature
    for (int i = 0; i <= dateIndex; i++) {
        double difference = avgTemps[i] - maxWeeklyAvgTemp;
        maxSumSquaredDifferences += difference * difference;
    }

    // Iterate through avgTemps array and calculate the average for minimum temperature
    for (int i = 0; i <= dateIndex; i++) {
        minWeeklyAvgTemp += avgTemps[i];
    }

    if (dateIndex > 0) {
        minWeeklyAvgTemp /= (double)(dateIndex + 1);
    }
    // Calculate standard deviation for minimum temperature
for (int i = 0; i <= dateIndex; i++) {
    double difference = avgTemps[i] - minWeeklyAvgTemp;
    minSumSquaredDifferences += difference * difference;
}

    // Update the global standard deviation variable
    std_dev = sqrt(minSumSquaredDifferences / (double)(dateIndex + 1));

    // Print the weekly average temperature and standard deviation for maximum temperature
    //printf("Weekly Average Maximum Temperature: %.2f °C\n", maxWeeklyAvgTemp);
    // Print the weekly average temperature and standard deviation for minimum temperature
    //printf("Weekly Average Minimum Temperature: %.2f °C\n", minWeeklyAvgTemp);
    printf("Standard Deviation for Minimum Temperature: %.2f °C\n", sqrt(minSumSquaredDifferences / (double)(dateIndex + 1)));

    // Initialize cJSON pointers for temperature and time data
    cJSON *temperatureData = temperatureArray->child;
    cJSON *timeData = timeArray->child;

    // Iterate through hourly temperatures to check for outliers
    while (temperatureData != NULL && timeData != NULL) {
        // Extract date and hours
        date = strtok(timeData->valuestring, "T");
        double hourlyTemp = temperatureData->valuedouble;

        // Check for outliers for maximum temperature
        if (fabs(hourlyTemp - maxWeeklyAvgTemp) > 3 * sqrt(maxSumSquaredDifferences / (double)(dateIndex + 1))) {
            //printf("Max Temperature Outlier detected at %s: Hourly Temperature = %.2f °C\n", date, hourlyTemp);

            // Update maxOutlierTemp and maxOutlierDate if the current outlier is greater
            if (hourlyTemp > maxOutlierTemp) {
                maxOutlierTemp = hourlyTemp;
                strncpy(maxOutlierDate, date, sizeof(maxOutlierDate) - 1);
                maxOutlierDate[sizeof(maxOutlierDate) - 1] = '\0';
            }
        }

        // Check for outliers for minimum temperature
        if (fabs(hourlyTemp - minWeeklyAvgTemp) > 3 * sqrt(minSumSquaredDifferences / (double)(dateIndex + 1))) {
            //printf("Min Temperature Outlier detected at %s: Hourly Temperature = %.2f °C\n", date, hourlyTemp);

            // Update minOutlierTemp and minOutlierDate if the current outlier is smaller
            if (hourlyTemp < minOutlierTemp) {
                minOutlierTemp = hourlyTemp;
                strncpy(minOutlierDate, date, sizeof(minOutlierDate) - 1);
                minOutlierDate[sizeof(minOutlierDate) - 1] = '\0';
            }
        }

        // Move to the next elements in the arrays
        temperatureData = temperatureData->next;
        timeData = timeData->next;
    }

    // Check if there were multiple outliers in a day for maximum temperature and print the maximum temperature
    if (maxOutlierTemp > -DBL_MAX) {
        printf("Max Temp can rise up to %.2f °C on %s at %s\n", maxOutlierTemp, maxOutlierDate, maxOutlierTime);
    }

    // Check if there were multiple outliers in a day for minimum temperature and print the minimum temperature
    if (minOutlierTemp < DBL_MAX) {
        printf("Min Temp can drop to %.2f °C on %s at %s\n", minOutlierTemp, minOutlierDate, minOutlierTime);
    }
}






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

void generateAndSaveLineChartToPNG(const char *filename) {
    FILE *gnuplotPipe = popen("gnuplot -persistent", "w");
    
    if (!gnuplotPipe) {
        fprintf(stderr, "Error opening gnuplot pipe\n");
        return;
    }

    // Create a gnuplot script to plot the data
    fprintf(gnuplotPipe, "set terminal pngcairo\n");  // Change terminal type to pngcairo
    fprintf(gnuplotPipe, "set output '%s'\n", filename);
    fprintf(gnuplotPipe, "set xdata time\n");
    fprintf(gnuplotPipe, "set timefmt \"%%Y-%%m-%%d\"\n");
    fprintf(gnuplotPipe, "set format x \"%%m/%%d\"\n");
    fprintf(gnuplotPipe, "set xlabel 'Date'\n");
    fprintf(gnuplotPipe, "set ylabel 'Temperature (°C)'\n");
    fprintf(gnuplotPipe, "set lmargin at screen 0.15\n");

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

    // Generate and save the line chart to PNG
    const char *pngFilename = "temperature_chart.png";
    generateAndSaveLineChartToPNG(pngFilename);

    // Schedule the function to display temperature details in the next iteration of the GTK main loop
    g_idle_add((GSourceFunc)displayTemperatureDetailsToPDF, (gpointer)pngFilename);
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

            // Calculate the weekly average temperature
            double weeklyAvgTemp = 0.0;
            for (int i = 0; i <= dateIndex; i++) {
                weeklyAvgTemp += avgTemps[i];
            }

            if (dateIndex > 0) {
                weeklyAvgTemp /= (double)(dateIndex + 1);
            }

            // Print the maximum and minimum average temperatures along with their dates
            printf("Maximum Temperature: %s (%.2f °C)\n", dates[maxIndex], avgTemps[maxIndex]);
            printf("Minimum Temperature: %s (%.2f °C)\n", dates[minIndex], avgTemps[minIndex]);
            printf("Average weekly Temperature: %.2f °C\n", weeklyAvgTemp);
            
            analyzeTemperatureOutliers(avgTemps, dateIndex, temperatureArray, timeArray);

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


void displayTemperatureDetailsToPDF(GtkWidget *grid, gpointer user_data) {
    const char *pngFilename = (const char *)user_data;
    const char *pdfFilename = "temperature_details.pdf";
    double sumSquaredDifferences = 0.0;


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

    // Draw the plot using the PNG file
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
    
    // Set the heading
    cairo_set_font_size(cr, 16);
    cairo_move_to(cr, 50, 400);
    cairo_show_text(cr, "Anomalies");


    // Print standard deviation and average temperature of the entire week
    //double weeklyStdDev = sqrt(sumSquaredDifferences / (double)(dateIndex + 1));
    double weeklyAvgTemp = 0.0;
    for (int i = 0; i <= dateIndex; i++) {
        weeklyAvgTemp += avgTemps[i];
    }
    if (dateIndex > 0) {
        weeklyAvgTemp /= (double)(dateIndex + 1);
    }
    cairo_set_font_size(cr, 12);
    cairo_move_to(cr, 50, 420);
    cairo_show_text(cr, "Standard Deviation for Weekly Temperature :");
    cairo_move_to(cr, 50, 440);
    cairo_show_text(cr, "Average Temperature for Entire Week:");

    // Display the standard deviation and average temperature
    sprintf(tempString, "%.2f °C", std_dev);
    cairo_move_to(cr, 390, 420);
    cairo_show_text(cr, tempString);

    sprintf(tempString, "%.2f °C", weeklyAvgTemp);
    cairo_move_to(cr, 390, 440);
    cairo_show_text(cr, tempString);

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
