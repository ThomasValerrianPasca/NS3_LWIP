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
 * Author: Jaume Nin <jnin@cttc.cat>
 *         Nicola Baldo <nbaldo@cttc.cat>
 */


#include "ns3/epc-enb-application.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/ipv4.h"
#include "ns3/inet-socket-address.h"
#include "ns3/uinteger.h"
#include "lte-enb-rrc.h"
#include "epc-gtpu-header.h"
#include "eps-bearer-tag.h"
#include "ns3/net-device.h"
#include "ns3/ipv4-flow-probe.h"
#include "ns3/flow-monitor.h"
#include "ns3/routerlayer.h"
#include "ns3/udp-header.h"
#include "ns3/seq-ts-header.h"
#include "ns3/tcp-header.h"
namespace ns3 {



NS_LOG_COMPONENT_DEFINE ("EpcEnbApplication")
								  ;
//uint32_t pkt_cnt=0;
uint32_t packtno=0;
uint16_t temp_port=0;
uint32_t EpcEnbApplication::mode=1;
uint32_t current_value=0,past_value=0;
uint64_t past=0;
//THomas -- User defined class for collecting stats for TCP Window
RouterLayer router;
struct time_info{
	uint32_t source_port;
	int64_t past;
	SequenceNumber32 last_seqno;
	uint32_t dest_port;
	SequenceNumber32 window_size;
}s[10];
//bool flip=true;
/*void EpcEnbApplication::setobject(Ptr<EpcEnbApplication> epcenb) {
	RouterLayer::routerlayerepcenbapp_object=epcenb;
}*/
EpcEnbApplication::EpsFlowId_t::EpsFlowId_t ()
{
}

EpcEnbApplication::EpsFlowId_t::EpsFlowId_t (const uint16_t a, const uint8_t b)
: m_rnti (a),
  m_bid (b)
{
}

bool
operator == (const EpcEnbApplication::EpsFlowId_t &a, const EpcEnbApplication::EpsFlowId_t &b)
{
	return ( (a.m_rnti == b.m_rnti) && (a.m_bid == b.m_bid) );
}

bool
operator < (const EpcEnbApplication::EpsFlowId_t& a, const EpcEnbApplication::EpsFlowId_t& b)
{
	return ( (a.m_rnti < b.m_rnti) || ( (a.m_rnti == b.m_rnti) && (a.m_bid < b.m_bid) ) );
}


TypeId
EpcEnbApplication::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::EpcEnbApplication")
    								.SetParent<Object> ();
	return tid;
}

void
EpcEnbApplication::DoDispose (void)
{
	NS_LOG_FUNCTION (this);
	m_lteSocket = 0;
	m_s1uSocket = 0;
	delete m_s1SapProvider;
	delete m_s1apSapEnb;
}


EpcEnbApplication::EpcEnbApplication (Ptr<Socket> lteSocket, Ptr<Socket> s1uSocket, Ipv4Address enbS1uAddress, Ipv4Address sgwS1uAddress, uint16_t cellId)
: m_lteSocket (lteSocket),
  m_s1uSocket (s1uSocket),
  m_enbS1uAddress (enbS1uAddress),
  m_sgwS1uAddress (sgwS1uAddress),
  m_gtpuUdpPort (2152), // fixed by the standard
  m_s1SapUser (0),
  m_s1apSapMme (0),
  m_cellId (cellId)
{
	NS_LOG_FUNCTION (this << lteSocket << s1uSocket << sgwS1uAddress);
	m_s1uSocket->SetRecvCallback (MakeCallback (&EpcEnbApplication::RecvFromS1uSocket, this));
	m_lteSocket->SetRecvCallback (MakeCallback (&EpcEnbApplication::RecvFromLteSocket, this));
	m_s1SapProvider = new MemberEpcEnbS1SapProvider<EpcEnbApplication> (this);
	m_s1apSapEnb = new MemberEpcS1apSapEnb<EpcEnbApplication> (this);
}


EpcEnbApplication::~EpcEnbApplication (void)
{
	NS_LOG_FUNCTION (this);
}


void 
EpcEnbApplication::SetS1SapUser (EpcEnbS1SapUser * s)
{
	m_s1SapUser = s;
}


EpcEnbS1SapProvider* 
EpcEnbApplication::GetS1SapProvider ()
{
	return m_s1SapProvider;
}

void 
EpcEnbApplication::SetS1apSapMme (EpcS1apSapMme * s)
{
	m_s1apSapMme = s;
}


EpcS1apSapEnb* 
EpcEnbApplication::GetS1apSapEnb ()
{
	return m_s1apSapEnb;
}

void 
EpcEnbApplication::DoInitialUeMessage (uint64_t imsi, uint16_t rnti)
{
	NS_LOG_FUNCTION (this);
	// side effect: create entry if not exist
	m_imsiRntiMap[imsi] = rnti;
	m_s1apSapMme->InitialUeMessage (imsi, rnti, imsi, m_cellId);
}

void 
EpcEnbApplication::DoPathSwitchRequest (EpcEnbS1SapProvider::PathSwitchRequestParameters params)
{
	NS_LOG_FUNCTION (this);
	uint16_t enbUeS1Id = params.rnti;
	uint64_t mmeUeS1Id = params.mmeUeS1Id;
	uint64_t imsi = mmeUeS1Id;
	// side effect: create entry if not exist
	m_imsiRntiMap[imsi] = params.rnti;

	uint16_t gci = params.cellId;
	std::list<EpcS1apSapMme::ErabSwitchedInDownlinkItem> erabToBeSwitchedInDownlinkList;
	for (std::list<EpcEnbS1SapProvider::BearerToBeSwitched>::iterator bit = params.bearersToBeSwitched.begin ();
			bit != params.bearersToBeSwitched.end ();
			++bit)
	{
		EpsFlowId_t flowId;
		flowId.m_rnti = params.rnti;
		flowId.m_bid = bit->epsBearerId;
		uint32_t teid = bit->teid;

		EpsFlowId_t rbid (params.rnti, bit->epsBearerId);
		// side effect: create entries if not exist
		m_rbidTeidMap[params.rnti][bit->epsBearerId] = teid;
		m_teidRbidMap[teid] = rbid;

		EpcS1apSapMme::ErabSwitchedInDownlinkItem erab;
		erab.erabId = bit->epsBearerId;
		erab.enbTransportLayerAddress = m_enbS1uAddress;
		erab.enbTeid = bit->teid;

		erabToBeSwitchedInDownlinkList.push_back (erab);
	}
	m_s1apSapMme->PathSwitchRequest (enbUeS1Id, mmeUeS1Id, gci, erabToBeSwitchedInDownlinkList);
}

void 
EpcEnbApplication::DoUeContextRelease (uint16_t rnti)
{
	NS_LOG_FUNCTION (this << rnti);
	std::map<uint16_t, std::map<uint8_t, uint32_t> >::iterator rntiIt = m_rbidTeidMap.find (rnti);
	if (rntiIt != m_rbidTeidMap.end ())
	{
		for (std::map<uint8_t, uint32_t>::iterator bidIt = rntiIt->second.begin ();
				bidIt != rntiIt->second.end ();
				++bidIt)
		{
			uint32_t teid = bidIt->second;
			m_teidRbidMap.erase (teid);
		}
		m_rbidTeidMap.erase (rntiIt);
	}

}

void 
EpcEnbApplication::DoInitialContextSetupRequest (uint64_t mmeUeS1Id, uint16_t enbUeS1Id, std::list<EpcS1apSapEnb::ErabToBeSetupItem> erabToBeSetupList)
{
	NS_LOG_FUNCTION (this);

	for (std::list<EpcS1apSapEnb::ErabToBeSetupItem>::iterator erabIt = erabToBeSetupList.begin ();
			erabIt != erabToBeSetupList.end ();
			++erabIt)
	{
		// request the RRC to setup a radio bearer

		uint64_t imsi = mmeUeS1Id;
		std::map<uint64_t, uint16_t>::iterator imsiIt = m_imsiRntiMap.find (imsi);
		NS_ASSERT_MSG (imsiIt != m_imsiRntiMap.end (), "unknown IMSI");
		uint16_t rnti = imsiIt->second;

		struct EpcEnbS1SapUser::DataRadioBearerSetupRequestParameters params;
		params.rnti = rnti;
		params.bearer = erabIt->erabLevelQosParameters;
		params.bearerId = erabIt->erabId;
		params.gtpTeid = erabIt->sgwTeid;
		m_s1SapUser->DataRadioBearerSetupRequest (params);

		EpsFlowId_t rbid (rnti, erabIt->erabId);
		// side effect: create entries if not exist
		m_rbidTeidMap[rnti][erabIt->erabId] = params.gtpTeid;
		m_teidRbidMap[params.gtpTeid] = rbid;

	}
}

void 
EpcEnbApplication::DoPathSwitchRequestAcknowledge (uint64_t enbUeS1Id, uint64_t mmeUeS1Id, uint16_t gci, std::list<EpcS1apSapEnb::ErabSwitchedInUplinkItem> erabToBeSwitchedInUplinkList)
{
	NS_LOG_FUNCTION (this);
	//std::cout<<"RecvFromLteSocket"<<std::endl;
	uint64_t imsi = mmeUeS1Id;
	std::map<uint64_t, uint16_t>::iterator imsiIt = m_imsiRntiMap.find (imsi);
	NS_ASSERT_MSG (imsiIt != m_imsiRntiMap.end (), "unknown IMSI");
	uint16_t rnti = imsiIt->second;
	EpcEnbS1SapUser::PathSwitchRequestAcknowledgeParameters params;
	params.rnti = rnti;
	m_s1SapUser->PathSwitchRequestAcknowledge (params);
}

void 
EpcEnbApplication::RecvFromLteSocket (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this);
	NS_ASSERT (socket == m_lteSocket);
	Ptr<Packet> packet = socket->Recv ();


	SocketAddressTag satag;
	packet->RemovePacketTag (satag);

	EpsBearerTag tag;
	bool found = packet->RemovePacketTag (tag);
	NS_ASSERT (found);
	uint16_t rnti = tag.GetRnti ();
	uint8_t bid = tag.GetBid ();
	//CRAN
	//std::cout<<"Bearer ID: "<<(uint32_t)bid<<std::endl;
	Ipv4Header ipv4Header;
	packet->RemoveHeader(ipv4Header);
	TcpHeader seqheader;
	packet->PeekHeader(seqheader);
	packet->AddHeader(ipv4Header);
	//std::cout <<"UL src" << seqheader.GetSourcePort() << "dest " << seqheader.GetDestinationPort() << std::endl;
	//===============Insering source port  in map====================
	//std::cout << "==============================" << (uint32_t)seqheader.GetAckNumber().GetValue() << std::endl;
			int i=0;
			int64_t present=Simulator::Now ().GetMilliSeconds();
			int flag_updated=0;
			for(i=0; i<10;i++){
				if(s[i].source_port==seqheader.GetSourcePort() && s[i].dest_port==seqheader.GetDestinationPort())
				{

					if((present-s[i].past)>=1 && seqheader.GetSequenceNumber()>=s[i].last_seqno)
								{
									//std::cout<<"inside condition "<<seqheader.GetSourcePort()<<"," <<seqheader.GetDestinationPort()<<","<< seqheader.GetSequenceNumber()-s[i].last_seqno<<std::endl;
									s[i].past=  present;
									s[i].window_size=s[i].last_seqno;
									s[i].last_seqno=seqheader.GetSequenceNumber();
								}
					flag_updated=1;
					break;
				}
			if(s[i].source_port==0 && s[i].dest_port==0){
				//	New port number is found
					break;
				}
			}
			if(i<10&& flag_updated==0)
			{
				for(i=0; i<10;i++){
							if(s[i].source_port==0 && s[i].dest_port==0)
							{
								s[i].source_port=seqheader.GetSourcePort();
								s[i].past=0;
								s[i].dest_port=seqheader.GetDestinationPort();
								//s[i].last_seqno=seqheader.GetSequenceNumber();
								break;
							}

						}

			}
			for(i=0;i<8;i++) {
						//std::cout << s[i].source_port <<","<<s[i].dest_port <<","<<s[i].last_seqno-s[i].window_size << std::endl;
					}
			//========================
	//std::cout<<"Sequence Header ACK = "<<seqheader.GetAckNumber()<<std::endl;
	//std::cout<<"SourcePort no"<<seqheader.GetSourcePort()<<", Destination port"<<seqheader.GetDestinationPort()<<std::endl;

	//std::cout<<"UE address = "<<ipv4Header.GetSource()<<"    Remote host address = "<<ipv4Header.GetDestination();
	//BEARER_ID_UE_IP_MAP[bid]=ipv4Header.GetSource();
	//--------------
	NS_LOG_LOGIC ("received packet with RNTI=" << (uint32_t) rnti << ", BID=" << (uint32_t)  bid);
	std::map<uint16_t, std::map<uint8_t, uint32_t> >::iterator rntiIt = m_rbidTeidMap.find (rnti);
	if (rntiIt == m_rbidTeidMap.end ())
	{
		std::cout<<"UE context not found, discarding packet"<<std::endl;
		NS_LOG_WARN ("UE context not found, discarding packet");
	}
	else
	{
		std::map<uint8_t, uint32_t>::iterator bidIt = rntiIt->second.find (bid);
		NS_ASSERT (bidIt != rntiIt->second.end ());
		uint32_t teid = bidIt->second;
		SendToS1uSocket (packet, teid);
	}
	// std::cout<<"Context Release Rnti= "<<rnti<<std::endl;

}

void 
EpcEnbApplication::RecvFromS1uSocket (Ptr<Socket> socket)
{
	//std::cout<<"RecvFromS1uSocket"<<std::endl;
	//	std::cout<<"LTE Downlink";
	NS_LOG_FUNCTION (this << socket);
	NS_ASSERT (socket == m_s1uSocket);
	Ptr<Packet> packet = socket->Recv ();
	GtpuHeader gtpu;
	packet->RemoveHeader (gtpu);
	uint32_t teid = gtpu.GetTeid ();

	std::map<uint32_t, EpsFlowId_t>::iterator it = m_teidRbidMap.find (teid);
	// uint32_t tmp_id=it->first;
	// EpsFlowId_t flow_id=it->second;

	// std::cout<<"ID = "<<tmp_id <<" Transaction ID = "<<(uint32_t)flow_id.m_bid<<std::endl;
	//  NS_ASSERT (it != m_teidRbidMap.end ());

	/// \internal
	/// Workaround for \bugid{231}
	SocketAddressTag tag;
	packet->RemovePacketTag (tag);
	// CRAN
	SendToLteSocket (packet, it->second.m_rnti, it->second.m_bid);
}

void 
EpcEnbApplication::SendToLteSocket (Ptr<Packet> packet, uint16_t rnti, uint8_t bid)
{
	Ipv4Header ipv4Header;
	packet->RemoveHeader(ipv4Header);
	TcpHeader seqheader;
	packet->PeekHeader(seqheader);
	uint32_t desport=seqheader.GetDestinationPort();
	uint16_t pid=ipv4Header.GetIdentification();
	SequenceNumber32 ack= seqheader.GetAckNumber();
	packet->AddHeader(ipv4Header);
	//std::cout << "destport " << desport << std::endl;
	NS_LOG_FUNCTION (this << packet << rnti << (uint16_t) bid << packet->GetSize ());
	EpsBearerTag tag (rnti, bid);
	packet->AddPacketTag (tag);
	BEARER_ID_UE_IP_MAP[rnti]=ipv4Header.GetSource();
	//---------------------
	if(EpcEnbApplication::mode==1 ) {
		m_lteSocket->Send (packet);
	}

	else if(EpcEnbApplication::mode==2 ||  EpcEnbApplication::mode==4) {
		int i=0;
		int64_t present=Simulator::Now ().GetMilliSeconds();
		int flag_updated=0;
		for(i=0; i<10;i++){
			if(s[i].source_port==seqheader.GetSourcePort() && s[i].dest_port==seqheader.GetDestinationPort())
			{

				if((present-s[i].past)>=1 && seqheader.GetSequenceNumber()>=s[i].last_seqno)
							{
								//std::cout<<seqheader.GetSourcePort()<<","<<seqheader.GetDestinationPort()<<"," << seqheader.GetSequenceNumber()-s[i].last_seqno<<std::endl;
								s[i].past=  present;
								s[i].window_size=s[i].last_seqno;
								s[i].last_seqno=seqheader.GetSequenceNumber();
							}

				flag_updated=1;
				break;
			}
		if(s[i].source_port==0 && s[i].dest_port==0){
			//	New port number is found
				break;
			}
		}
		if(i<10&& flag_updated==0)
		{
			for(i=0; i<10;i++){
						if(s[i].source_port==0 && s[i].dest_port==0)
						{
							s[i].source_port=seqheader.GetSourcePort();
							s[i].past=0;
							s[i].dest_port=seqheader.GetDestinationPort();
							//s[i].last_seqno=seqheader.GetSequenceNumber();
							break;
						}

					}

		}
		for(i=0;i<8;i++) {
			//std::cout << s[i].source_port <<","<<s[i].dest_port <<","<<s[i].last_seqno-s[i].window_size << std::endl;
		}
		if(ack.GetValue()!=1) {
					//	std::cout<<"Down link through lte"<<std::endl;
						m_lteSocket->Send (packet);
					} else {
						router.senddownlink(packet,2048);
					}
	}

	else if(EpcEnbApplication::mode==3) {
		//===============Insering source port  in map====================
		int i=0;
				int64_t present=Simulator::Now ().GetMilliSeconds();
				int flag_updated=0;
				//std::cout << "==============================" << (uint32_t)ack.GetValue() << std::endl;
				for(i=0; i<10;i++){
					if(s[i].source_port==seqheader.GetSourcePort() && s[i].dest_port==seqheader.GetDestinationPort())
					{

						if((present-s[i].past)>=1 && seqheader.GetSequenceNumber()>=s[i].last_seqno)
									{
										//std::cout<<seqheader.GetSourcePort()<<","<<seqheader.GetDestinationPort()<<"," << seqheader.GetSequenceNumber()-s[i].last_seqno<<std::endl;
										s[i].past=  present;
										s[i].window_size=s[i].last_seqno;
										s[i].last_seqno=seqheader.GetSequenceNumber();
									}

						flag_updated=1;
						break;
					}
				if(s[i].source_port==0 && s[i].dest_port==0){
					//	New port number is found
						break;
					}
				}
				if(i<10&& flag_updated==0)
				{
					for(i=0; i<10;i++){
								if(s[i].source_port==0 && s[i].dest_port==0)
								{
									s[i].source_port=seqheader.GetSourcePort();
									s[i].past=0;
									s[i].dest_port=seqheader.GetDestinationPort();
									//s[i].last_seqno=seqheader.GetSequenceNumber();
									break;
								}

							}

				}
				for(i=0;i<8;i++) {
					//std::cout << s[i].source_port <<","<<s[i].dest_port <<","<<s[i].last_seqno-s[i].window_size << std::endl;
				}
				//========================
		//std::cout << "ltewifi in sendtoltesocket\n" <<std::endl;

		if(pid%4==0)
		{
			packtno++;
			m_lteSocket->Send (packet);
		}
		else{
		//	RouterLayer router;
			router.senddownlink(packet,2048);
			packtno++;
		}
	}
	else if(EpcEnbApplication::mode==5) {

			//===============Insering source port  in map====================
			int i=0;
					int64_t present=Simulator::Now ().GetMilliSeconds();
					int flag_updated=0;
					//std::cout << "==============================" << (uint32_t)ack.GetValue() << std::endl;
					for(i=0; i<10;i++){
						if(s[i].source_port==seqheader.GetSourcePort() && s[i].dest_port==seqheader.GetDestinationPort())
						{

							if((present-s[i].past)>=1 && seqheader.GetSequenceNumber()>=s[i].last_seqno)
										{
											//std::cout<<seqheader.GetSourcePort()<<","<<seqheader.GetDestinationPort()<<"," << seqheader.GetSequenceNumber()-s[i].last_seqno<<std::endl;
											s[i].past=  present;
											s[i].window_size=s[i].last_seqno;
											s[i].last_seqno=seqheader.GetSequenceNumber();
										}

							flag_updated=1;
							break;
						}
					if(s[i].source_port==0 && s[i].dest_port==0){
						//	New port number is found
							break;
						}
					}
					if(i<10&& flag_updated==0)
					{
						for(i=0; i<10;i++){
									if(s[i].source_port==0 && s[i].dest_port==0)
									{
										s[i].source_port=seqheader.GetSourcePort();
										s[i].past=0;
										s[i].dest_port=seqheader.GetDestinationPort();
										//s[i].last_seqno=seqheader.GetSequenceNumber();
										break;
									}

								}

					}
					for(i=0;i<8;i++) {
						//std::cout << s[i].source_port <<","<<s[i].dest_port <<","<<s[i].last_seqno-s[i].window_size << std::endl;
					}
					//========================
			//std::cout << "ltewifi in sendtoltesocket\n" <<std::endl;

			if(desport%2==0)
			{

				packtno++;
				m_lteSocket->Send (packet);
			}
			else{
			//	RouterLayer router;
				router.senddownlink(packet,2048);
				packtno++;
			}
		}
}


void 
EpcEnbApplication::SendToS1uSocket (Ptr<Packet> packet, uint32_t teid)
{
	// std::cout<<"SendToS1uSocket"<<std::endl;
	NS_LOG_FUNCTION (this << packet << teid <<  packet->GetSize ());
	GtpuHeader gtpu;
	gtpu.SetTeid (teid);
	// From 3GPP TS 29.281 v10.0.0 Section 5.1
	// Length of the payload + the non obligatory GTP-U header
	gtpu.SetLength (packet->GetSize () + gtpu.GetSerializedSize () - 8);
	packet->AddHeader (gtpu);
	uint32_t flags = 0;
	m_s1uSocket->SendTo (packet, flags, InetSocketAddress(m_sgwS1uAddress, m_gtpuUdpPort));

}
/*
void
EpcEnbApplication::usrDefinedReleaseUe(uint16_t rnti)
{
	m_s1SapUser->releaseusrDefinedReleaseUe(rnti);
}*/
}; // namespace ns3
