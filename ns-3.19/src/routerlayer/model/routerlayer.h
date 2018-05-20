/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef ROUTERLAYER_H
#define ROUTERLAYER_H
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/net-device.h"
#include "ns3/mac48-address.h"
#include "map"
#include "ns3/epc-enb-application.h"
namespace ns3 {

/* ... */
class RouterLayer {
        public:
        RouterLayer();
        ~RouterLayer();
        void senddownlink(Ptr<Packet> packet,uint16_t protocolNumber);
        void mapfunc(Mac48Address macaddr,Ipv4Address ipv4address);
        void senduplink(Ptr<Packet> p,Ptr<NetDevice> m_device,uint16_t protocolNumber);
        void forwardtolteepcenb(Ptr<Packet> p,uint32_t teid);
        static Ptr<NetDevice> epc_enb_wifi_netdevice;
        static Ptr<EpcEnbApplication> routerlayerepcenbapp_object;
       static std::map<Ipv4Address,Mac48Address> mapaddress;
       static Mac48Address wifi_Ap_macaddr;
};
}

#endif /* ROUTERLAYER_H */

