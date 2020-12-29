#!/bin/bash

# 802.11ax
./waf --run "scratch/wifi-he-grid --direction=0 --udp=false --mcs=7 --gi=3200 --channelWidth=40 --numStas=51 --allStas=true" > output/ax/ptx-20/chaWid-40/output-tcp-dl.txt 2>&1 &
./waf --run "scratch/wifi-he-grid --direction=2 --udp=false --mcs=7 --gi=3200 --channelWidth=40 --numStas=51 --allStas=true" > output/ax/ptx-20/chaWid-40/output-tcp-dlul.txt 2>&1 &
./waf --run "scratch/wifi-he-grid --direction=0 --udp=true --mcs=7 --gi=3200 --channelWidth=40 --numStas=51 --allStas=true" > output/ax/ptx-20/chaWid-40/output-udp-dl.txt 2>&1 &

# 802.11ac
./waf --run "scratch/wifi-vht-grid --direction=0 --udp=false --mcs=7 --sgi=1 --channelWidth=40 --numStas=51 --allStas=true" > output/ac/ptx-20/chaWid-40/output-tcp-dl.txt 2>&1 &
./waf --run "scratch/wifi-vht-grid --direction=2 --udp=false --mcs=7 --sgi=1 --channelWidth=40 --numStas=51 --allStas=true" > output/ac/ptx-20/chaWid-40/output-tcp-dlul.txt 2>&1 &

# 802.11n
./waf --run "scratch/wifi-ht-grid --direction=0 --udp=false --mcs=7 --sgi=1 --channelWidth=40 --numStas=51 --allStas=true" > output/n/ptx-20/chaWid-40/output-tcp-dl.txt 2>&1 &
./waf --run "scratch/wifi-ht-grid --direction=2 --udp=false --mcs=7 --sgi=1 --channelWidth=40 --numStas=51 --allStas=true" > output/n/ptx-20/chaWid-40/output-tcp-dlul.txt 2>&1 &



