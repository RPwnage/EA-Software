///////////////////////////////////////////////////////////////////////////////
// MantleFunctions.h
// 
// mantle api functions for dynamic binding
// if the mantle toolkit changes, those function signatures have to be updated as well!
//
// Created by Thomas Bruckschlegel
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifdef MANTLE_FUNCTION_EXTERN_DECLARATION

#undef MANTLEFN
#define MANTLEFN(retval, name, params) \
    typedef retval(GR_STDCALL *name ## Fn) params; \
    extern "C" name ## Fn _ ## name;

#else

#undef MANTLEFN
#define MANTLEFN(retval, name, params) \
    typedef retval(GR_STDCALL *name ## Fn) params; \
    name ## Fn _ ## name = 0;

#endif


#define BIND_MANTLE_FN(name, handle) \
    _ ## name = (name ## Fn)GetProcAddress(handle, #name); \
    if (!_ ## name) \
        OriginIGO::IGOLogWarn("BIND_MANTLE_FN failed: %s", #name); \

MANTLEFN(GR_RESULT, grInitAndEnumerateGpus, (
    const GR_APPLICATION_INFO* pAppInfo,
    const GR_ALLOC_CALLBACKS*  pAllocCb,
    GR_UINT*                   pGpuCount,
    GR_PHYSICAL_GPU            gpus[GR_MAX_PHYSICAL_GPUS]));

MANTLEFN(GR_RESULT, grGetGpuInfo, (
    GR_PHYSICAL_GPU gpu,
    GR_ENUM         infoType,      // GR_INFO_TYPE
    GR_SIZE*        pDataSize,
    GR_VOID*        pData));

// Device functions

MANTLEFN(GR_RESULT, grCreateDevice, (
    GR_PHYSICAL_GPU              gpu,
    const GR_DEVICE_CREATE_INFO* pCreateInfo,
    GR_DEVICE*                   pDevice));

MANTLEFN(GR_RESULT, grDestroyDevice, (
    GR_DEVICE device));

// Extension discovery functions

MANTLEFN(GR_RESULT, grGetExtensionSupport, (
    GR_PHYSICAL_GPU gpu,
    const GR_CHAR*  pExtName));

// Queue functions

MANTLEFN(GR_RESULT, grGetDeviceQueue, (
    GR_DEVICE device,
    GR_ENUM   queueType,        // GR_QUEUE_TYPE
    GR_UINT   queueIndex,
    GR_QUEUE* pQueue));

MANTLEFN(GR_RESULT, grQueueSubmit, (
    GR_QUEUE             queue,
    GR_UINT              cmdBufferCount,
    const GR_CMD_BUFFER* pCmdBuffers,
    GR_UINT              memRefCount,
    const GR_MEMORY_REF* pMemRefs,
    GR_FENCE             fence));

MANTLEFN(GR_RESULT, grQueueSetGlobalMemReferences, (
    GR_QUEUE             queue,
    GR_UINT              memRefCount,
    const GR_MEMORY_REF* pMemRefs));

MANTLEFN(GR_RESULT, grQueueWaitIdle, (
    GR_QUEUE queue));

MANTLEFN(GR_RESULT, grDeviceWaitIdle, (
    GR_DEVICE device));

// Memory functions

MANTLEFN(GR_RESULT, grGetMemoryHeapCount, (
    GR_DEVICE  device,
    GR_UINT*   pCount));

MANTLEFN(GR_RESULT, grGetMemoryHeapInfo, (
    GR_DEVICE device,
    GR_UINT   heapId,
    GR_ENUM   infoType,         // GR_INFO_TYPE
    GR_SIZE*  pDataSize,
    GR_VOID*  pData));

MANTLEFN(GR_RESULT, grAllocMemory, (
    GR_DEVICE                   device,
    const GR_MEMORY_ALLOC_INFO* pAllocInfo,
    GR_GPU_MEMORY*              pMem));

// to be used by IGO for our defered delete!!!
GR_RESULT GR_STDCALL _grFreeMemoryInternal(GR_GPU_MEMORY mem, GR_CMD_BUFFER cmd);

MANTLEFN(GR_RESULT, grFreeMemory, (
    GR_GPU_MEMORY mem));

MANTLEFN(GR_RESULT, grSetMemoryPriority, (
    GR_GPU_MEMORY mem,
    GR_ENUM       priority));    // GR_MEMORY_PRIORITY

MANTLEFN(GR_RESULT, grMapMemory, (
    GR_GPU_MEMORY mem,
    GR_FLAGS      flags,        // reserved
    GR_VOID**     ppData));

MANTLEFN(GR_RESULT, grUnmapMemory, (
    GR_GPU_MEMORY mem));

MANTLEFN(GR_RESULT, grPinSystemMemory, (
    GR_DEVICE      device,
    const GR_VOID* pSysMem,
    GR_SIZE        memSize,
    GR_GPU_MEMORY* pMem));

MANTLEFN(GR_RESULT, grRemapVirtualMemoryPages, (
    GR_DEVICE                            device,
    GR_UINT                              rangeCount,
    const GR_VIRTUAL_MEMORY_REMAP_RANGE* pRanges,
    GR_UINT                              preWaitSemaphoreCount,
    const GR_QUEUE_SEMAPHORE*            pPreWaitSemaphores,
    GR_UINT                              postSignalSemaphoreCount,
    const GR_QUEUE_SEMAPHORE*            pPostSignalSemaphores));

// Multi-device functions

MANTLEFN(GR_RESULT, grGetMultiGpuCompatibility, (
    GR_PHYSICAL_GPU            gpu0,
    GR_PHYSICAL_GPU            gpu1,
    GR_GPU_COMPATIBILITY_INFO* pInfo));

MANTLEFN(GR_RESULT, grOpenSharedMemory, (
    GR_DEVICE                  device,
    const GR_MEMORY_OPEN_INFO* pOpenInfo,
    GR_GPU_MEMORY*             pMem));

MANTLEFN(GR_RESULT, grOpenSharedQueueSemaphore, (
    GR_DEVICE                           device,
    const GR_QUEUE_SEMAPHORE_OPEN_INFO* pOpenInfo,
    GR_QUEUE_SEMAPHORE*                 pSemaphore));

MANTLEFN(GR_RESULT, grOpenPeerMemory, (
    GR_DEVICE                       device,
    const GR_PEER_MEMORY_OPEN_INFO* pOpenInfo,
    GR_GPU_MEMORY*                  pMem));

MANTLEFN(GR_RESULT, grOpenPeerImage, (
    GR_DEVICE                      device,
    const GR_PEER_IMAGE_OPEN_INFO* pOpenInfo,
    GR_IMAGE*                      pImage,
    GR_GPU_MEMORY*                 pMem));

// Generic API object functions

MANTLEFN(GR_RESULT, grDestroyObject, (
    GR_OBJECT object));

// to be used by IGO for our defered delete!!!
GR_RESULT GR_STDCALL _grDestroyObjectInternal(GR_OBJECT object, GR_CMD_BUFFER cmd);


MANTLEFN(GR_RESULT, grGetObjectInfo, (
    GR_BASE_OBJECT object,
    GR_ENUM        infoType,    // GR_INFO_TYPE
    GR_SIZE*       pDataSize,
    GR_VOID*       pData));

MANTLEFN(GR_RESULT, grBindObjectMemory, (
    GR_OBJECT     object,
    GR_GPU_MEMORY mem,
    GR_GPU_SIZE   offset));

// Fence functions

MANTLEFN(GR_RESULT, grCreateFence, (
    GR_DEVICE                   device,
    const GR_FENCE_CREATE_INFO* pCreateInfo,
    GR_FENCE*                   pFence));

MANTLEFN(GR_RESULT, grGetFenceStatus, (
    GR_FENCE fence));

MANTLEFN(GR_RESULT, grWaitForFences, (
    GR_DEVICE       device,
    GR_UINT         fenceCount,
    const GR_FENCE* pFences,
    GR_BOOL         waitAll,
    GR_FLOAT        timeout));

// Queue semaphore functions

MANTLEFN(GR_RESULT, grCreateQueueSemaphore, (
    GR_DEVICE                             device,
    const GR_QUEUE_SEMAPHORE_CREATE_INFO* pCreateInfo,
    GR_QUEUE_SEMAPHORE*                   pSemaphore));

MANTLEFN(GR_RESULT, grSignalQueueSemaphore, (
    GR_QUEUE queue,
    GR_QUEUE_SEMAPHORE semaphore));

MANTLEFN(GR_RESULT, grWaitQueueSemaphore, (
    GR_QUEUE queue,
    GR_QUEUE_SEMAPHORE semaphore));

// Event functions

MANTLEFN(GR_RESULT, grCreateEvent, (
    GR_DEVICE                   device,
    const GR_EVENT_CREATE_INFO* pCreateInfo,
    GR_EVENT*                   pEvent));

MANTLEFN(GR_RESULT, grGetEventStatus, (
    GR_EVENT event));

MANTLEFN(GR_RESULT, grSetEvent, (
    GR_EVENT event));

MANTLEFN(GR_RESULT, grResetEvent, (
    GR_EVENT event));

// Query functions

MANTLEFN(GR_RESULT, grCreateQueryPool, (
    GR_DEVICE                        device,
    const GR_QUERY_POOL_CREATE_INFO* pCreateInfo,
    GR_QUERY_POOL*                   pQueryPool));

MANTLEFN(GR_RESULT, grGetQueryPoolResults, (
    GR_QUERY_POOL queryPool,
    GR_UINT       startQuery,
    GR_UINT       queryCount,
    GR_SIZE*      pDataSize,
    GR_VOID*      pData));

// Format capabilities

MANTLEFN(GR_RESULT, grGetFormatInfo, (
    GR_DEVICE device,
    GR_FORMAT format,
    GR_ENUM   infoType,     // GR_INFO_TYPE
    GR_SIZE*  pDataSize,
    GR_VOID*  pData));

// Image functions

MANTLEFN(GR_RESULT, grCreateImage, (
    GR_DEVICE                   device,
    const GR_IMAGE_CREATE_INFO* pCreateInfo,
    GR_IMAGE*                   pImage));

MANTLEFN(GR_RESULT, grGetImageSubresourceInfo, (
    GR_IMAGE image,
    const GR_IMAGE_SUBRESOURCE* pSubresource,
    GR_ENUM  infoType,          // GR_INFO_TYPE
    GR_SIZE* pDataSize,
    GR_VOID* pData));

// Image view functions

MANTLEFN(GR_RESULT, grCreateImageView, (
    GR_DEVICE                        device,
    const GR_IMAGE_VIEW_CREATE_INFO* pCreateInfo,
    GR_IMAGE_VIEW*                   pView));

MANTLEFN(GR_RESULT, grCreateColorTargetView, (
    GR_DEVICE                               device,
    const GR_COLOR_TARGET_VIEW_CREATE_INFO* pCreateInfo,
    GR_COLOR_TARGET_VIEW*                   pView));

MANTLEFN(GR_RESULT, grCreateDepthStencilView, (
    GR_DEVICE                                device,
    const GR_DEPTH_STENCIL_VIEW_CREATE_INFO* pCreateInfo,
    GR_DEPTH_STENCIL_VIEW*                   pView));

// Shader functions

MANTLEFN(GR_RESULT, grCreateShader, (
    GR_DEVICE                    device,
    const GR_SHADER_CREATE_INFO* pCreateInfo,
    GR_SHADER*                   pShader));

// Pipeline functions

MANTLEFN(GR_RESULT, grCreateGraphicsPipeline, (
    GR_DEVICE                               device,
    const GR_GRAPHICS_PIPELINE_CREATE_INFO* pCreateInfo,
    GR_PIPELINE*                            pPipeline));

MANTLEFN(GR_RESULT, grCreateComputePipeline, (
    GR_DEVICE                              device,
    const GR_COMPUTE_PIPELINE_CREATE_INFO* pCreateInfo,
    GR_PIPELINE*                           pPipeline));

MANTLEFN(GR_RESULT, grStorePipeline, (
    GR_PIPELINE pipeline,
    GR_SIZE*    pDataSize,
    GR_VOID*    pData));

MANTLEFN(GR_RESULT, grLoadPipeline, (
    GR_DEVICE      device,
    GR_SIZE        dataSize,
    const GR_VOID* pData,
    GR_PIPELINE*   pPipeline));

// Sampler functions

MANTLEFN(GR_RESULT, grCreateSampler, (
    GR_DEVICE                     device,
    const GR_SAMPLER_CREATE_INFO* pCreateInfo,
    GR_SAMPLER*                   pSampler));

// Descriptor set functions

MANTLEFN(GR_RESULT, grCreateDescriptorSet, (
    GR_DEVICE                            device,
    const GR_DESCRIPTOR_SET_CREATE_INFO* pCreateInfo,
    GR_DESCRIPTOR_SET*                   pDescriptorSet));

MANTLEFN(GR_VOID, grBeginDescriptorSetUpdate, (
    GR_DESCRIPTOR_SET descriptorSet));

MANTLEFN(GR_VOID, grEndDescriptorSetUpdate, (
    GR_DESCRIPTOR_SET descriptorSet));

MANTLEFN(GR_VOID, grAttachSamplerDescriptors, (
    GR_DESCRIPTOR_SET descriptorSet,
    GR_UINT           startSlot,
    GR_UINT           slotCount,
    const GR_SAMPLER* pSamplers));

MANTLEFN(GR_VOID, grAttachImageViewDescriptors, (
    GR_DESCRIPTOR_SET                descriptorSet,
    GR_UINT                          startSlot,
    GR_UINT                          slotCount,
    const GR_IMAGE_VIEW_ATTACH_INFO* pImageViews));

MANTLEFN(GR_VOID, grAttachMemoryViewDescriptors, (
    GR_DESCRIPTOR_SET                 descriptorSet,
    GR_UINT                           startSlot,
    GR_UINT                           slotCount,
    const GR_MEMORY_VIEW_ATTACH_INFO* pMemViews));

MANTLEFN(GR_VOID, grAttachNestedDescriptors, (
    GR_DESCRIPTOR_SET                    descriptorSet,
    GR_UINT                              startSlot,
    GR_UINT                              slotCount,
    const GR_DESCRIPTOR_SET_ATTACH_INFO* pNestedDescriptorSets));

MANTLEFN(GR_VOID, grClearDescriptorSetSlots, (
    GR_DESCRIPTOR_SET descriptorSet,
    GR_UINT           startSlot,
    GR_UINT           slotCount));

// State object functions

MANTLEFN(GR_RESULT, grCreateViewportState, (
    GR_DEVICE                            device,
    const GR_VIEWPORT_STATE_CREATE_INFO* pCreateInfo,
    GR_VIEWPORT_STATE_OBJECT*            pState));

MANTLEFN(GR_RESULT, grCreateRasterState, (
    GR_DEVICE                          device,
    const GR_RASTER_STATE_CREATE_INFO* pCreateInfo,
    GR_RASTER_STATE_OBJECT*            pState));

MANTLEFN(GR_RESULT, grCreateMsaaState, (
    GR_DEVICE                        device,
    const GR_MSAA_STATE_CREATE_INFO* pCreateInfo,
    GR_MSAA_STATE_OBJECT*            pState));

MANTLEFN(GR_RESULT, grCreateColorBlendState, (
    GR_DEVICE                               device,
    const GR_COLOR_BLEND_STATE_CREATE_INFO* pCreateInfo,
    GR_COLOR_BLEND_STATE_OBJECT*            pState));

MANTLEFN(GR_RESULT, grCreateDepthStencilState, (
    GR_DEVICE                                 device,
    const GR_DEPTH_STENCIL_STATE_CREATE_INFO* pCreateInfo,
    GR_DEPTH_STENCIL_STATE_OBJECT*            pState));

// Command buffer functions

MANTLEFN(GR_RESULT, grCreateCommandBuffer, (
    GR_DEVICE                        device,
    const GR_CMD_BUFFER_CREATE_INFO* pCreateInfo,
    GR_CMD_BUFFER*                   pCmdBuffer));

MANTLEFN(GR_RESULT, grBeginCommandBuffer, (
    GR_CMD_BUFFER cmdBuffer,
    GR_FLAGS      flags));       // GR_CMD_BUFFER_BUILD_FLAGS

MANTLEFN(GR_RESULT, grEndCommandBuffer, (
    GR_CMD_BUFFER cmdBuffer));

MANTLEFN(GR_RESULT, grResetCommandBuffer, (
    GR_CMD_BUFFER cmdBuffer));

// Command buffer building functions

MANTLEFN(GR_VOID, grCmdBindPipeline, (
    GR_CMD_BUFFER cmdBuffer,
    GR_ENUM       pipelineBindPoint,            // GR_PIPELINE_BIND_POINT
    GR_PIPELINE   pipeline));

MANTLEFN(GR_VOID, grCmdBindStateObject, (
    GR_CMD_BUFFER   cmdBuffer,
    GR_ENUM         stateBindPoint,             // GR_STATE_BIND_POINT
    GR_STATE_OBJECT state));

MANTLEFN(GR_VOID, grCmdBindDescriptorSet, (
    GR_CMD_BUFFER     cmdBuffer,
    GR_ENUM           pipelineBindPoint,        // GR_PIPELINE_BIND_POINT
    GR_UINT           index,
    GR_DESCRIPTOR_SET descriptorSet,
    GR_UINT           slotOffset));

MANTLEFN(GR_VOID, grCmdBindDynamicMemoryView, (
    GR_CMD_BUFFER                     cmdBuffer,
    GR_ENUM                           pipelineBindPoint,    // GR_PIPELINE_BIND_POINT
    const GR_MEMORY_VIEW_ATTACH_INFO* pMemView));

MANTLEFN(GR_VOID, grCmdBindIndexData, (
    GR_CMD_BUFFER cmdBuffer,
    GR_GPU_MEMORY mem,
    GR_GPU_SIZE   offset,
    GR_ENUM       indexType));                   // GR_INDEX_TYPE

MANTLEFN(GR_VOID, grCmdBindTargets, (
    GR_CMD_BUFFER                     cmdBuffer,
    GR_UINT                           colorTargetCount,
    const GR_COLOR_TARGET_BIND_INFO*  pColorTargets,
    const GR_DEPTH_STENCIL_BIND_INFO* pDepthTarget));

MANTLEFN(GR_VOID, grCmdPrepareMemoryRegions, (
    GR_CMD_BUFFER                     cmdBuffer,
    GR_UINT                           transitionCount,
    const GR_MEMORY_STATE_TRANSITION* pStateTransitions));

MANTLEFN(GR_VOID, grCmdPrepareImages, (
    GR_CMD_BUFFER                    cmdBuffer,
    GR_UINT                          transitionCount,
    const GR_IMAGE_STATE_TRANSITION* pStateTransitions));

MANTLEFN(GR_VOID, grCmdDraw, (
    GR_CMD_BUFFER cmdBuffer,
    GR_UINT       firstVertex,
    GR_UINT       vertexCount,
    GR_UINT       firstInstance,
    GR_UINT       instanceCount));

MANTLEFN(GR_VOID, grCmdDrawIndexed, (
    GR_CMD_BUFFER cmdBuffer,
    GR_UINT       firstIndex,
    GR_UINT       indexCount,
    GR_INT        vertexOffset,
    GR_UINT       firstInstance,
    GR_UINT       instanceCount));

MANTLEFN(GR_VOID, grCmdDrawIndirect, (
    GR_CMD_BUFFER cmdBuffer,
    GR_GPU_MEMORY mem,
    GR_GPU_SIZE   offset));

MANTLEFN(GR_VOID, grCmdDrawIndexedIndirect, (
    GR_CMD_BUFFER cmdBuffer,
    GR_GPU_MEMORY mem,
    GR_GPU_SIZE   offset));

MANTLEFN(GR_VOID, grCmdDispatch, (
    GR_CMD_BUFFER cmdBuffer,
    GR_UINT       x,
    GR_UINT       y,
    GR_UINT       z));

MANTLEFN(GR_VOID, grCmdDispatchIndirect, (
    GR_CMD_BUFFER cmdBuffer,
    GR_GPU_MEMORY mem,
    GR_GPU_SIZE   offset));

MANTLEFN(GR_VOID, grCmdCopyMemory, (
    GR_CMD_BUFFER         cmdBuffer,
    GR_GPU_MEMORY         srcMem,
    GR_GPU_MEMORY         destMem,
    GR_UINT               regionCount,
    const GR_MEMORY_COPY* pRegions));

MANTLEFN(GR_VOID, grCmdCopyImage, (
    GR_CMD_BUFFER        cmdBuffer,
    GR_IMAGE             srcImage,
    GR_IMAGE             destImage,
    GR_UINT              regionCount,
    const GR_IMAGE_COPY* pRegions));

MANTLEFN(GR_VOID, grCmdCopyMemoryToImage, (
    GR_CMD_BUFFER               cmdBuffer,
    GR_GPU_MEMORY               srcMem,
    GR_IMAGE                    destImage,
    GR_UINT                     regionCount,
    const GR_MEMORY_IMAGE_COPY* pRegions));

MANTLEFN(GR_VOID, grCmdCopyImageToMemory, (
    GR_CMD_BUFFER               cmdBuffer,
    GR_IMAGE                    srcImage,
    GR_GPU_MEMORY               destMem,
    GR_UINT                     regionCount,
    const GR_MEMORY_IMAGE_COPY* pRegions));

MANTLEFN(GR_VOID, grCmdCloneImageData, (
    GR_CMD_BUFFER  cmdBuffer,
    GR_IMAGE       srcImage,
    GR_ENUM        srcImageState,       // GR_IMAGE_STATE
    GR_IMAGE       destImage,
    GR_ENUM        destImageState));     // GR_IMAGE_STATE

MANTLEFN(GR_VOID, grCmdUpdateMemory, (
    GR_CMD_BUFFER    cmdBuffer,
    GR_GPU_MEMORY    destMem,
    GR_GPU_SIZE      destOffset,
    GR_GPU_SIZE      dataSize,
    const GR_UINT32* pData));

MANTLEFN(GR_VOID, grCmdFillMemory, (
    GR_CMD_BUFFER cmdBuffer,
    GR_GPU_MEMORY destMem,
    GR_GPU_SIZE   destOffset,
    GR_GPU_SIZE   fillSize,
    GR_UINT32     data));


MANTLEFN(GR_VOID, grCmdClearColorImage, (
    GR_CMD_BUFFER                     cmdBuffer,
    GR_IMAGE                          image,
    const GR_FLOAT                    color[4],
    GR_UINT                           rangeCount,
    const GR_IMAGE_SUBRESOURCE_RANGE* pRanges));

MANTLEFN(GR_VOID, grCmdClearColorImageRaw, (
    GR_CMD_BUFFER                     cmdBuffer,
    GR_IMAGE                          image,
    const GR_UINT32                   color[4],
    GR_UINT                           rangeCount,
    const GR_IMAGE_SUBRESOURCE_RANGE* pRanges));

MANTLEFN(GR_VOID, grCmdClearDepthStencil, (
    GR_CMD_BUFFER                     cmdBuffer,
    GR_IMAGE                          image,
    GR_FLOAT                          depth,
    GR_UINT8                          stencil,
    GR_UINT                           rangeCount,
    const GR_IMAGE_SUBRESOURCE_RANGE* pRanges));

MANTLEFN(GR_VOID, grCmdResolveImage, (
    GR_CMD_BUFFER           cmdBuffer,
    GR_IMAGE                srcImage,
    GR_IMAGE                destImage,
    GR_UINT                 rectCount,
    const GR_IMAGE_RESOLVE* pRects));

MANTLEFN(GR_VOID, grCmdSetEvent, (
    GR_CMD_BUFFER cmdBuffer,
    GR_EVENT      event));

MANTLEFN(GR_VOID, grCmdResetEvent, (
    GR_CMD_BUFFER cmdBuffer,
    GR_EVENT      event));

MANTLEFN(GR_VOID, grCmdMemoryAtomic, (
    GR_CMD_BUFFER cmdBuffer,
    GR_GPU_MEMORY destMem,
    GR_GPU_SIZE   destOffset,
    GR_UINT64     srcData,
    GR_ENUM       atomicOp));            // GR_ATOMIC_OP

MANTLEFN(GR_VOID, grCmdBeginQuery, (
    GR_CMD_BUFFER cmdBuffer,
    GR_QUERY_POOL queryPool,
    GR_UINT       slot,
    GR_FLAGS      flags));               // GR_QUERY_CONTROL_FLAGS

MANTLEFN(GR_VOID, grCmdEndQuery, (
    GR_CMD_BUFFER cmdBuffer,
    GR_QUERY_POOL queryPool,
    GR_UINT       slot));

MANTLEFN(GR_VOID, grCmdResetQueryPool, (
    GR_CMD_BUFFER cmdBuffer,
    GR_QUERY_POOL queryPool,
    GR_UINT       startQuery,
    GR_UINT       queryCount));

MANTLEFN(GR_VOID, grCmdWriteTimestamp, (
    GR_CMD_BUFFER cmdBuffer,
    GR_ENUM       timestampType,            // GR_TIMESTAMP_TYPE
    GR_GPU_MEMORY destMem,
    GR_GPU_SIZE   destOffset));

MANTLEFN(GR_VOID, grCmdInitAtomicCounters, (
    GR_CMD_BUFFER    cmdBuffer,
    GR_ENUM          pipelineBindPoint,     // GR_PIPELINE_BIND_POINT
    GR_UINT          startCounter,
    GR_UINT          counterCount,
    const GR_UINT32* pData));

MANTLEFN(GR_VOID, grCmdLoadAtomicCounters, (
    GR_CMD_BUFFER cmdBuffer,
    GR_ENUM       pipelineBindPoint,        // GR_PIPELINE_BIND_POINT
    GR_UINT       startCounter,
    GR_UINT       counterCount,
    GR_GPU_MEMORY srcMem,
    GR_GPU_SIZE   srcOffset));

MANTLEFN(GR_VOID, grCmdSaveAtomicCounters, (
    GR_CMD_BUFFER cmdBuffer,
    GR_ENUM       pipelineBindPoint,        // GR_PIPELINE_BIND_POINT
    GR_UINT       startCounter,
    GR_UINT       counterCount,
    GR_GPU_MEMORY destMem,
    GR_GPU_SIZE   destOffset));

MANTLEFN(GR_VOID, grCmdDbgMarkerBegin, (
    GR_CMD_BUFFER  cmdBuffer,
    const GR_CHAR* pMarker));

MANTLEFN(GR_VOID, grCmdDbgMarkerEnd, (
    GR_CMD_BUFFER  cmdBuffer));


MANTLEFN(GR_RESULT, grDbgSetValidationLevel, (
    GR_DEVICE device,
    GR_ENUM   validationLevel));        // GR_VALIDATION_LEVEL

MANTLEFN(GR_RESULT, grDbgRegisterMsgCallback, (
    GR_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback,
    GR_VOID*                     pUserData));

MANTLEFN(GR_RESULT, grDbgUnregisterMsgCallback, (
    GR_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback));

MANTLEFN(GR_RESULT, grDbgSetMessageFilter, (
    GR_DEVICE device,
    GR_ENUM   msgCode,                 // GR_DBG_MSG_CODE
    GR_ENUM   filter));                 // GR_DBG_MSG_FILTER

MANTLEFN(GR_RESULT, grDbgSetObjectTag, (
    GR_BASE_OBJECT object,
    GR_SIZE        tagSize,
    const GR_VOID* pTag));

MANTLEFN(GR_RESULT, grDbgSetGlobalOption, (
    GR_ENUM        dbgOption,          // GR_DBG_GLOBAL_OPTION
    GR_SIZE        dataSize,
    const GR_VOID* pData));

MANTLEFN(GR_RESULT, grDbgSetDeviceOption, (
    GR_DEVICE      device,
    GR_ENUM        dbgOption,          // GR_DBG_DEVICE_OPTION
    GR_SIZE        dataSize,
    const GR_VOID* pData));

