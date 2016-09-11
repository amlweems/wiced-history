
#ifndef NX_MLD_H
#define NX_MLD_H

#include "nx_api.h"
#include "nx_ipv6.h"

#ifdef NX_IPV6_MULTICAST_ENABLE

UINT  _nxd_multicast_interface_join(NX_IP *ip_ptr, NXD_ADDRESS *group_address, UINT nx_interface_index);
UINT  _nxd_multicast_interface_leave(NX_IP *ip_ptr, NXD_ADDRESS *group_address, UINT nx_interface_index);

#endif /* NX_IPV6_MULTICAST_ENABLE  */
#endif