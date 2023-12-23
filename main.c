#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"

// Function to read the entire contents of a file into a string
char *FileToString(FILE *file);

int main() {
    CURL *curl;
    CURLcode res;
    FILE *jsonFile;

    double latitude, longitude;

    // Get latitude and longitude from the user
    printf("Enter latitude: ");
    scanf("%lf", &latitude);

    printf("Enter longitude: ");
    scanf("%lf", &longitude);

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Create a curl handle
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Error initializing libcurl\n");
        return 1;
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
        return 1;
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
        return 1;
    }

    // Cleanup libcurl
    curl_easy_cleanup(curl);

    // Close the JSON file
    fclose(jsonFile);

    // Open the JSON file for reading
    jsonFile = fopen("weather_data.json", "r");
    if (!jsonFile) {
        fprintf(stderr, "Error opening JSON file for reading\n");
        return 1;
    }

    // Parse JSON using cJSON
    char *jsonString = FileToString(jsonFile);
    cJSON *root = cJSON_Parse(jsonString);
    free(jsonString);

    if (!root) {
        fprintf(stderr, "Error parsing JSON\n");
        fclose(jsonFile);
        return 1;
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
            // Loop through temperature data and print for each hour
            int i;
            cJSON *temperatureData = temperatureArray->child;
            cJSON *timeData = timeArray->child;
            while (temperatureData != NULL && timeData != NULL) {
                printf("Hour: %s, Temperature: %.1f°C\n", timeData->valuestring, temperatureData->valuedouble);
                temperatureData = temperatureData->next;
                timeData = timeData->next;
            }
        } else {
            printf("Mismatch in temperature and time data or no data found.\n");
        }
    } else {
        printf("No hourly data found in the JSON array.\n");
    }

    // Cleanup cJSON and close the files
    cJSON_Delete(root);
    fclose(jsonFile);

    return 0;
}

// Function to read the entire contents of a file into a string
char *FileToString(FILE *file) {
    char *buffer;
    long length;

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the buffer
    buffer = (char *)malloc(length + 1);

    // Read the file into the buffer
    fread(buffer, 1, length, file);

    // Null-terminate the string
    buffer[length] = '\0';

    return buffer;
}
