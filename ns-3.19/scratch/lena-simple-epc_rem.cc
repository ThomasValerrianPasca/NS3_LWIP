/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/flow-monitor-module.h"
//#include "ns3/wifi-net-device.h"
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/epc-enb-application.h"
#include "ns3/routerlayer.h"
#include "map"
#include "sstream"
#include "ns3/radio-environment-map-helper.h"
//#include "ns3/ltewifiinterface.h"

//#include "ns3/gtk-config-store.h"

using namespace ns3;

/*void callbackltewifiinterface(Ptr<LteWifiInterface> ltewifi)
{
	ltewifi->ReadData();
	std::cout<<"callback called"<<std::endl;
}*/
/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */
NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");
int
main (int argc, char *argv[])
{

  uint16_t numberOfNodes = 1;
  uint16_t numberOfClients=4;
  double simTime = 3;
  double distance = 60.0;
  double interPacketInterval =8192;
  ns3::PacketMetadata::Enable ();
     // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of eNodeBs + UE pairs", numberOfNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.Parse(argc, argv);
LogComponentEnable("PacketSink",LOG_LEVEL_INFO);
LogComponentEnable("UdpClient",LOG_LEVEL_INFO);
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
  std::cout<<"Remote host Address  : "<<remoteHostAddr<<std::endl;

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  NodeContainer integrated_box;
  NodeContainer integrated_ue;
  //NodeContainer ueNodes;
  NodeContainer enbNodes;
  //NodeContainer wifiStaNodes;
  NodeContainer wifiAp;

  enbNodes.Create(numberOfNodes);
  wifiAp.Create(1);

//ADD wi-fi and LTE into integrated box
  integrated_box.Add(enbNodes); ///Be careful when more than 1 enb is added
  integrated_box.Add(wifiAp);

 //ADD Wi-Fi and LTE into UE (Lets call wifi with lte devices as integrated UE)

//  integrated_ue.Create(2);
  //for(uint16_t i=0;i<numberOfClients;i++) {
	 integrated_ue.Create(numberOfClients);
  //}

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfNodes; i++)
    {
      positionAlloc->Add (Vector(distance * i, 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);

  mobility.Install(integrated_box);
  //for (uint16_t i = 0; i < numberOfNodes; i++)
    //  {
	  	  mobility.Install(integrated_ue);
      //}

//  Ptr<LteWifiInterface> ltewifiinterface = CreateObject<LteWifiInterface>();

 // WifiNetDevice::ltewifiinterfaceobject=ltewifiinterface;
  //WiFi Channel is been set
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
    YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
    phy.SetChannel (channel.Create ());

    WifiHelper wifi = WifiHelper::Default ();
    wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
    wifi.SetStandard(WIFI_PHY_STANDARD_80211a);
    NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

    Ssid ssid = Ssid ("Integrated_Box");
    mac.SetType ("ns3::StaWifiMac",
                 "Ssid", SsidValue (ssid),
                 "ActiveProbing", BooleanValue (false));


  // Install LTE Devices to the nodes and wifi devices to node
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs;
 for(uint16_t i=0;i<numberOfClients;i++) {
   ueLteDevs.Add(lteHelper->InstallUeDevice (integrated_ue.Get(i)));
 }
  //ueLteDevs.Get(0)->setLteWifiInterface(ltewifiinterface);
 NetDeviceContainer wifiDevs[numberOfClients];
 for(uint16_t i=0;i<numberOfClients;i++) {
   wifiDevs[i]=wifi.Install(phy,mac,integrated_ue.Get(i));
 }

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer wifiApDevs =wifi.Install(phy,mac,wifiAp);

//  mac.SetType ("ns3::ApWifiMac","Ssid", SsidValue (ssid));

  // Install the IP stack on the UEs///////////////////////////////////
  InternetStackHelper stack;
  //for (uint16_t i = 0; i < numberOfNodes; i++)
    //  {
	  	  stack.Install (integrated_ue);
     // }
  stack.Install(wifiAp);
  /*std::cout<<"Number of net devices"<<integrated_ue.Get(0)->GetNDevices()<<std::endl;
  std::cout<<"Number of net deviceswifi"<<integrated_ue.Get(1)->GetNDevices()<<std::endl;
for (uint32_t i=0;i<integrated_ue.Get(0)->GetNDevices();i++){
	std::cout<<"Type ID:"<<integrated_ue.Get(0)->GetDevice(i)->GetTypeId().GetName()<<std::endl;
}*/

  //stack.Install (integrated_ue);
  //std::cout<<"Node size"<<integrated_ue.GetN()<<std::endl;
  //internet.Install (integrated_ue.Get(1));
  RouterLayer::epc_enb_wifi_netdevice=wifiApDevs.Get(0);


  //internet.Install (integrated_box);
  Ipv4InterfaceContainer ueIpIface;
   ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));

  Mac48Address wifiapmacaddress="22:33:44:55:66:69";
  wifiApDevs.Get(0)->SetAddress(wifiapmacaddress);
  ns3::RouterLayer::wifi_Ap_macaddr=wifiapmacaddress;
  Mac48Address wifimacaddress;
  ns3::RouterLayer router;


  for(uint16_t i=0;i<numberOfClients;i++) {
	  std::string mac="22:33:44:55:66:7";
	  int a = i;
	  std::stringstream ss;
	  ss << a;
	  std::string mac_last_prefix = ss.str();
	  mac.append(mac_last_prefix);
	  wifimacaddress=mac.c_str();
	  std::cout << "client mac address"<<wifimacaddress << std::endl;
	  wifiDevs[i].Get(0)->SetAddress(wifimacaddress);
	  router.mapfunc(wifimacaddress,ueIpIface.GetAddress(i));
  }



  //wifiDevs.Get(0)->setLteWifiInterface(ltewifiinterface);
 // ltewifiinterface->SetDevices(wifiDevs,ueLteDevs);


  //Ipv4Address ipaddr="7.0.0.2";
   //wifiDevs.Get(0)->SetAddress(ipaddr);

   Ipv4AddressHelper address;
  // address.SetBase ("8.0.0.0", "255.0.0.0");
   //Ipv4Address adr1="7.0.0.5";


   //Just commented
   for (uint16_t i = 0; i < numberOfClients; i++)
       {
		   Ipv4Address adr=ueIpIface.GetAddress(i,0);
		   Ipv4InterfaceContainer ueIpIface1;
		   ueIpIface1=address.assignaddress(wifiDevs[i],adr);
		  std::cout<<" UE Interface address"<<ueIpIface.GetAddress(i,0)<<std::endl;
		   std::cout<<" Wifi Interface address"<<ueIpIface1.GetAddress(0,0)<<std::endl;
       }




  //ueIpIface2=address.assignaddress(wifiApDevs,adr1);
//  address.Assign(wifiApDevs);

  //std::cout<<"Wfi AP address"<<ueIpIface2.GetAddress(0,0)<<std::endl;
  //wifiDevs.Get(0)->SetAddress()


  // Assign IP address to UEs, and install applications
//  for (uint32_t u = 0; u < integrated_ue.GetN (); ++u)
  for (uint32_t u = 0; u < numberOfClients; ++u)
    {
      Ptr<Node> ueNode = integrated_ue.Get (u);

      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

    }

  // Attach all UE to eNodeB

  //Only 1 ue
  for (uint16_t i = 0; i < numberOfClients; i++)
      {
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(0));

//        WifiNetDevice::LteDevs=ueLteDevs.Get(i);
        //Ptr<LteUeNetDevice> ueLteDevice = ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ();
        //Ptr<ns3::Packet> pkt;
        //ueLteDevice->GetNas()->DoRecvData(pkt);
       // WifiNetDevice::ueNas = ueLteDevice->GetNas ();
        // side effect: the default EPS bearer will be activated

      }


  //wifiDevs.Get(0)->SetAddress(ueLteDevs.Get(0)->GetAddress());//setting same mac address of LTE
   //Ipv4InterfaceContainer IpIfacewifiDevs=ipv4h.Assign(wifiDevs);
  //std::cout<<"wifi mac address"<<wifiDevs.Get(0)->GetAddress()<<std::endl;
  //std::cout<<"wifi Ap address"<<wifiApDevs.Get(0)->GetAddress()<<std::endl;
  //std::cout<<"wifi mac address"<<wifiDevs.Get(1)->GetAddress()<<std::endl;
  //std::cout<<"wifi Ap address"<<wifiApDevs.Get(1)->GetAddress()<<std::endl;

 // std::cout<<"epc_enb_wifi_netdevice"<<EpcEnbApplication::epc_enb_wifi_netdevice->GetAddress()<<std::endl;


  // Install and start applications on UEs and remote host
  uint16_t port=2000;
  uint16_t port1=2001;
  for (uint32_t u = 0; u < numberOfClients; ++u)
  {
	  BulkSendHelper source ("ns3::TcpSocketFactory",InetSocketAddress (remoteHostAddr, port));
	 	    // Set the amount of data to send in bytes.  Zero is unlimited.
	  source.SetAttribute ("MaxBytes", UintegerValue (0));
	  source.SetAttribute("SendSize",UintegerValue (1024));
	  ApplicationContainer sourceApps = source.Install (integrated_ue.Get(u));
	  sourceApps.Start (Seconds (1.0));
	   sourceApps.Stop (Seconds (3.0));
	   PacketSinkHelper sink ("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
	   ApplicationContainer sinkApps = sink.Install (remoteHost);
	   sinkApps.Start (Seconds (1.0));
	   sinkApps.Stop (Seconds (3.0));
	   BulkSendHelper source1 ("ns3::TcpSocketFactory",InetSocketAddress (ueIpIface.GetAddress (u), port1));
	   	 	    // Set the amount of data to send in bytes.  Zero is unlimited.
	   	  source1.SetAttribute ("MaxBytes", UintegerValue (0));
	   	  source1.SetAttribute("SendSize",UintegerValue (1024));
	   	  ApplicationContainer sourceApps1 = source1.Install (remoteHost);
	   	  sourceApps1.Start (Seconds (1.0));
	   	   sourceApps1.Stop (Seconds (3.0));
	   	   PacketSinkHelper sink1 ("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port1));
	   	   ApplicationContainer sinkApps1 = sink1.Install (integrated_ue.Get(u));
	   	   sinkApps1.Start (Seconds (1.0));
	   	   sinkApps1.Stop (Seconds (3.0));
  }
  /*uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  //uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  //for (uint32_t u = 0; u < integrated_ue.GetN (); ++u)

  for (uint32_t u = 0; u < numberOfClients; ++u)
    {
	  std::cout<<"UE number"<<u<<std::endl;
      ++ulPort;
      //++otherPort;
	  ++dlPort;
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
//      PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
      serverApps.Add (dlPacketSinkHelper.Install (integrated_ue.Get(u)));
     // serverApps.Add (ulPacketSinkHelper.Install (integrated_ue.Get(u)));

    //  serverApps.Add (dlPacketSinkHelper.Install (integrated_ue));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
//      serverApps.Add (packetSinkHelper.Install (integrated_ue.Get(u)));

      UdpClientHelper dlClient (remoteHostAddr, ulPort);

      dlClient.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
      dlClient.SetAttribute ("PacketSize", UintegerValue(1024));

//      UdpClientHelper ulClient (remoteHostAddr, ulPort);
//      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
//      ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
//
      UdpClientHelper client (ueIpIface.GetAddress (u), dlPort);
      client.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
      client.SetAttribute ("MaxPackets", UintegerValue(1000000));
      client.SetAttribute ("PacketSize", UintegerValue(1024));

      clientApps.Add (dlClient.Install (integrated_ue.Get(u)));
      clientApps.Add (client.Install (remoteHost));
//      if (u+1 < integrated_ue.GetN ())
//        {
//          clientApps.Add (client.Install (integrated_ue.Get(u+1)));
//        }
//      else
//        {
//          clientApps.Add (client.Install (integrated_ue.Get(0)));
//        }
    }
  serverApps.Start (Seconds (1.0));
  clientApps.Start (Seconds (1.0));*/
//  serverApps.Stop(Seconds(3.0));
//  clientApps.Stop(Seconds(3.0));
  lteHelper->EnableTraces ();
  
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor;

   monitor= flowmon.Install (integrated_ue);

  flowmon.Install (remoteHost);
  for(uint16_t i=0;i<numberOfClients;i++) {
	  phy.EnablePcap ("Wifi", wifiDevs[i]);
  }
  phy.EnablePcap ("WifiAp", wifiApDevs);


  Simulator::Stop(Seconds(simTime));

  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lena-epc-first");
  //Simulator::Schedule(MilliSeconds(100),&callbackltewifiinterface,ltewifiinterface);
  //AsciiTraceHelper ascii;
  //epcHelper->EnableAsciiAll (ascii.CreateFileStream ("trace.tr"));


//REM----------------------------

//~ Ptr<RadioEnvironmentMapHelper> remHelper = CreateObject<RadioEnvironmentMapHelper> ();
//~ remHelper->SetAttribute ("ChannelPath", StringValue ("/ChannelList/2"));
//~ remHelper->SetAttribute ("OutputFile", StringValue ("rem.out"));
//~ remHelper->SetAttribute ("XMin", DoubleValue (-400.0));
//~ remHelper->SetAttribute ("XMax", DoubleValue (400.0));
//~ remHelper->SetAttribute ("XRes", UintegerValue (100));
//~ remHelper->SetAttribute ("YMin", DoubleValue (-300.0));
//~ remHelper->SetAttribute ("YMax", DoubleValue (300.0));
//~ remHelper->SetAttribute ("YRes", UintegerValue (75));
//~ remHelper->SetAttribute ("Z", DoubleValue (0.0));
//~ remHelper->SetAttribute ("UseDataChannel", BooleanValue (true));
//~ remHelper->SetAttribute ("RbId", IntegerValue (10));
//~ remHelper->Install ();

//-------------------------------
  Simulator::Run();

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/
  
    monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
 
	double Thrpt=0;double received_bytes=0; ns3::Time total_time;
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
          std::cout << "  Delay = " << i->second.timeLastRxPacket-i->second.timeFirstTxPacket<< std::endl;
          total_time+=i->second.timeLastRxPacket-i->second.timeFirstTxPacket;
          std::cout << "  Throughput: " << ( ((double)i->second.rxBytes*8) / (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()/1024/1024 ) << "Mbps" << std::endl;
          Thrpt +=( ((double)i->second.rxBytes*8) / (i->second.timeLastRxPacket - i->second.timeFirstTxPacket).GetSeconds()/1024/1024 );

       }
   }  
  std::cout << " Total Rx Bytes: " << received_bytes;
  std::cout << " Total Delay : " << total_time;
  //std::cout << " Expected Throughput : " << (received_bytes*8)/total_time;
  std::cout << " Total Throughput: " << Thrpt <<std::endl;



  Simulator::Destroy();
  return 0;

}

