// header.h

#ifndef HEADER_H
#define HEADER_H

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

extern double maxOutlierTemp;
extern char maxOutlierDate[11];
extern char maxOutlierTime[9];
extern double minOutlierTemp;
extern char minOutlierDate[11];
extern char minOutlierTime[9];
extern char hottestDay[20];
extern char coldestDay[20];
extern char *dates[1000];
extern double avgTemps[1000];
extern int dateIndex;
extern int maxIndex;
extern int minIndex;
extern double std_dev;
extern double latitude;
extern double longitude;

char *FileToString(FILE *file);
void retrieveAndProcessData(double latitude, double longitude);
void generateAndSaveLineChartToPNG(const char *filename);
void generatePlot(GtkButton *button, gpointer user_data);
void analyzeTemperatureOutliers(double *avgTemps, int dateIndex, cJSON *temperatureArray, cJSON *timeArray);
void displayTemperatureDetailsToPDF(GtkWidget *grid, gpointer user_data);
void sendEmailNotification(const char *subject, const char *body, const char *recipientEmail, const char *attachmentPath);

#endif /* HEADER_H */
