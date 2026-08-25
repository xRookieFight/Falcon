#include "Network/Handler/NetworkHandler.h"

#include "Core/Utility/BinaryStream.h"
#include "Protocol/Packet.h"

#include <algorithm>
#include <chrono>
#include <utility>

NetworkHandler::Connection::Connection(const NetworkIdentifier &id, std::shared_ptr<NetworkPeer> peer)
        : mId(id), mPeer(std::move(peer)) {
    mCompressedPeer = std::make_shared<CompressedNetworkPeer>(mPeer);
    mBatchedPeer = std::make_shared<BatchedNetworkPeer>(mCompressedPeer);
}

NetworkHandler::NetworkHandler(std::unique_ptr<Connector> connector)
        : mConnector(std::move(connector)), mIoRunning(false), mConnectionCount(0) {
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
    stopIoThread();

    mConnector->disconnect();
    mConnections.clear();
    mConnectionCount.store(0);
}

void NetworkHandler::startIoThread() {
    if (mIoRunning.load())
        return;

    mIoRunning.store(true);
    mIoThread = std::thread(&NetworkHandler::_ioLoop, this);
}

void NetworkHandler::stopIoThread() {
    if (!mIoRunning.load())
        return;

    mIoRunning.store(false);

    if (mIoThread.joinable())
        mIoThread.join();

    mOutbound.close();
    mInbound.close();
}

void NetworkHandler::_ioLoop() {
    while (mIoRunning.load()) {
        _pumpIo();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    _pumpIo();
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
    OutboundCommand command;
    command.mKind = OutboundCommand::Kind::Flush;
    command.mId = id;
    mOutbound.push(std::move(command));
}

void NetworkHandler::enableCompression(const NetworkIdentifier &id,
                                       CompressedNetworkPeer::CompressionAlgorithm algorithm,
                                       unsigned short threshold) {
    OutboundCommand command;
    command.mKind = OutboundCommand::Kind::EnableCompression;
    command.mId = id;
    command.mAlgorithm = algorithm;
    command.mThreshold = threshold;
    mOutbound.push(std::move(command));
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
    BinaryStream stream;
    packet.writeWithHeader(stream, context);

    send(id, stream.getBuffer(), _toPeerReliability(packet), _toPeerCompressibility(packet));
}

void NetworkHandler::sendToAll(const Packet &packet, const PacketCodecContext &context) {
    BinaryStream stream;
    packet.writeWithHeader(stream, context);

    sendToAll(stream.getBuffer(), _toPeerReliability(packet), _toPeerCompressibility(packet));
}

void NetworkHandler::send(const NetworkIdentifier &id, const std::string &data, NetworkPeer::Reliability reliability,
                          Compressibility compressibility) {
    OutboundCommand command;
    command.mKind = OutboundCommand::Kind::Data;
    command.mId = id;
    command.mData = data;
    command.mReliability = reliability;
    command.mCompressibility = compressibility;
    mOutbound.push(std::move(command));
}

void NetworkHandler::sendToAll(const std::string &data, NetworkPeer::Reliability reliability,
                               Compressibility compressibility) {
    OutboundCommand command;
    command.mKind = OutboundCommand::Kind::Data;
    command.mData = data;
    command.mBroadcast = true;
    command.mReliability = reliability;
    command.mCompressibility = compressibility;
    mOutbound.push(std::move(command));
}

void NetworkHandler::_applyOutbound(OutboundCommand &command) {
    if (command.mKind == OutboundCommand::Kind::Data) {
        if (command.mBroadcast) {
            for (auto &entry: mConnections)
                entry.second->getBatchedPeer()->sendPacket(command.mData, command.mReliability,
                                                           command.mCompressibility);
            return;
        }

        Connection *connection = getConnection(command.mId);
        if (connection != nullptr)
            connection->getBatchedPeer()->sendPacket(command.mData, command.mReliability, command.mCompressibility);
        return;
    }

    Connection *connection = getConnection(command.mId);
    if (connection == nullptr)
        return;

    if (command.mKind == OutboundCommand::Kind::Flush)
        connection->getBatchedPeer()->flush();
    else
        connection->getCompressedPeer()->enableCompression(command.mAlgorithm, command.mThreshold);
}

void NetworkHandler::_pumpIo() {
    mConnector->runEvents();

    std::vector<OutboundCommand> commands = mOutbound.drain();
    for (OutboundCommand &command: commands)
        _applyOutbound(command);

    std::string data;

    for (auto &entry: mConnections) {
        NetworkPeer *peer = entry.second->getBatchedPeer();

        while (peer->receivePacket(data) == NetworkPeer::DataStatus::HasData) {
            InboundEvent event;
            event.mKind = InboundEvent::Kind::Data;
            event.mId = entry.first;
            event.mData = data;
            mInbound.push(std::move(event));
        }

        peer->update();
        peer->flush();
    }
}

void NetworkHandler::runEvents() {
    if (!mIoRunning.load())
        _pumpIo();

    std::vector<InboundEvent> events = mInbound.drain();

    for (InboundEvent &event: events) {
        switch (event.mKind) {
            case InboundEvent::Kind::Opened:
                for (Listener *listener: mListeners)
                    listener->onNewIncomingConnection(event.mId);
                break;

            case InboundEvent::Kind::Closed:
                for (Listener *listener: mListeners)
                    listener->onConnectionClosed(event.mId, event.mReason, event.mData);
                break;

            default:
                for (Listener *listener: mListeners)
                    listener->onDataReceived(event.mId, event.mData);
                break;
        }
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
    mConnectionCount.store(mConnections.size());

    InboundEvent event;
    event.mKind = InboundEvent::Kind::Opened;
    event.mId = id;
    mInbound.push(std::move(event));
}

void NetworkHandler::onConnectionClosed(const NetworkIdentifier &id, DisconnectFailReason reason,
                                        const std::string &message) {
    if (mConnections.erase(id) == 0)
        return;

    mConnectionCount.store(mConnections.size());

    InboundEvent event;
    event.mKind = InboundEvent::Kind::Closed;
    event.mId = id;
    event.mReason = reason;
    event.mData = message;
    mInbound.push(std::move(event));
}
