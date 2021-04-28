// Copyright 2014 Electronic Arts Inc.

#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H 1

#include <QtCore/QMap>
#include <QtCore/QSharedPointer>
#include <QtMultimedia/QSoundEffect>

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class AudioPlayer : public QObject
{
    Q_OBJECT

public:

    AudioPlayer();

    Q_INVOKABLE int play(const QString& audioFilePath, int numberOfLoops = 1);
    Q_INVOKABLE void stop(int ref);

private slots:

    void onPlayingChanged();

private:

    typedef QSharedPointer<QSoundEffect> SoundRef;
    typedef QMap<int, SoundRef> Sounds;

    int ticket;
    Sounds playedSounds;
};

inline AudioPlayer::AudioPlayer()
    : QObject()
    , ticket(0)
    , playedSounds()
{
}


}
}
}

#endif