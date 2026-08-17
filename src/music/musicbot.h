#pragma once

#include <QObject>
#include "opuscodec.h"

class Server;
class UserModel;
class Channel;

class MusicBot : public QObject
{
    Q_OBJECT

public:
    explicit MusicBot(Server *server,
                      QObject *parent = nullptr);

    ~MusicBot() override = default;

    bool initialize();

    UserModel *user() const;

    bool isInitialized() const;

    bool isSpeaking() const;

    bool isInChannel() const;

    Channel *currentChannel() const;

    bool joinChannel(Channel *channel);

    void leaveChannel();

    void startSpeaking();

    void stopSpeaking();

    void setDisplayName(const QString &name);

    void setAvatarHash(const QString &hash);

    void sendPcmFrame(const QByteArray &pcm);

signals:

    void joinedChannel(Channel *channel);

    void leftChannel(Channel *channel);

    void speakingChanged(bool speaking);

private:

    Server *m_server = nullptr;

    OpusCodec m_opus;
    quint32 m_sequence = 0;

    UserModel *m_user = nullptr;

    bool m_initialized = false;

    bool m_speaking = false;
};
