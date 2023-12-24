set terminal png
set output 'weather_plot.png'
set xdata time
set timefmt '%Y-%m-%d'
set format x '%m/%d'
plot 'processed_data.txt' using 1:2 with linespoints title 'Average Temperature'
