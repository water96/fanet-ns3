#ifndef ADHOC_H
#define ADHOC_H

#include <stdint.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

class RoutingStats
{
public:
  /**
   * \brief Constructor
   * \return none
   */
  RoutingStats ();

  /**
   * \brief Returns the number of bytes received
   * \return the number of bytes received
   */
  uint32_t GetRxBytes ();

  /**
   * \brief Returns the cumulative number of bytes received
   * \return the cumulative number of bytes received
   */
  uint32_t GetCumulativeRxBytes ();

  /**
   * \brief Returns the count of packets received
   * \return the count of packets received
   */
  uint32_t GetRxPkts ();

  /**
   * \brief Returns the cumulative count of packets received
   * \return the cumulative count of packets received
   */
  uint32_t GetCumulativeRxPkts ();

  /**
   * \brief Increments the number of (application-data)
   * bytes received, not including MAC/PHY overhead
   * \param rxBytes the number of bytes received
   * \return none
   */
  void IncRxBytes (uint32_t rxBytes);

  /**
   * \brief Increments the count of packets received
   * \return none
   */
  void IncRxPkts ();

  /**
   * \brief Sets the number of bytes received.
   * \param rxBytes the number of bytes received
   * \return none
   */
  void SetRxBytes (uint32_t rxBytes);

  /**
   * \brief Sets the number of packets received
   * \param rxPkts the number of packets received
   * \return none
   */
  void SetRxPkts (uint32_t rxPkts);

  /**
   * \brief Returns the number of bytes transmitted
   * \return the number of bytes transmitted
   */
  uint32_t GetTxBytes ();

  /**
   * \brief Returns the cumulative number of bytes transmitted
   * \return none
   */
  uint32_t GetCumulativeTxBytes ();

  /**
   * \brief Returns the number of packets transmitted
   * \return the number of packets transmitted
   */
  uint32_t GetTxPkts ();

  /**
   * \brief Returns the cumulative number of packets transmitted
   * \return the cumulative number of packets transmitted
   */
  uint32_t GetCumulativeTxPkts ();

  /**
   * \brief Increment the number of bytes transmitted
   * \param txBytes the number of additional bytes transmitted
   * \return none
   */
  void IncTxBytes (uint32_t txBytes);

  /**
   * \brief Increment the count of packets transmitted
   * \return none
   */
  void IncTxPkts ();

  /**
   * \brief Sets the number of bytes transmitted
   * \param txBytes the number of bytes transmitted
   * \return none
   */
  void SetTxBytes (uint32_t txBytes);

  /**
   * \brief Sets the number of packets transmitted
   * \param txPkts the number of packets transmitted
   * \return none
   */
  void SetTxPkts (uint32_t txPkts);

private:
  uint32_t m_RxBytes; ///< reeive bytes
  uint32_t m_cumulativeRxBytes; ///< cumulative receive bytes
  uint32_t m_RxPkts; ///< receive packets
  uint32_t m_cumulativeRxPkts; ///< cumulative receive packets
  uint32_t m_TxBytes; ///< transmit bytes
  uint32_t m_cumulativeTxBytes; ///< cumulative transmit bytes
  uint32_t m_TxPkts; ///< transmit packets
  uint32_t m_cumulativeTxPkts; ///< cumulative transmit packets
};


class RoutingHelper : public ns3::Object
{
public:
  /**
   * \brief Get class TypeId
   * \return the TypeId for the class
   */
  static ns3::TypeId GetTypeId (void);

  static const double TOTAL_SIM_TIME;
  static const uint16_t PORT;

  enum class ROUTING_PROTOCOL
  {
    NONE = 0u,
    OLSR,
    AODV,
    DSDV,
    DSR
  };

  /**
   * \brief Constructor
   * \return none
   */
  RoutingHelper ();

  /**
   * \brief Destructor
   * \return none
   */
  virtual ~RoutingHelper ();

  /**
   * \brief Installs routing functionality on nodes and their
   * devices and interfaces.
   * \param c node container
   * \param d net device container
   * \param i IPv4 interface container
   * \param totalTime the total time that nodes should attempt to
   * route data
   * \param protocol the routing protocol (1=OLSR;2=AODV;3=DSDV;4=DSR)
   * \param nSinks the number of nodes which will act as data sinks
   * \param routingTables dump routing tables at t=5 seconds (0=no;1=yes)
   * \return none
   */
  void Install (ns3::NodeContainer & c,
                ns3::NetDeviceContainer & d,
                ns3::Ipv4InterfaceContainer & i,
                double totalTime,
                ROUTING_PROTOCOL protocol,
                uint32_t nSinks,
                int routingTables);

  /**
   * \brief Trace the receipt of an on-off-application generated packet
   * \param context this object
   * \param packet a received packet
   * \return none
   */
  void OnOffTrace (std::string context, ns3::Ptr<const ns3::Packet> packet);

  /**
   * \brief Returns the RoutingStats instance
   * \return the RoutingStats instance
   */
  RoutingStats & GetRoutingStats ();

  /**
   * \brief Enable/disable logging
   * \param log non-zero to enable logging
   * \return none
   */
  void SetLogging (int log);

private:
  /**
   * \brief Sets up the protocol protocol on the nodes
   * \param c node container
   * \return none
   */
  void SetupRoutingProtocol (ns3::NodeContainer & c);

  /**
   * \brief Assigns IPv4 addresses to net devices and their interfaces
   * \param d net device container
   * \param adhocTxInterfaces IPv4 interface container
   * \return none
   */
  void AssignIpAddresses (ns3::NetDeviceContainer & d,
                          ns3::Ipv4InterfaceContainer & adhocTxInterfaces);

  /**
   * \brief Sets up routing messages on the nodes and their interfaces
   * \param c node container
   * \param adhocTxInterfaces IPv4 interface container
   * \return none
   */
  void SetupRoutingMessages (ns3::NodeContainer & c,
                             ns3::Ipv4InterfaceContainer & adhocTxInterfaces);

  /**
   * \brief Sets up a routing packet for tranmission
   * \param addr destination address
   * \param node source node
   * \return Socket to be used for sending/receiving a routed data packet
   */
  ns3::Ptr<ns3::Socket> SetupRoutingPacketReceive (ns3::Ipv4Address addr, ns3::Ptr<ns3::Node> node);

  /**
   * \brief Process a received routing packet
   * \param socket the receiving socket
   * \return none
   */
  void ReceiveRoutingPacket (ns3::Ptr<ns3::Socket> socket);

  double m_TotalSimTime;        ///< seconds
  ROUTING_PROTOCOL m_protocol;       ///< routing protocol; 0=NONE, 1=OLSR, 2=AODV, 3=DSDV, 4=DSR
  uint32_t m_port;           ///< port
  uint32_t m_nSinks;              ///< number of sink nodes (< all nodes)
  int m_routingTables;      ///< dump routing table (at t=5 sec).  0=No, 1=Yes
  RoutingStats routingStats; ///< routing statistics
  std::string m_protocolName; ///< protocol name
  int m_log; ///< log
};



#endif // ADHOC_H
