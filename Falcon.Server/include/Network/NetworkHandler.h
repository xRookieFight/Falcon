#pragma once

#include "Network/BatchedNetworkPeer.h"
#include "Network/CompressedNetworkPeer.h"
#include "Network/Connector.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Packet;
class PacketCodecContext;

class NetworkHandler : public Connector::ConnectionCallbacks {
public:
    class Connection {
    public:
        Connection(const NetworkIdentifier &id, std::shared_ptr<NetworkPeer> peer);

        const NetworkIdentifier &getId() const { return mId; }

        NetworkPeer *getPeer() const { return mPeer.get(); }

        CompressedNetworkPeer *getCompressedPeer() const { return mCompressedPeer.get(); }

        BatchedNetworkPeer *getBatchedPeer() const { return mBatchedPeer.get(); }

    private:
        NetworkIdentifier mId;
        std::shared_ptr<NetworkPeer> mPeer;
        std::shared_ptr<CompressedNetworkPeer> mCompressedPeer;
        std::shared_ptr<BatchedNetworkPeer> mBatchedPeer;
    };

    class Listener {
    public:
        virtual ~Listener() = default;

        virtual bool onValidateIncomingConnection(const NetworkIdentifier &) { return true; }

        virtual void onNewIncomingConnection(const NetworkIdentifier &) {}

        virtual void onConnectionClosed(const NetworkIdentifier &, DisconnectFailReason, const std::string &) {}

        virtual void onDataReceived(const NetworkIdentifier &, const std::string &) {}
    };

    explicit NetworkHandler(std::unique_ptr<Connector> connector);

    ~NetworkHandler() override;

    bool host(const ConnectionDefinition &definition);

    void disconnect();

    void runEvents();

    void send(const NetworkIdentifier &id, const Packet &packet, const PacketCodecContext &context);

    void sendToAll(const Packet &packet, const PacketCodecContext &context);

    void send(const NetworkIdentifier &id, const std::string &data,
              NetworkPeer::Reliability reliability = NetworkPeer::Reliability::ReliableOrdered,
              Compressibility compressibility = Compressibility::Compressible);

    void sendToAll(const std::string &data,
                   NetworkPeer::Reliability reliability = NetworkPeer::Reliability::ReliableOrdered,
                   Compressibility compressibility = Compressibility::Compressible);

    Connection *getConnection(const NetworkIdentifier &id);

    // Flushes the pending batch immediately, needed before the connection settings change
    void flush(const NetworkIdentifier &id);

    void enableCompression(const NetworkIdentifier &id, CompressedNetworkPeer::CompressionAlgorithm algorithm,
                           unsigned short threshold);

    size_t getConnectionCount() const { return mConnections.size(); }

    Connector &getConnector() { return *mConnector; }

    void addListener(Listener *listener);

    void removeListener(Listener *listener);

private:
    bool onValidateIncomingConnection(const NetworkIdentifier &id) override;

    void onNewIncomingConnection(const NetworkIdentifier &id, std::shared_ptr<NetworkPeer> peer) override;

    void onConnectionClosed(const NetworkIdentifier &id, DisconnectFailReason reason,
                            const std::string &message) override;

    static NetworkPeer::Reliability _toPeerReliability(const Packet &packet);

    static Compressibility _toPeerCompressibility(const Packet &packet);

    std::unique_ptr<Connector> mConnector;
    std::unordered_map<NetworkIdentifier, std::unique_ptr<Connection>, NetworkIdentifier::Hasher> mConnections;
    std::vector<Listener *> mListeners;
};
