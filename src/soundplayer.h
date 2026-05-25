#ifndef SOUNDPLAYER_H
#define SOUNDPLAYER_H

#include <QObject>
#include <QString>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QQueue>
#include <QPair>

class SoundPlayer : public QObject {
    Q_OBJECT

public:
    static constexpr const char *SOUNDS_URL = "https://api.pushover.net/sounds";

    explicit SoundPlayer(QObject *parent = nullptr);

    void play(const QString &soundName, const QString &cacheDir);

signals:
    void downloadFailed(const QString &soundName, const QString &error);

private slots:
    void onMediaStateChanged(QMediaPlayer::State state);
    void onMediaError(QMediaPlayer::Error error);

private:
    QString cachePath(const QString &soundName, const QString &cacheDir) const;
    bool soundExists(const QString &soundName, const QString &cacheDir) const;
    void downloadSound(const QString &soundName, const QString &cacheDir);
    void playFromCache(const QString &soundName, const QString &cacheDir);
    void processNextDownload();

    QMediaPlayer *m_player;
    QNetworkAccessManager *m_network;
    QQueue<QPair<QString, QString>> m_downloadQueue;
    bool m_downloading;
};

#endif // SOUNDPLAYER_H
