// TestBed.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "math.h"
#include <Windows.h>
#include "NALU.h"

#include "Origin_h264_encoder.h"

//#define READ_FROM_BUFFER

typedef struct 
{
    char filename[256];
    int startFrame;
    int endFrame;
} InputArgs;

bool ProcessCommandLineArgs( int argc, _TCHAR* argv[], InputArgs &inputArgs )
{
    if( argc > 1 )
    {
        if( argc >= 2 )
        {
            sprintf_s(inputArgs.filename, "%S", argv[1]);
        }

        char frame[8];
        if( argc >= 3 )
        {
            sprintf_s(frame, "%S", argv[2]);
            inputArgs.startFrame = atoi(frame);
            inputArgs.endFrame = inputArgs.startFrame;
        }

        if( argc >= 4 )
        {
            sprintf_s(frame, "%S", argv[3]);
            inputArgs.endFrame = atoi(frame);
        }

        return true;
    }

    return false;
}

void ReadFrameDataFromFile(FILE *f, int &startFrame, int &endFrame, int &width, int &height, int &frame_count, uint8_t ***y_data, uint8_t ***uv_data )
{
    fread(&width, sizeof(int), 1, f);
    fread(&height, sizeof(int), 1, f);
    fread(&frame_count, sizeof(int), 1, f);
    int format;
    fread(&format, sizeof(int), 1, f);

    if( startFrame >= 0 && endFrame >= 0 && startFrame < frame_count && endFrame < frame_count )
    {
        if( endFrame < startFrame )
            endFrame = startFrame;

        int numFrames = endFrame - startFrame + 1;
        int y_length_per_frame = width * height;
        int uv_length_per_frame = width * height / 2;

#ifndef READ_FROM_BUFFER
        numFrames = 1;
#endif
        // allocate memory
        int i;
        *y_data = (uint8_t**) malloc( numFrames * sizeof(uint8_t*) );
        for( i = 0; i < numFrames; ++i )
        {
            (*y_data)[i] = (uint8_t *) malloc( y_length_per_frame * sizeof(uint8_t) );
        }

        *uv_data = (uint8_t**) malloc( numFrames * sizeof(uint8_t*) );
        for( i = 0; i < numFrames; ++i )
        {
            (*uv_data)[i] = (uint8_t *) malloc( uv_length_per_frame * sizeof(uint8_t) );
        }

        // skip to the startFrame
        int startFramePos = startFrame * (y_length_per_frame + uv_length_per_frame);
        fseek(f, startFramePos, SEEK_CUR);
        for( i = 0; i < numFrames; ++i )
        {
            // y
            fread((*y_data)[i], sizeof(uint8_t), y_length_per_frame, f);

            // uv
            fread((*uv_data)[i], sizeof(uint8_t), uv_length_per_frame, f);
        }
    }
}

void CleanFrameData(uint8_t **y_data, uint8_t **uv_data, int numFrames)
{
    if( y_data && uv_data )
    {
        for( int i = 0; i < numFrames; ++i )
        {
            free(y_data[i]);
            free(uv_data[i]);
        }
        free(y_data);
        free(uv_data);
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
	int nal_count;
	NAL_Type *nal_out;
	Origin_H264_EncoderInputType enc_input;

	enc_input.frame_type = ORIGIN_H264_IDR;
	enc_input.qp = 28;

    int width = 320;//160;
    int height = 224;//128;

#ifdef READ_FRAMES_FROM_FILE
    int frame_count = 45;
    uint8_t **y_data = 0;
    uint8_t **uv_data = 0; // interleaved

    InputArgs inputArgs;
    FILE *frameDataInputFile = NULL;
    if( ProcessCommandLineArgs( argc, argv, inputArgs ) )
    {
        fopen_s(&frameDataInputFile, inputArgs.filename, "rb");
        if( frameDataInputFile )
        {
//			frame_count = inputArgs.endFrame - inputArgs.startFrame + 1;
            ReadFrameDataFromFile(frameDataInputFile, inputArgs.startFrame, inputArgs.endFrame, width, height, frame_count, &y_data, &uv_data );
            frame_count = inputArgs.endFrame - inputArgs.startFrame + 1;
        }
    }
#endif

	OriginEncoder_InputParamsType params;
	Origin_H264_EncoderOutputType output;

	params.width = width;
	params.height = height;
	params.target_frame_rate = 30;	// in frames per second
	params.target_bitrate = 3100;	// in kbps
	params.max_bitrate = 3500;	// in kbps
	params.filtering_on = 1;	// deblocking filter
	params.subpixel_motion_est = 1;
	params.variable_qp = 1;
	params.b_slice_on = 2;
	params.idr_frame_interval = 60;	// frames between idr frames
	params.rate_control = 1;	// 0 - off, 1 - on
	params.max_qp_delta = 8;

	Origin_H264_Encoder_Init(&params);

#ifdef READ_FRAMES_FROM_FILE
	enc_input.frame_type = ORIGIN_H264_DEFAULT_TYPE;
	uint32_t ts = 0;
    enc_input.timestamp = 0;

	for (int i = 0; i < frame_count; i += 1)
	{
		if (i == 0)
			enc_input.qp = 31;
		else
			enc_input.qp = 32;
		if ((i % 60) == 0)
			enc_input.frame_type = ORIGIN_H264_IDR;
		else
			enc_input.frame_type = ORIGIN_H264_DEFAULT_TYPE;

#ifdef READ_FROM_BUFFER // read from buffer
		int index = i;
		enc_input.y_data = (uint8_t *) y_data[index];
		enc_input.u_data = (uint8_t *) uv_data[index];
#else
        if (frame_count > 0)
        {
            int y_length_per_frame = width * height;
            int uv_length_per_frame = width * height / 2;
            fread(y_data[0], sizeof(uint8_t), y_length_per_frame, frameDataInputFile);

            // uv
            fread(uv_data[0], sizeof(uint8_t), uv_length_per_frame, frameDataInputFile);
        }
        enc_input.y_data = (uint8_t *) y_data[0];
        enc_input.u_data = (uint8_t *) uv_data[0];
#endif
		enc_input.v_data = NULL;
		enc_input.timestamp += 1000 / params.target_frame_rate;
//		ts += 1000 / params.target_frame_rate;
		Origin_H264_Encoder_Encode(&enc_input, &output);
	}
#else

#endif

	while (Origin_H264_Encoder_Encode(NULL, &output))
		Sleep(10);	// while there are still buffered frames

	Origin_H264_Encoder_Close();

#ifdef READ_FRAMES_FROM_FILE
#ifdef READ_FROM_BUFFER
    int numFrames = inputArgs.endFrame - inputArgs.startFrame;
#else
    int numFrames = 1;

#endif
    CleanFrameData(y_data, uv_data, numFrames);

    fclose(frameDataInputFile);

#endif

	return 0;
}

