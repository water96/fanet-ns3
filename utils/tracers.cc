#include "tracers.h"

//Tracers
const std::string TracerBase::m_delimeter = ",\t";
const std::string TracerBase::m_q = "\"";
ExpResults TracerBase::m_all;

StatsCollector Ipv4L3ProtocolTracer::m_stats;
std::ofstream Ipv4L3ProtocolTracer::m_stats_out;

//
StatsCollector::StatsCollector ()
  : m_RxBytes (0),
    m_cumulativeRxBytes (0),
    m_RxPkts (0),
    m_cumulativeRxPkts (0),
    m_TxBytes (0),
    m_cumulativeTxBytes (0),
    m_TxPkts (0),
    m_cumulativeTxPkts (0)
{
}

uint32_t
StatsCollector::GetRxBytes ()
{
  return m_RxBytes;
}

uint32_t
StatsCollector::GetCumulativeRxBytes ()
{
  return m_cumulativeRxBytes;
}

uint32_t
StatsCollector::GetRxPkts ()
{
  return m_RxPkts;
}

uint32_t
StatsCollector::GetCumulativeRxPkts ()
{
  return m_cumulativeRxPkts;
}

void
StatsCollector::IncRxBytes (uint32_t rxBytes)
{
  m_RxBytes += rxBytes;
  m_cumulativeRxBytes += rxBytes;
}

void
StatsCollector::IncRxPkts ()
{
  m_RxPkts++;
  m_cumulativeRxPkts++;
}

void
StatsCollector::SetRxBytes (uint32_t rxBytes)
{
  m_RxBytes = rxBytes;
}

void
StatsCollector::SetRxPkts (uint32_t rxPkts)
{
  m_RxPkts = rxPkts;
}

uint32_t
StatsCollector::GetTxBytes ()
{
  return m_TxBytes;
}

uint32_t
StatsCollector::GetCumulativeTxBytes ()
{
  return m_cumulativeTxBytes;
}

uint32_t
StatsCollector::GetTxPkts ()
{
  return m_TxPkts;
}

uint32_t
StatsCollector::GetCumulativeTxPkts ()
{
  return m_cumulativeTxPkts;
}

void
StatsCollector::IncTxBytes (uint32_t txBytes)
{
  m_TxBytes += txBytes;
  m_cumulativeTxBytes += txBytes;
}

void
StatsCollector::IncTxPkts ()
{
  m_TxPkts++;
  m_cumulativeTxPkts++;
}

void
StatsCollector::SetTxBytes (uint32_t txBytes)
{
  m_TxBytes = txBytes;
}

void
StatsCollector::SetTxPkts (uint32_t txPkts)
{
  m_TxPkts = txPkts;
}

