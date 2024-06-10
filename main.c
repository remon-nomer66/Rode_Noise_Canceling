#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <portaudio.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 1024
#define NUM_CHANNELS 1
#define DURATION 10 // 録音時間（秒）

typedef struct
{
    float *recordedSamples;
    int frameIndex;
    int maxFrameIndex;
    float *averageNoise;
}
paTestData;

// エラーチェック用のマクロ
#define CHECK_ERROR(err) \
    if (err != paNoError) \
    { \
        fprintf(stderr, "PortAudio error: %s\n", Pa_GetErrorText(err)); \
        Pa_Terminate(); \
        return -1; \
    }

static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData)
{
    paTestData *data = (paTestData*)userData;
    const float *in = (const float*)inputBuffer;
    float *out = (float*)outputBuffer;
    unsigned long i;

    (void) outputBuffer; // 未使用

    if (inputBuffer == NULL)
    {
        for (i = 0; i < framesPerBuffer; i++)
        {
            data->recordedSamples[data->frameIndex++] = 0;
        }
    }
    else
    {
        for (i = 0; i < framesPerBuffer; i++)
        {
            data->recordedSamples[data->frameIndex++] = *in++;
        }
    }

    if (data->frameIndex >= data->maxFrameIndex)
    {
        return paComplete;
    }

    return paContinue;
}

static int playbackCallback(const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData)
{
    paTestData *data = (paTestData*)userData;
    float *out = (float*)outputBuffer;
    unsigned long i;

    (void) inputBuffer; // 未使用

    for (i = 0; i < framesPerBuffer; i++)
    {
        if (data->frameIndex < data->maxFrameIndex)
        {
            // ノイズキャンセリング処理（平均的な逆位相を適用）
            *out++ = data->recordedSamples[data->frameIndex++] - data->averageNoise[i % FRAMES_PER_BUFFER];
        }
        else
        {
            *out++ = 0;
        }
    }

    if (data->frameIndex >= data->maxFrameIndex)
    {
        return paComplete;
    }

    return paContinue;
}

void calculateAverageNoise(paTestData *data, int totalFrames, int bufferSize)
{
    int i, j;
    float sum;

    for (i = 0; i < bufferSize; i++)
    {
        sum = 0;
        for (j = 0; j < totalFrames / bufferSize; j++)
        {
            sum += data->recordedSamples[i + j * bufferSize];
        }
        data->averageNoise[i] = sum / (totalFrames / bufferSize);
    }
}

int main(void)
{
    PaStreamParameters inputParameters, outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;
    int totalFrames;
    int numSamples;
    int numBytes;
    float max, val;
    int i;

    printf("PortAudio Test: input and output.\n");

    data.maxFrameIndex = totalFrames = DURATION * SAMPLE_RATE;
    data.frameIndex = 0;
    numSamples = totalFrames * NUM_CHANNELS;
    numBytes = numSamples * sizeof(float);
    data.recordedSamples = (float *)malloc(numBytes);
    data.averageNoise = (float *)malloc(FRAMES_PER_BUFFER * sizeof(float));

    if (data.recordedSamples == NULL || data.averageNoise == NULL)
    {
        printf("Could not allocate record array.\n");
        return -1;
    }
    for (i = 0; i < numSamples; i++)
    {
        data.recordedSamples[i] = 0;
    }

    err = Pa_Initialize();
    CHECK_ERROR(err);

    inputParameters.device = Pa_GetDefaultInputDevice();
    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount = NUM_CHANNELS;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&stream,
                        &inputParameters,
                        NULL, // outputは最初はNULL
                        SAMPLE_RATE,
                        FRAMES_PER_BUFFER,
                        paClipOff,
                        recordCallback,
                        &data);
    CHECK_ERROR(err);

    err = Pa_StartStream(stream);
    CHECK_ERROR(err);

    printf("Now recording!! Please speak into the microphone.\n");
    while (Pa_IsStreamActive(stream) == 1)
    {
        Pa_Sleep(1000);
        printf("index = %d\n", data.frameIndex);
    }

    err = Pa_CloseStream(stream);
    CHECK_ERROR(err);

    // ノイズの平均値を計算
    calculateAverageNoise(&data, totalFrames, FRAMES_PER_BUFFER);

    // 再生用のストリームを開く
    data.frameIndex = 0;
    err = Pa_OpenStream(&stream,
                        NULL, // inputはNULL
                        &outputParameters,
                        SAMPLE_RATE,
                        FRAMES_PER_BUFFER,
                        paClipOff,
                        playbackCallback,
                        &data);
    CHECK_ERROR(err);

    err = Pa_StartStream(stream);
    CHECK_ERROR(err);

    printf("Now playing back.\n");
    while (Pa_IsStreamActive(stream) == 1)
    {
        Pa_Sleep(1000);
        printf("index = %d\n", data.frameIndex);
    }

    err = Pa_CloseStream(stream);
    CHECK_ERROR(err);

    free(data.recordedSamples);
    free(data.averageNoise);
    Pa_Terminate();

    printf("Done.\n");
    return 0;
}