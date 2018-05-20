/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "routerlayer.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/mac48-address.h"
#include "ns3/ipv4.h"
#include "ns3/epc-enb-application.h"
namespace ns3 {

/* ... */
Ptr<NetDevice> RouterLayer::epc_enb_wifi_netdevice;
Mac48Address RouterLayer::wifi_Ap_macaddr;
std::map<Ipv4Address,Mac48Address> RouterLayer::mapaddress;
Ptr<EpcEnbApplication> RouterLayer::routerlayerepcenbapp_object;
RouterLayer::RouterLayer() {
	//NS_LOG_FUNCTION(this);
}

RouterLayer::~RouterLayer() {
	//NS_LOG_FUNCTION(this);
}
void RouterLayer::senddownlink(Ptr<Packet> packet,uint16_t protocolNumber) {
	//NS_LOG_FUNCTION(this);

	Ipv4Header ipv4Header;
	packet->PeekHeader(ipv4Header);
	Mac48Address mac=mapaddress.find(ipv4Header.GetDestination())->second;
	epc_enb_wifi_netdevice->Send(packet,mac,2048);
}
void RouterLayer::mapfunc(Mac48Address macaddr,Ipv4Address ipv4address) {
	mapaddress[ipv4address]=macaddr;
}
void RouterLayer::senduplink(Ptr<Packet> p,Ptr<NetDevice> m_device,uint16_t protocolNumber) {
	m_device->Send(p,wifi_Ap_macaddr,protocolNumber);
}
void RouterLayer::forwardtolteepcenb(Ptr<Packet> p,uint32_t teid) {
	routerlayerepcenbapp_object->SendToS1uSocket(p,teid);
}
}

