#ifndef GRAPH_H
#define GRAPH_H

#include <map>
#include <vector>
#include <algorithm>
#include <numeric>
#include <set>

template <class T_data>
class Graph
{
public:
  enum class GraphConnectivity
  {
    NOT_CONNECTED = 0,
    CONNECTED,
    UNDEFINED
  };

private:
  std::map<T_data, uint32_t> m_node_id;
  std::map<uint32_t, std::set<uint32_t> > m_graph;
  uint32_t m_id_cnter;
  GraphConnectivity m_conn;
  uint32_t m_full_node_connections;

  uint32_t AddNode(const T_data& node)
  {
    uint32_t ret;
    auto it_find = m_node_id.find(node);
    if(it_find == m_node_id.end())
    {
      m_node_id.insert(std::make_pair(node, m_id_cnter));
      ret = m_id_cnter;
      m_id_cnter++;
      m_graph.insert(std::make_pair(ret, std::set<uint32_t>()));
    }
    else
    {
      ret = it_find->second;
    }
    return ret;
  }

  void DFS(uint32_t node_id, std::vector<uint8_t>& visited)
  {
    visited[node_id] = true;

    for (auto i : m_graph[node_id])
        if (!visited[i])
            DFS(i, visited);
  }

public:
  Graph() : m_id_cnter(0), m_conn(GraphConnectivity::UNDEFINED), m_full_node_connections(0) {}
  ~Graph() {}

  void AddNodeAndItsLinks(const T_data& node, const std::set<T_data>& vec)
  {
    uint32_t n_id = AddNode(node);
    for(auto i : vec)
    {
      uint32_t l = AddNode(i);
      m_graph.at(n_id).insert(l);
      m_full_node_connections++;
    }
    m_conn = GraphConnectivity::UNDEFINED;
  }

  void AddNodeAndItsLinks(const T_data& node, const T_data& dest)
  {
    uint32_t n_id = AddNode(node);
    uint32_t d = AddNode(dest);
    m_graph.at(n_id).insert(d);
    m_full_node_connections++;

    m_conn = GraphConnectivity::UNDEFINED;
  }

  uint32_t GetNodeDegree(const T_data& node)
  {
    uint32_t id = AddNode(node);
    return m_graph.at(id).size();
  }

  bool IsConnected()
  {
    if(m_conn != GraphConnectivity::UNDEFINED)
    {
      return (m_conn == GraphConnectivity::CONNECTED);
    }

    std::vector<uint8_t> vis(m_id_cnter, 0);

    DFS(0, vis);
    uint32_t s = std::accumulate(vis.begin(), vis.end(), 0);
    if(s != m_id_cnter)
    {
      m_conn = GraphConnectivity::NOT_CONNECTED;
      return false;
    }

    vis = std::vector<uint8_t>(m_id_cnter, 0);

    auto g_bkcp = m_graph;
    uint32_t a = 0;
    std::vector<std::pair<uint32_t, uint32_t> > adj;
    for(auto& i : m_graph)
    {
      uint32_t id = i.first;
      std::for_each(i.second.begin(),
                    i.second.end(),
                    [&adj, id](uint32_t p) {
                                             adj.push_back(std::make_pair(id, p));
                                           });
      i.second.clear();
    }
    //Create transpose m_graph
    for(auto i : adj)
    {
      m_graph.at(i.second).insert(i.first);
    }
    DFS(0, vis);
    m_graph = g_bkcp;

    s = std::accumulate(vis.begin(), vis.end(), 0);
    if(s != m_id_cnter)
    {
      m_conn = GraphConnectivity::NOT_CONNECTED;
      return false;
    }

    m_conn = GraphConnectivity::CONNECTED;
    return true;
  }

};

#endif // GRAPH_H
