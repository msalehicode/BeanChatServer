#include "musicbot.h"

#include "../server/server.h"
#include "../models/user.h"
#include "../models/channel.h"
#include <crypto/Crypto.h>

MusicBot::MusicBot(Server *server,
                   QObject *parent)
    : QObject(parent),
    m_server(server)
{
}

bool MusicBot::initialize()
{
    if (m_initialized)
        return true;


    if (!m_opus.initialize(
            48000,
            1,
            32000))
    {
        qCritical() << "Failed to initialize music bot Opus encoder.";
        return false;
    }


    m_user = new UserModel;

    //
    // Virtual user defaults
    //

    m_user->id = 500; //for now we assuming we have only one bot and reserver id 500+ for bots

    QByteArray pubkey, pvkey;
    if(!BeanChatCommon::Crypto::generateKeyPair(pubkey,pvkey))
    {
        qDebug() << "failed to generate key pair/identity for bot, id=" << m_user->id;
        delete m_user;
        return false;
    }

    m_user->identity = QString::fromUtf8(pubkey.toBase64());

    qDebug() << "bot init identity=" << QString::fromUtf8(pubkey.toBase64());

    m_user->isAdmin=true;

    m_user->username = "Music Bot";

    m_user->connected = true;

    m_user->status = BeanChatCommon::Presence::Online;

    m_user->muted = false;

    m_user->deafened = false;

    m_user->camera = false;

    m_user->udpRegistered=true;

    //
    // Future
    //
    // m_user->isBot = true;
    //

    //
    // Register into server.
    //
    m_server->registerVirtualUser(m_user);

    m_initialized = true;

    return true;
}


void MusicBot::sendPcmFrame(const QByteArray &pcm)
{
    if (!m_user)
        return;

    if (!m_user->currentChannel)
        return;

    if (pcm.isEmpty())
        return;

    QByteArray opus = m_opus.encode(pcm);

    if (opus.isEmpty())
        return;

    VoicePacket packet;

    packet.senderId = m_user->id;
    packet.sequence = ++m_sequence;
    packet.audioData = opus;

    m_server->broadcastVoice(packet, m_user);
}

UserModel *MusicBot::user() const
{
    return m_user;
}

bool MusicBot::isInitialized() const
{
    return m_initialized;
}

bool MusicBot::isSpeaking() const
{
    return m_speaking;
}

bool MusicBot::isInChannel() const
{
    return m_user &&
           m_user->currentChannel;
}

Channel *MusicBot::currentChannel() const
{
    if (!m_user)
        return nullptr;

    return m_user->currentChannel;
}

bool MusicBot::joinChannel(Channel *channel)
{
    qDebug() << "musicbot->join channel";
    if (!m_initialized)
        return false;

    if (!channel)
        return false;

    if (channel == m_user->currentChannel)
        return true;

    BeanChatCommon::ChatMessageChunkPacket dummy;

    QByteArray result =
        m_server->joinChannel(
            m_user,
            channel->id,
            QString(),
            dummy);

    if (result.isEmpty())
    {
        qDebug() << "channel not found or password is incorrect";
        return false;
    }


    //notify everyone.
    Packet packet;
    packet.type = PacketType::UserJoinedChannel;
    packet.payload = result;
    m_server->lastTcpActivity.restart(); //keep trace of when was last time we sent something to everyone.
    QByteArray bytes = packet.serialize();
    for (UserModel *user : m_server->users())
    {
        if (user->socket)
            user->socket->write(bytes);
    }



    qDebug() << "musicbot joined channel!";
    m_server->printChannelWithUsersIn();

    emit joinedChannel(channel);

    return true;
}

void MusicBot::leaveChannel()
{
    if (!m_initialized)
        return;

    if (!m_user->currentChannel)
        return;

    Channel *old = m_user->currentChannel;

    BeanChatCommon::ChatMessageChunkPacket dummy;

    QByteArray result =
        m_server->joinChannel(
            m_user,
            0,
            QString(),
            dummy);

    if (result.isEmpty())
    {
        qDebug() << "channel not found or password is incorrect";
        return;
    }


    //notify everyone.
    Packet packet;
    packet.type = PacketType::UserJoinedChannel;
    packet.payload = result;
    m_server->lastTcpActivity.restart(); //keep trace of when was last time we sent something to everyone.
    QByteArray bytes = packet.serialize();
    for (UserModel *user : m_server->users())
    {
        if (user->socket)
            user->socket->write(bytes);
    }


    qDebug() << "musicbot left channel!";
    m_server->printChannelWithUsersIn();


    emit leftChannel(old);
}

void MusicBot::startSpeaking()
{
    if (m_speaking)
        return;

    m_speaking = true;

    emit speakingChanged(true);
}

void MusicBot::stopSpeaking()
{
    if (!m_speaking)
        return;

    m_speaking = false;

    emit speakingChanged(false);
}

void MusicBot::setDisplayName(const QString &name)
{
    if (!m_user)
        return;

    m_user->username = name;
}

void MusicBot::setAvatarHash(const QString &hash)
{
    if (!m_user)
        return;

    m_user->avatarHash = hash;
}
