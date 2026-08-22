#include "Network/NetworkHandler.h"

#include "Core/Utility/BinaryStream.h"
#include "Protocol/Packet.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <mutex>

namespace {

    std::mutex gPacketLogMutex;

    void logPacketToFile(const char *direction, const Packet &packet, const std::string &payload) {
        std::lock_guard<std::mutex> lock(gPacketLogMutex);

        static std::ofstream file("packet.txt", std::ios::trunc);
        if (!file.is_open())
            return;

        file << direction << " " << packet.getName() << " id=" << (int) packet.getId()
             << " size=" << payload.size() << "\n";

        const size_t limit = payload.size() < 256 ? payload.size() : 256;
        char hex[4];
        for (size_t i = 0; i < limit; i++) {
            snprintf(hex, sizeof(hex), "%02x ", (unsigned char) payload[i]);
            file << hex;

            if ((i + 1) % 32 == 0)
                file << "\n";
        }

        file << "\n\n";
    }

}

NetworkHandler::Connection::Connection(const NetworkIdentifier &id, std::shared_ptr<NetworkPeer> peer)
        : mId(id), mPeer(std::move(peer)) {
    mCompressedPeer = std::make_shared<CompressedNetworkPeer>(mPeer);
    mBatchedPeer = std::make_shared<BatchedNetworkPeer>(mCompressedPeer);
}

NetworkHandler::NetworkHandler(std::unique_ptr<Connector> connector) : mConnector(std::move(connector)) {
    mConnector->setCallbacks(this);
}

NetworkHandler::~NetworkHandler() {
    disconnect();
    mConnector->setCallbacks(nullptr);
}

bool NetworkHandler::host(const ConnectionDefinition &definition) {
    return mConnector->host(definition);
}

void NetworkHandler::disconnect() {
    mConnector->disconnect();
    mConnections.clear();
}

void NetworkHandler::addListener(Listener *listener) {
    if (listener != nullptr && std::find(mListeners.begin(), mListeners.end(), listener) == mListeners.end())
        mListeners.push_back(listener);
}

void NetworkHandler::removeListener(Listener *listener) {
    mListeners.erase(std::remove(mListeners.begin(), mListeners.end(), listener), mListeners.end());
}

NetworkHandler::Connection *NetworkHandler::getConnection(const NetworkIdentifier &id) {
    auto it = mConnections.find(id);
    return it == mConnections.end() ? nullptr : it->second.get();
}

void NetworkHandler::flush(const NetworkIdentifier &id) {
    Connection *connection = getConnection(id);
    if (connection != nullptr)
        connection->getBatchedPeer()->flush();
}

void NetworkHandler::enableCompression(const NetworkIdentifier &id,
                                       CompressedNetworkPeer::CompressionAlgorithm algorithm,
                                       unsigned short threshold) {
    Connection *connection = getConnection(id);
    if (connection != nullptr)
        connection->getCompressedPeer()->enableCompression(algorithm, threshold);
}

NetworkPeer::Reliability NetworkHandler::_toPeerReliability(const Packet &packet) {
    switch (packet.mReliability) {
        case Packet::Reliability::Reliable:
            return NetworkPeer::Reliability::Reliable;
        case Packet::Reliability::Unreliable:
            return NetworkPeer::Reliability::Unreliable;
        case Packet::Reliability::UnreliableSequenced:
            return NetworkPeer::Reliability::UnreliableSequenced;
        default:
            return NetworkPeer::Reliability::ReliableOrdered;
    }
}

Compressibility NetworkHandler::_toPeerCompressibility(const Packet &packet) {
    return packet.mCompressible == Packet::Compressibility::Incompressible
           ? Compressibility::Incompressible
           : Compressibility::Compressible;
}

void NetworkHandler::send(const NetworkIdentifier &id, const Packet &packet, const PacketCodecContext &context) {
    Connection *connection = getConnection(id);
    if (connection == nullptr)
        return;

    BinaryStream stream;
    packet.writeWithHeader(stream, context);

    logPacketToFile("SEND", packet, stream.getBuffer());

    connection->getBatchedPeer()->sendPacket(stream.getBuffer(), _toPeerReliability(packet),
                                             _toPeerCompressibility(packet));
}

void NetworkHandler::sendToAll(const Packet &packet, const PacketCodecContext &context) {
    BinaryStream stream;
    packet.writeWithHeader(stream, context);

    const NetworkPeer::Reliability reliability = _toPeerReliability(packet);
    const Compressibility compressibility = _toPeerCompressibility(packet);

    for (auto &entry: mConnections)
        entry.second->getBatchedPeer()->sendPacket(stream.getBuffer(), reliability, compressibility);
}

void NetworkHandler::send(const NetworkIdentifier &id, const std::string &data, NetworkPeer::Reliability reliability,
                          Compressibility compressibility) {
    Connection *connection = getConnection(id);
    if (connection == nullptr)
        return;

    connection->getBatchedPeer()->sendPacket(data, reliability, compressibility);
}

void NetworkHandler::sendToAll(const std::string &data, NetworkPeer::Reliability reliability,
                               Compressibility compressibility) {
    for (auto &entry: mConnections)
        entry.second->getBatchedPeer()->sendPacket(data, reliability, compressibility);
}

void NetworkHandler::runEvents() {
    mConnector->runEvents();

    std::string data;

    for (auto &entry: mConnections) {
        NetworkPeer *peer = entry.second->getBatchedPeer();

        while (peer->receivePacket(data) == NetworkPeer::DataStatus::HasData) {
            for (Listener *listener: mListeners)
                listener->onDataReceived(entry.first, data);
        }

        peer->update();
        peer->flush();
    }
}

bool NetworkHandler::onValidateIncomingConnection(const NetworkIdentifier &id) {
    for (Listener *listener: mListeners) {
        if (!listener->onValidateIncomingConnection(id))
            return false;
    }

    return true;
}

void NetworkHandler::onNewIncomingConnection(const NetworkIdentifier &id, std::shared_ptr<NetworkPeer> peer) {
    mConnections[id] = std::unique_ptr<Connection>(new Connection(id, std::move(peer)));

    for (Listener *listener: mListeners)
        listener->onNewIncomingConnection(id);
}

void NetworkHandler::onConnectionClosed(const NetworkIdentifier &id, DisconnectFailReason reason,
                                        const std::string &message) {
    if (mConnections.erase(id) == 0)
        return;

    for (Listener *listener: mListeners)
        listener->onConnectionClosed(id, reason, message);
}
