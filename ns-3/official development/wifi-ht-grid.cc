/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 SEBASTIEN DERONNE
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Sebastien Deronne <sebastien.deronne@gmail.com>
 *
 * Modificado por: Fabián Frommel
 */

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/yans-wifi-channel.h"
#include <math.h>

// This is a simple example in order to show how to configure an IEEE 802.11ax Wi-Fi network.
//
// It outputs the UDP or TCP goodput for every HE MCS value, which depends on the MCS value (0 to 11),
// the channel width (20, 40, 80 or 160 MHz) and the guard interval (800, 1600 or 3200 ns).
// The PHY bitrate is constant over all the simulation run.
//
// Packets in this simulation belong to BestEffort Access Class (AC_BE).

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ht-wifi-grid");

double simulationTime = 10; //seconds
uint32_t payloadSize; //1500 bytes IP packet
double len = 9.8; //meters
double wid = 7.4; //meters

Ptr<ListPositionAllocator> getCoordinates (uint16_t numStas) {
	double side = sqrt (len * wid / numStas); // lado del cuadrado de cada STA
	uint16_t stasInLen = round (len / side); // #STAs en el largo
	uint16_t stasInWid = round (wid / side); // #STAs en el ancho
	if (stasInLen * stasInWid < numStas) {
		stasInWid ++;
	}
	double sepInLen = len / stasInLen;
	double sepInWid = wid / stasInWid;
	//NS_LOG_UNCOND ("sep largo: " + std::to_string(sepInLen) + " - sep ancho: " + std::to_string(sepInWid));
	double i = sepInLen / 2;
	double j = sepInWid / 2;
	uint16_t counter = 1;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (len / 2, wid / 2, 2)); // Coordenadas del AP
	NS_LOG_UNCOND ("Coordenadas de STAs:");
	while (i < len) {
		while (j < wid) {
			if (counter <= numStas) {
				NS_LOG_UNCOND ("(" + std::to_string(i) + ",\t" + std::to_string(j) + ",\t0)");
				positionAlloc->Add (Vector (i, j, 0)); // Coordenadas de la STA k
			}
			j += sepInWid;
			counter++;
		}
		i += sepInLen;
		j = sepInWid / 2;
	}
	NS_LOG_UNCOND("");
	return positionAlloc;
}

// Tráfico UDP: cli -> srv
ApplicationContainer generateUdpTraffic (Ptr<Node> srvNode, Ptr<Node> cliNode, Ipv4Address address, uint16_t p) {
	ApplicationContainer serverApp;
        uint16_t port = 9 + p;
        UdpServerHelper server (port);
        serverApp = server.Install (srvNode);
        serverApp.Start (Seconds (0.0 + p/1000));
        serverApp.Stop (Seconds (simulationTime + 1));

        UdpClientHelper client (address, port);
        client.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
        client.SetAttribute ("Interval", TimeValue (Time ("0.00002"))); //packets/s
        client.SetAttribute ("PacketSize", UintegerValue (payloadSize));
        ApplicationContainer clientApp = client.Install (cliNode);
        clientApp.Start (Seconds (1.0 + p/1000));
        clientApp.Stop (Seconds (simulationTime + 1));

	return serverApp;
}

// Tráfico TCP: cli -> srv
ApplicationContainer generateTcpTraffic (Ptr<Node> srvNode, Ptr<Node> cliNode, Ipv4Address address, uint16_t p) {
	ApplicationContainer serverApp;
	uint16_t port = 50000 + p;
	Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
        PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", localAddress);
        serverApp = packetSinkHelper.Install (srvNode);
        serverApp.Start (Seconds (0.0 + p/1000));
        serverApp.Stop (Seconds (simulationTime + 1));

        OnOffHelper onoff ("ns3::TcpSocketFactory", Ipv4Address::GetAny ());
        onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
        onoff.SetAttribute ("PacketSize", UintegerValue (payloadSize));
	onoff.SetAttribute ("DataRate", DataRateValue (1000000000)); //bit/s
        AddressValue remoteAddress (InetSocketAddress (address, port));
        onoff.SetAttribute ("Remote", remoteAddress);
        ApplicationContainer clientApp = onoff.Install (cliNode);
        clientApp.Start (Seconds (1.0 + p/1000));
	clientApp.Stop (Seconds (simulationTime + 1));

	return serverApp;
}

int main (int argc, char *argv[]) {
	double minExpectedThroughput = 0;
        double maxExpectedThroughput = 0;
	double frequency = 5.0; //whether 2.4 or 5.0 GHz
	double apTxPowerStart = 20.0; // dbm
        double apTxPowerEnd = 20.0; // dbm
        double staTxPowerStart = 22.0; // dbm
        double staTxPowerEnd = 22.0; // dbm
        int mcs = -1; // -1 indicates an unset value
        int sgi = -1;
        int channelWidth = -1;
	uint16_t direction = 0; // 0 = DL, 1 = UL, 2 = DL + UL
	uint16_t numStas = 1;
        bool useRts = false;
	bool udp = true;
	bool allStas = false;

	CommandLine cmd (__FILE__);
	cmd.AddValue ("frequency", "Banda de frecuencia (2.4, 5.0 GHz)", frequency);
	cmd.AddValue ("simulationTime", "Tiempo de simulación en segundos", simulationTime);
	cmd.AddValue ("numStas", "Cantidad de estaciones (1, 3, 6, 9, ..., 51)", numStas);
	cmd.AddValue ("allStas", "Probar de 3 en 3 todas las STAs hasta 'numStas'", allStas);
        cmd.AddValue ("mcs", "Limitar pruebas a MCS específico (0-7)", mcs);
	cmd.AddValue ("sgi", "Limitar pruebas a GI largo (0) o corto (1)", sgi);
        cmd.AddValue ("channelWidth", "Limitar pruebas a canal de ancho específico (20, 40 MHz)", channelWidth);
	cmd.AddValue ("useRts", "Activar/desactivar RTS/CTS", useRts);
	cmd.AddValue ("direction", "Sentido de envíos (0: DL, 1: UL, 2: DL + UL)", direction);
	cmd.AddValue ("udp", "Envíos UDP si 'true', TCP si 'false'", udp);
	//cmd.AddValue ("minExpectedThroughput", "if set, simulation fails if the lowest throughput is below this value", minExpectedThroughput);
	//cmd.AddValue ("maxExpectedThroughput", "if set, simulation fails if the highest throughput is above this value", maxExpectedThroughput);
	cmd.Parse (argc,argv);

	if (useRts) {
		Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
	}

	double prevThroughput [8];
	for (uint32_t l = 0; l < 8; l++) {
		prevThroughput[l] = 0;
	}

	if ((numStas != 1 && numStas <= 51 && (numStas % 3) != 0) || numStas > 51) {
		NS_LOG_ERROR ("Cantidad de estaciones no soportada!");
		exit (1);
	}

	int minMcs = 0;
	int maxMcs = 7; // MCSs mayores corresponden a 2 o más SS
	if (mcs >= 0 && mcs <= 7) {
		minMcs = mcs;
		maxMcs = mcs;
	}
	int minSgi = 0;
        int maxSgi = 1;
        if (sgi >= 0 && sgi <= 1) {
		minSgi = sgi;
		maxSgi = sgi;
	}
        int minChannelWidth = 20;
        int maxChannelWidth = 40;
	if (channelWidth >= minChannelWidth && channelWidth <= maxChannelWidth) {
		if (channelWidth != 20 && channelWidth != 40) {
			NS_LOG_ERROR ("Ancho de canal no soportado!");
			exit (1);
		} else {
			minChannelWidth = channelWidth;
			maxChannelWidth = channelWidth;
		}
	}

	int stas = 1;
	bool firstTime = true;
	if (!allStas) {
		stas = numStas;
	}

	for (; stas <= numStas; stas += 3) {
		for (int mcs = minMcs; mcs <= maxMcs; mcs++) {
			uint8_t index = 0;
			double previous = 0;
			for (int channelWidth = minChannelWidth; channelWidth <= maxChannelWidth; ) { //MHz
				for (int sgi = minSgi; sgi <= maxSgi; sgi++) {
					// Payload size = MTU - IPv4 header - (TCP o UDP) header
					// Valores de headers tomados de: https://cs.fit.edu/~mmahoney/cse4232/tcpip.html
					if (udp) {
						payloadSize = 1472; //bytes
					} else {
						payloadSize = 1460; //bytes
						Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
					}

					NodeContainer staNodes, apNode;
					staNodes.Create (stas);
					apNode.Create (1);

					YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
					YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();

					phy.SetChannel (channel.Create ());

					WifiHelper wifi;
					if (frequency == 5.0) {
						wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
					} else if (frequency == 2.4) {
						wifi.SetStandard (WIFI_PHY_STANDARD_80211n_2_4GHZ);
						Config::SetDefault ("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue (40.046));
					} else {
						NS_LOG_ERROR ("Banda de frecuencia no soportada!");
						exit (1);
					}
					WifiMacHelper mac;

					std::ostringstream oss;
					oss << "HtMcs" << mcs;

					wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
						"DataMode", StringValue (oss.str ()),
						"ControlMode", StringValue (oss.str ()));

					// Configurar potencia de tx de las STAs
                                        phy.Set ("TxPowerStart", DoubleValue (staTxPowerStart));
                                        phy.Set ("TxPowerEnd", DoubleValue (staTxPowerEnd));

					Ssid ssid = Ssid ("ns3-80211n");

                                        mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));

					NetDeviceContainer staDevices;
					staDevices = wifi.Install (phy, mac, staNodes);

					// Configurar potencia de tx del AP
                                        phy.Set ("TxPowerStart", DoubleValue (apTxPowerStart));
                                        phy.Set ("TxPowerEnd", DoubleValue (apTxPowerEnd));

					mac.SetType ("ns3::ApWifiMac", "EnableBeaconJitter", BooleanValue (false), "Ssid", SsidValue (ssid));

					NetDeviceContainer apDevice;
					apDevice = wifi.Install (phy, mac, apNode);

					// Set channel width
                                        Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (channelWidth));

                                        // Set guard interval
                                        Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HtConfiguration/ShortGuardIntervalSupported", BooleanValue (sgi));

					// Mobility
					MobilityHelper mobility;
					mobility.SetPositionAllocator (getCoordinates(stas));
					mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
					mobility.Install (apNode);
					mobility.Install (staNodes);

					// Internet stack
					InternetStackHelper stack;
					stack.Install (apNode);
					stack.Install (staNodes);

					Ipv4AddressHelper address;
					address.SetBase ("192.168.1.0", "255.255.255.0");
					Ipv4InterfaceContainer apNodeInterface = address.Assign (apDevice);
					Ipv4InterfaceContainer staNodesInterface = address.Assign (staDevices);

					// Setting applications
					std::vector<ApplicationContainer> serversApp;
					for (uint16_t sta = 0; sta < stas; sta++) {
						switch (direction) {
							case 0: // Sentido DL
								if (udp) {
									serversApp.push_back (generateUdpTraffic (staNodes.Get (sta), apNode.Get (0), staNodesInterface.GetAddress (sta), sta));
								} else {
									serversApp.push_back (generateTcpTraffic (staNodes.Get (sta), apNode.Get (0), staNodesInterface.GetAddress (sta), sta));
								}
								break;
							case 1: // Sentido UL
								if (udp) {
                	                                		serversApp.push_back (generateUdpTraffic (apNode.Get (0), staNodes.Get (sta), apNodeInterface.GetAddress (0), sta));
                        	                        	} else {
                                	                        	serversApp.push_back (generateTcpTraffic (apNode.Get (0), staNodes.Get (sta), apNodeInterface.GetAddress (0), sta));
                                        	        	}
								break;
							case 2: // Sentido DL + UL (todas las STAs envían y reciben)
								if (udp) {
									// Envíos DL
                                                                        serversApp.push_back (generateUdpTraffic (staNodes.Get (sta), apNode.Get (0), staNodesInterface.GetAddress (sta), sta));
									// Envíos UL
									serversApp.push_back (generateUdpTraffic (apNode.Get (0), staNodes.Get (sta), apNodeInterface.GetAddress (0), sta));
                                                                } else {
									// Envíos DL
                                                                        serversApp.push_back (generateTcpTraffic (staNodes.Get (sta), apNode.Get (0), staNodesInterface.GetAddress (sta), sta));
									// Envíos UL
									serversApp.push_back (generateTcpTraffic (apNode.Get (0), staNodes.Get (sta), apNodeInterface.GetAddress (0), sta));
                                                                }
								break;
							default: // Error
								NS_LOG_ERROR ("Sentido de envíos inválido!");
                                                		exit (1);
						}
					}

					Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

					Simulator::Stop (Seconds (simulationTime + 1));
					Simulator::Run ();

					uint16_t i = 1, counter = 1;
					bool printDL = false, printUL = false, reset = false;
					double aggThroughput = 0.0;
					for (std::vector<ApplicationContainer>::iterator it = serversApp.begin() ; it != serversApp.end(); ++it) {
						uint64_t rxBytes = 0;
						if (udp) {
        	                                        rxBytes = payloadSize * DynamicCast<UdpServer> (it->Get (0))->GetReceived ();
                	                        } else {
                        	                        rxBytes = DynamicCast<PacketSink> (it->Get (0))->GetTotalRx ();
                                	        }
                                        	double throughput = (rxBytes * 8) / (simulationTime * 1000000.0); //Mbit/s
						if (direction == 2) {
							if (counter <= stas) {
								if (!printDL) {
									std::cout << "Envíos DL:\n";
									printDL = true;
								}
							} else {
								if (!printUL) {
									std::cout << "Envíos UL:\n";
									printUL = true;
								}
								if (!reset) {
									i = 1;
									reset = true;
								}
							}
						}
						std::cout << "STA " << i << " throughput: " << throughput << " Mbit/s" << '\n';
						aggThroughput+=throughput;
						i++;
						counter++;
					}
					std::cout << '\n';

					Simulator::Destroy ();

					std::cout << "MCS value" << "\t\t" << "Channel width" << "\t\t" << "short GI" << "\t\t" << "Agg. Throughput" << '\n';
					std::cout << mcs << "\t\t\t" << channelWidth << " MHz\t\t\t" << sgi << "\t\t\t" << aggThroughput << " Mbit/s" << std::endl;
					std::cout << '\n';

					// Test first element
					if (mcs == 0 && channelWidth == 20 && sgi == 0) {
						if (aggThroughput < minExpectedThroughput) {
							NS_LOG_ERROR ("Obtained throughput " << aggThroughput << " is not expected!");
							exit (1);
						}
					}
					// Test last element
					if (mcs == 7 && channelWidth == 40 && sgi == 1) {
						if (maxExpectedThroughput > 0 && aggThroughput > maxExpectedThroughput) {
							NS_LOG_ERROR ("Obtained throughput " << aggThroughput << " is not expected!");
							exit (1);
						}
					}
					// Test previous throughput is smaller (for the same mcs)
					if (aggThroughput > previous) {
						previous = aggThroughput;
					} else {
						NS_LOG_ERROR ("Obtained throughput " << aggThroughput << " is not expected!");
						//exit (1);
					}
					// Test previous throughput is smaller (for the same channel width and GI)
					if (aggThroughput > prevThroughput [index]) {
						prevThroughput [index] = aggThroughput;
					} else {
						NS_LOG_ERROR ("Obtained throughput " << aggThroughput << " is not expected!");
						//exit (1);
					}
					index++;
				}
				channelWidth *= 2;
			}
		}
		if (firstTime) {
			stas--;
			firstTime = false;
		}
	}
	return 0;
}
