#include "fanetmobility.h"

#include "ns3/paparazzi-mobility-model.h"
#include <cstdlib>

using namespace ns3;

//Creator

FanetMobilityCreator& FanetMobilityCreator::Inst()
{
  static FanetMobilityCreator inst;
  return inst;
}

FanetMobilityCreator::FanetMobilityCreator() : m_inst(nullptr)
{
  m_models.insert(std::make_pair("RWP", new RWPFANETMobility));
  m_models.insert(std::make_pair("CONST", new CONSTFANETMobility));
  m_models.insert(std::make_pair("GM", new GMFANETMobility));
  m_models.insert(std::make_pair("PPRZ", new PPRZFANETMobility));
  m_models.insert(std::make_pair("RPGM", new RPGMFANETMobility));

  m_default = m_models.begin()->first;
}

std::string FanetMobilityCreator::GetModelsList()
{
  std::string ret;
  for(auto& it : m_models)
  {
    ret += it.first + ",";
  }

  return ret.substr(0, ret.length() - 1);
}

std::string FanetMobilityCreator::GetDefaultModel()
{
  return m_default;
}

FanetMobilityCreator::~FanetMobilityCreator()
{
  for(auto& it : m_models)
  {
    delete it.second;
  }

  DestroyMobilityModel();
}

int FanetMobilityCreator::CreateMobilityModel(const std::string &model)
{
  auto if_find = m_models.find(model);
  if(if_find != m_models.end())
  {
    m_inst = if_find->second->Clone();
  }
  else
  {
    m_inst = nullptr;
  }

  return (m_inst != nullptr) ? 0 : 1;
}

FanetMobility& FanetMobilityCreator::GetMobilityModel()
{
  NS_ASSERT(m_inst != nullptr);
  return *m_inst;
}

void FanetMobilityCreator::DestroyMobilityModel()
{
  if(m_inst)
  {
    delete m_inst;
    m_inst = nullptr;
  }
}


//===========================================================//


FanetMobility::FanetMobility()
{

}

FanetMobility::~FanetMobility(){}

void FanetMobility::SetSimulationTime(const ns3::Time& t)
{
  m_sim_time = t;
}

void FanetMobility::SetMobilityAreaAndSpeed(const ns3::Vector3D& vec, double speed)
{
  m_area = vec;
  m_speed = speed;
}

ns3::Ptr<ns3::PositionAllocator> FanetMobility::CreateInitialPositionAllocater()
{
  Ptr<GridPositionAllocator> initial_alloc = CreateObject<GridPositionAllocator>();
  m_sindex += initial_alloc->AssignStreams(m_sindex);

  double step = 1000.0;

  NS_ASSERT( (m_nodes.GetN() * step) < m_area.x);

  double center = m_area.x / 2.0;
  double min_x;
  min_x = center - step * (double)(m_nodes.GetN() - 1) / 2.0;

  initial_alloc->SetZ(m_area.z);
  initial_alloc->SetMinX(min_x);
  initial_alloc->SetMinY(0.0);
  initial_alloc->SetDeltaX(step);
  initial_alloc->SetDeltaY(0.0);
  initial_alloc->SetN(m_nodes.GetN());
  initial_alloc->SetLayoutType(GridPositionAllocator::ROW_FIRST);
  return ns3::DynamicCast<PositionAllocator>(initial_alloc);
}

void FanetMobility::ConfigureMobilityTracing()
{
  //Create trace object
  m_all_nodes_mobility_trace = ns3::CreateObject<AllNodesMobilityTracer>();
  //

  //Through nodes
  for(auto it = m_nodes.Begin(); it != m_nodes.End(); it++)
  {
    Ptr<Node> n = *it;
    std::string n_name = Names::FindName(n);

    //Mobility
    Ptr<MobilityModel> node_mob = (n)->GetObject<MobilityModel>();
    if(node_mob)
    {
      //add node to all mob trace
      std::size_t s = n_name.find_last_of('n');
      m_all_nodes_mobility_trace->AddNodeMobilityModel(node_mob, n_name.substr(s+1));
    }
  }

  m_all_nodes_mobility_trace->CreateOutput("all-nodes-mobs.csv");
  ns3::Simulator::Schedule(Time(0.0), &AllNodesMobilityTracer::DumperCb, m_all_nodes_mobility_trace, 1.0);
}

//=====================================//

RWPFANETMobility::~RWPFANETMobility()
{

}

FanetMobility* RWPFANETMobility::Clone() const
{
  return new RWPFANETMobility();
}

uint32_t RWPFANETMobility::Install(const ns3::NodeContainer& c, uint32_t stream_index)
{
  m_nodes = c;
  m_sindex = stream_index;

  MobilityHelper mobilityAdhoc;

  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomBoxPositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(m_area.x) + "]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=" + std::to_string(m_area.y) + "]"));
  pos.Set ("Z", StringValue ("ns3::UniformRandomVariable[Min=" + std::to_string(m_area.z * 0.8) + "|Max=" + std::to_string(m_area.z * 1.2) + "]"));

  Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
  m_sindex += taPositionAlloc->AssignStreams (m_sindex);

  std::stringstream ssSpeed;
  ssSpeed << "ns3::UniformRandomVariable[Min=" << m_speed * 0.8 << "|Max=" << m_speed << "]";
  std::stringstream ssPause;
  ssPause << "ns3::ConstantRandomVariable[Constant=0]";
  mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                  "Speed", StringValue (ssSpeed.str ()),
                                  "Pause", StringValue (ssPause.str ()),
                                  "PositionAllocator", PointerValue (taPositionAlloc));
  mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
  mobilityAdhoc.Install (m_nodes);
  m_sindex += mobilityAdhoc.AssignStreams (m_nodes, m_sindex);

  //Initial pos
  Ptr<PositionAllocator> initial_pos = CreateInitialPositionAllocater();

  for(auto it = m_nodes.Begin(); it != m_nodes.End(); it++)
  {
    Ptr<MobilityModel> node_mob = (*it)->GetObject<MobilityModel>();
    if(node_mob)
    {
      node_mob->SetPosition(initial_pos->GetNext());
    }
  }

  return m_sindex;
}

//=====================================//


GMFANETMobility::~GMFANETMobility()
{

}

FanetMobility* GMFANETMobility::Clone() const
{
  return new GMFANETMobility();
}


uint32_t GMFANETMobility::Install(const ns3::NodeContainer& c, uint32_t stream_index)
{
  m_nodes = c;
  m_sindex = stream_index;

  MobilityHelper mobilityAdhoc;

  double d_dev = (M_PI /(3 * 3));
  d_dev = d_dev * d_dev;

  double p_dev = (M_PI /(8 * 3));
  p_dev = p_dev * p_dev;

  double speed_dev = 0.05 * m_speed;
  speed_dev = speed_dev * speed_dev;

  mobilityAdhoc.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
                                  "Bounds", BoxValue (Box (0, m_area.x, 0, m_area.y, 0.8*m_area.z, 1.2*m_area.z)),
                                  "TimeStep", TimeValue (Seconds (2.0)),
                                  "Alpha", DoubleValue (0.95),
                                  "MeanVelocity", StringValue ("ns3::ConstantRandomVariable[Constant=" + std::to_string(m_speed) + "]"),
                                  //"MeanDirection", StringValue ("ns3::UniformRandomVariable[Min=" + std::to_string(M_PI/4.0) + "|Max=" + std::to_string(3*M_PI/4.0) + "]"),
                                  "MeanDirection", StringValue ("ns3::ConstantRandomVariable[Constant=" + std::to_string(M_PI / 3.0) + "]"),
                                  "MeanPitch", StringValue ("ns3::ConstantRandomVariable[Constant=0]"),
                                  "NormalVelocity", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=" + std::to_string(speed_dev) + "|Bound=" + std::to_string(3*sqrt(speed_dev)) + "]"),
                                  "NormalDirection", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=" + std::to_string(d_dev) + "|Bound=" + std::to_string(3*sqrt(d_dev)) + "]"),
                                  "NormalPitch", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=" + std::to_string(p_dev) + "|Bound=" + std::to_string(sqrt(p_dev)) + "]"));

  mobilityAdhoc.Install (m_nodes);
  m_sindex += mobilityAdhoc.AssignStreams (m_nodes, m_sindex);

  //Initial pos
  Ptr<PositionAllocator> initial_pos = CreateInitialPositionAllocater();

  for(auto it = m_nodes.Begin(); it != m_nodes.End(); it++)
  {
    Ptr<MobilityModel> node_mob = (*it)->GetObject<MobilityModel>();
    if(node_mob)
    {
      node_mob->SetPosition(initial_pos->GetNext());
    }
  }

  return m_sindex;
}

//=====================================//


CONSTFANETMobility::~CONSTFANETMobility()
{

}

FanetMobility* CONSTFANETMobility::Clone() const
{
  return new CONSTFANETMobility();
}

ns3::Ptr<ns3::PositionAllocator> CONSTFANETMobility::CreateInitialPositionAllocater()
{
  Ptr<GridPositionAllocator> initial_alloc = CreateObject<GridPositionAllocator>();
  m_sindex += initial_alloc->AssignStreams(m_sindex);

  double step = 1000.0;

  NS_ASSERT( (m_nodes.GetN() * step) < m_area.x);

  double center = m_area.x / 2.0;
  double min_x;
  min_x = center - step * (double)(m_nodes.GetN() - 1) / 2.0;

  initial_alloc->SetZ(m_area.z);
  initial_alloc->SetMinX(min_x);
  initial_alloc->SetMinY(m_area.y);
  initial_alloc->SetDeltaX(step);
  initial_alloc->SetDeltaY(0.0);
  initial_alloc->SetN(m_nodes.GetN());
  initial_alloc->SetLayoutType(GridPositionAllocator::ROW_FIRST);
  return ns3::DynamicCast<PositionAllocator>(initial_alloc);
}

uint32_t CONSTFANETMobility::Install(const ns3::NodeContainer& c, uint32_t stream_index)
{
  m_nodes = c;
  m_sindex = stream_index;

  MobilityHelper mobilityAdhoc;

  mobilityAdhoc.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobilityAdhoc.Install (m_nodes);
  m_sindex += mobilityAdhoc.AssignStreams (m_nodes, m_sindex);

  //Initial pos
  Ptr<PositionAllocator> initial_pos = CreateInitialPositionAllocater();

  for(auto it = m_nodes.Begin(); it != m_nodes.End(); it++)
  {
    Ptr<MobilityModel> node_mob = (*it)->GetObject<MobilityModel>();
    if(node_mob)
    {
      node_mob->SetPosition(initial_pos->GetNext());
    }
  }

  return m_sindex;
}

//=====================================//


PPRZFANETMobility::~PPRZFANETMobility()
{

}

FanetMobility* PPRZFANETMobility::Clone() const
{
  return new PPRZFANETMobility();
}


uint32_t PPRZFANETMobility::Install(const ns3::NodeContainer& c, uint32_t stream_index)
{
  m_nodes = c;
  m_sindex = stream_index;

  //Initial pos
  Ptr<PositionAllocator> initial_pos = CreateInitialPositionAllocater();
  double a;

  if(m_area.x > m_area.y)
  {
    a = m_area.y;
  }
  else
  {
    a = m_area.x;
  }

  MobilityHelper mobilityAdhoc;

  for(auto it = m_nodes.Begin(); it != m_nodes.End(); it++)
  {
    MobilityHelper mobilityAdhoc;
    ns3::UniformRandomVariable radius;
    radius.SetStream(m_sindex);
    NodeContainer one_node(*it);

    mobilityAdhoc.SetMobilityModel ("ns3::PaparazziMobilityModel",
                                    "Radius", DoubleValue (a / radius.GetValue(2.0, 10.0)),
                                    "Bounds", BoxValue (Box (0, m_area.x, 0, m_area.y, 0.8*m_area.z, 1.2*m_area.z)),
                                    "Speed", DoubleValue(m_speed),
                                    "TimeStep", DoubleValue(radius.GetValue(0.5, 2.5)),
                                    "CircleRad", DoubleValue(a / radius.GetValue(25, 150)));

    mobilityAdhoc.SetPositionAllocator(initial_pos);
    mobilityAdhoc.Install (one_node);
    m_sindex += mobilityAdhoc.AssignStreams (one_node, m_sindex);
  }

  return m_sindex;
}

//=====================================//

RPGMFANETMobility::~RPGMFANETMobility()
{

}

FanetMobility* RPGMFANETMobility::Clone() const
{
  return new RPGMFANETMobility();
}

std::string RPGMFANETMobility::GetScenarioName(uint32_t n, double s)
{
  std::stringstream ss;
  ss << "RPGM_" << n << "_" << s;
  return ss.str();
}

std::string RPGMFANETMobility::CreateBmCommandString(const std::string& bm_path, uint32_t n, uint32_t r_seed, double s, double t, const ns3::Vector3D& area)
{
  std::stringstream ss;
  double r;
  if(area.x > area.y)
  {
    r = area.y * 0.1;
  }
  else
  {
    r = area.x * 0.1;
  }
  ss << bm_path << "/bm -f RPGM_" << n << "_" << s << " RPGM "
     << "-d " << t << " -i 100 -n " << n << " -x " << area.x << " -y " << area.y << " -J 2D -R " << r_seed
     << " -h " << (s + 10) << " -l " << (s - 10) << " -p 0 "
     << "-a " << (double)(n / 3.0) << " -c 0.75 -r " << r << " -s " << (double)(n / 2.0);

  ss << "; " << bm_path << "/bm NSFile -f RPGM_" << n << "_" << s << " -b";
  return ss.str();
}

uint32_t RPGMFANETMobility::Install(const ns3::NodeContainer& c, uint32_t stream_index)
{
  m_nodes = c;
  m_sindex = stream_index;

  Ptr<ns3::UniformRandomVariable> var = CreateObject<ns3::UniformRandomVariable>();
  var->SetStream(m_sindex);

  char* bm_tool_path = std::getenv("BM_TOOL");
  if(bm_tool_path == nullptr)
  {
    std::cerr << "Can not find bm tool environment variable!\n Set BM_TOOL var\n";
    return m_sindex;
  }

  std::string cmd_str = CreateBmCommandString(bm_tool_path,
                                              c.GetN(),
                                              var->GetInteger(),
                                              m_speed,
                                              m_sim_time.GetSeconds(),
                                              m_area);

  int res = std::system(cmd_str.c_str());

  if(res != 0)
  {
    std::cerr << "Error to create trace file!\n";
    return 1;
  }

  std::string filename = GetScenarioName(c.GetN(), m_speed) + ".ns_movements";
  Ns2MobilityHelper ns2m(filename);

  var->SetAttribute ("Min", DoubleValue (m_area.z - 300));
  var->SetAttribute ("Max", DoubleValue (m_area.z + 300));
  ns2m.SetZCoord(var);
  ns2m.Install(c.Begin(), c.End());

  return (m_sindex++);
}

//=====================================//
