# SurroundMixxx guide

SurroundMixxx is an experimental Mixxx fork. Windows and 5.1 are the tested
platform and layout; other configurations need further development and
testing. Back up your Mixxx settings and library before testing: the fork may
use the same settings directory as an existing Mixxx installation.

## Sound hardware setup

1. Connect a device and speaker system capable of six-channel PCM output.
   The tested layout is a home-theater-style 5.1 arrangement: front left and
   right, center, LFE/subwoofer, and surround left and right.
2. In **Preferences > Sound Hardware**, select **Windows WASAPI (Shared)** or
   **Windows WASAPI (Exclusive)** and the appropriate output device. Exclusive
   mode requires the device to accept the selected format and prevents other
   applications from using it. It may offer lower latency, depending on the
   device and driver, but bypasses Windows audio processing objects (APOs), so
   APO-based equalizers such as Equalizer APO will not process Mixxx's
   exclusive-mode output. Shared mode keeps Windows audio processing in the
   signal path, which can allow APO-based device effects and equalizers to
   process audio when supported and configured for that device. If exclusive
   mode cannot open, try a supported format or Shared mode.
3. Configure the three surround outputs on the device:

   | Mixxx output | Channels | Typical device channels |
   | --- | --- | --- |
   | Surround Front L/R | Front left, front right | 1-2 |
   | Surround Center/Sub | Center, LFE | 3-4 |
   | Surround L/R | Surround left, surround right | 5-6 |

   Device channel ordering and speaker setup can differ. Verify the mapping
   with known channel-identification audio at a safe volume; do not assume
   that a channel number always corresponds to the same speaker.
4. Choose a sample rate supported by the configured audio devices. **48 kHz**
   is common for surround material; **44.1 kHz** is common for stereo music.
   Some DJ controller soundcards may support only certain rates. Mixxx must
   resample tracks whose rate differs from the engine rate. When using
   multiple devices, check that they support the chosen rate and configure
   synchronization appropriately.
5. Select a buffer that plays reliably on your system. Around **21.3 ms at
   48 kHz** has been used in testing, but it is not a universal optimum.
   Increase it if you hear dropouts or see underflows; lower values reduce
   latency but give the system less time to process audio. For the tested
   single-device setup, use the soundcard clock.

I use WASAPI Shared mode with Equalizer APO to configure custom crossovers and
redirect bass from satellite speakers to the subwoofer. This is my setup, not
a requirement; results depend on the sound device, APO installation and
configuration.

Start with one track and check every speaker, then test with additional decks,
effects and any controller or headphone devices you intend to use. Different
devices, drivers and formats may behave differently.

## How it works

A 5.1 track is handled as six channels in three pairs: front left/right,
center/LFE, and surround left/right. Deck gain is applied to the multichannel
audio, and the processing chain works on pairs using separate effect states.
The mixer combines those pairs and routes them to the three surround outputs.

There is also a separate stereo mix path for Mixxx's stereo outputs, including
headphones, booth, broadcasting and stereo recording. It is not simply a final
downmix of the finished surround output: some processing happens independently
on the two paths, so their effect behavior can differ. 5.1 recording instead
captures the internal surround mix. Beat and key analysis use the track's front
left/right pair; waveform generation still processes multichannel input.

Stereo tracks remain stereo and are sent to the front pair of the surround
mix. They are not automatically expanded into surround sound.

## Current limitations

- **5.1 only:** 7.1 and custom speaker layouts are not implemented as surround
  output layouts. Windows is the tested platform for this fork's surround
  functionality.
- **Not native Atmos rendering:** only the 5.1 bed is playable in Atmos files. The object data is not decoded and Atmos objects are not rendered.
- **Pair-based effects:** center and LFE are independent channels, not a stereo
  image. Some effects have special center/LFE handling, but it is not applied
  consistently at every processing stage. Other effects process pairs
  independently, which can change the spatial image. Test effects before use.
- **Stereo monitoring:** headphones and booth use the separate stereo path,
  not a guaranteed match for the final surround output.
- **Layout:** the current arrangement targets a home-theater-style speaker
  layout, commonly used for consumer surround masters. That layout and its
  channel-based mixes may not be ideal for a dancefloor.
- **Testing:** hardware, formats and features have not all been tested.
  Dropouts, routing problems or unexpected behavior are possible.

## Recording a 5.1 mix

In **Preferences > Recording**, select **6 channel (5.1)** and choose WAV PCM
or FLAC. The recording captures the finished internal surround mix, including
main-output effects and balance, in front left, front right, center, LFE,
surround left, surround right order. Stereo recording and broadcasting continue
to use the separate stereo path.

Recording limitations:

- Only WAV PCM and FLAC are available in 5.1 mode. AIFF and lossy formats are
  disabled; AIFF channel-layout metadata is not reliably provided by the
  current encoder.
- A configured external Record/Broadcast input is stereo, so 5.1 recording is
  unavailable until that input is disconnected.
- Device-level processing, such as Equalizer APO, is not captured. In direct
  microphone monitoring mode, the microphone is mixed externally and is not
  included in the recording.

## Finding multichannel music

Purchasable, downloadable multichannel music can be harder to find than
streaming releases. Sources to explore include commercially released SACD,
DVD-Audio and Blu-ray discs, as well as authorized digital releases from
[NativeDSD](https://www.nativedsd.com/) and [Immersive Audio Album](https://immersiveaudioalbum.com/). Availability,
formats and channel layouts vary by release, so check product details to
confirm format compatibility with your playback setup.
[HelloAtmos](https://helloatmos.com/) is a useful reference for finding which
titles have a Dolby Atmos mix; it helps identify releases but is not itself a
source for audio files. Look for files offered or authorized by artists,
labels or other rights holders. A streaming subscription does not necessarily
grant the right to extract files or use music for DJ performances.

Purchasing a disc does not automatically grant permission to copy it or
perform its contents publicly. Check the applicable rights, service terms
and local law for copying, public performance, recording and distribution.

## Future direction

These are ideas for future development, not current features or release
commitments:

- 7.1 output and broader platform support, including macOS and Linux.
- Dolby Atmos object support, subject to feasible decoding, licensing and
  output support.
- A multichannel engine redesigned around spatial audio rather than stereo
  pairs, with rewritten effects and new native spatial effects.
- Speaker configuration for custom placement, with layouts and panning
  algorithms better suited to dancefloors.

## Distributing experimental builds

A public download is not available yet. Experimental builds may be shared for
testing directly; a private contact link will be added to the
[README](../README.md) when available.
