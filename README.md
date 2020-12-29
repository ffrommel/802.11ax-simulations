# 802.11ax-simulations
This repository gathers all the programs built to apply simulations of 802.11ax, 802.11ac and 802.11n networks with the ns-3 and Komondor network simulators, as well as auxiliary programs for preparing input files and processing logs.

## ns-3: https://www.nsnam.org/
The official code of ns-3 ("official development") was tested and also two third-party developments that seek to extend the official code to implement OFDMA: "third party development 1" (https://github.com/cisco/ns3- 802.11ax-simulator) and "third party development 2" (https://github.com/ieee80211axsimulator/muofdma).

## Komondor: https://github.com/wn-upf/Komondor
This simulator was tested, making small changes to the official code such as setting the MCS fixed to 7 and allowing a maximum transmit power of 22 dBm.
