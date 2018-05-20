/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 * Copyright (c) 2013 Budiarto Herman
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
 * Original work authors (from lte-enb-rrc.cc):
 * - Nicola Baldo <nbaldo@cttc.es>
 * - Marco Miozzo <mmiozzo@cttc.es>
 * - Manuel Requena <manuel.requena@cttc.es>
 *
 * Converted to handover algorithm interface by:
 * - Budiarto Herman <budiarto.herman@magister.fi>
 */

#include "a2-a3-a4-rsrq-handover-algorithm.h"
#include <ns3/log.h>
#include <ns3/uinteger.h>

NS_LOG_COMPONENT_DEFINE ("A2A3A4RsrqHandoverAlgorithm");


namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (A2A3A4RsrqHandoverAlgorithm)
  ;

uint8_t cellid[7];
///////////////////////////////////////////
// Handover Management SAP forwarder
///////////////////////////////////////////


A2A3A4RsrqHandoverAlgorithm::A2A3A4RsrqHandoverAlgorithm ()
  : m_a2MeasId (0),
    m_a4MeasId (0),
    m_servingCellThreshold (30),
    m_neighbourCellOffset (1),
    m_handoverManagementSapUser (0)
{
  NS_LOG_FUNCTION (this);
  m_handoverManagementSapProvider = new MemberLteHandoverManagementSapProvider<A2A3A4RsrqHandoverAlgorithm> (this);
}


A2A3A4RsrqHandoverAlgorithm::~A2A3A4RsrqHandoverAlgorithm ()
{
  NS_LOG_FUNCTION (this);
}


TypeId
A2A3A4RsrqHandoverAlgorithm::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::A2A3A4RsrqHandoverAlgorithm")
    .SetParent<LteHandoverAlgorithm> ()
    .AddConstructor<A2A3A4RsrqHandoverAlgorithm> ()
    .AddAttribute ("ServingCellThreshold",
                   "If the RSRQ of the serving cell is worse than this threshold, "
                   "neighbour cells are consider for handover",
                   UintegerValue (30),
                   MakeUintegerAccessor (&A2A3A4RsrqHandoverAlgorithm::m_servingCellThreshold),
                   MakeUintegerChecker<uint8_t> (0, 34)) // RSRQ range is [0..34] as per Section 9.1.7 of 3GPP TS 36.133
    .AddAttribute ("NeighbourCellOffset",
                   "Minimum offset between serving and best neighbour cell to trigger the Handover",
                   UintegerValue (1),
                   MakeUintegerAccessor (&A2A3A4RsrqHandoverAlgorithm::m_neighbourCellOffset),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("TimeToTrigger",
                   "Time during which neighbour cell's RSRP "
                   "must continuously higher than serving cell's RSRP "
                   "in order to trigger a handover",
                   TimeValue (MilliSeconds (256)), // 3GPP time-to-trigger median value as per Section 6.3.5 of 3GPP TS 36.331
                   MakeTimeAccessor (&A2A3A4RsrqHandoverAlgorithm::m_timeToTrigger),
                   MakeTimeChecker ())
  ;
  return tid;
}


void
A2A3A4RsrqHandoverAlgorithm::SetLteHandoverManagementSapUser (LteHandoverManagementSapUser* s)
{
  NS_LOG_FUNCTION (this << s);
  m_handoverManagementSapUser = s;
}


LteHandoverManagementSapProvider*
A2A3A4RsrqHandoverAlgorithm::GetLteHandoverManagementSapProvider ()
{
  NS_LOG_FUNCTION (this);
  return m_handoverManagementSapProvider;
}


void
A2A3A4RsrqHandoverAlgorithm::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  cellid[0]=0;
  cellid[1]=0;
  cellid[2]=0;
  cellid[3]=0;
  cellid[4]=0;
  cellid[5]=0;
  cellid[6]=0;
  NS_LOG_LOGIC (this << " requesting Event A2 measurements"
                     << " (threshold=" << (uint16_t) m_servingCellThreshold << ")");
  LteRrcSap::ReportConfigEutra reportConfigA2;
  reportConfigA2.eventId = LteRrcSap::ReportConfigEutra::EVENT_A2;
  reportConfigA2.threshold1.choice = LteRrcSap::ThresholdEutra::THRESHOLD_RSRQ;
  reportConfigA2.threshold1.range = m_servingCellThreshold;
  reportConfigA2.timeToTrigger = m_timeToTrigger.GetMilliSeconds ();
  reportConfigA2.triggerQuantity = LteRrcSap::ReportConfigEutra::RSRQ;
  reportConfigA2.reportInterval = LteRrcSap::ReportConfigEutra::MS240;
  m_a2MeasId = m_handoverManagementSapUser->AddUeMeasReportConfigForHandover (reportConfigA2);

  NS_LOG_LOGIC (this << " requesting Event A4 measurements"
                     << " (threshold=0)");
  LteRrcSap::ReportConfigEutra reportConfigA4;
  reportConfigA4.eventId = LteRrcSap::ReportConfigEutra::EVENT_A4;
  reportConfigA4.threshold1.choice = LteRrcSap::ThresholdEutra::THRESHOLD_RSRQ;
  reportConfigA4.threshold1.range = 0; // intentionally very low threshold
  reportConfigA4.triggerQuantity = LteRrcSap::ReportConfigEutra::RSRQ;
  reportConfigA4.reportInterval = LteRrcSap::ReportConfigEutra::MS480;
  m_a4MeasId = m_handoverManagementSapUser->AddUeMeasReportConfigForHandover (reportConfigA4);

  LteHandoverAlgorithm::DoInitialize ();
}


void
A2A3A4RsrqHandoverAlgorithm::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  delete m_handoverManagementSapProvider;
}


void
A2A3A4RsrqHandoverAlgorithm::DoReportUeMeas (uint16_t rnti,
                                           LteRrcSap::MeasResults measResults)
{
  NS_LOG_FUNCTION (this << rnti << (uint16_t) measResults.measId);

  if (measResults.measId == m_a2MeasId)
    {
      NS_ASSERT_MSG (measResults.rsrqResult <= m_servingCellThreshold,
                     "Invalid UE measurement report");
      EvaluateHandover (rnti, measResults);
    }
  else if (measResults.measId == m_a4MeasId)
    {
      if (measResults.haveMeasResultNeighCells
          && !measResults.measResultListEutra.empty ())
        {
          for (std::list <LteRrcSap::MeasResultEutra>::iterator it = measResults.measResultListEutra.begin ();
               it != measResults.measResultListEutra.end ();
               ++it)
            {
              NS_ASSERT_MSG (it->haveRsrqResult == true,
                             "RSRQ measurement is missing from cellId " << it->physCellId);
              UpdateNeighbourMeasurements (rnti, it->physCellId, it->rsrqResult);
            }
        }
      else
        {
          NS_LOG_WARN (this << " Event A4 received without measurement results from neighbouring cells");
        }
    }
  else
    {
      NS_LOG_WARN ("Ignoring measId " << (uint16_t) measResults.measId);
    }

} // end of DoReportUeMeas


void
A2A3A4RsrqHandoverAlgorithm::EvaluateHandover (uint16_t rnti,
                                             LteRrcSap::MeasResults measResults)
{
  NS_LOG_FUNCTION (this << rnti << (uint16_t) measResults.rsrqResult);

  MeasurementTable_t::iterator it1;
  it1 = m_neighbourCellMeasures.find (rnti);

  if (it1 == m_neighbourCellMeasures.end ())
    {
      NS_LOG_WARN ("Skipping handover evaluation for RNTI " << rnti << " because neighbour cells information is not found");
    }
  else
    {
      // Find the best neighbour cell (eNB)
      NS_LOG_LOGIC ("Number of neighbour cells = " << it1->second.size ());
      uint16_t bestNeighbourCellId = 0;
      uint8_t bestNeighbourRsrq = 0;
      MeasurementRow_t::iterator it2;
      uint16_t dif=0;
      uint16_t neighbourrsrp=0;
     // uint8_t c=0;
       uint16_t neighbourcellid=0;
      for (it2 = it1->second.begin (); it2 != it1->second.end (); ++it2)
        {
          neighbourcellid=it2->first;
          neighbourrsrp=it2->second->m_rsrq;
          dif=neighbourrsrp-measResults.rsrqResult;
          cellid[neighbourcellid]=(uint16_t)(0.4*cellid[neighbourcellid]+0.6*dif);
          NS_LOG_WARN("dif = " << dif << " ncellrsrp = " << (uint16_t)cellid[neighbourcellid] << " ncellrsrp = " << (uint16_t)measResults.rsrqResult);
          if ( neighbourrsrp > bestNeighbourRsrq &&  cellid[neighbourcellid] > m_neighbourCellOffset
              && IsValidNeighbour (neighbourcellid))
            {
              bestNeighbourCellId = it2->first;
              bestNeighbourRsrq = neighbourrsrp;
            }
        }

      // Trigger Handover, if needed
      if (bestNeighbourCellId > 0)
        {
          NS_LOG_LOGIC ("Best neighbour cellId " << bestNeighbourCellId);
          if ((bestNeighbourRsrq - measResults1.rsrqResult) >= m_neighbourCellOffset)
            {
         
              NS_LOG_LOGIC ("Trigger Handover to cellId " << bestNeighbourCellId);
              NS_LOG_LOGIC ("target cell RSRQ " << (uint16_t) bestNeighbourRsrq);
              NS_LOG_LOGIC ("serving cell RSRQ " << (uint16_t) measResults.rsrqResult);
                NS_LOG_WARN("rnti " << rnti << " bestnc " << bestNeighbourCellId);
              // Inform eNodeB RRC about handover
              m_handoverManagementSapUser->TriggerHandover (rnti,
                                                            bestNeighbourCellId);
            }
        }

    } // end of else of if (it1 == m_neighbourCellMeasures.end ())

} // end of EvaluateMeasurementReport


bool
A2A3A4RsrqHandoverAlgorithm::IsValidNeighbour (uint16_t cellId)
{
  NS_LOG_FUNCTION (this << cellId);

  /**
   * \todo In the future, this function can be expanded to validate whether the
   *       neighbour cell is a valid target cell, e.g., taking into account the
   *       NRT in ANR and whether it is a CSG cell with closed access.
   */

  return true;
}


void
A2A3A4RsrqHandoverAlgorithm::UpdateNeighbourMeasurements (uint16_t rnti,
                                                        uint16_t cellId,
                                                        uint8_t rsrq)
{
  NS_LOG_FUNCTION (this << rnti << cellId << (uint16_t) rsrq);
  MeasurementTable_t::iterator it1;
  it1 = m_neighbourCellMeasures.find (rnti);

  if (it1 == m_neighbourCellMeasures.end ())
    {
      // insert a new UE entry
      MeasurementRow_t row;
      std::pair<MeasurementTable_t::iterator, bool> ret;
      ret = m_neighbourCellMeasures.insert (std::pair<uint16_t, MeasurementRow_t> (rnti, row));
      NS_ASSERT (ret.second);
      it1 = ret.first;
    }

  NS_ASSERT (it1 != m_neighbourCellMeasures.end ());
  Ptr<UeMeasure> neighbourCellMeasures;
  std::map<uint16_t, Ptr<UeMeasure> >::iterator it2;
  it2 = it1->second.find (cellId);

  if (it2 != it1->second.end ())
    {
      neighbourCellMeasures = it2->second;
      neighbourCellMeasures->m_cellId = cellId;
      neighbourCellMeasures->m_rsrp = 0;
      neighbourCellMeasures->m_rsrq = rsrq;
    }
  else
    {
      // insert a new cell entry
      neighbourCellMeasures = Create<UeMeasure> ();
      neighbourCellMeasures->m_cellId = cellId;
      neighbourCellMeasures->m_rsrp = 0;
      neighbourCellMeasures->m_rsrq = rsrq;
      it1->second[cellId] = neighbourCellMeasures;
    }

} // end of UpdateNeighbourMeasurements


} // end of namespace ns3
