#include "soundplayer.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QUrl>
#include <QTimer>

Q_LOGGING_CATEGORY(lcSoundPlayer, "net.sailpush.sailfish.sound")

SoundPlayer::SoundPlayer(QObject *parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this))
    , m_network(new QNetworkAccessManager(this))
    , m_downloading(false)
{
    connect(m_player, &QMediaPlayer::stateChanged, this, &SoundPlayer::onMediaStateChanged);
    connect(m_player, static_cast<void(QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error),
            this, &SoundPlayer::onMediaError);
}

void SoundPlayer::play(const QString &soundName, const QString &cacheDir)
{
    if (soundName.isEmpty()) return;

    if (soundExists(soundName, cacheDir)) {
        playFromCache(soundName, cacheDir);
    } else {
        downloadSound(soundName, cacheDir);
    }
}

void SoundPlayer::onMediaStateChanged(QMediaPlayer::State state)
{
    if (state == QMediaPlayer::StoppedState) {
        m_player->setMedia(QUrl());
    }
}

void SoundPlayer::onMediaError(QMediaPlayer::Error error)
{
    Q_UNUSED(error);
    qCWarning(lcSoundPlayer) << "Playback error:" << m_player->errorString();
}

QString SoundPlayer::cachePath(const QString &soundName, const QString &cacheDir) const
{
    return QDir(cacheDir).filePath(soundName + ".mp3");
}

bool SoundPlayer::soundExists(const QString &soundName, const QString &cacheDir) const
{
    return QFile::exists(cachePath(soundName, cacheDir));
}

void SoundPlayer::downloadSound(const QString &soundName, const QString &cacheDir)
{
    m_downloadQueue.enqueue(qMakePair(soundName, cacheDir));
    if (!m_downloading) {
        processNextDownload();
    }
}

void SoundPlayer::processNextDownload()
{
    if (m_downloadQueue.isEmpty()) {
        m_downloading = false;
        return;
    }

    m_downloading = true;
    QPair<QString, QString> next = m_downloadQueue.dequeue();
    QString soundName = next.first;
    QString cacheDir = next.second;

    QUrl url(QString("%1/%2.mp3").arg(SOUNDS_URL, soundName));
    QNetworkRequest request(url);
    QNetworkReply *reply = m_network->get(request);
    QTimer::singleShot(30000, reply, [reply]() { if (reply->isRunning()) reply->abort(); });
    reply->setProperty("soundName", soundName);
    reply->setProperty("cacheDir", cacheDir);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QString soundName = reply->property("soundName").toString();
        QString cacheDir = reply->property("cacheDir").toString();

        reply->deleteLater();
        m_downloading = false;

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(lcSoundPlayer) << "Sound download failed:" << reply->errorString();
            emit downloadFailed(soundName, reply->errorString());
        } else {
            QByteArray data = reply->readAll();
            QString path = cachePath(soundName, cacheDir);
            QDir dir = QFileInfo(path).dir();
            if (!dir.exists() && !dir.mkpath(".")) {
                qCWarning(lcSoundPlayer) << "Failed to create cache directory:" << dir.path();
                emit downloadFailed(soundName, "Failed to create cache directory");
                processNextDownload();
                return;
            }

            QFile file(path);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();
                qCInfo(lcSoundPlayer) << "Sound downloaded:" << soundName;
                playFromCache(soundName, cacheDir);
            } else {
                qCWarning(lcSoundPlayer) << "Failed to save sound:" << file.errorString();
            }
        }

        processNextDownload();
    });

    qCInfo(lcSoundPlayer) << "Downloading sound:" << soundName;
}

void SoundPlayer::playFromCache(const QString &soundName, const QString &cacheDir)
{
    QString path = cachePath(soundName, cacheDir);
    m_player->setMedia(QUrl::fromLocalFile(path));
    m_player->play();
    qCInfo(lcSoundPlayer) << "Playing sound:" << soundName;
}
