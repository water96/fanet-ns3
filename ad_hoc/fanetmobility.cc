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

//=====================================//


RWPFANETMobility::~RWPFANETMobility()
{

}

FanetMobility* RWPFANETMobility::Clone() const
{
  return new RWPFANETMobility();
}

void RWPFANETMobility::SetSimulationTime(const ns3::Time& t)
{
  m_sim_time = t;
}

void RWPFANETMobility::SetMobilityAreaAndSpeed(const ns3::Vector3D& vec, double speed)
{
  m_area = vec;
  m_speed = speed;
}

ns3::Ptr<ns3::PositionAllocator> RWPFANETMobility::CreateInitialPositionAllocater()
{
  Ptr<GridPositionAllocator> initial_alloc = CreateObject<GridPositionAllocator>();
  m_sindex += initial_alloc->AssignStreams(m_sindex);

  double step = 1000.0;

  NS_ASSERT( (m_nodes.GetN() * step) < m_area.x);

  initial_alloc->SetZ(m_area.z);
  initial_alloc->SetMinX(0.0);
  initial_alloc->SetMinY(0.0);
  initial_alloc->SetDeltaX(step);
  initial_alloc->SetDeltaY(0.0);
  initial_alloc->SetN(m_nodes.GetN());
  initial_alloc->SetLayoutType(GridPositionAllocator::ROW_FIRST);
  return ns3::DynamicCast<PositionAllocator>(initial_alloc);
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

void RWPFANETMobility::ConfigureMobilityTracing()
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
