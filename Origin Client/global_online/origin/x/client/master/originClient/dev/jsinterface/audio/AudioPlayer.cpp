// Copyright 2014 Electronic Arts Inc.

#include <jsinterface/audio/AudioPlayer.h>

#include <services/debug/DebugService.h>
#include <services/log/LogService.h>
#include <services/platform/PlatformService.h>

#include <QtCore/QDir>
#include <QtCore/QMutableMapIterator>

namespace Origin
{
namespace Client
{
namespace JsInterface
{


int AudioPlayer::play(const QString& audioFilePath, int numberOfLoops)
{
    QString fullPath(QDir::toNativeSeparators(Origin::Services::PlatformService::resourcesDirectory() + "/" + audioFilePath));
    ORIGIN_LOG_DEBUG << "Playing " << fullPath;
    SoundRef soundToPlay(new QSoundEffect(), &QObject::deleteLater);
    soundToPlay->setSource(QUrl::fromLocalFile(fullPath));
    soundToPlay->setLoopCount((numberOfLoops == -1) ? QSoundEffect::Infinite : numberOfLoops);
    soundToPlay->setVolume(Origin::Services::PlatformService::GetCurrentApplicationVolume());
    playedSounds[++ticket] = soundToPlay;
    ORIGIN_VERIFY_CONNECT(soundToPlay.data(), SIGNAL(playingChanged()), this, SLOT(onPlayingChanged()));
    soundToPlay->play();
    return ticket;
}

void AudioPlayer::stop(int soundToStop)
{
    if (playedSounds.contains(soundToStop))
    {
        SoundRef stoppedSound = playedSounds.take(soundToStop);
        stoppedSound->stop();
    }
}

void AudioPlayer::onPlayingChanged()
{
    //Sounds::iterator last(playedSounds.end());
    //for (Sounds::iterator i(playedSounds.begin()); i != last; ++i)
    //{
    //    if (!(**i).isPlaying())
    //    {

    //    }
    //}

    typedef QMutableMapIterator<int, SoundRef> MutableSoundsIterator;
    MutableSoundsIterator i(playedSounds);
    while (i.hasNext())
    {
        i.next();
        if (!i.value()->isPlaying())
        {
            i.remove();
        }
    }
}

}
}
}
