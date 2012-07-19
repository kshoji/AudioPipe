#include <AudioUnit/AudioUnit.h>
#include <CoreServices/CoreServices.h>
#include <unistd.h>
#include <signal.h>

AudioUnit outputUnit;
Float32 volume;
long bytesPlayed;
int verbose;
int output;

/**
 * Fill the audio unit's buffer
 */
OSStatus fillAudioUnitBuffer(void 		*inRefCon, 
			AudioUnitRenderActionFlags 	*ioActionFlags, 
			const AudioTimeStamp 		*inTimeStamp, 
			UInt32 						inBusNumber, 
			UInt32 						inNumberFrames, 
			AudioBufferList 			*ioData) {
	int i;
	unsigned char buffer[inNumberFrames];
	int count = fread(buffer, 1, inNumberFrames, stdin);
	bytesPlayed += count;

	if (output) {
		for (i = 0; i < count; i++) {
			putchar(buffer[i]);
		}
	}
	if (count < inNumberFrames) {
		for (i = count; i < inNumberFrames; i++) {
			buffer[i] = 0x80;
		}
	}

	int channel, frame;
	i = 0;
	for (channel = ioData->mNumberBuffers - 1; channel >= 0; channel--) {
		for (frame = inNumberFrames - 1; frame >= 0; frame--) {
			((Float32 *)(ioData->mBuffers[channel].mData))[frame] = buffer[frame] / 256.f * volume;
			if (++i >= inNumberFrames) {
				break;
			}
		}
		if (i >= inNumberFrames) {
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
	switch (signal) {
		case SIGINT:
			if (verbose) {
				fprintf(stderr, "%ld bytes played.\n", bytesPlayed);
			}
		case SIGTERM:
			closeAudioOutput();
			exit(0);
	}
}

void usage() {
	fprintf(stderr, "Usage: cat /dev/urandom | audiopipe -r 44100 -g 0.5\n");
	fprintf(stderr, "  -r 8000\t\tspecify sample rate(in Hz), default: 8000\n");
	fprintf(stderr, "  -g 1.0\t\tspecify volume(0.0 to 1.0), default: 1.0\n");
	fprintf(stderr, "  -o    \t\toutput played buffer to stdout, default: unset\n");
	fprintf(stderr, "  -v    \t\tdisplays played file size when the process finished by ctrl-c, default: unset\n");
	exit(1);
}

int main(int argc, char** argv) {
	int sample_rate = 8000;
	volume = 1.f;
	bytesPlayed = 0;
	verbose = 0;
	output = 0;

	if (isatty(fileno(stdin))) {
		// run from command line, not pipe
		usage();
	}

	int opt;
 	extern char	*optarg;
	while ((opt = getopt(argc, argv, "g:r:vo")) != -1) {
		switch (opt) {
			case 'g':
				volume = atof(optarg);
				if (volume < 0.f) {
					volume = 0.f;
				}
				if (volume > 1.f) {
					volume = 1.f;
				}
				break;
			case 'r':
				sample_rate = atoi(optarg);
				break;
			case 'o':
				output = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			default:
				usage();
		}
	}

	freopen(NULL, "rb", stdin);

	if (signal(SIGINT, signalHandler) == SIG_ERR) {
		exit(1);
	}
	if (signal(SIGTERM, signalHandler) == SIG_ERR) {
		exit(1);
	}

	if (!openAudioOutput(sample_rate)) {
		exit(1);
	}

	while (1);
}
