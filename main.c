#include <AudioUnit/AudioUnit.h>
#include <CoreServices/CoreServices.h>
#include <unistd.h>
#include <signal.h>

AudioUnit outputUnit;

/**
 * Fill the audio unit's buffer
 */
OSStatus fillAudioUnitBuffer(void 		*inRefCon, 
			AudioUnitRenderActionFlags 	*ioActionFlags, 
			const AudioTimeStamp 		*inTimeStamp, 
			UInt32 						inBusNumber, 
			UInt32 						inNumberFrames, 
			AudioBufferList 			*ioData) {

	unsigned char buffer[inNumberFrames];
	int count = fread(buffer, 1, inNumberFrames, stdin);
	if (!count) {
		return noErr;
	}

	int channel, frame, i = 0;
	for (channel = ioData->mNumberBuffers - 1; channel >= 0; channel--) {
		for (frame = inNumberFrames - 1; frame >= 0; frame--) {
			((Float32 *)(ioData->mBuffers[channel].mData))[frame] = buffer[frame] / 256.f;
			if (++i >= count) {
				break;
			}
		}
		if (i >= count) {
			break;
		}
	}

	return noErr;
}

/**
 * @param sampleRate sample rate
 * @return 0: failed, 1: succeed
 */
int openAudioOutput(int sampleRate) {
	OSStatus status = noErr;

	ComponentDescription desc;
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_DefaultOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;

	Component component = FindNextComponent(NULL, &desc);
	if (!component) {
		fprintf(stderr, "FindNextComponent\n");
		return 0;
	}

	status = OpenAComponent(component, &outputUnit);
	if (!component) {
		fprintf(stderr, "OpenAComponent failed: %ld\n", (long int)status);
		return 0;
	}

    AURenderCallbackStruct input;
	input.inputProc = fillAudioUnitBuffer;

	status = AudioUnitSetProperty(outputUnit, 
								kAudioUnitProperty_SetRenderCallback, 
								kAudioUnitScope_Input,
								0, 
								&input, 
								sizeof(input));
	if (status) {
		fprintf(stderr, "AudioUnitSetProperty failed: %ld\n", (long int)status);
		return 0;
	}
    
	AudioStreamBasicDescription streamFormat;
	streamFormat.mSampleRate = sampleRate;
	streamFormat.mFormatID = kAudioFormatLinearPCM;
	streamFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
	streamFormat.mFramesPerPacket = 1;
	streamFormat.mBytesPerFrame = 4;
	streamFormat.mBytesPerPacket = 4;
	streamFormat.mBitsPerChannel = 32;
	streamFormat.mChannelsPerFrame = 1;

	status = AudioUnitSetProperty(outputUnit,
								kAudioUnitProperty_StreamFormat,
								kAudioUnitScope_Input,
								0,
								&streamFormat,
								sizeof(AudioStreamBasicDescription));
	if (status) {
		fprintf(stderr, "AudioUnitSetProperty failed: %ld\n", (long int)status);
		return 0;
	}

	status = AudioUnitInitialize(outputUnit);
	if (status) {
		fprintf(stderr, "AudioUnitInitialize failed: %ld\n", (long int)status);
		return 0;
	}
    
	status = AudioOutputUnitStart(outputUnit);
	if (status) {
		fprintf(stderr, "AudioOutputUnitStart failed: %ld\n", (long int)status);
		return 0;
	}

	return 1;
}

void closeAudioOutput() {
	OSStatus status = noErr;

	verify_noerr(AudioOutputUnitStop(outputUnit));

	status = AudioUnitUninitialize(outputUnit);
	if (status) {
		fprintf(stderr, "AudioUnitUninitialize failed: %ld\n", (long int)status);
		return;
	}

	CloseComponent(outputUnit);
}

void signalHandler(int signal) {
	closeAudioOutput();
	exit(0);
}

void usage() {
	fprintf(stderr, "usage: cat /dev/urandom | audiopipe -r 44100\n");
	exit(1);
}

int main(int argc, char** argv) {
	int sample_rate = 8000;

	int opt;
 	extern char	*optarg;
	while ((opt = getopt(argc, argv, "r:")) != -1) {
		switch (opt) {
			case 'r':
				sample_rate = atoi(optarg);
				break;
			default:
				usage();
		}
	}

	freopen(NULL, "rb", stdin);

	if (signal(SIGTERM, signalHandler) == SIG_ERR) {
		exit(1);
	}

	if (!openAudioOutput(sample_rate)) {
		exit(1);
	}

	while (1);
}
