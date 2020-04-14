# 9front macOS drawterm client with Audio
This is a fork of the 9front drawterm work done by [cinap_lenrek](http://felloff.net/usr/cinap_lenrek/)

[http://drawterm.9front.org/](http://drawterm.9front.org)

To add /dev/audio support for the macOS build.  I do a lot with audio, so I need this to just work.

## Prerequisite

* macOS - duh!  Here are my machine specs

```bash
$ sw_vers
ProductName:	Mac OS X
ProductVersion:	10.14.6
BuildVersion:	18G95

$ uname -a
Darwin apollo.local 18.7.0 Darwin Kernel Version 18.7.0: Tue Aug 20 16:57:14 PDT 2019; root:xnu-4903.271.2~2/RELEASE_X86_64 x86_64
```

#### XCode
You will need this installed to do a proper build.  [kern/devaudio-macosx](https://github.com/tmendoza/drawterm-macos-audio/blob/macos-audio/kern/devaudio-macosx.c) has Apple Framework dependencies.  YAY!

```bash
$ clang -v
Apple clang version 11.0.0 (clang-1100.0.33.17)
Target: x86_64-apple-darwin18.7.0
Thread model: posix
InstalledDir: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin
```

#### PortAudio
Audio playback is done using PortAudio.  Sorry, but I didn't have the desire to dig thru Apple's AUHAL API's.  Nope. 

Good luck trying to compile PortAudio yourself.  I have better things to do with my life.  Easiest way to obtain this is thru [Homebrew](https://brew.sh/).  

```bash
$ brew install portaudio
==> Installing portaudio
==> Downloading https://homebrew.bintray.com/bottles/portaudio-19.6.0.mojave.bottle.tar.gz
Already downloaded: /Users/tmendoza/Library/Caches/Homebrew/downloads/fcec114fb3bcd4d36e80451544371de751b76af689f0d52dcf260954f3a91784--portaudio-19.6.0.mojave.bottle.tar.gz
==> Pouring portaudio-19.6.0.mojave.bottle.tar.gz
🍺  /usr/local/Cellar/portaudio/19.6.0: 33 files, 452.4KB
```

Depending on your system, you might need need to update [Make.osx-cocoa](https://github.com/tmendoza/drawterm-macos-audio/blob/macos-audio/Make.osx-cocoa) with explicit references to the portaudio headers and libs.  Mine "Just Worked".  

## Extra Features
I have added volume control thru /dev/volume.  Any 'audio source' commands written to /dev/volume will modify the master volume on the OS.  Apple doesn't seem to have an API for per-application volume control, so whatever you write to /dev/volume (1-100) changes the entire system volume for the machine.    Also, any changes made to the volume controls using standard OS tools or volume buttons on your laptop will be reflected when reading /dev/volume.

## TODO
### Official Builds
I am thinking about creating an official build for macOS.  If anyone is interested let me know and I can make it a priority.  [Submit a Feature Request](https://github.com/tmendoza/drawterm-macos-audio/labels/enhancement) for stuff you want.

### Older macOS Version Support
Maybe, but I am no expert Apple Developer so I imagine this will be a non-trivial exercise.  [Submit a Feature Request](https://github.com/tmendoza/drawterm-macos-audio/labels/enhancement) if this is desired.

### Read Audio Support
~~I don't think it would be too hard to add /dev/audio read support.  Would be nice to capture audio from the built in microphone or from a external mic plugged into a higher-end external audio interface like [the one I have](https://focusrite.com/en/usb-c-audio-interface/clarett-usb/clarett-8pre-usb).~~  **COMPLETED**

This could lead to a rudimentary audio conferencing solution over [9P2000](https://en.wikipedia.org/wiki/9P_(protocol)).  If anyone is interested in this [let me know](https://github.com/tmendoza/drawterm-macos-audio/labels/enhancement).

At the very least, /dev/audio read capabilities would allow me to continue my DSP work using [sample based synthesis](https://en.wikipedia.org/wiki/Sample-based_synthesis) on [9front](http://9front.org/).

## Examples

```bash
# Play an Audio File directly to your Default CoreAudio device
% audio/wavdec < piano.wav > /dev/audio

# Capture audio from the builtin microphone of your Mac, convert to Ogg Vorbis then playback
% cat /dev/audio > /usr/glenda/recorded.raw
% audio/oggenc < /usr/glenda/recorded.raw > /usr/glenda/recorded.ogg
% audio/oggdec < /usr/glenda/recorded.ogg > /dev/audio

# Play multiple audio files thru mixfs (mixfs is a multiplexer for /dev/audio device)
% audio/mixfs
% audio/oggdec < /usr/glenda/recorded1.ogg > /dev/audio &
% audio/wavdec < /usr/glenda/recorded2.wav > /dev/audio &
% audio/mp3dec < /usr/glenda/recorded3.mp3 > /dev/audio

```

## Bugs
This was a weekend hack so your mileage may vary.   If anyone tries this out and has issues let me know and I will try and resolve.   [Submit an Issue](https://github.com/tmendoza/drawterm-macos-audio/issues) for Boog fixes.