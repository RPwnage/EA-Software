///////////////////////////////////////////////////////////////////////////////
// DX8Hook.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _WIN64

#include "IGO.h"
#include "DX8Hook.h"

#if defined(ORIGIN_PC)

#include <windows.h>
// Now that we support from XP to Windows 10, we can't include the DirectX 8 path directly in the build file, otherwise we'll get #define conflicts
//#include <d3d8.h>
#include "../../../../DirectX/8.1/include/d3d8.h"

#include "HookAPI.h"
#include "IGOApplication.h"
#include "InputHook.h"

#include "EASTL/hash_map.h"

#include "IGOTrace.h"
#include "IGOLogger.h"
#include "IGOSharedStructs.h"
#include "IGOTelemetry.h"

namespace OriginIGO {

    // render state
    DWORD gStateBlock = 0;
    EA::Thread::Futex DX8Hook::mInstanceHookMutex;
    extern DWORD gPresentHookThreadId;
    extern volatile DWORD gPresentHookCalled;
    extern volatile bool gTestCooperativeLevelHook_or_ResetHook_Called;


    // these are defined in DX9Hook.cpp
    // we share them so we don't have to was more space
    extern D3DMATRIX gMatOrtho;
    extern D3DMATRIX gMatIdentity;
    extern bool gInputHooked;
    extern volatile bool gQuitting;
    extern bool gSDKSignaled;

    bool DX8Hook::isHooked = false;
    bool DX8Hook::mDX8Initialized = false;
    LONG_PTR DX8Hook::mDX8Reset = NULL;
    LONG_PTR DX8Hook::mDX8Present = NULL;
    LONG_PTR DX8Hook::mDX8CreateDevice = NULL;
    DWORD DX8Hook::mLastHookTime = 0;

    void buildProjectionMatrixDX8(int width, int height, int n, int f)
    {
	    ZeroMemory(&gMatOrtho, sizeof(gMatOrtho));
	    ZeroMemory(&gMatIdentity, sizeof(gMatIdentity));
	    gMatIdentity._11 = gMatIdentity._22 = gMatIdentity._33 = gMatIdentity._44 = 1.0f;

	    gMatOrtho._11 = 2.0f/(width==0?1:width);
	    gMatOrtho._22 = 2.0f/(height==0?1:height);
	    gMatOrtho._33 = -2.0f/(n - f);
	    gMatOrtho._43 = 2.0f/(n - f);
	    gMatOrtho._44 = 1.0f;
    }

    wchar_t* D3D8errorValueToString(DWORD res)
    {
	    switch(res)
		    {
		    case D3DERR_WRONGTEXTUREFORMAT              : return L"D3DERR_WRONGTEXTUREFORMAT";
		    case D3DERR_UNSUPPORTEDCOLOROPERATION       : return L"D3DERR_UNSUPPORTEDCOLOROPERATION";
		    case D3DERR_UNSUPPORTEDCOLORARG             : return L"D3DERR_UNSUPPORTEDCOLORARG";
		    case D3DERR_UNSUPPORTEDALPHAOPERATION       : return L"D3DERR_UNSUPPORTEDALPHAOPERATION";
		    case D3DERR_UNSUPPORTEDALPHAARG             : return L"D3DERR_UNSUPPORTEDALPHAARG";
		    case D3DERR_TOOMANYOPERATIONS               : return L"D3DERR_TOOMANYOPERATIONS";
		    case D3DERR_CONFLICTINGTEXTUREFILTER        : return L"D3DERR_CONFLICTINGTEXTUREFILTER";
		    case D3DERR_UNSUPPORTEDFACTORVALUE          : return L"D3DERR_UNSUPPORTEDFACTORVALUE";
		    case D3DERR_CONFLICTINGRENDERSTATE          : return L"D3DERR_CONFLICTINGRENDERSTATE";
		    case D3DERR_UNSUPPORTEDTEXTUREFILTER        : return L"D3DERR_UNSUPPORTEDTEXTUREFILTER";
		    case D3DERR_CONFLICTINGTEXTUREPALETTE       : return L"D3DERR_CONFLICTINGTEXTUREPALETTE";
		    case D3DERR_DRIVERINTERNALERROR             : return L"D3DERR_DRIVERINTERNALERROR";
		    case D3DERR_NOTFOUND                        : return L"D3DERR_NOTFOUND";
		    case D3DERR_MOREDATA                        : return L"D3DERR_MOREDATA";
		    case D3DERR_DEVICELOST                      : return L"D3DERR_DEVICELOST";
		    case D3DERR_DEVICENOTRESET                  : return L"D3DERR_DEVICENOTRESET";
		    case D3DERR_NOTAVAILABLE                    : return L"D3DERR_NOTAVAILABLE";
		    case D3DERR_OUTOFVIDEOMEMORY                : return L"D3DERR_OUTOFVIDEOMEMORY";
		    case D3DERR_INVALIDDEVICE                   : return L"D3DERR_INVALIDDEVICE";
		    case D3DERR_INVALIDCALL                     : return L"D3DERR_INVALIDCALL";
		    case D3DERR_DRIVERINVALIDCALL               : return L"D3DERR_DRIVERINVALIDCALL";
		    case D3D_OK: return L"D3D_OK";
		    case E_FAIL: return L"E_FAIL";
		    case E_INVALIDARG: return L"E_INVALIDARG";
		    case E_NOINTERFACE: return L"E_NOINTERFACE";
		    case E_NOTIMPL: return L"E_NOTIMPL";
		    case E_OUTOFMEMORY: return L"E_OUTOFMEMORY"; 
		    default: return L"unknown error";
	    }
    }

    bool saveRenderState(IDirect3DDevice8 *pDev)
    {
        SAFE_CALL_LOCK_AUTO
    
        if (!gStateBlock)
	    {
		    HRESULT hr = pDev->CreateStateBlock(D3DSBT_ALL, &gStateBlock);
		    if (FAILED(hr))
		    {
			    IGO_TRACEF("CreateStateBlock() failed. error = %x", hr);
			    return false;
		    }
	    }

	    if (gStateBlock)
		    pDev->CaptureStateBlock(gStateBlock);

	    const float f0 = 0.0f;
	    const float f1 = 1.0f;
	    const float f64 = 64.0f;
	    const unsigned int *Float0 = reinterpret_cast<const unsigned int*>(&f0);
	    const unsigned int *Float1 = reinterpret_cast<const unsigned int*>(&f1);
	    const unsigned int *Float64 = reinterpret_cast<const unsigned int*>(&f64);

	    // clear out all the settings
	    pDev->SetPixelShader(NULL);
	    pDev->SetVertexShader(NULL);

	    pDev->SetRenderState( D3DRS_ZENABLE, FALSE ); // TRUE
	    pDev->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
	    pDev->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );              
	    pDev->SetRenderState( D3DRS_ZWRITEENABLE, FALSE ); // TRUE          
	    pDev->SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );     
	    pDev->SetRenderState( D3DRS_LASTPIXEL, TRUE );                
	    pDev->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );               
	    pDev->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );        
	    pDev->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW);               
	    pDev->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL);                
	    pDev->SetRenderState( D3DRS_ALPHAREF, 0x00000000 );                 
	    pDev->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_ALWAYS );               
	    pDev->SetRenderState( D3DRS_DITHERENABLE, FALSE );              
	    pDev->SetRenderState( D3DRS_ALPHABLENDENABLE , FALSE );         
	    pDev->SetRenderState( D3DRS_FOGENABLE, FALSE );               
	    pDev->SetRenderState( D3DRS_SPECULARENABLE, FALSE );        
	    pDev->SetRenderState( D3DRS_FOGCOLOR, 0x00FFFFFF );                  
	    pDev->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );              
	    pDev->SetRenderState( D3DRS_FOGSTART, (unsigned int) *Float0 );                  
	    pDev->SetRenderState( D3DRS_FOGEND, (unsigned int) *Float1 );                    
	    pDev->SetRenderState( D3DRS_FOGDENSITY, (unsigned int) *Float1 );                
	    pDev->SetRenderState( D3DRS_RANGEFOGENABLE, FALSE );            
	    pDev->SetRenderState( D3DRS_STENCILENABLE, FALSE );             
	    pDev->SetRenderState( D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP );              
	    pDev->SetRenderState( D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP );            
	    pDev->SetRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_KEEP );              
	    pDev->SetRenderState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );               
	    pDev->SetRenderState( D3DRS_STENCILREF, 0x00000000 );                
	    pDev->SetRenderState( D3DRS_STENCILMASK, 0xFFFFFFFF );              
	    pDev->SetRenderState( D3DRS_STENCILWRITEMASK, 0xFFFFFFFF );          
	    pDev->SetRenderState( D3DRS_TEXTUREFACTOR, 0xFFFFFFFF );             
	    pDev->SetRenderState( D3DRS_WRAP0, 0x00000000 );                     
	    pDev->SetRenderState( D3DRS_WRAP1, 0x00000000 );                     
	    pDev->SetRenderState( D3DRS_WRAP2, 0x00000000 );                     
	    pDev->SetRenderState( D3DRS_WRAP3, 0x00000000 );                     
	    pDev->SetRenderState( D3DRS_WRAP4, 0x00000000 );                     
	    pDev->SetRenderState( D3DRS_WRAP5, 0x00000000 );                     
	    pDev->SetRenderState( D3DRS_WRAP6, 0x00000000 );                     
	    pDev->SetRenderState( D3DRS_WRAP7, 0x00000000 );                     
	    pDev->SetRenderState( D3DRS_CLIPPING, TRUE );                  
	    pDev->SetRenderState( D3DRS_LIGHTING, FALSE);                  
	    pDev->SetRenderState( D3DRS_AMBIENT, 0x00000000 );                   
	    pDev->SetRenderState( D3DRS_FOGVERTEXMODE, D3DFOG_NONE );             
	    pDev->SetRenderState( D3DRS_COLORVERTEX, TRUE );               
	    pDev->SetRenderState( D3DRS_LOCALVIEWER, TRUE );               
	    pDev->SetRenderState( D3DRS_NORMALIZENORMALS, FALSE );          
	    pDev->SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1 );     
	    pDev->SetRenderState( D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR2 );    
	    pDev->SetRenderState( D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL );     
	    pDev->SetRenderState( D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL );    
	    pDev->SetRenderState( D3DRS_VERTEXBLEND, D3DVBF_DISABLE );               
	    pDev->SetRenderState( D3DRS_CLIPPLANEENABLE, 0x00000000 );           
	    pDev->SetRenderState( D3DRS_POINTSIZE, (unsigned int) *Float1 );                  
	    pDev->SetRenderState( D3DRS_POINTSIZE_MIN, (unsigned int) *Float1 );             
	    pDev->SetRenderState( D3DRS_POINTSPRITEENABLE, FALSE );          
	    pDev->SetRenderState( D3DRS_POINTSCALEENABLE, FALSE );          
	    pDev->SetRenderState( D3DRS_POINTSCALE_A, (unsigned int) *Float1 );               
	    pDev->SetRenderState( D3DRS_POINTSCALE_B, (unsigned int) *Float0 );               
	    pDev->SetRenderState( D3DRS_POINTSCALE_C, (unsigned int) *Float0 );               
	    pDev->SetRenderState( D3DRS_MULTISAMPLEANTIALIAS, TRUE );      
	    pDev->SetRenderState( D3DRS_MULTISAMPLEMASK, 0xFFFFFFFF );           
	    pDev->SetRenderState( D3DRS_PATCHEDGESTYLE, D3DPATCHEDGE_DISCRETE );            
	    pDev->SetRenderState( D3DRS_DEBUGMONITORTOKEN, D3DDMT_DISABLE );         
	    pDev->SetRenderState( D3DRS_POINTSIZE_MAX, (unsigned int) *Float64 );             
	    pDev->SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE );  
	    pDev->SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_RED );         
	    pDev->SetRenderState( D3DRS_TWEENFACTOR, (unsigned int) *Float0 );               
	    pDev->SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );       
	    /*
	    pDev->SetRenderState( D3DRS_POSITIONDEGREE, D3DDEGREE_CUBIC );            
	    pDev->SetRenderState( D3DRS_NORMALDEGREE, D3DDEGREE_LINEAR );              
	    pDev->SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );         
	    pDev->SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS, 0x00000000 );      
	    pDev->SetRenderState( D3DRS_ANTIALIASEDLINEENABLE, FALSE );     
	    pDev->SetRenderState( D3DRS_MINTESSELLATIONLEVEL, (unsigned int) *Float1 );      
	    pDev->SetRenderState( D3DRS_MAXTESSELLATIONLEVEL, (unsigned int) *Float1 );      
	    pDev->SetRenderState( D3DRS_ADAPTIVETESS_X, (unsigned int) *Float0 );            
	    pDev->SetRenderState( D3DRS_ADAPTIVETESS_Y, (unsigned int) *Float0 );            
	    pDev->SetRenderState( D3DRS_ADAPTIVETESS_Z, (unsigned int) *Float1 );            
	    pDev->SetRenderState( D3DRS_ADAPTIVETESS_W, (unsigned int) *Float0 );            
	    pDev->SetRenderState( D3DRS_ENABLEADAPTIVETESSELLATION, FALSE ); 
	    pDev->SetRenderState( D3DRS_TWOSIDEDSTENCILMODE, FALSE );       
	    pDev->SetRenderState( D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_KEEP );           
	    pDev->SetRenderState( D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_KEEP );           
	    pDev->SetRenderState( D3DRS_CCW_STENCILPASS, D3DSTENCILOP_KEEP );            
	    pDev->SetRenderState( D3DRS_CCW_STENCILFUNC, D3DCMP_ALWAYS );            
	    pDev->SetRenderState( D3DRS_COLORWRITEENABLE1, 0x0000000F );          
	    pDev->SetRenderState( D3DRS_COLORWRITEENABLE2, 0x0000000F );          
	    pDev->SetRenderState( D3DRS_COLORWRITEENABLE3, 0x0000000F );          
	    pDev->SetRenderState( D3DRS_BLENDFACTOR, 0xFFFFFFFF );                
	    pDev->SetRenderState( D3DRS_SRGBWRITEENABLE, FALSE );            
	    pDev->SetRenderState( D3DRS_DEPTHBIAS, (unsigned int) *Float0 );                  
	    pDev->SetRenderState( D3DRS_WRAP8, 0x00000000 );                     
	    pDev->SetRenderState( D3DRS_WRAP9, 0x00000000 );                     
	    pDev->SetRenderState( D3DRS_WRAP10, 0x00000000 );                    
	    pDev->SetRenderState( D3DRS_WRAP11, 0x00000000 );                    
	    pDev->SetRenderState( D3DRS_WRAP12, 0x00000000 );                    
	    pDev->SetRenderState( D3DRS_WRAP13, 0x00000000 );                    
	    pDev->SetRenderState( D3DRS_WRAP14, 0x00000000 );                    
	    pDev->SetRenderState( D3DRS_WRAP15, 0x00000000 );                    
	    pDev->SetRenderState( D3DRS_SEPARATEALPHABLENDENABLE, FALSE );  
	    pDev->SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_ONE );             
	    pDev->SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_ZERO );            
	    pDev->SetRenderState( D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD );              
	    */

	    for (DWORD i = 0; i < 8; i++)
	    {
		    pDev->SetTexture(i, NULL );
	    }

	    /*
	    for (DWORD i = 0; i < 16; i++)
	    {
		    pDev->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
		    pDev->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
		    pDev->SetSamplerState(i, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP );
		    pDev->SetSamplerState(i, D3DSAMP_BORDERCOLOR, 0x00000000 );
		    pDev->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
		    pDev->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_POINT );
		    pDev->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
		    pDev->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, (unsigned int) *Float0 );
		    pDev->SetSamplerState(i, D3DSAMP_MAXMIPLEVEL, 0x00000000 );
		    pDev->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, 0x00000001 );
		    pDev->SetSamplerState(i, D3DSAMP_SRGBTEXTURE, FALSE );
		    pDev->SetSamplerState(i, D3DSAMP_ELEMENTINDEX, 0x00000000 );
		    pDev->SetSamplerState(i, D3DSAMP_DMAPOFFSET, 256 );
	    }
	    */

	    for (DWORD i = 0; i < 8; i++)
	    {
		    pDev->SetTextureStageState(i, D3DTSS_COLOROP, i==0?D3DTOP_MODULATE:D3DTOP_DISABLE );
		    pDev->SetTextureStageState(i, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		    pDev->SetTextureStageState(i, D3DTSS_COLORARG2, D3DTA_CURRENT );
		    pDev->SetTextureStageState(i, D3DTSS_ALPHAOP, i==0?D3DTOP_SELECTARG1:D3DTOP_DISABLE );
		    pDev->SetTextureStageState(i, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		    pDev->SetTextureStageState(i, D3DTSS_ALPHAARG2, D3DTA_CURRENT );
		    pDev->SetTextureStageState(i, D3DTSS_BUMPENVMAT00, (unsigned int) *Float0 );
		    pDev->SetTextureStageState(i, D3DTSS_BUMPENVMAT01, (unsigned int) *Float0 );
		    pDev->SetTextureStageState(i, D3DTSS_BUMPENVMAT10, (unsigned int) *Float0 );
		    pDev->SetTextureStageState(i, D3DTSS_BUMPENVMAT11, (unsigned int) *Float0 );
		    pDev->SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, i );
		    pDev->SetTextureStageState(i, D3DTSS_BUMPENVLSCALE, (unsigned int) *Float0 );
		    pDev->SetTextureStageState(i, D3DTSS_BUMPENVLOFFSET, (unsigned int) *Float0 );
		    pDev->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE );
		    pDev->SetTextureStageState(i, D3DTSS_COLORARG0,  D3DTA_CURRENT );
		    pDev->SetTextureStageState(i, D3DTSS_ALPHAARG0, D3DTA_CURRENT );
		    pDev->SetTextureStageState(i, D3DTSS_RESULTARG, D3DTA_CURRENT );
		    //pDev->SetTextureStageState(i, D3DTSS_CONSTANT, 0xFFFFFFFF );	
	    }

	    // Setup 
	    pDev->SetVertexShader((D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1));

	    // Setup blending
	    pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	    pDev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	    pDev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	    pDev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD );

	    //CurrentDevice->SetTexture(0, g_renderTargetTexture);

	    // setup texture addressing settings
	    //pDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	    //pDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	    //pDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	    //pDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	    //pDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	    // setup colour calculations
	    pDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	    pDev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	    pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);

	    // setup alpha calculations
	    pDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	    pDev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	    pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

	    pDev->SetTransform(D3DTS_PROJECTION, &gMatOrtho);
	    pDev->SetTransform(D3DTS_WORLD, &gMatIdentity);
	    pDev->SetTransform(D3DTS_VIEW, &gMatIdentity);

	
	    D3DVIEWPORT8 vp;
	    vp.X = vp.Y = 0;
	    vp.Width = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth);
	    vp.Height = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight);
	    vp.MaxZ = 1.0f;
	    vp.MinZ = 0.0f;
		if (vp.Height > 0 && vp.Width > 0)
			pDev->SetViewport(&vp);

	    return true;
    }

    void restoreRenderState(IDirect3DDevice8 *pDev)
    {
	    if (gStateBlock)
		    pDev->ApplyStateBlock(gStateBlock);
    }

    void updateWindowScreenSizeDX8()
    {
	    RECT rect = {0};
	    GetClientRect(gRenderingWindow, &rect);
	    UINT width = rect.right - rect.left;
	    UINT height = rect.bottom - rect.top;
        SAFE_CALL_LOCK_AUTO
	    if(SAFE_CALL(IGOApplication::instance(), &IGOApplication::getWindowScreenWidth) != width || SAFE_CALL(IGOApplication::instance(), &IGOApplication::getWindowScreenHeight) != height )
	    {
		    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
		    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowScreenSize, width, height);
	    }
    }


    DEFINE_HOOK_SAFE(HRESULT, CreateDevice8Hook, (IDirect3D8 *pd3d8,  UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice8** ppReturnedDeviceInterface))
    
	    IGO_TRACE("CreateDevice8Hook() called");
	    IGOLogInfo("CreateDevice8Hook() called");

	    HRESULT hr = CreateDevice8HookNext(pd3d8, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
	
	    gWindowedMode = pPresentationParameters->Windowed == TRUE;

	    return hr;
    }


    // Present Hook
    DEFINE_HOOK_SAFE(HRESULT, Present8Hook, (LPDIRECT3DDEVICE8 pDev, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion))
    
        gPresentHookCalled = GetTickCount();
        IGO_TRACE("Present8Hook() called");
	    IGOLogInfo("Present8Hook() called");

	    UNHOOK_SAFE(CreateDevice8Hook); // UNHOOK_SAFE CreateDevice8Hook here, to not cause problems with PunkBuster
	    // because we may never get a call to CreateDevice8Hook, because IGO's attaching might be too slow, so we
	    // can miss the device creation call
	
        if( IGOApplication::isPendingDeletion())
        {
            IGOApplication::deleteInstance();
            InputHook::CleanupLater();
        }
        
        if (!gInputHooked && !gQuitting)
		    InputHook::TryHookLater(&gInputHooked);

	    // create the IGO IGOApplication::instance()
	    if (!IGOApplication::instance())
        {
            HRESULT hr = Present8HookNext(pDev, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
		    if (!IGOApplication::instance())
                IGOApplication::createInstance(RendererDX8, pDev);

	        D3DDEVICE_CREATION_PARAMETERS d3dcp;
	        pDev->GetCreationParameters(&d3dcp);
	        gRenderingWindow = d3dcp.hFocusWindow;
            gPresentHookThreadId = GetCurrentThreadId();

            return hr;
        }

        // verify that we can actually render
	    HRESULT hr=pDev->TestCooperativeLevel();
	    if (hr == D3D_OK)
	    {
            // hook windows message
            if (!gQuitting)
            {
	            InputHook::HookWindow(gRenderingWindow);
            }

		    RECT rect;
		    GetWindowRect(gRenderingWindow, &rect);
		    if (((rect.right - rect.left) * (rect.bottom-rect.top)) != 0) 
		    {
				int width = 0;
				int height = 0;
                if (pSourceRect)
                {
                    int srcWidth = (pSourceRect->right - pSourceRect->left);
                    int srcHeight = (pSourceRect->bottom - pSourceRect->top);
                    if (srcWidth > 0 && srcHeight > 0)
                    {
                        width = srcWidth;
                        height = srcHeight;
                    }
                }

                if (width == 0 || height == 0)
                {
				    IDirect3DSurface8 *bb = NULL;
				    hr = pDev->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &bb);
				    if (SUCCEEDED(hr))
				    {
					    D3DSURFACE_DESC desc;
					    bb->GetDesc(&desc);
					    width = desc.Width;
					    height = desc.Height;
					    bb->Release();
				    }
                }

				SAFE_CALL_LOCK_ENTER
                SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
				SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, width, height);
				SAFE_CALL_LOCK_LEAVE
				buildProjectionMatrixDX8(width, height, 1, 10);

				updateWindowScreenSizeDX8();

                    
                SAFE_CALL_LOCK_ENTER
			    SAFE_CALL(IGOApplication::instance(), &IGOApplication::startTime);
                SAFE_CALL_LOCK_LEAVE
			    if (saveRenderState(pDev))
			    {
				    if (pDev->BeginScene() == D3D_OK)
				    {
                        SAFE_CALL_LOCK_ENTER
					    SAFE_CALL(IGOApplication::instance(), &IGOApplication::render, pDev);
                        SAFE_CALL_LOCK_LEAVE

					    pDev->EndScene();
				    }
				    restoreRenderState(pDev);
			    }
                SAFE_CALL_LOCK_ENTER
			    SAFE_CALL(IGOApplication::instance(), &IGOApplication::stopTime);
                SAFE_CALL_LOCK_LEAVE
		    }
	    }
	    return Present8HookNext(pDev, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    }


    // Release Hook
    DEFINE_HOOK_SAFE(ULONG, Release8Hook, (LPDIRECT3DDEVICE8 pDev))
    
	    static bool bInRelease = false;

	    pDev->AddRef();
	    ULONG currRefCount = Release8HookNext(pDev);

	    if (bInRelease)
		    return Release8HookNext(pDev);

	    //IGO_TRACEF("Release = %d", currRefCount);

	    // 2 for texture & vertex buffer
	    // 1 for render state
	    // 1 for current number
        bool locked = SAFE_CALL_LOCK_TRYREADLOCK_COND;    // DX8 release is called very often from many threads, so keep an eye on the write lock!!!
        if (locked)
        {
            if (IGOApplication::instance())
	        {
		        ULONG refCount = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getWindowCount)*2 + 1 + 1;

		        if (currRefCount == refCount)
		        {
			        bInRelease = true;
			        // The device is being destroyed
					SAFE_CALL(IGOApplication::instance(), &IGOApplication::clearWindows, NULL);
			        if (gStateBlock)
			        {
				        pDev->DeleteStateBlock(gStateBlock);
				        gStateBlock = 0;
			        }

			        bInRelease = false;
		        }
	        }
            SAFE_CALL_LOCK_LEAVE
        }
	    ULONG result = Release8HookNext(pDev);
	    return result;
    }

    // Reset 8Hook
    DEFINE_HOOK_SAFE(HRESULT, Reset8Hook, (LPDIRECT3DDEVICE8 pDev, D3DPRESENT_PARAMETERS *pPresentationParameters))
    
        IGOLogInfo("Reset8Hook() called");
        gTestCooperativeLevelHook_or_ResetHook_Called = true;

        // clear all objects
        if (gStateBlock)
        {
            pDev->DeleteStateBlock(gStateBlock);
            gStateBlock = 0;
        }

        IGOApplication::deleteInstance();
        InputHook::CleanupLater(); 

        if (pPresentationParameters)
		    gWindowedMode = pPresentationParameters->Windowed==TRUE;
	
	    IGOLogInfo("game is windowed = %d", gWindowedMode==true?1:0);
	    HRESULT hr = Reset8HookNext(pDev, pPresentationParameters);

        return hr;
    }

    DX8Hook::DX8Hook()
    {
        isHooked = false;
    }


    DX8Hook::~DX8Hook()
    {
    }

    bool DX8Hook::IsPrecalculationDone()
    {
        return mDX8Reset && mDX8Present && mDX8CreateDevice;
    }

    PRE_DEFINE_HOOK_SAFE(IDirect3D8*, Direct3DCreate8Hook, (UINT SDK_VERSION));

    bool D3D8Init(HMODULE hD3D8, bool calculateOffsetsOnly)
    {
	    IGOLogInfo("D3D8Init(%i) called", calculateOffsetsOnly);

        if (DX8Hook::IsBroken())
            return false;

        if (DX8Hook::IsPrecalculationDone())
        {
            IGOLogWarn("Using precalculated offsets.");

            if (!isReset8Hooked)
                HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D8 + (LONG_PTR)DX8Hook::mDX8Reset), Reset8Hook);

            if (!isPresent8Hooked)
                HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D8 + (LONG_PTR)DX8Hook::mDX8Present), Present8Hook);

            if (!isCreateDevice8Hooked)
                HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D8 + (LONG_PTR)DX8Hook::mDX8CreateDevice), CreateDevice8Hook);

			if (!isPresent8Hooked || !isReset8Hooked || !isCreateDevice8Hooked)
			{
				CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_MHOOK, TelemetryRenderer_DX9, "Hooking DX8 via precalculated offsets failed.  (3rd party tool running?)"));

				UNHOOK_SAFE(Present8Hook);
				UNHOOK_SAFE(Reset8Hook);
				UNHOOK_SAFE(CreateDevice8Hook);
				return false;
			}
            else
                return true;
        }

        else
        {
            if (!calculateOffsetsOnly)
            {
                IGOLogWarn("No precalculated offsets available, trying direct mode.");
            }
        }


        UNHOOK_SAFE(Present8Hook);
        //UNHOOK_SAFE(Release8Hook);
        UNHOOK_SAFE(Reset8Hook);
        UNHOOK_SAFE(CreateDevice8Hook);

        if (!isDirect3DCreate8Hooked || !Direct3DCreate8HookNext)
        {
            // a special case, if we are loaded by the SDK and no offsets have been calculated, try this workaround
			// During dev, the game team may start the game outside Origin -> the injection won't happen immediately as when started from Origin -> the Direct3D Dlls may be loaded by the time the game loads OIG -> we wouldn't get the trigger we're waiting for to hook the CreateDevice methods and OIG would never work; so we use an external signal to let us know whether this is an SDK game and we should take the risk of not using our pre-computed offsets.
            if (gSDKSignaled && !calculateOffsetsOnly)
            {
                HOOKAPI_SAFE("d3d8.dll", "Direct3DCreate8", Direct3DCreate8Hook);   // do D3D8Init(...) from the create call and calculate the offsets from there!!!
                if (!isDirect3DCreate8Hooked || !Direct3DCreate8HookNext)
                {
                    IGOLogWarn("Hooking Direct3DCreat8 failed. Aborting IGO.");
                    return false;
                }
            }
            else
                return false;
        }

        LPDIRECT3DDEVICE8 pDevice = NULL;

        WNDCLASSEX wc;

        // clear out the window class for use
        ZeroMemory(&wc, sizeof(WNDCLASSEX));

        HINSTANCE hInstance = GetModuleHandle(NULL);

        // fill in the struct with the needed information
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
        wc.lpszClassName = L"IGO Window Class DX8";

        RegisterClassEx(&wc);

        // create the window and use the result as the handle
        HWND hWnd = CreateWindowEx(NULL,
            L"IGO Window Class DX8",    // name of the window class
            L"IGO Window Title DX8",   // title of the window
            WS_OVERLAPPEDWINDOW,    // window style
            300,    // x-position of the window
            300,    // y-position of the window
            500,    // width of the window
            400,    // height of the window
            NULL,    // we have no parent window, NULL
            NULL,    // we aren't using menus, NULL
            hInstance,    // application handle
            NULL);    // used with multiple windows, NULL


        IDirect3D8 *d3d8 = NULL;
        d3d8 = Direct3DCreate8HookNext(D3D_SDK_VERSION);
        if (d3d8)
        {
            D3DDISPLAYMODE currentDisplayMode = { 0 };
            HRESULT hr = d3d8->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &currentDisplayMode);
            if (SUCCEEDED(hr))
            {
                // common back buffer formats
                D3DFORMAT bbFormats[] = {
                        currentDisplayMode.Format,
                        D3DFMT_R5G6B5,
                        D3DFMT_X1R5G5B5,
                        D3DFMT_A1R5G5B5,
                        D3DFMT_A8R8G8B8,
                        D3DFMT_X8R8G8B8,
                    };

                int formatIndex = -1;
                for (int i = 0; i < sizeof(bbFormats) / sizeof(bbFormats[0]); i++)
                {
                    hr = d3d8->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, currentDisplayMode.Format, bbFormats[i], TRUE);
                    if (SUCCEEDED(hr))
                    {
                        formatIndex = i;
                        break;
                    }
                }

                if (SUCCEEDED(hr) && formatIndex != -1)
                {
                    unsigned int s1 = 0;
                    unsigned int s2 = 0;
#ifdef _WIN64
                    _controlfp_s(&s1, 0, 0);
#else
                    __control87_2(0, 0, &s1, &s2);
#endif
                    IGOLogWarn("FPU (before): %x %x", s1, s2);

                    D3DPRESENT_PARAMETERS d3dpp;
                    ZeroMemory(&d3dpp, sizeof(d3dpp));

                    d3dpp.BackBufferWidth = 0;
                    d3dpp.BackBufferHeight = 0;
                    d3dpp.BackBufferFormat = bbFormats[formatIndex];;
                    d3dpp.BackBufferCount = 1;
                    d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
                    d3dpp.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
                    d3dpp.Flags = 0;
                    d3dpp.Windowed = TRUE;
                    d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
                    d3dpp.hDeviceWindow = hWnd;

                    hr = d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &pDevice); 
                    if (FAILED(hr) || !pDevice)
                    {
                        IGOLogWarn("D3DCREATE_HARDWARE_VERTEXPROCESSING failed, trying D3DCREATE_MIXED_VERTEXPROCESSING %S.", D3D8errorValueToString(hr));
                        hr = d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &pDevice);

                        if (FAILED(hr) || !pDevice)
                        {
                            IGOLogWarn("D3DCREATE_MIXED_VERTEXPROCESSING failed, trying D3DCREATE_SOFTWARE_VERTEXPROCESSING %S.", D3D8errorValueToString(hr));
                            hr = d3d8->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice);

						    if (FAILED(hr) || !pDevice)
						    {
							    IGOLogWarn("D3DCREATE_SOFTWARE_VERTEXPROCESSING failed, trying precalculated offsets %S.", D3D8errorValueToString(hr));
							    IGOLogWarn("mDX8Reset %p mDX8Present %p mDX8CreateDevice %p", DX8Hook::mDX8Reset, DX8Hook::mDX8Present, DX8Hook::mDX8CreateDevice);
							    if (DX8Hook::mDX8Reset && DX8Hook::mDX8Present && DX8Hook::mDX8CreateDevice)
							    {
                                    HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D8 + (LONG_PTR)DX8Hook::mDX8Reset), Reset8Hook);
                                    HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D8 + (LONG_PTR)DX8Hook::mDX8Present), Present8Hook);
                                    HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D8 + (LONG_PTR)DX8Hook::mDX8CreateDevice), CreateDevice8Hook);

								    d3d8->Release();

								    DestroyWindow(hWnd);
								    UnregisterClass(L"IGO Window Class DX8", hInstance);
								    if (!isPresent8Hooked || !isReset8Hooked || !isCreateDevice8Hooked)
								    {
									    IGOLogWarn("Hooking DX8 via precalculated offsets failed. Quiting IGO.");

									    UNHOOK_SAFE(Present8Hook);
									    UNHOOK_SAFE(Reset8Hook);
									    UNHOOK_SAFE(CreateDevice8Hook);
									    return false;
								    }

								    return true;
							    }
							    else
								    IGOLogWarn("No precalculated offsets available.");
						    }
                        }
                    }

                    unsigned int s3 = 0;
                    unsigned int s4 = 0;
#ifdef _WIN64
                    _controlfp_s(&s3, 0, 0);
                    IGOLogWarn("FPU (after): %x %x", s3, s4);
                    // SSE
                    _controlfp_s(&s3, s1, _MCW_EM);
                    _controlfp_s(&s3, s1, _MCW_RC);
                    _controlfp_s(&s3, s1, _MCW_DN);

                    _controlfp_s(&s3, 0, 0);
#else
                    __control87_2(0, 0, &s3, &s4);
                    IGOLogWarn("FPU (after): %x %x", s3, s4);
                    // x87
                    __control87_2(s1, _MCW_EM, &s3, 0);
                    __control87_2(s1, _MCW_RC, &s3, 0);
                    __control87_2(s1, _MCW_PC, &s3, 0);
                    __control87_2(s1, _MCW_IC, &s3, 0);
                    __control87_2(s1, _MCW_DN, &s3, 0);

                    // SSE
                    __control87_2(s2, _MCW_EM, 0, &s4);
                    __control87_2(s2, _MCW_RC, 0, &s4);
                    __control87_2(s2, _MCW_PC, 0, &s4);
                    __control87_2(s2, _MCW_IC, 0, &s4);
                    __control87_2(s2, _MCW_DN, 0, &s4);

                    __control87_2(0, 0, &s3, &s4);
#endif
                    IGOLogWarn("FPU (after reset): %x %x", s3, s4);

                    // Hook the device
                    if (SUCCEEDED(hr) && pDevice)
                    {
                        hr = pDevice->TestCooperativeLevel();
                        if (SUCCEEDED(hr))
                        {
                            pDevice->Present(0, 0, 0, 0);   // init the API, just in case it is hooked by some other DLL!

                            // CreateDevice is required, so that we can get the windowed / fullscreen flags from DX8 - after that, we UNHOOK_SAFE it, because of PunkBuster!
                            if (calculateOffsetsOnly)
                                DX8Hook::mDX8CreateDevice = (LONG_PTR)GetInterfaceMethod(d3d8, 15) - (LONG_PTR)hD3D8;
                            else
                                HOOKCODE_SAFE((LPVOID)GetInterfaceMethod(d3d8, 15), CreateDevice8Hook);

                            if (isCreateDevice8Hooked || calculateOffsetsOnly)
                            {
                                // present
                                if (calculateOffsetsOnly)
                                    DX8Hook::mDX8Present = (LONG_PTR)GetInterfaceMethod(pDevice, 15) - (LONG_PTR)hD3D8;
                                else
                                    HOOKCODE_SAFE((LPVOID)GetInterfaceMethod(pDevice, 15), Present8Hook);

                                if (isPresent8Hooked || calculateOffsetsOnly)
                                {
                                    // reset
                                    if (calculateOffsetsOnly)
                                        DX8Hook::mDX8Reset = (LONG_PTR)GetInterfaceMethod(pDevice, 14) - (LONG_PTR)hD3D8;
                                    else
                                        HOOKCODE_SAFE((LPVOID)GetInterfaceMethod(pDevice, 14), Reset8Hook);
                                }

                                // release
                                //HOOKCODE_SAFE((LPVOID)GetInterfaceMethod(pDevice, 2), Release8Hook);
                            }
                        }
                        else
                            IGOLogWarn("TestCooperativeLevel() failed. hr = %S", D3D8errorValueToString(hr));

                        if (Release8HookNext)
                            Release8HookNext(pDevice);
                        else
                            pDevice->Release();
                    }

                    else
                    {
                        IGOLogWarn("CreateDevice() failed. hr = %S", D3D8errorValueToString(hr));
                    }
                }

                else
                {
                    IGOLogWarn("CheckDeviceType() failed. hr = %S", D3D8errorValueToString(hr));
                }
            }

            else
            {
                IGOLogWarn("GetAdapterDisplayMode() failed. hr = %S", D3D8errorValueToString(hr));
            }

            d3d8->Release();
        }
        else
        {
            IGOLogWarn("Direct3DCreate8() failed");
        }

        DestroyWindow(hWnd);
        UnregisterClass(L"IGO Window Class DX8", hInstance);

        if (!isCreateDevice8Hooked || !isPresent8Hooked || !isReset8Hooked)
	    {
            if (!calculateOffsetsOnly)
                IGOLogWarn("Hooking DX8 failed. Quitting IGO.");

		    UNHOOK_SAFE(Present8Hook);
		    //UNHOOK_SAFE(Release8Hook);
		    UNHOOK_SAFE(Reset8Hook);
		    UNHOOK_SAFE(CreateDevice8Hook);
            return false;
	    }

        return true;
    }

    PRE_DEFINED_HOOK_SAFE_NO_CHECK(IDirect3D8*, Direct3DCreate8Hook, (UINT SDK_VERSION))

        IGOLogWarn("Direct3DCreate8 called...");
        if (!OriginIGO::CheckHook((LPVOID *)&(Direct3DCreate8HookNext), Direct3DCreate8Hook))
        {
            char details[128] = {0};
            OriginIGO::GetLastHookErrorDetails(details, sizeof(details));
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_MHOOK, TelemetryRenderer_DX8, "Direct3DCreate8 hook check failed (nVidia/detoured.dll?) - %s", details));
        }

        EA::Thread::AutoFutex lock(DX8Hook::mInstanceHookMutex);
        DX8Hook::mDX8Initialized = true;

        if (!DX8Hook::IsPrecalculationDone())
        {
            HMODULE hD3D8 = GetModuleHandle(L"d3d8.dll");
            if (hD3D8)
            {
                D3D8Init(hD3D8, true);
            }
        }
        return Direct3DCreate8HookNext(SDK_VERSION);
    }

    bool DX8Hook::TryHook(bool calculateOffsetsOnly/* = false*/)
    {
        if (DX8Hook::mInstanceHookMutex.TryLock())
        {
            // fix for a special recursive error, if we are called from Direct3DCreate8Hook, UNHOOK_SAFE(Direct3DCreate8Hook) will hang
            // https://developer.origin.com/support/browse/EBIBUGS-28931
            if (Direct3DCreate8HookMutex.IsReadLocked())
            {
                DX8Hook::mInstanceHookMutex.Unlock();
                return isHooked;
            }

            mLastHookTime = GetTickCount();

            if (calculateOffsetsOnly)
            {
                DX8Hook::mDX8Initialized = false;

                UNHOOK_SAFE(Direct3DCreate8Hook);
                HOOKAPI_SAFE("d3d8.dll", "Direct3DCreate8", Direct3DCreate8Hook);   // do D3D8Init(...) from the create call and calculate the offsets from there!!!
            }
            else
            {
    	        // do not hook another GFX api, if we already have IGO running in this process!!!
                SAFE_CALL_LOCK_AUTO
	            if (IGOApplication::instance()!=NULL && (IGOApplication::instance()->rendererType() != RendererDX8))
                {
                    UNHOOK_SAFE(Direct3DCreate8Hook);   // unhook this call, otherwise PunkBuster will kick us!
                    DX8Hook::mInstanceHookMutex.Unlock();
		            return false;
                }

                HMODULE hD3D8D = GetModuleHandle(L"d3d8.dll");
                if (hD3D8D)
                {
	                isHooked = D3D8Init(hD3D8D, calculateOffsetsOnly) && !calculateOffsetsOnly;  // do not set isHooked, when we use calculateOffsetsOnly
                    if (isHooked)
                       CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_GENERIC, TelemetryRenderer_DX8, "Hooked at least once"))

                    UNHOOK_SAFE(Direct3DCreate8Hook);   // unhook this call, otherwise PunkBuster will kick us!
                }
            }

            DX8Hook::mInstanceHookMutex.Unlock();
        }
	    return isHooked;
    }

    bool DX8Hook::IsReadyForRehook()
    {
        // limit re-hooking to have a 15 second timeout!
        EA::Thread::AutoFutex lock(DX8Hook::mInstanceHookMutex);
        if (
            (!DX8Hook::mDX8Initialized && !gSDKSignaled)    // if we have a signal from the SDK, don't rely on mDX8Initialized, because we may never reach Direct3DCreate8Hook
            || (GetTickCount() - mLastHookTime < REHOOKCHECK_DELAY))
            return false;

        return true;
    }

    bool DX8Hook::IsBroken()
    {
        bool broken = false;

        if (!isHooked)
            return broken;

        if (!OriginIGO::CheckHook((LPVOID *)&(CreateDevice8HookNext), CreateDevice8Hook, true) ||
            !OriginIGO::CheckHook((LPVOID *)&(Reset8HookNext), Reset8Hook, true) ||
            //!OriginIGO::CheckHook((LPVOID *)&(Release8HookNext), Release8Hook, true) ||
            !OriginIGO::CheckHook((LPVOID *)&(Present8HookNext), Present8Hook, true))
        {
            IGOLogWarn("DX8Hook broken.");
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_THIRD_PARTY, TelemetryRenderer_DX8, "DX8Hook is broken."));
            broken = true;

            // disable logging, if IGO is broken
            if (OriginIGO::IGOLogger::instance())
            {
                OriginIGO::IGOLogger::instance()->enableLogging(getIGOConfigurationFlags().gEnabledLogging);
            }

        }

        return broken;
    }


    void DX8Hook::Cleanup()
    {
        DX8Hook::mInstanceHookMutex.Lock();
        if (!IsBroken())
        {
            UNHOOK_SAFE(Present8Hook);
            //UNHOOK_SAFE(Release8Hook);
            UNHOOK_SAFE(Reset8Hook);
            UNHOOK_SAFE(CreateDevice8Hook);

            isHooked = false;
        }

        // only kill IGO if it was created by this render
        SAFE_CALL_LOCK_ENTER
            if (IGOApplication::instance() != NULL && (IGOApplication::instance()->rendererType() == RendererDX8))
            {
            bool safeToCall = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isThreadSafe);
            SAFE_CALL_LOCK_LEAVE

                if (safeToCall)
                    IGOApplication::deleteInstance();

                else
                    IGOApplication::deleteInstanceLater();

            }
            else
            {
                SAFE_CALL_LOCK_LEAVE
            }

        DX8Hook::mInstanceHookMutex.Unlock();
    }
}
#endif // ORIGIN_PC

#endif // _WIN64
