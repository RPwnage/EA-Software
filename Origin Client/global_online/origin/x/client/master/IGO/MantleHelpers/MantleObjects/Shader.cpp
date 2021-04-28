#include <assert.h>
#include "Shader.h"
#include "..\MantleAppWsi.h"

static const wchar_t *FXC_PATH_STRING = L"fxc.bat";

extern GR_RESULT FindResourceFile( WCHAR* strDestPath, int destSize, const WCHAR* strFilename );

Shader::Shader()
{
    ILC_DX_RESOURCE_MAPPING resMap={};
    m_ResMap = resMap;

    static GR_PIPELINE_SHADER emptyPipelineShaderInfo =
    {
         GR_NULL_HANDLE,        // GR_SHADER
         {
             {
                 0,                     // GR_UINT descriptorCount;
                 GR_NULL_HANDLE         // const GR_DESCRIPTOR_SLOT_INFO* pDescriptorInfo;
             },                     // GR_DESCRIPTOR_SET_MAPPING
             {
                 0, 
                 GR_NULL_HANDLE
             },                     // GR_DESCRIPTOR_SET_MAPPING
         },                     // GR_DESCRIPTOR_SET_MAPPING[GR_MAX_DESCRIPTOR_SETS=2]
         0,                     // GR_UINT linkConstBufferCount
         NULL,                  // GR_LINK_CONST_BUFFER pLinkConstBufferInfo
         {
            GR_SLOT_UNUSED,         // GR_DESCRIPTOR_SET_SLOT_TYPE
            0                       // GR_UINT shaderEntityIndex;
         }                      // GR_DYNAMIC_MEMORY_VIEW_SLOT_INFO
    };

    m_pipelineShaderInfo = emptyPipelineShaderInfo;
}


Shader::~Shader()
{
}

void Shader::dealloc(GR_CMD_BUFFER cmd)
{
	if (m_pipelineShaderInfo.descriptorSetMapping[0].pDescriptorInfo)
		delete[] m_pipelineShaderInfo.descriptorSetMapping[0].pDescriptorInfo;

	_grDestroyObjectInternal(m_pipelineShaderInfo.shader, cmd);

}

GR_RESULT Shader::initFromBinaryHLSL(const char* blob, unsigned int blobSize, ILC_VS_INPUT_LAYOUT& vsInputLayout, const DX_SLOT_INFO* dynamicMapping, GR_UINT gpu)
{
	GR_RESULT grresult = GR_ERROR_BAD_SHADER_CODE;
	m_gpu = gpu;

	//
	// Prepare for IL conversion
	//

	// Shader source codes
	ILC_DATA shSrcCode = { 0 };
	shSrcCode.pData = (void*)blob;
	shSrcCode.dataSize = (SIZE_T)blobSize;

	// Metadata
	ILC_METADATA shMetadata = { 0 };
	//            shMetadata.flags |= ILC_FLAG_EMBED_SOURCE_DASM;      // Embedded DASM text
	//            shMetadata.flags |= ILC_FLAG_FORCE_DIRECT_TRANSLATE; // Force direct translation

	// Shader binary object and error message
	ILC_DATA shBinObjIl = { 0 };
	ILC_DATA errMsg = { 0 };

	//
	// IL conversion
	//
	ILC_RESULT result = ConvertShader(
		&shSrcCode,
		&shMetadata,
		&vsInputLayout,
		&m_ResMap,
		&shBinObjIl,
		&errMsg,
		MemAlloc);

	// Check the conversion result
	if (result == ILC_SUCCESS)
	{
		// call grCreateShader to generate a mantle shader
		GR_SHADER_CREATE_INFO cs_info = {};
		cs_info.pCode = shBinObjIl.pData;
		cs_info.codeSize = shBinObjIl.dataSize;
		_grCreateShader(OriginIGO::getMantleDevice(m_gpu), &cs_info, &m_pipelineShaderInfo.shader);

		MemFree(shBinObjIl.pData);

		grresult = GR_SUCCESS;
	}
	else
	{
		grresult = (GR_RESULT)result;
		// Conversion failed
		if (errMsg.dataSize > 0)
		{
			// Output the error message
			//OutputDebugString(TEXT(__FUNCTION__)L": Conversion failed!\n");

			// Free error message content
			MemFree(errMsg.pData);
			errMsg.pData = NULL;
			errMsg.dataSize = 0;
		}
		else
		{
			//OutputDebugString(TEXT(__FUNCTION__)L": Conversion failed!\n");
		}
	}
	initDescriptorSlotInfo(dynamicMapping);
	
	return grresult;
}

void Shader::initFromHLSL( const WCHAR* wsFilename, const WCHAR* wsParameters, ILC_VS_INPUT_LAYOUT& vsInputLayout, const DX_SLOT_INFO* dynamicMapping, GR_UINT gpu)
{
    m_gpu = gpu;

	wchar_t wsEXE[MAX_PATH] = L"C:\\temp\\fxc.bat";

    WCHAR binfilename[MAX_PATH] = { 0 };
    wcscpy_s(binfilename, wsFilename);
    WCHAR* strLastdot = wcsrchr( binfilename, TEXT( '.' ) );
    *strLastdot = 0;
    wcscat_s(binfilename, L".bin");

    WCHAR commandLine[MAX_PATH] = { 0 };
    swprintf_s( commandLine, L"%s %s /Fo%s", wsFilename, wsParameters, binfilename);

    // start fxc to generate the IL binary from HLSL
    SHELLEXECUTEINFO shExecInfo={0};
    shExecInfo.cbSize = sizeof( SHELLEXECUTEINFO );
    shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shExecInfo.hwnd = NULL;
    shExecInfo.lpFile = wsEXE;
    shExecInfo.nShow = SW_HIDE;
    shExecInfo.hInstApp = NULL;
    shExecInfo.lpParameters = commandLine;
    ShellExecuteEx( &shExecInfo );
    
    if(shExecInfo.hProcess !=NULL)
    {
        ::WaitForSingleObject(shExecInfo.hProcess, INFINITE);
        ::CloseHandle(shExecInfo.hProcess);
    }

    // call ProcessFile to generate convertedIL
    ProcessFile( binfilename, vsInputLayout, m_ResMap );

    initDescriptorSlotInfo( dynamicMapping);
}

void Shader::countUsedDescriptorSlots()
{
    m_DxOffsets[DXST_VERTEXBUFFER] = 0;
    m_DxOffsets[DXST_CONSTANTS] = 0;
    for(unsigned int i = 0; i<ILC_DX_VERTEX_BUFFER_COUNT; ++i)
    {
        if((m_ResMap.vb[i].used != false))// && (m_ResMap.vb[i].usage != ILC_RESOURCE_USAGE_LINK_TIME_CONST))
        {
            ++m_DxOffsets[DXST_CONSTANTS];
        }
    }

    m_DxOffsets[DXST_TEXTURE] = m_DxOffsets[DXST_CONSTANTS];
    for(unsigned int i = 0; i<ILC_DX_CONST_BUFFER_COUNT; ++i)
    {
		if ((m_ResMap.cb[i].used != false) && (m_ResMap.cb[i].mapping != ILC_CB_MAPPING_LINK_TIME_CONST))
        {
            ++m_DxOffsets[DXST_TEXTURE];
        }
    }

    m_DxOffsets[DXST_UAV] = m_DxOffsets[DXST_TEXTURE];
    for(unsigned int i = 0; i<ILC_DX_TEXTURE_RESOURCE_COUNT; ++i)
    {
		if ((m_ResMap.tex[i].used != false))// && (m_ResMap.tex[i].usage != ILC_RESOURCE_USAGE_LINK_TIME_CONST))
        {
            ++m_DxOffsets[DXST_UAV];
        }
    }

    m_DxOffsets[DXST_ATOMIC_COUNTER] = m_DxOffsets[DXST_UAV];
    for(unsigned int i = 0; i<ILC_DX_UAV_COUNT; ++i)
    {
		if ((m_ResMap.uav[i].used != false))// && (m_ResMap.uav[i].usage != ILC_RESOURCE_USAGE_LINK_TIME_CONST))
        {
            ++m_DxOffsets[DXST_ATOMIC_COUNTER];
        }
    }

    m_DxOffsets[DXST_SAMPLER] = m_DxOffsets[DXST_ATOMIC_COUNTER];
    for(unsigned int i = 0; i<ILC_DX_UAV_COUNT; ++i)
    {
		if ((m_ResMap.atomicCounter[i].used != false))// && (m_ResMap.atomicCounter[i].usage != ILC_RESOURCE_USAGE_LINK_TIME_CONST))
        {
            ++m_DxOffsets[DXST_SAMPLER];
        }
    }

    m_DxOffsets[DXST_COUNT] = m_DxOffsets[DXST_SAMPLER];
    for(unsigned int i = 0; i<ILC_DX_SAMPLER_COUNT; ++i)
    {
		if ((m_ResMap.sampler[i].used != false))// && (m_ResMap.sampler[i].usage != ILC_RESOURCE_USAGE_LINK_TIME_CONST))
        {
            ++m_DxOffsets[DXST_COUNT];
        }
    }
}

void Shader::initDescriptorSlotInfo(const DX_SLOT_INFO* dynamic)
{
    countUsedDescriptorSlots();
    
    if(0!=numSlots())
    {
        GR_DESCRIPTOR_SLOT_INFO* descriptors = new GR_DESCRIPTOR_SLOT_INFO[numSlots()];
        initDescriptors(numSlots(), descriptors, dynamic);

        m_pipelineShaderInfo.descriptorSetMapping[0].descriptorCount = numSlots();
        m_pipelineShaderInfo.descriptorSetMapping[0].pDescriptorInfo = descriptors;
    }
}

void Shader::initDescriptors(unsigned int numDescriptors, GR_DESCRIPTOR_SLOT_INFO* descriptors, const DX_SLOT_INFO* dynamicMapping)
{
    GR_UINT32 baseSlot = 0;
    for(unsigned int i=0;i<ILC_DX_VERTEX_BUFFER_COUNT;i++)
    {
        if(m_ResMap.vb[i].used != false)
        {
            if((NULL!=dynamicMapping) && (dynamicMapping->slotObjectType == DXST_VERTEXBUFFER)&&(dynamicMapping->slotIndex==i))
            {
                m_pipelineShaderInfo.dynamicMemoryViewMapping.slotObjectType = GR_SLOT_SHADER_RESOURCE;
                m_pipelineShaderInfo.dynamicMemoryViewMapping.shaderEntityIndex = m_ResMap.vb[i].ilSlot;

                descriptors[baseSlot+m_ResMap.vb[i].ilSlot].slotObjectType = GR_SLOT_UNUSED;
                descriptors[baseSlot+m_ResMap.vb[i].ilSlot].shaderEntityIndex = 0;
            }
            else
            {
                descriptors[baseSlot+m_ResMap.vb[i].ilSlot].slotObjectType = GR_SLOT_SHADER_RESOURCE;
                descriptors[baseSlot+m_ResMap.vb[i].ilSlot].shaderEntityIndex = m_ResMap.vb[i].ilSlot;
            }
        }
    }

    for(unsigned int i=0;i<ILC_DX_CONST_BUFFER_COUNT;i++)
    {
        if(m_ResMap.cb[i].used != false)
        {
            if((NULL!=dynamicMapping) && (dynamicMapping->slotObjectType == DXST_CONSTANTS)&&(dynamicMapping->slotIndex==i))
            {
                m_pipelineShaderInfo.dynamicMemoryViewMapping.slotObjectType = GR_SLOT_SHADER_RESOURCE;
                m_pipelineShaderInfo.dynamicMemoryViewMapping.shaderEntityIndex = m_ResMap.cb[i].ilSlot;

                descriptors[baseSlot+m_ResMap.cb[i].ilSlot].slotObjectType = GR_SLOT_UNUSED;
                descriptors[baseSlot+m_ResMap.cb[i].ilSlot].shaderEntityIndex = 0;
            }
            else
            {
                descriptors[baseSlot+m_ResMap.cb[i].ilSlot].slotObjectType = GR_SLOT_SHADER_RESOURCE;
                descriptors[baseSlot+m_ResMap.cb[i].ilSlot].shaderEntityIndex = m_ResMap.cb[i].ilSlot;
            }
        }
    }
     
    for(unsigned int i=0;i<ILC_DX_TEXTURE_RESOURCE_COUNT;i++)
    {
        if(m_ResMap.tex[i].used != false)
        {
            if((NULL!=dynamicMapping) && (dynamicMapping->slotObjectType == DXST_TEXTURE)&&(dynamicMapping->slotIndex==i))
            {
                m_pipelineShaderInfo.dynamicMemoryViewMapping.slotObjectType = GR_SLOT_SHADER_RESOURCE;
                m_pipelineShaderInfo.dynamicMemoryViewMapping.shaderEntityIndex = m_ResMap.tex[i].ilSlot;

                descriptors[baseSlot+m_ResMap.tex[i].ilSlot].slotObjectType = GR_SLOT_UNUSED;
                descriptors[baseSlot+m_ResMap.tex[i].ilSlot].shaderEntityIndex = 0;
            }
            else
            {
                descriptors[baseSlot+m_ResMap.tex[i].ilSlot].slotObjectType = GR_SLOT_SHADER_RESOURCE;
                descriptors[baseSlot+m_ResMap.tex[i].ilSlot].shaderEntityIndex = m_ResMap.tex[i].ilSlot;
            }
        }
    }

    baseSlot = m_DxOffsets[DXST_UAV];
    for(unsigned int i=0;i<ILC_DX_UAV_COUNT;i++)
    {
        if(m_ResMap.uav[i].used != false)
        {
            if((NULL!=dynamicMapping) && (dynamicMapping->slotObjectType == DXST_UAV)&&(dynamicMapping->slotIndex==i))
            {
                m_pipelineShaderInfo.dynamicMemoryViewMapping.slotObjectType = GR_SLOT_SHADER_UAV;
                m_pipelineShaderInfo.dynamicMemoryViewMapping.shaderEntityIndex = m_ResMap.uav[i].ilSlot;

                descriptors[baseSlot+m_ResMap.uav[i].ilSlot].slotObjectType = GR_SLOT_UNUSED;
                descriptors[baseSlot+m_ResMap.uav[i].ilSlot].shaderEntityIndex = 0;
            }
            else
            {
                descriptors[baseSlot+m_ResMap.uav[i].ilSlot].slotObjectType = GR_SLOT_SHADER_UAV;
                descriptors[baseSlot+m_ResMap.uav[i].ilSlot].shaderEntityIndex = m_ResMap.uav[i].ilSlot;
            }
        }
    }

    for(unsigned int i=0;i<ILC_DX_UAV_COUNT;i++)
    {
        if(m_ResMap.atomicCounter[i].used != false)
        {
            if((NULL!=dynamicMapping) && (dynamicMapping->slotObjectType == DXST_ATOMIC_COUNTER)&&(dynamicMapping->slotIndex==i))
            {
                m_pipelineShaderInfo.dynamicMemoryViewMapping.slotObjectType = GR_SLOT_SHADER_UAV;
                m_pipelineShaderInfo.dynamicMemoryViewMapping.shaderEntityIndex = m_ResMap.atomicCounter[i].ilSlot;

                descriptors[baseSlot+m_ResMap.atomicCounter[i].ilSlot].slotObjectType = GR_SLOT_UNUSED;
                descriptors[baseSlot+m_ResMap.atomicCounter[i].ilSlot].shaderEntityIndex = 0;
            }
            else
            {
                descriptors[baseSlot+m_ResMap.atomicCounter[i].ilSlot].slotObjectType = GR_SLOT_SHADER_UAV;
                descriptors[baseSlot+m_ResMap.atomicCounter[i].ilSlot].shaderEntityIndex = m_ResMap.atomicCounter[i].ilSlot;
            }
        }
    }

    baseSlot = m_DxOffsets[DXST_SAMPLER];
    for(unsigned int i=0;i<ILC_DX_SAMPLER_COUNT;i++)
    {
        if(m_ResMap.sampler[i].used != false)
        {
            if((NULL!=dynamicMapping) && (dynamicMapping->slotObjectType == DXST_SAMPLER)&&(dynamicMapping->slotIndex==i))
            {
                m_pipelineShaderInfo.dynamicMemoryViewMapping.slotObjectType = GR_SLOT_SHADER_SAMPLER;
                m_pipelineShaderInfo.dynamicMemoryViewMapping.shaderEntityIndex = m_ResMap.sampler[i].ilSlot;

                descriptors[baseSlot+m_ResMap.sampler[i].ilSlot].slotObjectType = GR_SLOT_UNUSED;
                descriptors[baseSlot+m_ResMap.sampler[i].ilSlot].shaderEntityIndex = 0;
            }
            else
            {
                descriptors[baseSlot+m_ResMap.sampler[i].ilSlot].slotObjectType = GR_SLOT_SHADER_SAMPLER;
                descriptors[baseSlot+m_ResMap.sampler[i].ilSlot].shaderEntityIndex = m_ResMap.sampler[i].ilSlot;
            }
        }
    }
}


//
// Memory allocator
//
VOID* ILC_STDCALL Shader::MemAlloc(
    SIZE_T          dataSize,
    ILC_ALLOC_TYPE  allocType)
{
    void* pMem = NULL;

    switch (allocType)
    {
    case ILC_ALLOC_BINARY_IL:
        // for binary IL
        _ASSERT(dataSize % sizeof(UINT) == 0); // IL binary is always UINT-based stream
        break;
    case ILC_ALLOC_ERROR_MESSAGE:
        // for error message
        break;
    default:
        // unexpected type
        _ASSERT(0);
        break;
    }

    // Allocate memory based on the requested data size
    pMem = new BYTE[dataSize];

    return pMem;
}

//
// Memory freer
//
void Shader::MemFree(
    void* pMem)
{
    _ASSERT(pMem != NULL);
    delete[] pMem;
}

//
// Process shader object file and do IL conversion
//
void Shader::ProcessFile(
    const WCHAR* pFileName, ILC_VS_INPUT_LAYOUT& vsInputLayout, ILC_DX_RESOURCE_MAPPING& vsResMap)
{
    _ASSERT(pFileName != NULL);

    HANDLE hFile = CreateFile( pFileName,               // file to open
                                GENERIC_READ,           // open for reading
                                0,                      // share for reading
                                NULL,                   // default security
                                OPEN_EXISTING,          // existing file only
                                FILE_FLAG_DELETE_ON_CLOSE, // normal file
                                NULL);                  // no attr. template
    if (hFile != INVALID_HANDLE_VALUE) 
    { 
        LARGE_INTEGER fileSize;
        GetFileSizeEx( hFile, &fileSize );
        void* pData = new char[fileSize.LowPart];

        DWORD dwBytesRead = 0;
        if( FALSE != ReadFile(hFile, pData, fileSize.LowPart, &dwBytesRead, NULL) )
        {
            //
            // Prepare for IL conversion
            //

            // Shader source codes
            ILC_DATA shSrcCode = {0};
            shSrcCode.pData    = pData;
            shSrcCode.dataSize = fileSize.LowPart;

            // Metadata
            ILC_METADATA shMetadata = {0};
//            shMetadata.flags |= ILC_FLAG_EMBED_SOURCE_DASM;      // Embedded DASM text
//            shMetadata.flags |= ILC_FLAG_FORCE_DIRECT_TRANSLATE; // Force direct translation

            // Shader binary object and error message
            ILC_DATA shBinObjIl = {0};
            ILC_DATA errMsg     = {0};

            //
            // IL conversion
            //
            ILC_RESULT result = ConvertShader(
                                    &shSrcCode,
                                    &shMetadata,
                                    &vsInputLayout,
                                    &vsResMap,
                                    &shBinObjIl,
                                    &errMsg,
                                    MemAlloc);

            // Check the conversion result
            if (result == ILC_SUCCESS)
            {
                // call grCreateShader to generate a mantle shader
                GR_SHADER_CREATE_INFO cs_info = {};
                cs_info.pCode    = shBinObjIl.pData;
                cs_info.codeSize = shBinObjIl.dataSize;
                _grCreateShader(OriginIGO::getMantleDevice(m_gpu), &cs_info, &m_pipelineShaderInfo.shader);

                MemFree(shBinObjIl.pData);
            }
            else
            {
                // Conversion failed
                if (errMsg.dataSize > 0)
                {
                    // Output the error message
                    //printf("Conversion error: %s\n", reinterpret_cast<const char*>(errMsg.pData));

                    OutputDebugString(TEXT(__FUNCTION__)L": Conversion failed!\n");

                    // Free error message content
                    MemFree(errMsg.pData);
                    errMsg.pData    = NULL;
                    errMsg.dataSize = 0;
                }
                else
                {
                    OutputDebugString(TEXT(__FUNCTION__)L": Conversion failed!\n");
                }
            }
        }
        else
        {
            OutputDebugString(TEXT(__FUNCTION__)L": ReadFile failed.\n");
        }

        delete[] pData;
        CloseHandle( hFile);
    }
    else
    {
        OutputDebugString(TEXT(__FUNCTION__)L": CreateFile failed.\n");
    }
}
