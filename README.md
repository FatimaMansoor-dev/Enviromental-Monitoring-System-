"# Enviromental-Monitoring-System-" 
**Overview**
The Weather Analysis project is a program written in C that retrieves and processes weather data, generates line charts, analyzes temperature outliers, and sends email notifications. The project utilizes external libraries such as libcurl, cJSON, GTK, and Cairo.
**Prerequisites**
Before running the program, ensure you have the following prerequisites installed:
1.C Compiler
2.libcurl
3.cJSON
4.GTK
5.Cairo
**Installation and execution**
--Clone the repository:
git clone https://github.com/FatimaMansoor-dev/Enviromental-Monitoring-System-
//To go into terminal of repositry: 
cd Environmental-Monitoring-System-
//to make run_program executable:
chmod +x run_program.sh
//to run run_program :
./run_program.sh
--Build the project:
cd weather-analysis
make
--Run the executable:
./weather-analysis
**Dependencies**
~libcurl
~cJSON
~GTK
~Cairo
~gcc
~make
~curl
~libcurl4-gnutls-dev
~libgtk-3-dev
~libcairo2-dev
**Usage**
1.Set global variables such as latitude, longitude, and recipientEmail in header.h.
2.Build the project using provided instructions.
3.Run the executable.
4.View console output for temperature analysis and potential outliers.
5.Check for generated charts and PDF reports.
6.Email alerts are sent for extreme temperatures.
