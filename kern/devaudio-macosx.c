/*
 * Linux and BSD
 */
//#include <Cocoa/Cocoa.h>
//#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioServices.h>

#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"
#include	"devaudio.h"
#include    "portaudio.h"

#define	THRESHOLD	0.005	
#define NUM_CHANNELS  (2)
#define SAMPLE_RATE   (44100)
#define FRAMES_PER_BUFFER  (2048)
#define PA_SAMPLE_TYPE (paInt16)
#define SAMPLE_SILENCE  (0)
#define SAMPLE_SIZE (2)
#define FRAME_SIZE (4)

const PaDeviceInfo *deviceInfo;
PaStreamParameters outputParameters;
PaStream *stream;
PaError err;
uchar *outputBuffer = NULL;
int devcnt, i, numBytes;
int leftvol, rightvol;
int gain;

float normalize(float oldval) {
	float newmin = 0.01;
	float newmax = 1.0;
	float oldmin = 1.0;
	float oldmax = 100.0;

	float newval, oldrange, newrange;

	oldrange = (oldmax-oldmin);
	newrange = (newmax-newmin);
	newval = (((oldval - oldmin) * newrange) / oldrange) + newmin;
	return newval;
}

float denormalize(float oldval) {
	float oldmin = 0.01;
	float oldmax = 1.0;
	float newmin = 1.0;
	float newmax = 100.0;

	float newval, oldrange, newrange;

	oldrange = (oldmax-oldmin);
	newrange = (newmax-newmin);
	newval = (((oldval - oldmin) * newrange) / oldrange) + newmin;
	return newval;
}

AudioDeviceID obtainDefaultOutputDevice(void);
float systemVolume(void);
void setSystemVolume(float);

/* maybe this should return -1 instead of sysfatal */
void
audiodevopen(void)
{
    err = Pa_Initialize();

    if (err != paNoError) 
		oserror();

	// Check to make sure there are audio devices present
    devcnt = Pa_GetDeviceCount();
    if( devcnt < 0 )
		oserror();

    deviceInfo = Pa_GetDeviceInfo( Pa_GetDefaultInputDevice() );
    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    outputParameters.channelCount = NUM_CHANNELS;
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    /* -- setup stream -- */
    err = Pa_OpenStream(
          &stream,
          NULL,
          &outputParameters,
          SAMPLE_RATE,
          FRAMES_PER_BUFFER,
          paClipOff,      /* we won't output out of range samples so don't bother clipping them */
          NULL, /* no callback, use blocking API */
          NULL ); /* no callback, so no callback userData */

    if( err != paNoError )
		oserror();

    numBytes = FRAMES_PER_BUFFER * NUM_CHANNELS * SAMPLE_SIZE ;
    outputBuffer = (uchar *) malloc(numBytes);
    memset( outputBuffer, SAMPLE_SILENCE, numBytes );

    /* -- start stream -- */
    err = Pa_StartStream( stream );

    if( err != paNoError ) 
        oserror();

	leftvol = rightvol = (int) denormalize(systemVolume());

	return;
}

void
audiodevclose(void)
{
    /* -- Now we stop the stream -- */
    err = Pa_StopStream( stream );

    if( err != paNoError ) 
        oserror();

    /* -- don't forget to cleanup! -- */
    err = Pa_CloseStream( stream );

    if( err != paNoError ) 
        oserror();

    err = Pa_Terminate();
    if( err != paNoError )
        oserror();
	return;
}

int
audiodevread(void *a, int n)
{
	error("no audio read support");
	return -1;
}

int
audiodevwrite(void *v, int n)
{
	int m;
	int totalFrames;
	m = 0;

	while(n > sizeof(outputBuffer)){
		audiodevwrite(v, sizeof(outputBuffer));
		v = (uchar*)v + sizeof(outputBuffer);
		n -= sizeof(outputBuffer);
		m += sizeof(outputBuffer);
	}

	memmove(outputBuffer, v, n);

	totalFrames = sizeof(outputBuffer)/FRAME_SIZE;

	err = Pa_WriteStream( stream, outputBuffer, totalFrames );
	    
	if( err != paNoError )
		oserror();

	return m + n;	
}

void
audiodevsetvol(int what, int left, int right)
{
	if(what == Vaudio) {
		leftvol = left;
		rightvol = right;
		setSystemVolume(normalize((float)left));
	}
}

void
audiodevgetvol(int what, int *left, int *right)
{
	switch(what) {
		case Vspeed:
			*left = *right = SAMPLE_RATE;
			break;
		case Vaudio:
			leftvol = rightvol = (int) denormalize(systemVolume());
			*left = *right = rightvol;
			break;
		case Vtreb:
		case Vbass:
			*left = *right = 50;
			break;
		case Vpcm:
			*left = *right = 16;
		default:
			*left = *right = 0;
	}
}

AudioDeviceID obtainDefaultOutputDevice()
{
    AudioDeviceID theAnswer = kAudioObjectUnknown;
    UInt32 theSize = sizeof(AudioDeviceID);
    AudioObjectPropertyAddress theAddress;
	
	theAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
	theAddress.mScope = kAudioObjectPropertyScopeGlobal;
	theAddress.mElement = kAudioObjectPropertyElementMaster;
	
	//first be sure that a default device exists
	if (! AudioObjectHasProperty(kAudioObjectSystemObject, &theAddress) )	{
		printf("Unable to get default audio device\n");
		return theAnswer;
	}
	//get the property 'default output device'
    OSStatus theError = AudioObjectGetPropertyData(kAudioObjectSystemObject, &theAddress, 0, NULL, &theSize, &theAnswer);
    if (theError != noErr) {
		printf("Unable to get output audio device\n");
		return theAnswer;
	}
    return theAnswer;
}

float systemVolume(void)
{	
	AudioDeviceID				defaultDevID = kAudioObjectUnknown;
	UInt32						theSize = sizeof(Float32);
	OSStatus					theError;
	Float32						theVolume = 0;
	AudioObjectPropertyAddress	theAddress;
	
	defaultDevID = obtainDefaultOutputDevice();
	if (defaultDevID == kAudioObjectUnknown) return 0.0;		//device not found: return 0
	
	theAddress.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume;
	theAddress.mScope = kAudioDevicePropertyScopeOutput;
	theAddress.mElement = kAudioObjectPropertyElementMaster;
	
	//be sure that the default device has the volume property
	if (! AudioObjectHasProperty(defaultDevID, &theAddress) ) {
		printf("No volume control for device 0x%0x\n",defaultDevID);
		return 0.0;
	}
	
	//now read the property and correct it, if outside [0...1]
	theError = AudioObjectGetPropertyData(defaultDevID, &theAddress, 0, NULL, &theSize, &theVolume);
	if ( theError != noErr )	{
		printf("Unable to read volume for device 0x%0x\n", defaultDevID);
		return 0.0;
	}
	theVolume = theVolume > 1.0 ? 1.0 : (theVolume < 0.0 ? 0.0 : theVolume);
	
	return theVolume;
}

void setSystemVolume(float theVolume)
{
	float						newValue = theVolume;
	AudioObjectPropertyAddress	theAddress;
	AudioDeviceID				defaultDevID;
	OSStatus					theError = noErr;
	UInt32						muted;
	Boolean						canSetVol = 1;
	Boolean						muteValue;
	Boolean						hasMute = 1;
	Boolean						canMute = 1;
	
	defaultDevID = obtainDefaultOutputDevice();
	if (defaultDevID == kAudioObjectUnknown) {			//device not found: return without trying to set
		printf("Device unknown");
		return;
	}
	
		//check if the new value is in the correct range - normalize it if not
	newValue = theVolume > 1.0 ? 1.0 : (theVolume < 0.0 ? 0.0 : theVolume);
	if (newValue != theVolume) {
		printf("Tentative volume (%5.2f) was out of range; reset to %5.2f", theVolume, newValue);
	}
	
	theAddress.mElement = kAudioObjectPropertyElementMaster;
	theAddress.mScope = kAudioDevicePropertyScopeOutput;
	
		//set the selector to mute or not by checking if under threshold and check if a mute command is available
	if ( (muteValue = (newValue < THRESHOLD)) )
	{
		theAddress.mSelector = kAudioDevicePropertyMute;
		hasMute = AudioObjectHasProperty(defaultDevID, &theAddress);
		if (hasMute)
		{
			theError = AudioObjectIsPropertySettable(defaultDevID, &theAddress, &canMute);
			if (theError != noErr || !canMute)
			{
				canMute = 0;
				printf("Should mute device 0x%0x but did not success",defaultDevID);
			}
		}
		else canMute = 0;
	}
	else
	{
		theAddress.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume;
	}
	
// **** now manage the volume following the what we found ****
	
		//be sure the device has a volume command
	if (! AudioObjectHasProperty(defaultDevID, &theAddress))
	{
		printf("The device 0x%0x does not have a volume to set", defaultDevID);
		return;
	}
	
		//be sure the device can set the volume
	theError = AudioObjectIsPropertySettable(defaultDevID, &theAddress, &canSetVol);
	if ( theError!=noErr || !canSetVol )
	{
		printf("The volume of device 0x%0x cannot be set", defaultDevID);
		return;
	}
	
		//if under the threshold then mute it, only if possible - done/exit
	if (muteValue && hasMute && canMute)
	{
		muted = 1;
		theError = AudioObjectSetPropertyData(defaultDevID, &theAddress, 0, NULL, sizeof(muted), &muted);
		if (theError != noErr)
		{
			printf("The device 0x%0x was not muted",defaultDevID);
			return;
		}
	}
	else		//else set it
	{
		theError = AudioObjectSetPropertyData(defaultDevID, &theAddress, 0, NULL, sizeof(newValue), &newValue);
		if (theError != noErr)
		{
			printf("The device 0x%0x was unable to set volume", defaultDevID);
		}
			//if device is able to handle muting, maybe it was muted, so unlock it
		if (hasMute && canMute)
		{
			theAddress.mSelector = kAudioDevicePropertyMute;
			muted = 0;
			theError = AudioObjectSetPropertyData(defaultDevID, &theAddress, 0, NULL, sizeof(muted), &muted);
		}
	}
	if (theError != noErr) {
		printf("Unable to set volume for device 0x%0x", defaultDevID);
	}
}

