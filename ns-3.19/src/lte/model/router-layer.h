#include <ns3/lte-pdcp-sap.h>
#include "ns3/net-device.h"
#include "ns3/ptr.h"

namespace ns3 {
class router_layer {
	public:
	router_layer();
	~router_layer();
    Ptr<NetDevice> decidenetdevice(Ptr<NetDevice> m_device);
};

}
