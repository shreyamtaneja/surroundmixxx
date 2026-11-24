# SurroundMixxx

SurroundMixxx is an experimental fork of [Mixxx](https://mixxx.org/), the
open-source DJ software, exploring spatial audio for DJing. It adds playback
and mixing for 5.1 surround tracks while retaining Mixxx's familiar decks and
controls.

**Status:** Windows is the tested platform, and 5.1 is the tested surround
layout. This is a work in progress, not an official Mixxx release.

## What works today

- 5.1 playback through configured six-channel audio hardware, alongside stereo
  tracks.
- Deck gain, EQ, Quick Effects, effect units, faders, tempo and key lock on the
  surround path.
- BPM and key analysis and beat grids for 5.1 tracks; beat and key analysis use
  the front left and right channels.
- 5.1 recording of the internal surround mix to WAV or FLAC.
- Windows WASAPI Shared and Exclusive audio output options.

Surround channels are currently processed as three stereo pairs, with a
separate stereo path for outputs such as headphones, broadcasting and stereo
recording. 5.1 recording does not capture external device processing, and is
unavailable when an external Record/Broadcast input is configured. AIFF and
lossy formats are not available for 5.1 recording. See the
[surround guide](docs/surround-guide.md) for recording and other limitations.

## Getting started

You will need Windows, a 5.1-capable audio device and a suitable speaker setup.
The [surround guide](docs/surround-guide.md) covers output routing, sample
rates, latency, limitations and the project's future direction.

Experimental builds may be available for testing by contacting the maintainer;
a private contact link will be added here when available. There is no public
download link yet. For building from source, see
[CONTRIBUTING.md](CONTRIBUTING.md).

Before testing, back up your Mixxx settings and library: this fork may use the
same settings directory as an existing Mixxx installation.

## Future direction

Planned exploration includes 7.1, native Dolby Atmos support, macOS and Linux,
a redesigned multichannel engine, spatial effects, and configurable speaker
layouts and panning for dancefloors. These are goals, not available features
or commitments to a release date.

## Credits and license

SurroundMixxx builds on the work of the
[Mixxx project and its contributors](https://mixxx.org/). For general Mixxx
usage, see the [Mixxx manual](https://manual.mixxx.org/).

Mixxx is free software distributed under the GNU General Public License
version 2.
See [LICENSE](LICENSE), [COPYING](COPYING), and the accompanying third-party
and skin notices for the applicable terms.
