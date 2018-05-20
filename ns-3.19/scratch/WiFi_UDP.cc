/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                								 AP
//  * 			   *   			 *    			 *
//  |  			   | 		     |   			 |    10.1.1.0        10.1.1.2,
// n5   		  n6  			 n7  			 n0 -------------------- n1
// 10.1.3.1      10.1.3.2      10.1.3.3	      	10.1.3.4				  |
//                                   			10.1.1.1
//                                     								Remote Host in Internet	(Server)



// A simple program where a node n7 connected to access point sends data to Remote host in Internet -- Traffic is UDP


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nWifi = 3;
double distance = 10;
double interPacketInterval =80;
  CommandLine cmd;
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

  cmd.Parse (argc,argv);

// Number of nodes connected to one wifi device is limited by 18
  if (nWifi > 18)
    {
      std::cout << "Number of wifi nodes " << nWifi << 
                   " specified exceeds the mobility bounding box" << std::endl;
      exit (1);
    }

  if (verbose)
    {
      LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
      LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    }

  //Node container for installing a point to point communication between wireless node and Remote Host
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  //Create a Remote Host(SERVER)
  NodeContainer RemoteHostNodes;
  RemoteHostNodes.Add (p2pNodes.Get (1));
  //Wi-Fi(WLAN) iterface is implemented in one of the interface of P2P Node
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  //Node 0 is Wifi AP
  NodeContainer wifiApNode = p2pNodes.Get (0);


//WiFi Channel is been set
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

//Mobility area with in which WiFi nodes are allowed to move

 

Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	for (uint16_t i = 0; i < nWifi; i++)
	{
		positionAlloc->Add (Vector(distance, 0, 0));
	}
	MobilityHelper mobility;
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.SetPositionAllocator(positionAlloc);
 
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);


  //Install all the protocol stack in both Wireless Nodes including AP and Remote Host


  InternetStackHelper stack;
  stack.Install (RemoteHostNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);


  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer st=address.Assign (staDevices);
  address.Assign (apDevices);



//UDP Application======================================
	uint16_t port=2000;
  uint16_t port1=1001;
	 for (uint32_t u = 0; u < nWifi; ++u)
  {
	   UdpClientHelper source (p2pInterfaces.GetAddress(1,0), port);
	 	    // Set the amount of data to send in bytes.  Zero is unlimited.
	  source.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
	  source.SetAttribute ("MaxPackets", UintegerValue(1000000));
	  source.SetAttribute ("PacketSize", UintegerValue(1024));
	  ApplicationContainer sourceApps = source.Install (wifiStaNodes.Get (u));
	  sourceApps.Start (Seconds (1.0));
	   sourceApps.Stop (Seconds (2.0));
	   PacketSinkHelper sink ("ns3::UdpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
	   ApplicationContainer sinkApps = sink.Install (RemoteHostNodes.Get (0));
	   sinkApps.Start (Seconds (1.0));
	   sinkApps.Stop (Seconds (2.0));
	   port++;

	  UdpClientHelper source01 (p2pInterfaces.GetAddress(1,0),port);
	   	 	    // Set the amount of data to send in bytes.  Zero is unlimited.
	   	  source01.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
	   	  source01.SetAttribute ("MaxPackets", UintegerValue(1000000));
	   	  source01.SetAttribute ("PacketSize", UintegerValue(1024));
	   	  ApplicationContainer sourceApps01 = source01.Install (wifiStaNodes.Get (u));
	   	  sourceApps01.Start (Seconds (1.0));
	   	   sourceApps01.Stop (Seconds (2.0));
	   	   PacketSinkHelper sink01 ("ns3::UdpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
	   	   ApplicationContainer sinkApps01 = sink01.Install (RemoteHostNodes.Get (0));
	   	   sinkApps01.Start (Seconds (1.0));
	   	   sinkApps01.Stop (Seconds (2.0));
	   	   port++;


	   	std::cout<<"IP address"<<st.GetAddress(u,0)<<std::endl;
	   UdpClientHelper source1 (st.GetAddress(u,0), port1);
	   	 	    // Set the amount of data to send in bytes.  Zero is unlimited.
	    // Set the amount of data to send in bytes.  Zero is unlimited.
	     source1.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
	  	  source1.SetAttribute ("MaxPackets", UintegerValue(1000000));
	  	  source1.SetAttribute ("PacketSize", UintegerValue(1024));
	   	  ApplicationContainer sourceApps1 = source1.Install (RemoteHostNodes.Get (0));
	   	  sourceApps1.Start (Seconds (1.0));
	   	   sourceApps1.Stop (Seconds (2.0));
	   	   PacketSinkHelper sink1 ("ns3::UdpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port1));
	   	   ApplicationContainer sinkApps1 = sink1.Install (wifiStaNodes.Get (u));
	   	   sinkApps1.Start (Seconds (1.0));
	   	   sinkApps1.Stop (Seconds (2.0));
	   	   port1++;

	   	   //------------------------test
	   		   UdpClientHelper source11 (st.GetAddress(u,0), port1);
	   		   	 	    // Set the amount of data to send in bytes.  Zero is unlimited.
	   		   	  source11.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
	   		   	  source11.SetAttribute ("MaxPackets", UintegerValue(1000000));
	   		   	  source11.SetAttribute ("PacketSize", UintegerValue(1024));
	   		   	  ApplicationContainer sourceApps11 = source11.Install (RemoteHostNodes.Get (0));
	   		   	  sourceApps11.Start (Seconds (1.0));
	   		   	   sourceApps11.Stop (Seconds (2.0));
	   		   	   PacketSinkHelper sink11 ("ns3::UdpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port1));
	   		   	   ApplicationContainer sinkApps11 = sink11.Install (wifiStaNodes.Get (u));
	   		   	   sinkApps11.Start (Seconds (1.0));
	   		   	   sinkApps11.Stop (Seconds (2.0));
	   		   	   port1++;
	   		   //-----------------------------try
  }

/*
  PacketSinkHelper RemoteHostSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (p2pInterfaces.GetAddress (1), 9));
// Creating server and client application
//Server apps are installed in  Remote Host
  ApplicationContainer serverApps = RemoteHostSinkHelper.Install (RemoteHostNodes.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));
 
 
 
   
		UdpClientHelper ulClient (p2pInterfaces.GetAddress(1,0), 9);
		ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(10)));
		ulClient.SetAttribute ("MaxPackets",UintegerValue(4));
		ulClient.SetAttribute ("PacketSize", UintegerValue(40));
		ApplicationContainer clientApps =     ulClient.Install (wifiStaNodes.Get (nWifi - 1));
//Client app is installed in WiFi node# 2

  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));*/

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor;

	monitor= flowmon.Install (wifiStaNodes);

	flowmon.Install (RemoteHostNodes);

  Simulator::Stop (Seconds (10.0));

  pointToPoint.EnablePcapAll ("third_modified");
  phy.EnablePcap ("third_modified", apDevices.Get (0));
  //csma.EnablePcap ("third", csmaDevices.Get (0), true);

  Simulator::Run ();
  monitor->CheckForLostPackets ();
	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
	//uint32_t lostpkt=0;
	double Thrpt=0;double received_bytes=0, received_pkts=0, transmitted_packets=0;ns3::Time total_time;
	double Delay=0,PLoss=0;
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
	{

		if (1)
		{
			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
			std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
			std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
			std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
			received_bytes+=i->second.rxBytes;
			std::cout << "  Transmitted Packets: " << i->second.txPackets << std::endl;
			std::cout << "  Received Packets: " << i->second.rxPackets << std::endl;
			std::cout << "  First Tx time:   " << i->second.timeFirstTxPacket << "\n";
			std::cout << "  Last Rx time:   " << i->second.timeLastRxPacket << "\n";

			total_time+=i->second.timeLastRxPacket-i->second.timeFirstTxPacket;
			std::cout << "  Throughput: " << ( ((double)i->second.rxBytes*8) / (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()/1024/1024 ) << "Mbps" << std::endl;
			Thrpt +=( ((double)i->second.rxBytes*8) / (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()/1024/1024 );
			Delay += i->second.delaySum.GetSeconds();
			received_pkts+=i->second.rxPackets;
			//  std::cout << "  local Delay = " << i->second.delaySum.GetSeconds()/i->second.rxPackets << "\n";
			PLoss+=i->second.txPackets-i->second.rxPackets ;
			transmitted_packets+=i->second.txPackets;
			//lostpkt+=i->second.lostPackets;
			//    std::iterator<int,float> s=i->second.packetsDropped.iterator;

		}
	}
	std::cout << "  Packet loss = " << PLoss<< "\n";
	std::cout << "Percentage of Lost packets = "<<((PLoss/transmitted_packets)*100)<<std::endl;
	std::cout << "  Delay = " << (Delay/received_pkts)<< "\n";
	std::cout << " Total Rx Bytes: " << received_bytes;
	// std::cout << " Total Lost Pkt : " << lostpkt;
	//std::cout << " Expected Throughput : " << (received_bytes*8)/total_time;
	std::cout << " Total Throughput: " << Thrpt <<std::endl;
  Simulator::Destroy ();
  return 0;
}
