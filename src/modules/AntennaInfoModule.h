#pragma once
#include "ProtobufModule.h"
#include "Status.h"

/*
 * AntennaInfo module for sending antenna information to the mesh
 */
class AntennaInfoModule : public ProtobufModule<meshtastic_AntennaInfo>, private concurrency::OSThread
{
    CallbackObserver<AntennaInfoModule, const meshtastic::Status *> nodeStatusObserver =
        CallbackObserver<AntennaInfoModule, const meshtastic::Status *>(this, &AntennaInfoModule::handleStatusUpdate);

  public:
    /*
     * Expose the constructor
     */
    AntennaInfoModule();

  protected:
    /*
     * Called to handle a particular incoming message
     * @return true if you've guaranteed you've handled this message and no other handlers should be considered for it
     */
    virtual bool handleReceivedProtobuf(const meshtastic_MeshPacket &mp, meshtastic_AntennaInfo *ai) override;

    /*
     * Collect antenna info from the module config
     * Assumes that the antennaInfo packet has been allocated
     * @returns true if valid antenna info was collected
     */
    bool collectAntennaInfo(meshtastic_AntennaInfo *antennaInfo);

    /*
     * Send antenna info to the mesh
     */
    void sendAntennaInfo(NodeNum dest = NODENUM_BROADCAST, bool wantReplies = false);

    /*
     * Handle status updates from the node status module
     */
    int handleStatusUpdate(const meshtastic::Status *status);

    /* Does our periodic broadcast */
    int32_t runOnce() override;

    /* These are for debugging only */
    void printAntennaInfo(const char *header, const meshtastic_AntennaInfo *ai);
};

extern AntennaInfoModule *antennaInfoModule; 