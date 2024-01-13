#ifndef WEATHER_ANALYSIS_HEADER_H
#define WEATHER_ANALYSIS_HEADER_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include "cJSON.h"
#include <cairo-pdf.h>
#include <math.h>
#include <time.h>



char *FileToString(FILE *file);
void retrieveAndProcessData(double latitude, double longitude);
void generateAndSaveLineChartToPNG(const char *filename);
void analyzeTemperatureOutliers(double *avgTemps, int dateIndex, cJSON *temperatureArray, cJSON *timeArray);
gboolean displayTemperatureDetailsToPDF(gpointer user_data);
void sendEmailNotification(const char *subject, const char *body, const char *recipientEmail, const char *attachmentPath);

#endif //WEATHER_ANALYSIS_HEADER_H
