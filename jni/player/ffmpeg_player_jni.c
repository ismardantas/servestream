#include <jni.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <android/log.h>

#define AUDIO_DATA_ID 1

//audio
int gAudioStreamIdx;
jbyteArray gAudioFrameRef; //reference to a java variable
char* gAudioFrameRefBuffer;
int gAudioFrameRefBufferMaxSize;
jintArray gAudioFrameDataLengthRef; //reference to a java variable
int* gAudioFrameDataLengthRefBuffer;

typedef int bool;
#define true 1
#define false 0

static int m_sampleRateInHz = 0;
static int m_channelConfig = 0;

static AVFormatContext *pFormatCtx;
static AVCodecContext *pCodecCtx;
static AVCodec *dec;

//static JavaVM * m_vm;

jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    __android_log_print(ANDROID_LOG_INFO, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "JNI_OnLoad()");

    // TODO:  new code
	//m_vm = vm;

	pFormatCtx = NULL;

    //audio
    pCodecCtx = NULL;
    gAudioStreamIdx = -1;
    gAudioFrameRef = NULL;
    gAudioFrameRefBuffer = NULL;
    gAudioFrameRefBufferMaxSize = 0;
    gAudioFrameDataLengthRef = NULL;
    gAudioFrameDataLengthRefBuffer = NULL;

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
Java_net_sourceforge_servestream_player_FFmpegPlayer_native_1init(JNIEnv * env, jclass obj) {
    __android_log_write(ANDROID_LOG_INFO, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "native_init()");

    // Initialize libavformat and register all the muxers, demuxers and protocols.
    avcodec_register_all();
    av_register_all();
}

JNIEXPORT jint JNICALL
Java_net_sourceforge_servestream_player_FFmpegPlayer_nativeSetDataSource(JNIEnv* env, jobject obj, jstring path) {
    __android_log_print(ANDROID_LOG_INFO, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "nativeOpenFromFile()");

    const char * uri;

    // URI is null
    if (path == NULL) {
    	return -1;
    }

    uri = (*env)->GetStringUTFChars(env, path, NULL);
    //(*env)->ReleaseStringUTFChars(env, mediafile, mfileName); //always release the java string reference
    __android_log_print(ANDROID_LOG_INFO, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "opening %s", uri);

    if (avformat_open_input(&pFormatCtx, uri, NULL, NULL) != 0) {
        __android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "avformat_open_input() failed");
    	return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        __android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "avformat_find_stream_info() failed");
    	return -1;
    }

    // Dump information about file onto standard error
    //dump_format(pFormatCtx, 0, uri, 0);

    __android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "nativeOpenFromFile() succeeded");
    return 0;
}

JNIEXPORT jint JNICALL
Java_net_sourceforge_servestream_player_FFmpegPlayer_nativeOpenAudio(JNIEnv* env, jobject obj, jbyteArray audioframe, jintArray audioframelength) {
    __android_log_print(ANDROID_LOG_INFO, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "nativeOpenAudio()");

	int i = 0;
	int audioStreamIndex = -1;

	if (gAudioFrameRef) {
		__android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "call nativeCloseAudio() before calling this function");
	    return -1;
	}

	if ((*env)->IsSameObject(env, audioframe, NULL)) {
		__android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "invalid arguments");
	    return -1;
	}

	//audio frame buffer
	gAudioFrameRef = (*env)->NewGlobalRef(env, audioframe); //lock the array preventing the garbage collector from destructing it
	if (gAudioFrameRef == NULL) {
		__android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "NewGlobalRef() for audioframe failed");
	    return -1;
	}

	jboolean test;
	gAudioFrameRefBuffer = (*env)->GetByteArrayElements(env, gAudioFrameRef, &test);
	if (gAudioFrameRefBuffer == 0 || test == JNI_TRUE) {
		__android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "failed to get audio frame reference or reference copied");
	    return -1;
	}

	gAudioFrameRefBufferMaxSize = (*env)->GetArrayLength(env, gAudioFrameRef);
	if (gAudioFrameRefBufferMaxSize < AVCODEC_MAX_AUDIO_FRAME_SIZE) {
		__android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "failed to read or incorrect buffer length: %d", gAudioFrameRefBufferMaxSize);
	    return -1;
	}

	__android_log_print(ANDROID_LOG_INFO, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "buffer length: %d", gAudioFrameRefBufferMaxSize);

	//audio frame data size
	gAudioFrameDataLengthRef = (*env)->NewGlobalRef(env, audioframelength); //lock the variable preventing the garbage collector from destructing it
	if (gAudioFrameDataLengthRef == NULL) {
		__android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "NewGlobalRef() for audioframelength failed");
	    return -1;
	}

	gAudioFrameDataLengthRefBuffer = (*env)->GetIntArrayElements(env, gAudioFrameDataLengthRef, &test);
	if (gAudioFrameDataLengthRefBuffer == 0 || test == JNI_TRUE) {
		__android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "failed to get audio data length reference or reference copied");
	    return -1;
	}

	int audioDataLength = (*env)->GetArrayLength(env, gAudioFrameDataLengthRef);
	if (audioDataLength != 1) {
		__android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "failed to read or incorrect size of the audio data length reference: %d", audioDataLength);
	    return -1;
	}

	__android_log_print(ANDROID_LOG_INFO, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "size of the audio data length reference: %d", audioDataLength);

    // Find the first audio stream
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
    	if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
    		audioStreamIndex = i;
    		break;
    	}
    }

    if (audioStreamIndex == -1) {
        __android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "audio stream not found");
    	return -1; // Didn't find an audio stream
    }

    // Get a pointer to the codec context for the audio stream
    pCodecCtx = pFormatCtx->streams[audioStreamIndex]->codec;

    // Find the decoder for the audio stream
    dec = avcodec_find_decoder(pCodecCtx->codec_id);

    if(dec == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "avcodec_find_decoder() failed to find audio decoder");
    	return -1;
    }

    // Open the codec
    if (avcodec_open2(pCodecCtx, dec, NULL) < 0) {
    	__android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "avcodec_open2() failed");
    	return -1;
    }

    // set the sample rate and channels since these
    // will be required when creating the AudioTrack object
    m_sampleRateInHz = pCodecCtx->sample_rate;
    m_channelConfig = pCodecCtx->channels;

    //all good, set index so that nativeProcess() can now recognise the audio stream
    gAudioStreamIdx = audioStreamIndex;

    return 0;
}

JNIEXPORT void JNICALL
Java_net_sourceforge_servestream_player_FFmpegPlayer_nativeCloseAudio(JNIEnv* env, jobject obj) {
    __android_log_print(ANDROID_LOG_INFO, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "nativeCloseAudio()");

    if (pCodecCtx) {
        avcodec_close(pCodecCtx);
        pCodecCtx = NULL;
    }

    if (gAudioFrameRef) {
        if (gAudioFrameRefBuffer) {
            (*env)->ReleaseByteArrayElements(env, gAudioFrameRef, gAudioFrameRefBuffer, 0);
            gAudioFrameRefBuffer = NULL;
        }

        (*env)->DeleteGlobalRef(env, gAudioFrameRef);
        gAudioFrameRef = NULL;
    }

    gAudioFrameRefBufferMaxSize = 0;

    if (gAudioFrameDataLengthRef) {
        if (gAudioFrameDataLengthRefBuffer) {
            (*env)->ReleaseIntArrayElements(env, gAudioFrameDataLengthRef, gAudioFrameDataLengthRefBuffer, 0);
            gAudioFrameDataLengthRefBuffer = NULL;
        }

        (*env)->DeleteGlobalRef(env, gAudioFrameDataLengthRef);
        gAudioFrameDataLengthRef = NULL;
    }

    gAudioStreamIdx = -1;
}

JNIEXPORT void JNICALL
Java_net_sourceforge_servestream_player_FFmpegPlayer_nativeClose(JNIEnv* env, jobject obj) {
    __android_log_print(ANDROID_LOG_INFO, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "nativeClose()");

    Java_net_sourceforge_servestream_player_FFmpegPlayer_nativeCloseAudio(env, obj);

    if (pFormatCtx) {
    	avformat_close_input(&pFormatCtx);
    	pFormatCtx = NULL;
    }
}

int decodeFrameFromPacket(AVPacket* aPacket) {
    if (aPacket->stream_index == gAudioStreamIdx) {
        int dataLength = gAudioFrameRefBufferMaxSize;
        if (avcodec_decode_audio3(pCodecCtx, (int16_t*)gAudioFrameRefBuffer, &dataLength, aPacket) <= 0) {
            __android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "avcodec_decode_audio4() decoded no frame");
            gAudioFrameDataLengthRefBuffer[0] = 0;
            return -2;
        }

        gAudioFrameDataLengthRefBuffer[0] = dataLength;
        return AUDIO_DATA_ID;
    }

    return 0;
}

JNIEXPORT jint JNICALL
Java_net_sourceforge_servestream_player_FFmpegPlayer_nativeDecodeFrameFromFile(JNIEnv* env, jobject obj) {
    AVPacket packet;
    memset(&packet, 0, sizeof(packet)); //make sure we can safely free it

    int i;
    for (i = 0; i < pFormatCtx->nb_streams; ++i) {
        //av_init_packet(&packet);
        if (av_read_frame(pFormatCtx, &packet) != 0) {
            __android_log_print(ANDROID_LOG_ERROR, "Java_net_sourceforge_servestream_player_FFmpegPlayer", "av_read_frame() failed");
            return -1;
        }

        int ret = decodeFrameFromPacket(&packet);
        av_free_packet(&packet);

        if (ret != 0) { //an error or a frame decoded
        	return ret;
        }
    }

    return 0;
}

