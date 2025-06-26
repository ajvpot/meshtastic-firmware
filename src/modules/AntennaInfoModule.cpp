#include "AntennaInfoModule.h"
#include "Default.h"
#include "MeshService.h"
#include "NodeDB.h"
#include "RTC.h"
#include "Status.h"
#include <Throttle.h>

AntennaInfoModule *antennaInfoModule;

/*
Prints a single antenna info packet
Uses LOG_DEBUG, which equates to Console.log
NOTE: For debugging only
*/
void AntennaInfoModule::printAntennaInfo(const char *header, const meshtastic_AntennaInfo *ai)
{
    LOG_DEBUG("%s ANTENNAINFO PACKET from Node 0x%x", header, ai->node_id);
    LOG_DEBUG("Height: %.2f m, Azimuth: %d°, Orientation: %d°, EIRP: %.1f dBm", 
              ai->height_m, ai->azimuth_degrees, ai->orientation_degrees, ai->eirp_dbm);
}

/* Constructor */
AntennaInfoModule::AntennaInfoModule()
    : ProtobufModule("antennainfo", meshtastic_PortNum_ANTENNA_INFO_APP, &meshtastic_AntennaInfo_msg),
      concurrency::OSThread("AntennaInfo")
{
    ourPortNum = meshtastic_PortNum_ANTENNA_INFO_APP;
    nodeStatusObserver.observe(&nodeStatus->onNewStatus);

    if (moduleConfig.antenna_info.enabled) {
        setIntervalFromNow(Default::getConfiguredOrDefaultMs(moduleConfig.antenna_info.update_interval,
                                                             default_antenna_info_broadcast_secs));
    } else {
        LOG_DEBUG("AntennaInfoModule is disabled");
        disable();
    }
}

/*
Collect antenna info from the module config
Assumes that the antennaInfo packet has been allocated
@returns true if valid antenna info was collected
*/
bool AntennaInfoModule::collectAntennaInfo(meshtastic_AntennaInfo *antennaInfo)
{
    NodeNum my_node_id = nodeDB->getNodeNum();
    antennaInfo->node_id = my_node_id;
    antennaInfo->height_m = moduleConfig.antenna_info.height_m;
    antennaInfo->azimuth_degrees = moduleConfig.antenna_info.azimuth_degrees;
    antennaInfo->orientation_degrees = moduleConfig.antenna_info.orientation_degrees;
    antennaInfo->eirp_dbm = moduleConfig.antenna_info.eirp_dbm;
    antennaInfo->last_updated = getTime();

    printAntennaInfo("COLLECTING", antennaInfo);
    return true;
}

/* Send antenna info to the mesh */
void AntennaInfoModule::sendAntennaInfo(NodeNum dest, bool wantReplies)
{
    meshtastic_AntennaInfo antennaInfo = meshtastic_AntennaInfo_init_zero;
    if (collectAntennaInfo(&antennaInfo)) {
        meshtastic_MeshPacket *p = allocDataProtobuf(antennaInfo);
        p->to = dest;
        p->decoded.want_response = wantReplies;
        p->priority = meshtastic_MeshPacket_Priority_BACKGROUND;
        printAntennaInfo("SENDING", &antennaInfo);
        service->sendToMesh(p, RX_SRC_LOCAL, true);
    }
}

/*
Encompasses the full construction and sending packet to mesh
Will be used for broadcast.
*/
int32_t AntennaInfoModule::runOnce()
{
    if (moduleConfig.antenna_info.transmit_over_lora &&
        (!channels.isDefaultChannel(channels.getPrimaryIndex()) || !RadioInterface::uses_default_frequency_slot) &&
        airTime->isTxAllowedChannelUtil(true) && airTime->isTxAllowedAirUtil()) {
        sendAntennaInfo(NODENUM_BROADCAST, false);
    } else {
        sendAntennaInfo(NODENUM_BROADCAST_NO_LORA, false);
    }
    return Default::getConfiguredOrDefaultMs(moduleConfig.antenna_info.update_interval, default_antenna_info_broadcast_secs);
}

/*
Collect a received antenna info packet from another node
Pass it to an upper client; do not persist this data on the mesh
*/
bool AntennaInfoModule::handleReceivedProtobuf(const meshtastic_MeshPacket &mp, meshtastic_AntennaInfo *ai)
{
    if (ai) {
        printAntennaInfo("RECEIVED", ai);
        // Allow others to handle this packet
        return false;
    }
    return false;
}

/*
 * Handle status updates from the node status module
 */
int AntennaInfoModule::handleStatusUpdate(const meshtastic::Status *status)
{
    // Only respond to node status updates
    if (status->getStatusType() == STATUS_TYPE_NODE) {
        if (moduleConfig.antenna_info.enabled) {
            setIntervalFromNow(Default::getConfiguredOrDefaultMs(moduleConfig.antenna_info.update_interval,
                                                                 default_antenna_info_broadcast_secs));
        } else {
            disable();
        }
    }
    return 0;
} 