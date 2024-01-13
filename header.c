#include "header.h"

// Global variable definitions
double latitude = 24.8607;
double longitude = 67.0011;
char tempString[200];
double maxOutlierTemp = -DBL_MAX;
char *maxOutlierDate = NULL;
char *maxOutlierTime = NULL;
double minOutlierTemp = DBL_MAX;
char *minOutlierDate = NULL;
char *minOutlierTime = NULL;
char *dates[1000];
double avgTemps[1000];
int dateIndex = -1;
int maxIndex = 0;
int minIndex = 0;
double std_dev = 0.0;
const char *recipientEmail = "zoubiyanasim@gmail.com";


void sendEmailNotification(const char *subject, const char *body, const char *recipientEmail,const char *attachmentPath) {
    CURL *curl;
    CURLcode res;

    // Initialize the curl session
    curl = curl_easy_init();

    // Check if the initialization is successful
    if (curl) {
        // Set the email server details
        curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

        // Set the username and password for SMTP authentication
        curl_easy_setopt(curl, CURLOPT_USERNAME, "fatima.mansoorali03@gmail.com"); // Replace with your email
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "zvjx hfvu utmp ucep"); // Replace with your password

        // Set the authentication method
        curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=PLAIN");

        // Set the email headers
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "From: fatima.mansoorali03@gmail.com"); 
        headers = curl_slist_append(headers, recipientEmail); 
        // Add Subject to the headers
char subjectHeader[100];
snprintf(subjectHeader, sizeof(subjectHeader), "Subject: %s", subject);
headers = curl_slist_append(headers, subjectHeader);
        // Set the custom headers for the email
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Create the mime structure
        struct curl_mime *mime = curl_mime_init(curl);

        // Attach the message to the email
        struct curl_mimepart *part = curl_mime_addpart(mime);
        curl_mime_data(part, body, CURL_ZERO_TERMINATED); 

	// Attach the processed_data.txt as an attachment
        part = curl_mime_addpart(mime);
        curl_mime_filedata(part, attachmentPath);
        

        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);

        // Set the recipients
        struct curl_slist *recipients = NULL;
        recipients = curl_slist_append(recipients, recipientEmail); 
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        // Perform the email sending
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        // Clean up
        curl_slist_free_all(headers);
        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
        curl_mime_free(mime); // Free the mime structure
    }
}



void analyzeTemperatureOutliers(double *avgTemps, int dateIndex, cJSON *temperatureArray, cJSON *timeArray) {
    // Initialize weekly average temperature and standard deviation variables for maximum temperature
    double maxWeeklyAvgTemp = 0.0;
    double maxSumSquaredDifferences = 0.0;

    // Initialize weekly average temperature and standard deviation variables for minimum temperature
    double minWeeklyAvgTemp = 0.0;
    double minSumSquaredDifferences = 0.0;

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
    printf("Standard Deviation for Temperature: %.2f °C\n", std_dev);

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
    // Update maxOutlierTemp and maxOutlierDate if the current outlier is greater
    if (hourlyTemp > maxOutlierTemp) {
        maxOutlierTemp = hourlyTemp;

        // Free previously allocated memory and allocate new memory for maxOutlierDate
        free(maxOutlierDate);
        maxOutlierDate = (char *)malloc(strlen(date) + 1);
        strcpy(maxOutlierDate, date);

        // Extract and update maxOutlierTime
        char *timeToken = strchr(timeData->valuestring, 'T');
        if (timeToken != NULL) {
            // Move to the next character after 'T'
            timeToken++;
            free(maxOutlierTime);
            maxOutlierTime = (char *)malloc(strlen(timeToken) + 1);
            strcpy(maxOutlierTime, timeToken);
        } else {
            // Handle case where 'T' is not found
            free(maxOutlierTime);
            maxOutlierTime = NULL;
        }
    }
}

// Check for outliers for minimum temperature
if (fabs(hourlyTemp - minWeeklyAvgTemp) < (3 * sqrt(minSumSquaredDifferences / (double)(dateIndex + 1)))) {
    // Update minOutlierTemp and minOutlierDate if the current outlier is smaller
    if (hourlyTemp < minOutlierTemp) {
        minOutlierTemp = hourlyTemp;

        // Free previously allocated memory and allocate new memory for minOutlierDate
        free(minOutlierDate);
        minOutlierDate = (char *)malloc(strlen(date) + 1);
        strcpy(minOutlierDate, date);

        // Extract and update minOutlierTime
        char *timeToken = strchr(timeData->valuestring, 'T');
        if (timeToken != NULL) {
            // Move to the next character after 'T'
            timeToken++;
            free(minOutlierTime);
            minOutlierTime = (char *)malloc(strlen(timeToken) + 1);
            strcpy(minOutlierTime, timeToken);
        } else {
            // Handle case where 'T' is not found
            free(minOutlierTime);
            minOutlierTime = NULL;
        }
    }
}

        // Move to the next elements in the arrays
        temperatureData = temperatureData->next;
        timeData = timeData->next;
    }

    // Check if there were multiple outliers in a day for maximum temperature and print the maximum temperature
    if (maxOutlierTemp > -DBL_MAX) {
        printf("Temp can rise up to %.2f °C on %s \n", maxOutlierTemp, maxOutlierDate);
    }

    // Check if there were multiple outliers in a day for minimum temperature and print the minimum temperature
    if (minOutlierTemp < DBL_MAX) {
        printf("Temp can drop to %.2f °C on %s \n", minOutlierTemp, minOutlierDate);
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
    fprintf(gnuplotPipe, "set terminal pngcairo\n");  
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


// function to retrieve and process weather data
void retrieveAndProcessData(double latitude, double longitude) {
    CURL *curl;
    CURLcode res;
    FILE *jsonFile;
    FILE *processedDataFile;  // Declare processedDataFile

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
    snprintf(url, sizeof(url), "https://api.open-meteo.com/v1/forecast?latitude=%lf&longitude=%lf&hourly=temperature_2m,precipitation_probability,visibility", latitude, longitude);

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
    
    // Initialize memory for max and min outlier date and time
    maxOutlierDate = (char *)malloc(11 * sizeof(char));
    maxOutlierTime = (char *)malloc(9 * sizeof(char));
    minOutlierDate = (char *)malloc(11 * sizeof(char));
    minOutlierTime = (char *)malloc(9 * sizeof(char));


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

            // Loop through temperature and time data
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

            // Print average weekly temperature
            printf("******************************************\n");
            printf("Environment Monitoring System For Karachi \n");
            printf("******************************************\n");
            printf("Longitude : %.6f  Latitude : %.6f \n", longitude, latitude);
            printf("-------------------------------------\n");
            printf("Average weekly Temperature: %.2f °C\n", weeklyAvgTemp);

            // Call the function to analyze temperature outliers
            analyzeTemperatureOutliers(avgTemps, dateIndex, temperatureArray, timeArray);

            // Save the processed data to a file
            processedDataFile = fopen("processed_data.txt", "w");
            if (!processedDataFile) {
                fprintf(stderr, "Error opening processed data file for writing\n");
                cJSON_Delete(root);
                fclose(jsonFile);
                return;
            }

            // Print daily average temperatures to file
            fprintf(processedDataFile, "Daily Average Temperatures\n");
            for (int i = 0; i <= dateIndex; i++) {
                fprintf(processedDataFile, "%s %.2f\n", dates[i], avgTemps[i]);
            }
            
            // Print header for hourly temperature data
fprintf(processedDataFile, "\nHourly Temperature Data\n");
fprintf(processedDataFile, "Date        Time       Temperature (°C)\n");

cJSON *temData = cJSON_GetObjectItem(hourlyArray, "temperature_2m")->child;
cJSON *times = cJSON_GetObjectItem(hourlyArray, "time")->child;

while (temData != NULL && times != NULL) {
    // Extract date and time
    char *date = strtok(times->valuestring, "T");
                    int hour = 0;
                    for (int j = 0; j < 24; j++) {
                    // Print date, time, and temperature to file
    		    	fprintf(processedDataFile, "%s  %d:00 	  %.2f\n", date, hour, temData->valuedouble);

    			// Move to the next elements in the arrays
    			temData = temData->next;
    			times = times->next;
                        hour += 1; 
                }
    
}



            // Call the function to extract and print precipitation data
            cJSON *precipitationArray = cJSON_GetObjectItem(hourlyArray, "precipitation_probability");

            // Check if the precipitation array exists
            if (precipitationArray) {
                cJSON *precipitationDataNode = precipitationArray->child;

                fprintf(processedDataFile, "\n\nHourly Precipitation Percentages:\n\n");

                for (int i = 0; i <= 2; i++) {
                    fprintf(processedDataFile, "%s: \n", dates[i]);
                    int zeroCount = 0;
                    int hour = 0;

                    for (int j = 0; j < 24; j++) {
                        if (precipitationDataNode != NULL) {
                            double precipitationPercentage = precipitationDataNode->valuedouble;

                            if (precipitationPercentage == 0) {
                                hour += 1;
                                zeroCount += 1;
                            } else {
                                if (hour == 0) {
                                    fprintf(processedDataFile, "%.2f%% at 00:00 \n", precipitationPercentage);
                                    hour += 1;
                                } else {
                                    fprintf(processedDataFile, "%.2f%% at %.2d:00 \n ", precipitationPercentage, hour);
                                    hour += 1;
                                }
                            }
                            if (zeroCount == 23) {
                                fprintf(processedDataFile, "No chance of precipitation for the entire day");
                            }

                            precipitationDataNode = precipitationDataNode->next;
                        } else {
                            printf("NO Data for Precipitation");
                        }
                    }
                    fprintf(processedDataFile, "\n");
                }
            } else {
                printf("No precipitation data available.\n");
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

    // Generate and save the line chart to PNG
    const char *pngFilename = "temperature_chart.png";
    generateAndSaveLineChartToPNG(pngFilename);

    // Schedule the function to display temperature details in the next iteration of the GTK main loop
    displayTemperatureDetailsToPDF((gpointer)"temperature_chart.png");
    free(maxOutlierDate);
    free(maxOutlierTime);
    free(minOutlierDate);
    free(minOutlierTime);

    
}

gboolean displayTemperatureDetailsToPDF(gpointer user_data){

    const char *pngFilename = (const char *)user_data;
    const char *pdfFilename = "temperature_details.pdf";
    double sumSquaredDifferences = 0.0;

    // Print the hottest and coldest days along with the PDF filename
    printf("Hottest Day of Week on Average: %s (%.2f °C)\n", dates[maxIndex], avgTemps[maxIndex]);
    printf("Coldest Day of Week on Average: %s (%.2f °C)\n", dates[minIndex], avgTemps[minIndex]);

    // Create a PDF surface
    cairo_surface_t *surface = cairo_pdf_surface_create(pdfFilename, 500, 700);

    cairo_t *cr = cairo_create(surface);

    // Set the heading
    cairo_set_font_size(cr, 18);
    cairo_move_to(cr, 150, 50);
    cairo_show_text(cr, "Weather Report");

    // Draw the plot using the PNG file
    cairo_scale(cr, 0.5, 0.5);  // Scale the image to half its size
    cairo_set_source_surface(cr, cairo_image_surface_create_from_png("temperature_chart.png"), 50, 320); // Adjust the y-coordinate here
    cairo_paint(cr);

    // Set the labels
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_scale(cr, 2.0, 2.0); 
    cairo_set_font_size(cr, 12);
    cairo_move_to(cr, 50, 130); 

    cairo_show_text(cr, "Hottest Day of Week on Average:");
    cairo_move_to(cr, 50, 150);
    cairo_show_text(cr, "Coldest Day of Week on Average:");

    // Display the temperatures for the hottest and coldest days
    char tempString[200]; // Buffer to store the temperature as a string

    // Convert temperatures to strings with the degree Celsius symbol
    sprintf(tempString, "%.2f °C", avgTemps[maxIndex]);
    cairo_move_to(cr, 390, 130);
    cairo_show_text(cr, tempString);

    sprintf(tempString, "%.2f °C", avgTemps[minIndex]);
    cairo_move_to(cr, 390, 150);
    cairo_show_text(cr, tempString);

    // Set the dates
    cairo_move_to(cr, 290, 130); 
    cairo_show_text(cr, dates[maxIndex]);
    cairo_move_to(cr, 290, 150);
    cairo_show_text(cr, dates[minIndex]);
    
    // Set the heading
    cairo_set_font_size(cr, 16);
    cairo_move_to(cr, 50, 450);
    cairo_show_text(cr, "Anomalies");

    // Print standard deviation and average temperature of the entire week
    double weeklyAvgTemp = 0.0;
    for (int i = 0; i <= dateIndex; i++) {
        weeklyAvgTemp += avgTemps[i];
    }
    if (dateIndex > 0) {
        weeklyAvgTemp /= (double)(dateIndex + 1);
    }
    cairo_set_font_size(cr, 12);
    cairo_move_to(cr, 50, 470);
    cairo_show_text(cr, "Standard Deviation for Weekly Temperature :");
    cairo_move_to(cr, 50, 490);
    cairo_show_text(cr, "Average Temperature for Entire Week:");

    // Display the standard deviation and average temperature
    sprintf(tempString, "%.2f °C", std_dev);
    cairo_move_to(cr, 390, 470);
    cairo_show_text(cr, tempString);

    sprintf(tempString, "%.2f °C", weeklyAvgTemp);
    cairo_move_to(cr, 390, 490);
    cairo_show_text(cr, tempString);

// Print the minimum temperature outlier details
if (maxOutlierTemp > -DBL_MAX) {
    sprintf(tempString, "Temp can rise up to %.2f °C on %s", maxOutlierTemp, maxOutlierDate);
    cairo_move_to(cr, 50, 510); 
    cairo_show_text(cr, tempString);
}

// Print the minimum temperature outlier details
if (minOutlierTemp < DBL_MAX) {
    sprintf(tempString, "Temp can drop to %.2f °C on %s", minOutlierTemp, minOutlierDate);
    cairo_move_to(cr, 50, 530); 
    cairo_show_text(cr, tempString);
}
    
// Display longitude
sprintf(tempString, "Longitude: %.2f", longitude);  
cairo_move_to(cr, 50, 100); 
cairo_show_text(cr, tempString);
// Display longitude
sprintf(tempString, "Latitude: %.2f", latitude);  
cairo_move_to(cr, 280, 100); 
cairo_show_text(cr, tempString);

// Finish the first page
    cairo_show_page(cr);

    // Start the new page
cairo_set_font_size(cr, 12);
cairo_move_to(cr, 50, 20);
//cairo_show_text(cr, "Precipitation Details:");

// Display precipitation details on the new page
FILE *processedDataFile = fopen("processed_data.txt", "r");
if (!processedDataFile) {
    fprintf(stderr, "Error opening processed data file for reading\n");
    cairo_surface_finish(surface);
    cairo_surface_destroy(surface);
    cairo_destroy(cr);
    return TRUE;
}

char line[256];
double y_position = 70;  // Adjust the starting y-position
int startDisplay = 0;    // Flag to start displaying lines

while (fgets(line, sizeof(line), processedDataFile) != NULL) {
    if (strstr(line, "Hourly Precipitation Percentages:") != NULL) {
        startDisplay = 1;  // Set the flag to start displaying lines
    }

    if (startDisplay) {
        // Split line into tokens based on newline character
        char *token = strtok(line, "\n");

        // Display each line of text
        cairo_move_to(cr, 50, y_position);
        cairo_show_text(cr, token);

        // Increment y_position for the next line
        y_position += 20;  // Adjust the vertical spacing as needed
    }
}
    fclose(processedDataFile);

// Finish the PDF surface
cairo_surface_finish(surface);
cairo_show_page(cr);

// Clean up Cairo resources
cairo_surface_destroy(surface);
cairo_destroy(cr);

     
        
// Open the generated PDF using xdg-open (you can replace it with gnome-open on GNOME systems)
    if (access(pdfFilename, F_OK) != -1) {
        system("xdg-open temperature_details.pdf");
    }
if (maxOutlierTemp > 40) {
printf("-------------------------------------\n");

            const char *emailSubject = "High Temperature Alert!";
            const char *emailBody = "The maximum temperature is likely to go above 40 °C. Please check the weather report and take necessary precautions. \n GOOD DAY ☺ ";
             // Call the sendEmailNotification function
    sendEmailNotification(emailSubject, emailBody,recipientEmail,"processed_data.txt");
    printf("EMAIL SENT SUCCESSFULLY \n");
    

     
        }
        
        if (minOutlierTemp < 10) {
        printf("-------------------------------------\n");

            const char *emailSubject = "Low Temperature Alert!";
            const char *emailBody = "The minimum temperature is likely to go below 10 °C this week. Please check the weather report and take necessary precautions. \n GOOD DAY ☺ ";
             // Call the sendEmailNotification function
    sendEmailNotification(emailSubject, emailBody,recipientEmail,"processed_data.txt");
    printf("EMAIL SENT SUCCESSFULLY \n");

  
        }
}