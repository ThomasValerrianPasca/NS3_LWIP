#include "ns3/log.h"
#include "ns3/router-layer.h"
#include "ns3/net-device.h"
#include "ns3/node.h"

NS_LOG_COMPONENT_DEFINE ("Routerlayer");
namespace ns3 {
router_layer::~router_layer ()
{
  NS_LOG_FUNCTION (this);
}

router_layer::router_layer ()
{
  NS_LOG_FUNCTION (this);
}

Ptr<NetDevice> router_layer::decidenetdevice(Ptr<NetDevice> m_device)
{
	NS_LOG_FUNCTION (this);
	if(m_device->virtualtypeid()=="ns3::WifiNetDevice")
	{
		if(m_device->GetNode()->GetDevice(0)->virtualtypeid()=="ns3::LteNetDevice")
			return m_device->GetNode()->GetDevice(0);
		else
			return m_device->GetNode()->GetDevice(1);
	}
	else if(m_device->virtualtypeid()=="ns3::LteNetDevice") 
	{
		if(m_device->GetNode()->GetDevice(0)->virtualtypeid()=="ns3::LteNetDevice")
			return m_device->GetNode()->GetDevice(0);
		else
			return m_device->GetNode()->GetDevice(1);
	}
	return m_device;
}

}
