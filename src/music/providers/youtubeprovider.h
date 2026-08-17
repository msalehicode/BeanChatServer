#pragma once

#include <QObject>
#include <QProcess>

#include "../track.h"

class YoutubeProvider : public QObject
{
    Q_OBJECT

public:
    explicit YoutubeProvider(QObject *parent = nullptr);

    void search(const QString &query);

signals:
    void trackReady(const Track &track);
    void errorOccurred(const QString &error);

private slots:
    void onSearchFinished(int exitCode,
                          QProcess::ExitStatus status);

    void onGetUrlFinished(int exitCode,
                          QProcess::ExitStatus status);

private:
    void requestStreamUrl();

private:
    QProcess m_process;

    QString m_query;

    Track m_track;
};
