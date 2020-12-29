#!/bin/bash

sim_time=10
seed=1234
vect_stas=( 1 3 6 9 12 15 18 21 24 27 30 33 36 39 42 45 48 51 )
for sta in "${vect_stas[@]}"
do
	echo "Prueba con ${sta} STAs"
	input_file=input/ptx-20/input_${sta}sta.csv
	./komondor_main $input_file $sim_time $seed > output/ptx-20/result_${sta}sta.txt 2>&1
done

