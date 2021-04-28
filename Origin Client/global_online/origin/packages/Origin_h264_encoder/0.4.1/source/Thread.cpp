#include <windows.h>
#include "Origin_h264_encoder.h"
#include "Cabac_H264.h"
#include "LoopFilter.h"
#include "Frame.h"
#include "MotionEstimation.h"
#include "Thread.h"
#include "RateControl.h"
#include "Trace.h"
#include "Timing.h"
#include <intrin.h>	// for __cpuid()

#define MAX_THREADS 64
#define MAXROWS (1200 / 16)

//#define TRACK_WAIT

#if 0
#define THREAD_TRACE(...) { \
	EA::Thread::AutoFutex m(gThreadPrintMutex);\
	char msg[4096]; \
	sprintf_s(msg,4096, __VA_ARGS__); \
	Log(msg); \
}
#else
#define THREAD_TRACE(...)
#endif


enum
{
    PROCESS_MB_THREAD = 0,
    ENCODE_MB_THREAD,
	INLOOP_FILTER_THREAD,
	SUBPIXEL_THREAD,
	CONVERSION_THREAD
};

enum
{
	MB_TYPE = 0,
	CONVERSION_TYPE
};

typedef struct  
{
	bool claimed;
	bool completed;
	int progress;
} MacroBlockRowStatusType;

typedef struct  
{
	bool active;
	bool exit_flag;
	int type;
	int index;
	int sub_index;	// used for indicating which sub-pixel function to call
	int mb_interval;	// number of MBs to process before checking for a semaphore/triggering one
	int trigger_cutoff;	// special trigger for mb processing thread that will stop early to be replaced by encoding thread on delay)
	YUVFrameType *dst_frame;
	uint8_t *src_444_frame;
	int start_height, end_height;	// for conversion
	HANDLE signal;
	HANDLE interval_signal;	// to start partner MB thread going on next available interval
	HANDLE trigger_signal;	// to start another thread up (i.e. encoder thread from mb processing threads)
	HANDLE in_loop_signal;	// to start another thread up (i.e. in-loop filtering threads from mb processing threads)
	HANDLE completion_signal;
} ThreadManagementType;

// main frame processing thread
static HANDLE sMainProcessingThread = NULL;
static HANDLE sMainProcessingSignal;
static EA::Thread::Futex gThreadMainProcessingMutex;
static bool sMainProcessingBusy = false;
static bool sMainProcessingThreadKillFlag = false;
static bool sMainProcessingWaitingForBGroup = false;
static bool sMainProcessingEmptyInputFrame = false;

// multi-threading optimization
static HANDLE gThreadHandles[MAX_THREADS] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static ThreadManagementType sThreadInfo[MAX_THREADS];
static MacroBlockRowStatusType sRowMBProcessed[MAXROWS];
static MacroBlockRowStatusType sFilterRowMBProcessed[MAXROWS];
static EA::Thread::Futex gThreadManageMutex;
static EA::Thread::Futex gThreadPrintMutex;
static EA::Thread::Futex gThreadTimingMutex;
static int sNumOfCores = 1;
static int sNumMBThreads = 0;
static int sNumBaseThreads = 0;
static int sConversionThreadsStartIndex = 0;	// first index in sThreadInfo[] array for conversion threads
static HANDLE sSubPixelTriggers[2] = { NULL, NULL };	// use to trigger sub-pixel threads on completion of MB processing

void AddThread(int thread_type, ThreadManagementType *thread_info);
static DWORD WINAPI MainProcessing_Async(LPVOID *lpParam);

void Get_CPU_ID(unsigned index, unsigned regs[4])
{
	__cpuid((int *)regs, (int) index);
}


int DetectNumOfCores()
{
	uint64_t process_affmask = 0, system_affmask = 0;
	GetProcessAffinityMask(GetCurrentProcess(), (PDWORD_PTR) &process_affmask, (PDWORD_PTR) &system_affmask);
	sNumOfCores = 0;
	for (int i = 0; i < 64; i++)
	{
		if (process_affmask & ((uint64_t) 1L << i))
		{
			sNumOfCores++;
		}
	}

#if 0	// not used
	uint32_t regs[4];
	// Get vendor
	char vendor[12];
	Get_CPU_ID(0, regs);
	((unsigned *)vendor)[0] = regs[1]; // EBX
	((unsigned *)vendor)[1] = regs[3]; // EDX
	((unsigned *)vendor)[2] = regs[2]; // ECX
	Get_CPU_ID(1, regs);

	if (strncmp(vendor, "GenuineIntel", 12) == 0)	// Intel
	{
		Get_CPU_ID(4, regs);
		sNumOfCores = ((regs[0] >> 26) & 0x3f) + 1; // EAX[31:26] + 1
	}
	else
		if (strncmp(vendor, "AuthenticAMD", 12) == 0)	// AMD
		{
			Get_CPU_ID(0x80000008, regs);
			sNumOfCores = ((unsigned)(regs[2] & 0xff)) + 1; // ECX[7:0] + 1
		}
#endif

	return sNumOfCores;
}

#define MAX_FRAMES_QUEUED 16
#define MAX_FRAMES_PROCESSING	4

typedef struct
{
	Origin_H264_EncoderInputType input;
	int frames_after_dropped;		// for accounting in rate control
	int b_slice_index;				// if a b-slice, what index is this (e.g. pbbp if first b then 0, if second then 1.
	int b_slice_group_size;			// how many b-slices + ref frame are there (e.g. bbp = 3, bbbp = 4)
} InputFrameQueueType;

static int sProcessFrameQueueSize = 0;
static InputFrameQueueType sProcessFrameQueue[MAX_FRAMES_QUEUED];
static int sFrameQueueBufferIndex = 0;
static uint8_t *sFrameQueueBuffers[MAX_FRAMES_QUEUED+MAX_FRAMES_PROCESSING];
static HANDLE sFrameQueueSignal;

void InitInputFrameQueue()
{
	sProcessFrameQueueSize = 0;
	sMainProcessingBusy = false;
	sMainProcessingThreadKillFlag = false;

	sFrameQueueBufferIndex = 0;
	for (int i = 0; i < (MAX_FRAMES_QUEUED+MAX_FRAMES_PROCESSING); i++ )
	{
		sFrameQueueBuffers[i] = (uint8_t *) _aligned_malloc ((gEncoderState.width_padded * gEncoderState.height_padded * 3 / 2), 32);
		if (sFrameQueueBuffers[i] == NULL)
		{
			// need error condition
			ENC_ASSERT(0);
		}
	}
}

void ExitInputFrameQueue()
{
	for (int i = 0; i < (MAX_FRAMES_QUEUED+MAX_FRAMES_PROCESSING); i++ )
	{
		if (sFrameQueueBuffers[i])
		{
			_aligned_free(sFrameQueueBuffers[i]);
		}
	}
}

bool AddInputFrameToQueue(Origin_H264_EncoderInputType *input)
{
	EA::Thread::AutoFutex m(gThreadMainProcessingMutex);

	if (sProcessFrameQueueSize >= MAX_FRAMES_QUEUED)
	{
//		ENC_ASSERT(0);
		return false;
	}

	// temp
	YUVFrameType tmp;

	tmp.width = gEncoderState.width;
	tmp.height = gEncoderState.height;

	sProcessFrameQueue[sProcessFrameQueueSize].input.y_data = (uint8_t *) sFrameQueueBuffers[sFrameQueueBufferIndex];
	sProcessFrameQueue[sProcessFrameQueueSize].input.u_data = (uint8_t *) (sFrameQueueBuffers[sFrameQueueBufferIndex] + (gEncoderState.width * gEncoderState.height));
	sFrameQueueBufferIndex = (sFrameQueueBufferIndex + 1) % (MAX_FRAMES_QUEUED+MAX_FRAMES_PROCESSING);

	tmp.Y_frame_buffer  = sProcessFrameQueue[sProcessFrameQueueSize].input.y_data ;
	tmp.UV_frame_buffer = (ChromaPixelType *) sProcessFrameQueue[sProcessFrameQueueSize].input.u_data;

	ENC_START_TIMER(20, "Convert Input Frame");
	if (input->v_data)
	{
		memcpy(tmp.Y_frame_buffer, input->y_data, tmp.width * tmp.height);
		for (int i = 0; i < (tmp.width * tmp.height / 4); i++)
		{
			tmp.UV_frame_buffer[i].u = input->u_data[i];
			tmp.UV_frame_buffer[i].v = input->v_data[i];
		}
	}
	else
	if (input->u_data)
	{
		memcpy(tmp.Y_frame_buffer, input->y_data, tmp.width * tmp.height);
		memcpy(tmp.UV_frame_buffer, input->u_data, tmp.width * tmp.height / 2);
	}
	else
	{
		Convert444to420(&tmp, input->y_data, 0, tmp.height);
	}
	ENC_PAUSE_TIMER(20, false);

//	ENC_TRACE("AddInputFrameToQueue:  Adding Frame %d %d\n", input->frame_type, input->timestamp);

	sProcessFrameQueue[sProcessFrameQueueSize].input.frame_type = input->frame_type;
	sProcessFrameQueue[sProcessFrameQueueSize].input.qp = input->qp;
	sProcessFrameQueue[sProcessFrameQueueSize].input.timestamp = input->timestamp;

	sProcessFrameQueue[sProcessFrameQueueSize].frames_after_dropped = 0;
	sProcessFrameQueue[sProcessFrameQueueSize].b_slice_index = -1;
	sProcessFrameQueue[sProcessFrameQueueSize].b_slice_group_size = 0;
	
	sProcessFrameQueueSize++;

	gEncoderState.frames_in_queue = sProcessFrameQueueSize;

	if (!sMainProcessingBusy)
		SetEvent(sMainProcessingSignal);	// wake up

	return true;
}

bool PopFromQueue(Origin_H264_EncoderInputType *input, int index)
{
	EA::Thread::AutoFutex m(gThreadMainProcessingMutex);

	if ((sProcessFrameQueueSize == 0) || (index >= sProcessFrameQueueSize))
	{
		return false;
	}

	*input = sProcessFrameQueue[index].input;

	// account for dropped frames here before the encoded frame because if this is the last P frame in the interval, then there is no
	// gain for the bits not used.
	RateControl_DropFrames(sProcessFrameQueue[index].frames_after_dropped);	

	sProcessFrameQueueSize--;
	for (int i = index; i < sProcessFrameQueueSize; i++)
	{
		sProcessFrameQueue[i] = sProcessFrameQueue[i+1];
	}

	sMainProcessingBusy = true;

	gEncoderState.frames_in_queue = sProcessFrameQueueSize;

	if (sProcessFrameQueueSize == (MAX_FRAMES_QUEUED - 1))
		SetEvent(sFrameQueueSignal);	// signal that we have free a spot

	return true;
}

bool GetFrameFromQueue(int index, InputFrameQueueType *qframe)
{
	EA::Thread::AutoFutex m(gThreadMainProcessingMutex);

	ENC_ASSERT(index < sProcessFrameQueueSize);
	if (index >= sProcessFrameQueueSize)
		return false;

	*qframe = sProcessFrameQueue[index];	// queue is protected by mutex so only send copy
	return (true);
}

void Set_BSlice_FrameInQueue(int index, int b_index, int b_group_size)
{
	EA::Thread::AutoFutex m(gThreadMainProcessingMutex);

	ENC_ASSERT(index < sProcessFrameQueueSize);
	if (index >= sProcessFrameQueueSize)
		return;

	sProcessFrameQueue[sProcessFrameQueueSize].b_slice_index = b_index;
	sProcessFrameQueue[sProcessFrameQueueSize].b_slice_group_size = b_group_size;
}

bool WaitForSpaceInFrameQueue(int ms_wait)
{
	DWORD wait_result = WaitForSingleObject(sFrameQueueSignal, ms_wait);

	ResetEvent(sFrameQueueSignal);

	return (wait_result == WAIT_OBJECT_0);	// returns TRUE if signal was triggered
}

void DropFrame()
{
	EA::Thread::AutoFutex m(gThreadMainProcessingMutex);

	if (sProcessFrameQueueSize == MAX_FRAMES_QUEUED)
	{
		sProcessFrameQueue[sProcessFrameQueueSize - 1].frames_after_dropped++;
	}
}

int InputFramesInQueue()
{
	EA::Thread::AutoFutex m(gThreadMainProcessingMutex);
	int frames_being_processed = sProcessFrameQueueSize;
	if (sMainProcessingBusy)	// currently working on one?
		frames_being_processed++;	// add it

	return (frames_being_processed);
}

void EmptyInputFrame()
{
	EA::Thread::AutoFutex m(gThreadMainProcessingMutex);

	if (sMainProcessingWaitingForBGroup && !sMainProcessingEmptyInputFrame)
	{
		sMainProcessingEmptyInputFrame = true;
		SetEvent(sMainProcessingSignal); // release
	}
}
void EncodeFrame(Origin_H264_EncoderInputType *input, int b_group_index = 0);

static DWORD WINAPI MainProcessing_Async(LPVOID *lpParam)
{
	Origin_H264_EncoderInputType input;

	for (;;)
	{
		if (sMainProcessingThreadKillFlag)
			return 0;

		if (gEncoderState.b_slice_on)
		{	// for b-slice processing, we must gather gEncoderState.b_slice_frames frames
			// assumption: if we stop getting input frames and the # of frames is less than b_slice_frames, once the NALU queue runs out, the manager will kill the stream
			//             and call Origin_H264_Encoder_Close() which will set the sMainProcessingThreadKillFlag and exit from here.
			int frames_to_process = 0;
			int frames_in_queue = InputFramesInQueue();
			InputFrameQueueType frame;
			// first test if next frame is IDR 
			
			if (frames_in_queue && GetFrameFromQueue(0, &frame) && (frame.input.frame_type == ORIGIN_H264_IDR))
			{
				frames_to_process = 1;	// just process the IDR

//				ENC_TRACE("MainProcessing_Async: IDR only\n");
			}
			else
			{
				if (frames_in_queue >= (gEncoderState.b_slice_frames + 1))
				{
					frames_to_process = (gEncoderState.b_slice_frames + 1);
//					ENC_TRACE("MainProcessing_Async: frames_to_process = %d\n", frames_to_process);
				}
				else
				{
					EA::Thread::AutoFutex m(gThreadMainProcessingMutex);

					if (sMainProcessingEmptyInputFrame)
					{
//						ENC_TRACE("MainProcessing_Async: sMainProcessingEmptyInputFrame\n");
						frames_to_process = 1;  // process as P slice
						sMainProcessingEmptyInputFrame = false;
						sMainProcessingWaitingForBGroup = false;
					}
				}

				// test for any IDR frames which will cause us to need to not wait for full b_slice_frames
				for (int i = 0; i < frames_to_process; i++)
				{
					GetFrameFromQueue(i, &frame);
					if (frame.input.frame_type == ORIGIN_H264_IDR)
					{
//						ENC_TRACE("MainProcessing_Async:  don't include IDR %d\n", i);
						frames_to_process = i;	// don't include IDR (IDR must be at proper display order)
						break;
					}
				}

				if (frames_to_process == 0)
				{
					if (frames_in_queue > 0)
					{
						EA::Thread::AutoFutex m(gThreadMainProcessingMutex);

						sMainProcessingWaitingForBGroup = true;
//						ENC_TRACE("MainProcessing_Async:  sMainProcessingWaitingForBGroup\n");
					}
					// in the future, we may want to use this time to test P slice cost of first X frames in queue
					WaitForSingleObject(sMainProcessingSignal, INFINITE);
					continue;
				}
			}

			for (int i = 0; i < frames_to_process; i++)
			{
				if (i == 0) // always grab forward reference frame and encode that first
				{
					if (PopFromQueue(&input, frames_to_process - 1) == false)
					{
						ENC_ASSERT(0);
					}
					else
					{
						if (input.frame_type != ORIGIN_H264_IDR)
							input.frame_type = ORIGIN_H264_P_SLICE;

//						ENC_TRACE("MainProcessing_Async:  EncodeFrame %d\n", input.frame_type);

						EncodeFrame(&input, frames_to_process - 1);
					}
				}
				else
				{
					if (PopFromQueue(&input, frames_to_process - i - 1) == false)
					{
						ENC_ASSERT(0);
					}
					else
					{
						input.frame_type = ORIGIN_H264_B_SLICE;
//						ENC_TRACE("MainProcessing_Async:  EncodeFrame %d\n", input.frame_type);
						EncodeFrame(&input, frames_to_process - i - 1);
					}
				}
			}
		}
		else
		{	// just grab next frame and encode it
			if (PopFromQueue(&input, 0) == false)
			{
				WaitForSingleObject(sMainProcessingSignal, INFINITE);
				continue;
			}

			if (input.frame_type == ORIGIN_H264_DEFAULT_TYPE)
				input.frame_type = ORIGIN_H264_P_SLICE;
			EncodeFrame(&input);
		}


		{
			EA::Thread::AutoFutex m(gThreadMainProcessingMutex);
			sMainProcessingBusy = false;
		}

	}
}

void InitThreads()
{
	InitInputFrameQueue();
	// create and start processing thread
	sMainProcessingSignal = CreateEvent(NULL, FALSE, FALSE, NULL);
	sMainProcessingThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainProcessing_Async, NULL, 0, NULL);
	ENC_ASSERT(sMainProcessingThread);

	sFrameQueueSignal = CreateEvent(NULL, FALSE, FALSE, NULL);

	gEncoderState.num_cpu_cores = sNumOfCores;

	if (gEncoderState.multi_threading_on == false)
		return;

	// assumes we have already called DetectCores() and sNumOfCores is set
	ENC_ASSERT(sNumOfCores > 1);

	gEncoderState.stream_encoding_row_start = 0;//gEncoderState.mb_height / 3;//* 2 / 3;

	sNumMBThreads = sNumOfCores;
	if (sNumMBThreads > 10)
		sNumMBThreads = 10;	// cap it at 10

	for (int i = 0; i < MAX_THREADS; i++)
	{
		sThreadInfo[i].active = false;
		sThreadInfo[i].exit_flag = false;
		sThreadInfo[i].type = MB_TYPE;
		sThreadInfo[i].index  = i;
		sThreadInfo[i].sub_index = 0;
		sThreadInfo[i].mb_interval = (gEncoderState.mb_width + (sNumMBThreads - 1)) / sNumMBThreads;
		sThreadInfo[i].trigger_cutoff = 0;	// default means off
		sThreadInfo[i].start_height = 0;
		sThreadInfo[i].end_height = 0;
		sThreadInfo[i].signal = NULL;
		sThreadInfo[i].completion_signal = NULL;
		sThreadInfo[i].trigger_signal = NULL;
		sThreadInfo[i].interval_signal = NULL;
		sThreadInfo[i].in_loop_signal = NULL;
	}

	// Setup threads
	sNumBaseThreads = sNumMBThreads;		// for mb processing
	int total_threads = sNumBaseThreads + 1;	// add 1 for encoding thread

	if (gEncoderState.in_loop_filtering_on)
		total_threads += 1;		// add more threads for in-loop filtering

	if (gEncoderState.subpixel_motion_est > 0)
		total_threads += 2;		// add two more threads for sub pixel plane calculations

	sConversionThreadsStartIndex = total_threads;

	for (int i = 0; i < total_threads; i++)
	{
		sThreadInfo[i].active = true;
		sThreadInfo[i].signal = CreateSemaphore(NULL, 0, 1000, NULL);	// use counting semaphore for signal thread
		sThreadInfo[i].completion_signal = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	int index = 0;
	// for MB to Encoder
	for (int i = 0; i < sNumMBThreads; i++)
	{
		sThreadInfo[i].trigger_signal = sThreadInfo[sNumMBThreads].signal;		// row complete signal to encoder thread
		if (gEncoderState.in_loop_filtering_on)
			sThreadInfo[i].in_loop_signal = sThreadInfo[sNumMBThreads+1].signal;	// row complete signal to in-loop thread
		sThreadInfo[i].interval_signal = sThreadInfo[(i+1) % sNumMBThreads].signal;	// signal next MB thread that current interval is complete
		AddThread(PROCESS_MB_THREAD, &sThreadInfo[index++]);
	}

	AddThread(ENCODE_MB_THREAD, &sThreadInfo[index++]);

	if (gEncoderState.in_loop_filtering_on)
		AddThread(INLOOP_FILTER_THREAD, &sThreadInfo[index++]);

	if (gEncoderState.subpixel_motion_est > 0)
	{
		sSubPixelTriggers[0] = sThreadInfo[index].signal;
		sSubPixelTriggers[1] = sThreadInfo[index+1].signal;
		AddThread(SUBPIXEL_THREAD, &sThreadInfo[index++]);
		sThreadInfo[index].sub_index = 1;	// tell this next thread to call sub-pixel-v function
		AddThread(SUBPIXEL_THREAD, &sThreadInfo[index++]);
	}
}

void ExitThreads()
{
	// set exit flag and send events to wakeup thread
	// first for main processing thread
	sMainProcessingThreadKillFlag = true;
	SetEvent(sMainProcessingSignal);
	DWORD wait_result = WaitForSingleObject(sMainProcessingThread, INFINITE);
    H264_UNUSED(wait_result);
	CloseHandle(sMainProcessingThread);
	CloseHandle(sMainProcessingSignal);
	CloseHandle(sFrameQueueSignal);

	if (gEncoderState.multi_threading_on)
	{
		// set exit flag and send events to wakeup threads
		int num_handles = 0;
		HANDLE handles[MAX_THREADS];

		for (int i = 0; i < MAX_THREADS; i++)
		{
			if (gThreadHandles[i])
			{
				sThreadInfo[i].exit_flag = true;
				ReleaseSemaphore(sThreadInfo[i].signal, 1, NULL);	// encoder thread uses semaphores

				handles[num_handles++] = gThreadHandles[i];
			}
		}

		DWORD waitResult = ::WaitForMultipleObjects(num_handles, handles, TRUE, INFINITE);
        H264_UNUSED(waitResult)
		//	DWORD waitResult = ::WaitForMultipleObjects(num_handles, handles, TRUE, 10000);
		for (int i = 0; i < MAX_THREADS; i++)
		{
			if (gThreadHandles[i])
			{
				CloseHandle(gThreadHandles[i]);
				gThreadHandles[i] = NULL;
			}

			if (sThreadInfo[i].active)
			{
				CloseHandle(sThreadInfo[i].signal);
				sThreadInfo[i].signal = NULL;
				CloseHandle(sThreadInfo[i].completion_signal);
				sThreadInfo[i].completion_signal = NULL;
			}
		}
	}

	ExitInputFrameQueue();
}

void ResetThreadData()
{
	for (int i = 0; i < gEncoderState.mb_height; i++)
	{
		sRowMBProcessed[i].claimed = 0;
		sRowMBProcessed[i].completed = 0;
		sRowMBProcessed[i].progress = 0;

		sFilterRowMBProcessed[i].claimed = 0;
		sFilterRowMBProcessed[i].completed = 0;
		sFilterRowMBProcessed[i].progress = 0;
	}
}


int CurrentRowProgress(int row)
{
	EA::Thread::AutoFutex m(gThreadManageMutex);

	if (sRowMBProcessed[row].completed)
		return (sRowMBProcessed[row].progress + 2);
	return (sRowMBProcessed[row].progress);
}

int CurrentFilterRowProgress(int row)
{
    EA::Thread::AutoFutex m(gThreadManageMutex);

    if (sFilterRowMBProcessed[row].completed)
        return (sFilterRowMBProcessed[row].progress + 2);
    return (sFilterRowMBProcessed[row].progress);
}



static DWORD WINAPI EncodeMB_Async(LPVOID *lpParam)
{
	ThreadManagementType *thread_info = (ThreadManagementType *) lpParam;

	for (;;)
	{
		int result = WaitForSingleObject(thread_info->signal, INFINITE);
        H264_UNUSED(result)
		if (thread_info->exit_flag)
		{
			THREAD_TRACE("exiting EncodeMB_Async\n");
			return 1;
		}

		for (int i = 0; i < gEncoderState.mb_height; i++)
		{
			THREAD_TRACE("Encoding started %d\n", i);
			{
				EA::Thread::AutoFutex m1(gThreadTimingMutex);	
				ENC_START_TIMER(4,"CABAC Encoding");
			}
#if 0
            int row = i * gEncoderState.mb_width;
			for (int column = 0; column < gEncoderState.mb_width; column++)
			{
				CABAC_Encode_MB_Layer(GetMacroBlock(column + row));
			}
#endif
			CABAC_Encode_RowStream(i);

			{
				EA::Thread::AutoFutex m1(gThreadTimingMutex);	
				ENC_PAUSE_TIMER(4,false);
			}

			if (i < (gEncoderState.mb_height-1))
			{
				int result = WaitForSingleObject(thread_info->signal, INFINITE);
                H264_UNUSED(result)
				if (thread_info->exit_flag)
				{
					THREAD_TRACE("exiting EncodeMB_Async\n");
					return 1;
				}
			}
		}
		THREAD_TRACE("Encoding complete\n");

		SetEvent(thread_info->completion_signal);
	}
}

static DWORD WINAPI InLoopFilter_Async(LPVOID *lpParam)
{
	// new in-loop filter thread
	ThreadManagementType *thread_info = (ThreadManagementType *) lpParam;

	for (;;)
	{
		int result = WaitForSingleObject(thread_info->signal, INFINITE);
		if (thread_info->exit_flag)
		{
			THREAD_TRACE("exiting InLoopFilter_Async\n");
			return 1;
		}
		THREAD_TRACE("Filtering started\n");

		// extra wait to start after 2nd row is finished being processed (we don't want to interfere with current IPRED calcs)
		result = WaitForSingleObject(thread_info->signal, INFINITE);
        H264_UNUSED(result)
		if (thread_info->exit_flag)
		{
			THREAD_TRACE("exiting InLoopFilter_Async\n");
			return 1;
		}


		for (int i = 0; i < gEncoderState.mb_height; i++)
		{
			THREAD_TRACE("Filtering Row[%d]\n", i);
			{
				EA::Thread::AutoFutex m1(gThreadTimingMutex);	
				ENC_START_TIMER(15,"In-Loop Filter");
			}

			FilterMacroBlockRow(i);

			{
				EA::Thread::AutoFutex m1(gThreadTimingMutex);	
				ENC_PAUSE_TIMER(15,false);
			}
			THREAD_TRACE("Filtering Row[%d] Complete\n", i);

			if (i < (gEncoderState.mb_height-2))
			{
				int result = WaitForSingleObject(thread_info->signal, INFINITE);
                H264_UNUSED(result)
				if (thread_info->exit_flag)
				{
					THREAD_TRACE("exiting InLoopFilter_Async\n");
					return 1;
				}
			}
		}


		PadFrameEdges_Luma(gEncoderState.current_frame);
		PadFrameEdges_Chroma(gEncoderState.current_frame);

		// trigger sub pixel calculation threads if used
		if (gEncoderState.subpixel_motion_est > 0)
		{
			ReleaseSemaphore(sSubPixelTriggers[0], 1, NULL);	
			ReleaseSemaphore(sSubPixelTriggers[1], 1, NULL);	
		}

		THREAD_TRACE("Filtering complete\n");

		SetEvent(thread_info->completion_signal);
	}
}


static DWORD WINAPI SubPixelCalculations_Async(LPVOID *lpParam)
{
	// new in-loop filter thread
	ThreadManagementType *thread_info = (ThreadManagementType *) lpParam;

	for (;;)
	{
		int result = WaitForSingleObject(thread_info->signal, INFINITE);
        H264_UNUSED(result)
		if (thread_info->exit_flag)
		{
			THREAD_TRACE("exiting SubPixelCalculations_Async\n");
			return 1;
		}
		THREAD_TRACE("SubPixelCalculations started for Frame %d [%d]\n", gEncoderState.total_frame_num, thread_info->sub_index);

		ENC_START_TIMER(18, "ComputeSubPixelImages");
		ME_ComputeSubPixelImages(thread_info->sub_index);
		ENC_PAUSE_TIMER(18,false);

		THREAD_TRACE("SubPixelCalculations complete [%d]\n", thread_info->sub_index);

		SetEvent(thread_info->completion_signal);
	}
}

static int sTimeSliceStart = 0;

static DWORD WINAPI ProcessMB_Async(LPVOID *lpParam)
{
	ThreadManagementType *thread_info = (ThreadManagementType *) lpParam;
	int row = 0;
	int i;

	int result = WaitForSingleObject(thread_info->signal, INFINITE);
    H264_UNUSED(result)
	if (thread_info->exit_flag)
	{
		THREAD_TRACE("exiting ProcessMB_Async\n");
		return 1;
	}
	THREAD_TRACE("starting ProcessMB_Async %d\n", thread_info->index);

	for (;;)
	{
		{
			EA::Thread::AutoFutex m(gThreadManageMutex);

			// find a row to work on
			for (i = 0; i < gEncoderState.mb_height; i++)
			{
				if ((sRowMBProcessed[i].claimed == 0) && ((i % sNumMBThreads) == thread_info->index))
				{
					row = i;
					sRowMBProcessed[i].claimed = true;
					break;
				}
			}
		}

		if (i == gEncoderState.mb_height)
		{
			THREAD_TRACE("All rows complete\n");

			SetEvent(thread_info->completion_signal);

			THREAD_TRACE("Completed ProcessMB_Async %d\n", thread_info->index);

			int result = WaitForSingleObject(thread_info->signal, INFINITE);
            H264_UNUSED(result)
			if (thread_info->exit_flag)
			{
				THREAD_TRACE("exiting ProcessMB_Async\n");
				return 1;
			}
			THREAD_TRACE("starting ProcessMB_Async %d\n", thread_info->index);
			continue;
		}
		else
		if ((sNumMBThreads > 1) && (row != thread_info->index))	// if not the first row this thread processes, wait for signal to start row
		{
			int result = WaitForSingleObject(thread_info->signal, INFINITE);
            H264_UNUSED(result)
			if (thread_info->exit_flag)
			{
				THREAD_TRACE("exiting ProcessMB_Async\n");
				return 1;
			}
			THREAD_TRACE("starting ProcessMB_Async %d\n", thread_info->index);
		}

		{
			EA::Thread::AutoFutex m(gThreadManageMutex);

			if (gEncoderState.timeslice_ms && (sTimeSliceStart == 0))
			{
				sTimeSliceStart = timeGetTime();
				ENC_TRACE("Set TS - ");
			}
		}

		int row_qp = RateControl_GetFrameQP();
		int next_interval = thread_info->mb_interval;

		THREAD_TRACE("Processing MB Row: %d\n", row);

		for (int i = 0; i < gEncoderState.mb_width; i++)
		{
			//THREAD_TRACE("Processing MB: %d\n", i);
            int index = i + (row * gEncoderState.mb_width);
			ProcessMacroBlock(gEncoderState.current_frame, index, row_qp);
			//THREAD_TRACE("MB: %d done\n", i);

			// update progress
			{
				EA::Thread::AutoFutex m(gThreadManageMutex);
				sRowMBProcessed[row].progress++;
			}

			if (i == (gEncoderState.mb_width - 1))	// last one?
			{
				{
					EA::Thread::AutoFutex m(gThreadManageMutex);
					sRowMBProcessed[row].completed = true;

					if (gEncoderState.timeslice_ms && (sTimeSliceStart > 0))
					{
						int delta = timeGetTime() - sTimeSliceStart;

						if (delta >= gEncoderState.timeslice_ms)
						{
							ENC_TRACE("Sleep %d - ", delta);
							Sleep(gEncoderState.timeslice_idle_ms);
							sTimeSliceStart = 0;
						}
					}
				}

				if ((sNumMBThreads > 1) && ((row+1) < gEncoderState.mb_height))	// as long as we are not processing the last row, it is ok to signal next MB thread
					ReleaseSemaphore(thread_info->interval_signal, 1, NULL);	// signal next MB thread it is safe to finish

				// signal encoder thread it is safe to process this row - ReleaseSemaphore() is a ref counter and on the encoder thread the WaitForSingleObject() decrements
				ReleaseSemaphore(thread_info->trigger_signal, 1, NULL);	

				if (thread_info->in_loop_signal)
					ReleaseSemaphore(thread_info->in_loop_signal, 1, NULL);		// signal In-Loop Filtering thread to start for this row

				if ((row + 1) == gEncoderState.mb_height)	// last row completed?
				{
					if (!gEncoderState.in_loop_filtering_on)
					{
						PadFrameEdges_Luma(gEncoderState.current_frame);
						PadFrameEdges_Chroma(gEncoderState.current_frame);

						// trigger sub pixel calculation threads if used
						if (gEncoderState.subpixel_motion_est > 0)
						{
							ReleaseSemaphore(sSubPixelTriggers[0], 1, NULL);	
							ReleaseSemaphore(sSubPixelTriggers[1], 1, NULL);	
						}
					}

					if (gEncoderState.timeslice_ms)
					{
						EA::Thread::AutoFutex m(gThreadManageMutex);
						ENC_TRACE("reset TS: %d\n", (timeGetTime() - sTimeSliceStart));
						sTimeSliceStart = 0;
					}
				}

				THREAD_TRACE("row %d complete\n", row);
			}
			else
			if ((sNumMBThreads > 1) && (i == next_interval))
			{
				if ((row+1) < gEncoderState.mb_height)	// as long as we are not processing the last row, it is ok to signal next MB thread
					ReleaseSemaphore(thread_info->interval_signal, 1, NULL);	// signal next MB thread it is safe to finish
				next_interval += thread_info->mb_interval;
				if (next_interval >= gEncoderState.mb_width)
					next_interval = gEncoderState.mb_width+2;
				THREAD_TRACE("MB[%d]: Row:%d Column:%d\n", thread_info->index, row, i);
			}
			else
			{
				if ((sNumMBThreads > 1) && ((i == (next_interval - 2)) && (row != 0)))
				{
					THREAD_TRACE("MB[%d]: Row:%d Column:%d - waiting for next interval\n", thread_info->index, row, i);
					int result = WaitForSingleObject(thread_info->signal, INFINITE);
                    H264_UNUSED(result)
					if (thread_info->exit_flag)
					{
						THREAD_TRACE("exiting ProcessMB_Async\n");
						return 1;
					}
					THREAD_TRACE("MB[%d]: Row:%d Column:%d - Starting next interval\n", thread_info->index, row, i);
				}
			}
		}

		THREAD_TRACE("Processing MB Row: %d Complete\n", row);

	}
}


static DWORD WINAPI Conversion_Async(LPVOID *lpParam)
{
	ThreadManagementType *thread_info = (ThreadManagementType *) lpParam;

	for (;;)
	{
		int result = WaitForSingleObject(thread_info->signal, INFINITE);
        H264_UNUSED(result)
		if (thread_info->exit_flag)
			return 1;

		Convert444to420(thread_info->dst_frame, thread_info->src_444_frame, thread_info->start_height, thread_info->end_height);

		SetEvent(thread_info->completion_signal);
	}
}

void AddThread(int thread_type, ThreadManagementType *thread_info)
{
	EA::Thread::AutoFutex m(gThreadManageMutex);

	int i;

	for (i = 0; i < MAX_THREADS; i++)
	{
		if (gThreadHandles[i] == NULL)
			break;
	}

	if (i == MAX_THREADS)
		return;

    switch(thread_type)
    {
    case PROCESS_MB_THREAD:
	    gThreadHandles[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ProcessMB_Async, thread_info, 0, NULL);
        break;
    case ENCODE_MB_THREAD:
        gThreadHandles[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)EncodeMB_Async, thread_info, 0, NULL);
        break;
    case INLOOP_FILTER_THREAD:
        gThreadHandles[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InLoopFilter_Async, thread_info, 0, NULL);
        break;
	case SUBPIXEL_THREAD:
		gThreadHandles[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SubPixelCalculations_Async, thread_info, 0, NULL);
		break;
	case CONVERSION_THREAD:
		gThreadHandles[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Conversion_Async, thread_info, 0, NULL);
		break;
    default:
        ENC_ASSERT(0);
        break;
    }
}

void WaitForCompletion(int type)
{
	int num_handles = 0;
	HANDLE handles[MAX_THREADS];

	for (int i = 0; i < MAX_THREADS; i++)
	{
		if (gThreadHandles[i] && sThreadInfo[i].type == type)
		{
			handles[num_handles++] = sThreadInfo[i].completion_signal;
		}
	}

	DWORD waitResult = ::WaitForMultipleObjects(num_handles, handles, TRUE, INFINITE);
    H264_UNUSED(waitResult)
	//	DWORD waitResult = ::WaitForMultipleObjects(num_handles, handles, TRUE, 10000);
}

void ManageConversion(YUVFrameType *dst_frame, uint8_t *src_444_frame)
{
	int num_cores = sNumBaseThreads + 1;
	int rows_per_core = dst_frame->height / (num_cores * 2);
	int start = 0;

	for (int i = sConversionThreadsStartIndex; i < (sConversionThreadsStartIndex + num_cores); i++)
	{
		sThreadInfo[i].dst_frame = dst_frame;
		sThreadInfo[i].src_444_frame = src_444_frame;
		sThreadInfo[i].start_height = start;
		sThreadInfo[i].end_height = start + (rows_per_core * 2);	// must be by twos
		start += (rows_per_core * 2);
	}
	sThreadInfo[sConversionThreadsStartIndex + num_cores-1].end_height = dst_frame->height;	// pick up remainder

	for (int index = sConversionThreadsStartIndex; index < (sConversionThreadsStartIndex + num_cores); index++)
	{
		SetEvent(sThreadInfo[index].signal);
	}

	WaitForCompletion(CONVERSION_TYPE);
}

void ManageThreads(YUVFrameType *dst_frame)
{
	ResetThreadData();

	int num_threads_to_start = sNumBaseThreads;	// MBs
    H264_UNUSED(num_threads_to_start)
	ReleaseSemaphore(sThreadInfo[0].signal, 1, NULL);

	WaitForCompletion(MB_TYPE);
}
