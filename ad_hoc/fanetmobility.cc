#include "fanetmobility.h"

using namespace ns3;

//Creator

FanetMobilityCreator& FanetMobilityCreator::Inst()
{
  static FanetMobilityCreator inst;
  return inst;
}

FanetMobilityCreator::FanetMobilityCreator() : m_inst(nullptr)
{
  //TODO: add constant mobility
  m_models.insert(std::make_pair("RWP", new RWPFANETMobility));
  m_models.insert(std::make_pair("CONST", new RWPFANETMobility));
  m_models.insert(std::make_pair("GM", new GMFANETMobility));

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

void FanetMobility::SetStreamIndex(uint32_t index)
{
  m_sindex = index;
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
      std::size_t s = n_name.find('-');
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

void RWPFANETMobility::Install(const ns3::NodeContainer& c)
{
  m_nodes = c;

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
}

//=====================================//


GMFANETMobility::~GMFANETMobility()
{

}

FanetMobility* GMFANETMobility::Clone() const
{
  return new GMFANETMobility();
}


void GMFANETMobility::Install(const ns3::NodeContainer& c)
{
  m_nodes = c;

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

  m_sindex += mobilityAdhoc.AssignStreams (m_nodes, m_sindex);
  mobilityAdhoc.Install (m_nodes);

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
}

//=====================================//


CONSTFANETMobility::~CONSTFANETMobility()
{

}

FanetMobility* CONSTFANETMobility::Clone() const
{
  return new CONSTFANETMobility();
}

void CONSTFANETMobility::Install(const ns3::NodeContainer& c)
{
  m_nodes = c;

  MobilityHelper mobilityAdhoc;

  mobilityAdhoc.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  m_sindex += mobilityAdhoc.AssignStreams (m_nodes, m_sindex);
  mobilityAdhoc.Install (m_nodes);

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
}
