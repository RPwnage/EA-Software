///////////////////////////////////////////////////////////////////////////////
// DX9Hook.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGO.h"
#include "DX9Hook.h"

#if defined(ORIGIN_PC)

#include <windows.h>
#include <d3d9.h>
#include <d3dx9effect.h>

#include "HookAPI.h"
#include "IGOApplication.h"
#include "InputHook.h"
#include "resource.h"

#include "EASTL/hash_map.h"

#include "IGOTrace.h"
#include "IGOLogger.h"
#include "Helpers.h"

#include "ScreenCopyDx9.h"
#include "twitchsdk.h"  // always include it before TwitchManager.h !!!
#include "TwitchManager.h"
#include "IGOSharedStructs.h"
#include "IGOTelemetry.h"

namespace OriginIGO{

    extern HWND gHwnd;

    static enum DX9Hook::ShaderSupport gShaderSupport = DX9Hook::ShaderSupport_UNKNOWN;

    // For shader
    ID3DXEffect *gDX9Effect = NULL;
    D3DXHANDLE gDX9EffectTechnique = NULL;
    D3DXHANDLE gDX9WorldVariable = NULL;
    D3DXHANDLE gDX9ViewProjectionVariable = NULL;
    D3DXHANDLE gDX9ColorVariable = NULL;
    D3DXHANDLE gDX9TextureVariable = NULL;
    // render state
    IDirect3DStateBlock9* gStateBlock = NULL;
    IDirect3DStateBlock9* gIGOStateBlock = NULL;
    D3DGAMMARAMP gSavedGammaRamp;
    D3DGAMMARAMP gDefaultGammaRamp;
    bool gIGOActiveState = false;
    bool gIGOSaveGammaState = false;
    bool gResetCheck = false;
    void* gRenderingDeviceDX9 = NULL;
    EA::Thread::Futex DX9Hook::mInstanceHookMutex;
    D3DMATRIX gMatOrtho;
    D3DMATRIX gMatIdentity;
    D3DMATRIX gMatWorld;
    D3DXMATRIX gDX9MatOrtho;
    D3DXMATRIX gDX9MatWorld;
    LPDIRECT3DVERTEXDECLARATION9 gDX9VertexLayout;
    bool gBackBufferUsable = true;

    extern HINSTANCE gInstDLL;
    extern bool gInputHooked;
    extern volatile bool gQuitting;
    extern bool gSDKSignaled;
    extern DWORD gPresentHookThreadId;
    extern volatile DWORD gPresentHookCalled;
    extern volatile bool gTestCooperativeLevelHook_or_ResetHook_Called;
    extern bool gLastTestCooperativeLevelError;
    
    bool DX9Hook::isHooked = false;
    bool DX9Hook::mDX9Initialized = false;
    LONG_PTR DX9Hook::mDX9Release = NULL;
    LONG_PTR DX9Hook::mDX9TestCooperativeLevel = NULL;
    LONG_PTR DX9Hook::mDX9Reset = NULL;
    LONG_PTR DX9Hook::mDX9SwapChainPresent = NULL;
    LONG_PTR DX9Hook::mDX9Present = NULL;
    LONG_PTR DX9Hook::mDX9SetGammaRamp = NULL;
    DWORD DX9Hook::mLastHookTime = 0;

    wchar_t* D3D9errorValueToString(DWORD res)
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
		    case D3DERR_WASSTILLDRAWING                 : return L"D3DERR_WASSTILLDRAWING";
		    case D3DOK_NOAUTOGEN                        : return L"D3DOK_NOAUTOGEN";
		    case D3DERR_DEVICEREMOVED                   : return L"D3DERR_DEVICEREMOVED";
		    case D3DERR_DEVICEHUNG                      : return L"D3DERR_DEVICEHUNG";
		    case D3DERR_UNSUPPORTEDOVERLAY              : return L"D3DERR_UNSUPPORTEDOVERLAY";
		    case D3DERR_UNSUPPORTEDOVERLAYFORMAT        : return L"D3DERR_UNSUPPORTEDOVERLAYFORMAT";
		    case D3DERR_CANNOTPROTECTCONTENT            : return L"D3DERR_CANNOTPROTECTCONTENT";
		    case D3DERR_UNSUPPORTEDCRYPTO               : return L"D3DERR_UNSUPPORTEDCRYPTO";
		    case D3DERR_PRESENT_STATISTICS_DISJOINT     : return L"D3DERR_PRESENT_STATISTICS_DISJOINT";
		    case D3D_OK: return L"D3D_OK";
		    case E_FAIL: return L"E_FAIL";
		    case E_INVALIDARG: return L"E_INVALIDARG";
		    case E_NOINTERFACE: return L"E_NOINTERFACE";
		    case E_NOTIMPL: return L"E_NOTIMPL";
		    case E_OUTOFMEMORY: return L"E_OUTOFMEMORY"; 
		    default: return L"unknown error";
	    }
    }

    void CreateScreenCopyDX9(LPDIRECT3DDEVICE9 pDev, int width, int height)
    {
        if(gScreenCopy == NULL)
        {
            gScreenCopy = new ScreenCopyDx9();
            gScreenCopy->Create(pDev, NULL, width, height);
        }
    }

    void DestroyScreenCopyDX9()
    {
        if(gScreenCopy)
        {
            gScreenCopy->Destroy();
            delete gScreenCopy;
            gScreenCopy = NULL;
        }
    }

    DEFINE_HOOK_SAFE_VOID( SetGammaRampHook, (LPDIRECT3DDEVICE9 pDev, UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp))
    
        // dead hook handling
        if (SetGammaRampHookNext == NULL || isSetGammaRampHooked == false)
        {
            return pDev->SetGammaRamp(iSwapChain, Flags, pRamp);
        }
    
        IGOLogInfo("SetGammaRampHook() called");
	    gIGOSaveGammaState = true;
	    SetGammaRampHookNext(pDev, iSwapChain, Flags, pRamp);
    }

    void buildProjectionMatrixDX9(int width, int height, int n, int f)
    {
	    ZeroMemory(&gMatOrtho, sizeof(gMatOrtho));
	    ZeroMemory(&gMatIdentity, sizeof(gMatIdentity));
	    gMatIdentity._11 = gMatIdentity._22 = gMatIdentity._33 = gMatIdentity._44 = 1.0f;

	    gMatOrtho._11 = 2.0f/(width==0?1:width);
	    gMatOrtho._22 = 2.0f/(height==0?1:height);
	    gMatOrtho._33 = -2.0f/(n - f);
	    gMatOrtho._43 = 2.0f/(n - f);
	    gMatOrtho._44 = 1.0f;

        gDX9MatOrtho = D3DXMATRIX(gMatOrtho);
    }

    struct OIGRenderStateExtras
    {
        IDirect3DSurface9 *pRT;
        // More to come?
    };

    bool saveRenderState(IDirect3DDevice9 *pDev, OIGRenderStateExtras* extras)
    {
        SAFE_CALL_LOCK_AUTO

        if (!extras)
            return false;

        memset(extras, 0, sizeof(OIGRenderStateExtras));

        // capture device states for a later restore
        if(!gStateBlock)
        {
		    HRESULT hr = pDev->CreateStateBlock(D3DSBT_ALL, &gStateBlock);
		    if (FAILED(hr))
		    {
                IGOLogWarn("CreateStateBlock() failed. error = %x", hr);
			    return false;
		    }			
        }
        gStateBlock->Capture();

        // create clean state block for IGO
        if(gIGOStateBlock)
	    {
    	    gIGOStateBlock->Apply();
        }
	    else
	    {
		    HRESULT hr = pDev->CreateStateBlock(D3DSBT_ALL, &gIGOStateBlock);
		    if (FAILED(hr))
		    {
                IGOLogWarn("CreateStateBlock() failed. error = %x", hr);
			    return false;
		    }

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

	        for (DWORD i = 0; i < 8; i++)
	        {
		        pDev->SetTexture(i, NULL );
	        }

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
		        pDev->SetTextureStageState(i, D3DTSS_CONSTANT, 0xFFFFFFFF );	
	        }

	        // Setup 
            if (DX9Hook::ShaderSupport() == DX9Hook::ShaderSupport_FULL)
            {
                pDev->SetFVF(0);
                pDev->SetVertexDeclaration(gDX9VertexLayout);
            }

            else // Back to fixed pipeline
	            pDev->SetFVF((D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1));

	        // Setup blending
	        pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	        pDev->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	        pDev->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	        pDev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD );

	        //CurrentDevice->SetTexture(0, g_renderTargetTexture);

	        // setup texture addressing settings
	        pDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	        pDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	        pDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	        pDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	        pDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	        // setup colour calculations
	        pDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	        pDev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	        pDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);

	        // setup alpha calculations
	        pDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	        pDev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	        pDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

    	    gIGOStateBlock->Capture();

        }

        // apply a couple of states, that might change frequently

	    pDev->SetTransform(D3DTS_PROJECTION, &gMatOrtho);
	    pDev->SetTransform(D3DTS_WORLD, &gMatIdentity);
	    pDev->SetTransform(D3DTS_VIEW, &gMatIdentity);

	    D3DVIEWPORT9 vp;
	    vp.X = vp.Y = 0;
        vp.Width = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenWidth);
	    vp.Height = SAFE_CALL(IGOApplication::instance(), &IGOApplication::getScreenHeight);
	    vp.MaxZ = 1.0f;
	    vp.MinZ = 0.0f;
		if (vp.Height > 0 && vp.Width > 0)
			pDev->SetViewport(&vp);

	    RECT rect;
	    ZeroMemory(&rect, sizeof(rect));
	    rect.right = vp.Width;
	    rect.bottom = vp.Height;
	    pDev->SetScissorRect(&rect);

        if (gBackBufferUsable) // if we have a valid back buffer, set it, otherwise leave the current render target!
        {
       	    IDirect3DSurface9 *pBB = NULL;
    	    IDirect3DSurface9 *pRT = NULL;

	        pDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBB);
	        pDev->GetRenderTarget(0, &pRT);

	        if (pBB != pRT)
            {
		        pDev->SetRenderTarget(0, pBB);
                extras->pRT = pRT;
            }
			else
            {
                RELEASE_IF(pRT);
            }

	        RELEASE_IF(pBB);
        }

	    // fix gamma when IGO is active
	    if (gIGOSaveGammaState)
	    {
		    if ((!gIGOActiveState || gResetCheck) && IGOApplication::instance()!=NULL && IGOApplication::instance()->isActive())
		    {
			    pDev->GetGammaRamp(0, &gSavedGammaRamp);
			    pDev->SetGammaRamp(0, D3DSGR_CALIBRATE, &gDefaultGammaRamp);
		    }
		    else if ((gIGOActiveState || gResetCheck) && IGOApplication::instance()!=NULL && IGOApplication::instance()->isActive())
		    {
                if (SetGammaRampHookNext)
			        SetGammaRampHookNext(pDev, 0, D3DSGR_CALIBRATE, &gSavedGammaRamp);
		    }
		    gResetCheck = false;
		    gIGOActiveState = IGOApplication::instance()!=NULL ? IGOApplication::instance()->isActive(): false;

	    }

	    return true;
    }

    void restoreRenderState(IDirect3DDevice9 *pDev, OIGRenderStateExtras* extras)
    {
	    if (gStateBlock)
		    gStateBlock->Apply();

        if (extras->pRT)
        {
            pDev->SetRenderTarget(0, extras->pRT);
            RELEASE_IF(extras->pRT);
        }
    }

    void updateWindowScreenSizeDX9()
    {
	    RECT rect = {0};
	    GetClientRect(gRenderingWindow, &rect);
	    uint32_t width = rect.right - rect.left;
	    uint32_t height = rect.bottom - rect.top;
        IGO_ASSERT(width > 0);
        IGO_ASSERT(height > 0);

        SAFE_CALL_LOCK_AUTO

        if(SAFE_CALL(IGOApplication::instance(), &IGOApplication::getWindowScreenWidth) != width || SAFE_CALL(IGOApplication::instance(), &IGOApplication::getWindowScreenHeight) != height )
	    {
		    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
		    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowScreenSize, width, height);
        }
    }

    ULONG DX9AllocatedObjectsCount()
    {
        ULONG refCount = gScreenCopy ? gScreenCopy->GetObjectCount() : 0;
        refCount += SAFE_CALL(IGOApplication::instance(), &IGOApplication::getWindowCount) * 2;
        refCount += gIGOStateBlock ? 1 : 0;
        refCount += gStateBlock ? 1 : 0;
        refCount += gDX9Effect ? 1 : 0;
        refCount += gDX9VertexLayout ? 1 : 0;

        return refCount;
    }

    void ReleaseD3D9Objects(void *userData = NULL)
    {
        IGOLogWarn("ReleaseD3D9Objects called");
        // clear all objects
        RELEASE_IF(gStateBlock);
        RELEASE_IF(gIGOStateBlock);
        RELEASE_IF(gDX9Effect);
        RELEASE_IF(gDX9VertexLayout);

        gShaderSupport = DX9Hook::ShaderSupport_UNKNOWN;
        gDX9EffectTechnique = NULL;
        gDX9WorldVariable = NULL;
        gDX9ViewProjectionVariable = NULL;
        gDX9ColorVariable = NULL;
        gDX9TextureVariable = NULL;

        DestroyScreenCopyDX9();
    }

    bool DX9Init(LPDIRECT3DDEVICE9 pDev)
    {
        HRESULT hr;

        ReleaseD3D9Objects();
        IGOLogDebug("Initialize DX9");

        if (!pDev)
            return false;

        // Have we tried to figure out whether to use shaders yet?
        if (gShaderSupport == DX9Hook::ShaderSupport_UNKNOWN)
        {
            gShaderSupport = DX9Hook::ShaderSupport_FIXED;

            D3DCAPS9 caps;
            HRESULT hr = pDev->GetDeviceCaps(&caps);
            if (SUCCEEDED(hr))
            {
                int vsModelMajor = D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion);
                int vsModelMinor = D3DSHADER_VERSION_MINOR(caps.VertexShaderVersion);

                int psModelMajor = D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion);
                int psModelMinor = D3DSHADER_VERSION_MINOR(caps.PixelShaderVersion);

                IGOLogDebug("Detected shader models: vs=%d.%d, ps=%d.%d", vsModelMajor, vsModelMinor, psModelMajor, psModelMinor);

                if (vsModelMajor > 1 && psModelMajor > 1)
                    gShaderSupport = DX9Hook::ShaderSupport_FULL;
            }

            else
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX9, "Failed to query device caps - hr=0x%08x", hr));                
        }

        //

        if (DX9Hook::ShaderSupport())
        {
            // Load Compiled Effect from DLL resource
            IGOLogDebug("Loading igo.fxo file");
            const char* pIGOFx;
            DWORD dwIGOFxSize = 0;
            if (!LoadEmbeddedResource(IDR_IGODX9FX, &pIGOFx, &dwIGOFxSize))
            {
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX9, "Failed to load shader resources."));
                ReleaseD3D9Objects();
                return false;
            }

            // Create Effect
            IGOLogDebug("Creating effect from igo.fxo");
            // The method we need to create an effect is in an extension DLL -> need to search the loaded modules for it
            typedef HRESULT(WINAPI *LPD3DXCREATEFFECT)(LPDIRECT3DDEVICE9 pDevice, LPCVOID pSrcData, UINT SrcDataLen, CONST D3DXMACRO* pDefines, LPD3DXINCLUDE pInclude, DWORD Flags, LPD3DXEFFECTPOOL pPool, LPD3DXEFFECT* ppEffect, LPD3DXBUFFER* ppCompilationErrors);
            LPD3DXBUFFER compileErrors = NULL;
            LPD3DXCREATEFFECT s_D3DXCreateEffect = (LPD3DXCREATEFFECT)FindModuleAPI(L"d3dx9", "D3DXCreateEffect");
            if (!s_D3DXCreateEffect)
                s_D3DXCreateEffect = (LPD3DXCREATEFFECT)FindModuleAPI(L"d3d9", "D3DXCreateEffect");

            if (s_D3DXCreateEffect)
            {
                hr = s_D3DXCreateEffect(pDev, pIGOFx, dwIGOFxSize, NULL, NULL, 0, NULL, &gDX9Effect, &compileErrors);
                if (FAILED(hr) || !gDX9Effect)
                {
                    if (compileErrors)
                    {
                        LPVOID buffer = compileErrors->GetBufferPointer();
                        DWORD bufferSize = compileErrors->GetBufferSize();

                        if (buffer && bufferSize > 0)
                            IGOLogWarn("DX9 Shader failed: %s", buffer);

                        RELEASE_IF(compileErrors);
                    }

                    CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX9, "D3DXCreateEffect() failed.hr = %x", hr));
                    gDX9Effect = NULL;
                    ReleaseD3D9Objects();
                    return false;
                }

                gDX9EffectTechnique = gDX9Effect->GetTechniqueByName("Render");
                IGO_ASSERT(gDX9EffectTechnique);

                gDX9WorldVariable = gDX9Effect->GetParameterByName(NULL, "World");
                gDX9ViewProjectionVariable = gDX9Effect->GetParameterByName(NULL, "ViewProj");
                gDX9ColorVariable = gDX9Effect->GetParameterByName(NULL, "Color");
                gDX9TextureVariable = gDX9Effect->GetParameterByName(NULL, "WindowTexture");

                IGO_ASSERT(gDX9WorldVariable);
                IGO_ASSERT(gDX9ViewProjectionVariable);
                IGO_ASSERT(gDX9ColorVariable);
                IGO_ASSERT(gDX9TextureVariable);

                if (!gDX9WorldVariable || !gDX9ViewProjectionVariable || !gDX9ColorVariable || !gDX9TextureVariable)
                {
                    ReleaseD3D9Objects();
                    IGOLogWarn("Shader variables not found!");
                    return false;
                }

                // Bind the variables
                gDX9MatWorld = D3DXMATRIX(gMatWorld);
                gDX9MatOrtho = D3DXMATRIX(gMatOrtho);
                gDX9Effect->SetMatrix(gDX9WorldVariable, &gDX9MatWorld);
                gDX9Effect->SetMatrix(gDX9ViewProjectionVariable, &gDX9MatOrtho);

                // Define the input layout
                D3DVERTEXELEMENT9 layout[] =
                {
                    { 0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
                    { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
                    D3DDECL_END()
                };

                hr = pDev->CreateVertexDeclaration(layout, &gDX9VertexLayout);
                if (FAILED(hr))
                {
                    CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX9, "CreateVertexDeclaration() failed.hr = %x", hr));
                    ReleaseD3D9Objects();
                    return false;
                }
            }

            else
            {
                // Let's go the fixed-pipeline way
                CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL, OriginIGO::TelemetryContext_RESOURCE, OriginIGO::TelemetryRenderer_DX9, "Failed to find D3DXCreateEffect - falling back to fixed-pipeline"));
                gShaderSupport = DX9Hook::ShaderSupport_FIXED;
            }
        }

        // At this point, should be safe to create the IGO IGOApplication::instance()
        if (!IGOApplication::instance())
            IGOApplication::createInstance(RendererType::RendererDX9, pDev);

        return true;
    }

    // TestCooperativeLevel Hook
    DEFINE_HOOK_SAFE(HRESULT, TestCooperativeLevelHook, (LPDIRECT3DDEVICE9 pDev))
    
        // dead hook handling
        if (TestCooperativeLevelHookNext == NULL || isTestCooperativeLevelHooked == false)
        {
            ULONG refCount = DX9AllocatedObjectsCount();
            if (refCount)
                IGOLogWarn("TestCooperativeLevelHook() dead hook + resource leak!");
            else
                IGOLogWarn("TestCooperativeLevelHook() dead hook!");

            return pDev->TestCooperativeLevel();
        }
    
        HRESULT hr = TestCooperativeLevelHookNext(pDev);
        gTestCooperativeLevelHook_or_ResetHook_Called = true;
        
        if (gPresentHookThreadId != 0 && gPresentHookThreadId != GetCurrentThreadId())
            IGOForceLogWarnOnce("TestCooperativeLevelHook(pre) called from the wrong thread! (%i)", gPresentHookThreadId);

        if (FAILED(hr))
        {
            gLastTestCooperativeLevelError = true;
            SAFE_CALL_LOCK_ENTER
            if (IGOApplication::instance())
            {

	            bool safeToCall = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isThreadSafe);
                SAFE_CALL_LOCK_LEAVE

            	IGOLogWarn("TestCooperativeLevelHook %i %S", safeToCall, D3D9errorValueToString(hr));
                if ((gPresentHookThreadId != 0) &&
                   ((gPresentHookThreadId == GetCurrentThreadId() && !safeToCall) ||
                     (gPresentHookThreadId != GetCurrentThreadId() && safeToCall)))
                        IGOLogWarn("TestCooperativeLevelHook thread conflict.");


                if (!safeToCall)
                {
                    bool isPendingDeletion = IGOApplication::isPendingDeletion();
                    if (!isPendingDeletion)
                    {
                        IGOLogWarn("TestCooperativeLevelHook->pending IGO deletion initiated.");
                        IGOApplication::deleteInstanceLater();
                    }
                    else
                    {
                        IGOLogWarn("TestCooperativeLevelHook->pending IGO deletion already initiated, forcing a deletion now.");
                        IGOApplication::deleteInstance();
                        ReleaseD3D9Objects();
                    }
                }
                else
                {
                    IGOApplication::deleteInstance();                    
                    ReleaseD3D9Objects();
                }                    
            }
            else
            {
                SAFE_CALL_LOCK_LEAVE
            }

            ULONG refCount = DX9AllocatedObjectsCount();
            if (refCount)
                IGOLogWarn("TestCooperativeLevelHook() resource leak!");
        }
        else
        {
            gLastTestCooperativeLevelError = false;
        }

        hr = TestCooperativeLevelHookNext(pDev);

        if (gPresentHookThreadId != 0 && gPresentHookThreadId != GetCurrentThreadId())
            IGOForceLogWarnOnce("TestCooperativeLevelHook(post) called from the wrong thread! (%i)", gPresentHookThreadId);

        return hr;
    }
    
    // Present Hook
    DEFINE_HOOK_SAFE(HRESULT, PresentHook, (LPDIRECT3DDEVICE9 pDev, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion))
    
        if (PresentHookNext == NULL || isPresentHooked == false)
        {
            return pDev->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
        }

        HRESULT hr = D3D_OK;
		bool broadcastFrameReady = false;
        gPresentHookCalled = GetTickCount();

        // is the device ready????
        {
            IGO_ASSERT(TestCooperativeLevelHookNext!=NULL);
            hr = (TestCooperativeLevelHookNext != NULL && isTestCooperativeLevelHooked) ? TestCooperativeLevelHookNext(pDev) : pDev->TestCooperativeLevel();
            if (FAILED(hr))
                IGOLogWarn("PresentHook->TestCooperativeLevel %S", D3D9errorValueToString(hr));
            gTestCooperativeLevelHook_or_ResetHook_Called = true;
            if (FAILED(hr))
            {
			    // The device is being destroyed
                ReleaseD3D9Objects();
                IGOApplication::deleteInstance();// important, delete the IGOApplication::instance() here!!!
                
                HRESULT hr = PresentHookNext(pDev, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
                IGOLogWarn("Present called / TestCooperativeLevel device not ready");
	            return hr;
            }
        }
                
        gRenderingDeviceDX9 = pDev;

        if( IGOApplication::isPendingDeletion())
        {
            ReleaseD3D9Objects();
    	    IGOApplication::deleteInstance();
            
            HRESULT hr = PresentHookNext(pDev, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
	        return hr;
        }


        if (!gInputHooked && !gQuitting)
		    InputHook::TryHookLater(&gInputHooked);

        if (IGOApplication::instance()==NULL)
	    {
            
		    IGOLogInfo("PresentInitHook() called");
		    HRESULT hr = PresentHookNext(pDev, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

		    // create the IGO IGOApplication::instance()
            if (!DX9Init(pDev))
                return hr;

		    D3DDEVICE_CREATION_PARAMETERS d3dcp;
		    hr = pDev->GetCreationParameters(&d3dcp);
			IGO_ASSERT(SUCCEEDED(hr));

			gRenderingWindow = d3dcp.hFocusWindow;
            gPresentHookThreadId = GetCurrentThreadId();
            IGOLogInfo("RenderingWindow=%p, PresentThreadId=%d", gRenderingWindow, gPresentHookThreadId);

            int width = 0;
		    int height = 0;
            if (pSourceRect)
            {
                // Right now, we're going to assume there isn't an offset to use!
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
		        IDirect3DSurface9 *bb;
		        hr = pDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
		        if (SUCCEEDED(hr))
		        {
			        D3DSURFACE_DESC desc;
			        bb->GetDesc(&desc);
			        width = desc.Width;
			        height = desc.Height;
			        bb->Release();
                    gBackBufferUsable = true;

                    IGO_ASSERT(width >= 16);
                    IGO_ASSERT(height >= 16);
		        }
            }

			if (gWindowedMode && (width == 0 || height == 0))
			{
				RECT rect = {0};
				if (d3dcp.hFocusWindow !=NULL)
					GetClientRect(d3dcp.hFocusWindow, &rect);
				else
					GetClientRect(gRenderingWindow, &rect);

				width = rect.right - rect.left;
				height = rect.bottom - rect.top;

                IGO_ASSERT(width > 0);
                IGO_ASSERT(height > 0);
			}
			
            if (pDestRect)
            {
                // Do not stretch the OIG desktop / keep the proper aspect ratio
                width = (pDestRect->right - pDestRect->left) > width ? (pDestRect->right - pDestRect->left) : width;
                height = (pDestRect->bottom - pDestRect->top) > height ? (pDestRect->bottom - pDestRect->top) : height;
            }

            IGO_ASSERT(width > 0);
            IGO_ASSERT(height > 0);
            
            SAFE_CALL_LOCK_ENTER
            SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, width, height);
            SAFE_CALL_LOCK_LEAVE

		    buildProjectionMatrixDX9(width, height, 1, 10);
            if (gDX9ViewProjectionVariable)
                gDX9Effect->SetMatrix(gDX9ViewProjectionVariable, &gDX9MatOrtho);

		    RECT rect;
		    GetClientRect(gRenderingWindow, &rect);
		    width = rect.right - rect.left;
		    height = rect.bottom - rect.top;

		
		    IDirect3DSwapChain9 *pSwapChain = NULL;
		    hr = pDev->GetSwapChain(0, &pSwapChain);
		    if (SUCCEEDED(hr) && pSwapChain)
		    {
			    D3DPRESENT_PARAMETERS pp={0};
			    hr = pSwapChain->GetPresentParameters(&pp);

			    gWindowedMode = pp.Windowed==TRUE;
			    pSwapChain->Release();
		    }

            SAFE_CALL_LOCK_ENTER
            SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
		    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowScreenSize, width, height);
            SAFE_CALL_LOCK_LEAVE

		    // hook windows message
            if (!gQuitting)
		        InputHook::HookWindow(gRenderingWindow);

		    // set up gamma ramp
		    for (WORD i = 0; i < 256; i++)
		    {
			    gDefaultGammaRamp.red[i] = (WORD)MulDiv(i, 65535, 255);
			    gDefaultGammaRamp.green[i] = (WORD)MulDiv(i, 65535, 255);
			    gDefaultGammaRamp.blue[i] = (WORD)MulDiv(i, 65535, 255);
		    }

		    return hr;
	    }
        else
        {

            // hook windows message
            if (!gQuitting)
                InputHook::HookWindow(gRenderingWindow);

            IDirect3DSwapChain9 *pSwapChain = NULL;
            HRESULT hr = pDev->GetSwapChain(0, &pSwapChain);
            HWND renderingWindow = NULL;
            if (SUCCEEDED(hr) && pSwapChain)
            {
                D3DPRESENT_PARAMETERS pp = { 0 };
                hr = pSwapChain->GetPresentParameters(&pp);

                gWindowedMode = pp.Windowed == TRUE;
                renderingWindow = pp.hDeviceWindow;
                pSwapChain->Release();
            }

            if (IGOApplication::instance() != NULL /*IGOApplication::instance() maybe deleted in TestCooperativeLevel, so check them again!!!*/)
            {
                if (renderingWindow != gRenderingWindow)
                {
                    // This piece of code is really to watch for a specific case particularly hard to reproduce - but I believe this is fixed now
                    // (the problem being that sometimes we wouldn't clean up the IGOApplication because we were not on the proper thread)
                    OriginIGO::IGOLogger::instance()->enableLogging(true);
                    IGOLogWarn("Different backbuffer rendering window: %p (%p) - Valid: %d (%d)", renderingWindow, gRenderingWindow, ::IsWindow(renderingWindow), ::IsWindow(gRenderingWindow));

                    ReleaseD3D9Objects();
    	            IGOApplication::deleteInstance();

                    OriginIGO::IGOLogger::instance()->enableLogging(getIGOConfigurationFlags().gEnabledLogging);
            
                    HRESULT hr = PresentHookNext(pDev, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
	                return hr;
                }

                RECT rect;
                GetWindowRect(gRenderingWindow, &rect);
                if (((rect.right - rect.left) * (rect.bottom - rect.top)) != 0)
                {

                    int width = 0;
                    int height = 0;
                    if (pSourceRect)
                    {
                        // Right now, we're going to assume there isn't an offset to use!
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
                        IDirect3DSurface9 *bb;
                        hr = pDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
                        if (SUCCEEDED(hr))
                        {
                            D3DSURFACE_DESC desc;
                            bb->GetDesc(&desc);
                            width = desc.Width;
                            height = desc.Height;
                            bb->Release();
                            gBackBufferUsable = true;

                            IGO_ASSERT(width >= 16);
                            IGO_ASSERT(height >= 16);
                        }
                    }

                    if (pDestRect)
                    {
                        // Do not stretch the OIG desktop / keep the proper aspect ratio
                        width = (pDestRect->right - pDestRect->left) > width ? (pDestRect->right - pDestRect->left) : width;
                        height = (pDestRect->bottom - pDestRect->top) > height ? (pDestRect->bottom - pDestRect->top) : height;
                    }

                    IGO_ASSERT(width > 0);
                    IGO_ASSERT(height > 0);

                    SAFE_CALL_LOCK_ENTER
                    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, width, height);
                    SAFE_CALL_LOCK_LEAVE
                    buildProjectionMatrixDX9(width, height, 1, 10);
                    if (gDX9ViewProjectionVariable)
                        gDX9Effect->SetMatrix(gDX9ViewProjectionVariable, &gDX9MatOrtho);

                    updateWindowScreenSizeDX9();
                    SAFE_CALL_LOCK_ENTER
                    SAFE_CALL(IGOApplication::instance(), &IGOApplication::startTime);
                    SAFE_CALL_LOCK_LEAVE

                    if (TwitchManager::IsBroadCastingInitiated())
                        CreateScreenCopyDX9(pDev, width, height);
                    else
                        DestroyScreenCopyDX9();

                    if (TwitchManager::IsBroadCastingInitiated())
                    {
                        TwitchManager::TTV_PollTasks();
                        if (TTV_SUCCEEDED(TwitchManager::IsReadyForStreaming()) && TwitchManager::IsBroadCastingRunning())
                        {
                            broadcastFrameReady = SAFE_CALL(gScreenCopy, &IScreenCopy::Render, pDev, NULL);
                        }
                    }

                    OIGRenderStateExtras extras;
                    if (saveRenderState(pDev, &extras))
                    {
                        if (pDev->BeginScene() == D3D_OK)
                        {
                            if (TwitchManager::IsBroadCastingInitiated())
                                CreateScreenCopyDX9(pDev, width, height);
                            else
                                DestroyScreenCopyDX9();

                            if (TwitchManager::IsBroadCastingInitiated())
                            {
                                TwitchManager::TTV_PollTasks();
                                if (TTV_SUCCEEDED(TwitchManager::IsReadyForStreaming()) && TwitchManager::IsBroadCastingRunning())
                                {
                                    bool s = SAFE_CALL(gScreenCopy, &IScreenCopy::Render, pDev, NULL);
                                    if (!s)
                                        DestroyScreenCopyDX9();
                                }
                            }

                            SAFE_CALL_LOCK_ENTER
                                SAFE_CALL(IGOApplication::instance(), &IGOApplication::render, pDev);
                            SAFE_CALL_LOCK_LEAVE
                                pDev->EndScene();
                        }

                        restoreRenderState(pDev, &extras);
                    }
                    SAFE_CALL_LOCK_ENTER
                        SAFE_CALL(IGOApplication::instance(), &IGOApplication::stopTime);
                    SAFE_CALL_LOCK_LEAVE
                }
            }
            else
            {
                ReleaseD3D9Objects();
                IGOApplication::deleteInstance();// important, delete the IGOApplication::instance() here!!!
            }
        }

        hr = PresentHookNext(pDev, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

		if (broadcastFrameReady)
            SAFE_CALL(gScreenCopy, &IScreenCopy::CopyRT, pDev, NULL);

	    return hr;
    }

    // Release Hook
    DEFINE_HOOK_SAFE(ULONG, ReleaseHook, (LPDIRECT3DDEVICE9 pDev))
       
        // dead hook handling
        if (ReleaseHookNext == NULL || isReleaseHooked == false)
        {
            if (!gCalledFromInsideIGO)
            {
                gCalledFromInsideIGO++;

                ReleaseD3D9Objects();
                IGOApplication::deleteInstance();// important, delete the IGOApplication::instance() here!!!

                gCalledFromInsideIGO--;
            }

            return pDev->Release();
        }
            
        // 2 for texture & vertex buffer
	    // 2 for render state
	    // 1 for current number
	
	    ULONG ul = 0;
        ULONG refCount = 0;
        bool locked = SAFE_CALL_LOCK_TRYREADLOCK_COND;    // DX9 release is called very often from many threads, so keep an eye on the write lock!!!
        if (locked)
        {
            if (gRenderingDeviceDX9 != NULL && gRenderingDeviceDX9 != pDev)
            {
                IGOLogWarn("ReleaseHook wrong device!");
                ULONG refCount = DX9AllocatedObjectsCount();

                if (refCount)
                    IGOLogWarn("ReleaseHook() possible resource leak!");
            }

            if (!gCalledFromInsideIGO && gRenderingDeviceDX9 == pDev && IGOApplication::instance()!=NULL)
	        {
		        gCalledFromInsideIGO++;
		        pDev->AddRef();
		        ULONG currRefCount = ReleaseHookNext(pDev);
            
                refCount = DX9AllocatedObjectsCount() + 1;
		        if (currRefCount == refCount)
		        {
			        // The device is being destroyed
                
                    ReleaseD3D9Objects();
                    SAFE_CALL_LOCK_LEAVE
			        IGOApplication::deleteInstance();// important, delete the IGOApplication::instance() here!!!	
                    IGOLogWarn("ReleaseHook -> closed IGO");
                    refCount = 0;

                    gCalledFromInsideIGO--;
	                ul = ReleaseHookNext(pDev) - refCount;
	                return ul;
		        }
                else
                {
                    // is the device broken, then release IGO
                    // we do this here, to prevent a hang, in case the device is lost,
                    // because some games will only call TestCooperativeLevel from a different thread like this:

                        // WARN	07:06 : 41 PM	5308	                NULL : 497		TestCooperativeLevelHook 0 D3DERR_DEVICENOTRESET
                        // WARN	07:06 : 41 PM	5308	                NULL : 506		TestCooperativeLevelHook->pending IGO deletion.
                        // WARN	07:06 : 41 PM	5308	                NULL : 522		TestCooperativeLevelHook() resource leak!

                    // and we then cannot release the IGO safely, because of the wrong caller thread!

                    IGO_ASSERT(TestCooperativeLevelHookNext!=NULL);
                    gTestCooperativeLevelHook_or_ResetHook_Called = true;
                    HRESULT hr = (TestCooperativeLevelHookNext != NULL && isTestCooperativeLevelHooked) ? TestCooperativeLevelHookNext(pDev) : pDev->TestCooperativeLevel();
                    if (FAILED(hr))
                    {
                        IGOLogWarn("ReleaseHook->TestCooperativeLevel %S", D3D9errorValueToString(hr));
                        ReleaseD3D9Objects();
                        SAFE_CALL_LOCK_LEAVE
                        IGOApplication::deleteInstance();// important, delete the IGOApplication::instance() here!!!	
                        IGOLogWarn("ReleaseHook -> closed IGO");
                        refCount = 0;
                    }
                    else
                    {
                        SAFE_CALL_LOCK_LEAVE
                    }
                    gCalledFromInsideIGO--;
	                ul = ReleaseHookNext(pDev) - refCount;
	                return ul;
                }
            }
            SAFE_CALL_LOCK_LEAVE
        }

        ul = ReleaseHookNext(pDev) - refCount;
	    return ul;
    }

    // Swapchain Hook
    DEFINE_HOOK_SAFE(HRESULT, SwapChainPresentHook, (IDirect3DSwapChain9 *pSC, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags))
    
        // dead hook handling
        if (SwapChainPresentHookNext == NULL || isSwapChainPresentHooked == false)
        {    
            return pSC->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
        }    
        
        HRESULT hr = D3D_OK;
		bool broadcastFrameReady = false;
        gPresentHookCalled = GetTickCount();

        // is the device ready????
        {
		    IDirect3DDevice9 *pDev = NULL;
		    hr = pSC->GetDevice(&pDev);
            if (SUCCEEDED(hr) && pDev)
            {
                IGO_ASSERT(TestCooperativeLevelHookNext!=NULL);
                hr = (TestCooperativeLevelHookNext != NULL && isTestCooperativeLevelHooked) ? TestCooperativeLevelHookNext(pDev) : pDev->TestCooperativeLevel();
                if (FAILED(hr))
                    IGOLogWarn("SwapChainPresentHook->TestCooperativeLevel %S", D3D9errorValueToString(hr));
                gTestCooperativeLevelHook_or_ResetHook_Called = true;

                // do not use pDev->Release() because we have hooked release !!!
		        IGO_ASSERT(ReleaseHookNext);
		        if (ReleaseHookNext)
			        ReleaseHookNext(pDev);
                else
                    pDev->Release();
            }
 
            if (FAILED(hr))
            {
			    // The device is being destroyed
                ReleaseD3D9Objects();
			    IGOApplication::deleteInstance();// important, delete the IGOApplication::instance() here!!!
       		    IGOLogWarn("SwapChainPresent called - device not ready");

                hr = SwapChainPresentHookNext(pSC, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
                IGOLogWarn("SwapChainPresentHook- GetDevice or TestCooperativeLevel %S", D3D9errorValueToString(hr));
                return hr;
            }
        }
        
        if(IGOApplication::isPendingDeletion())
        {
            ReleaseD3D9Objects();
            IGOApplication::deleteInstance();            
            hr = SwapChainPresentHookNext(pSC, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
            return hr;
        }
        
        if (!gInputHooked && !gQuitting)
		    InputHook::TryHookLater(&gInputHooked);
        
        if (IGOApplication::instance()==NULL)
	    {
		    IGOLogInfo("SwapChainPresentInitHook() called");
		    hr = SwapChainPresentHookNext(pSC, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

		    IDirect3DDevice9 *pDev = NULL;
		    pSC->GetDevice(&pDev);
            gRenderingDeviceDX9 = pDev;

            if (!DX9Init(pDev))
                return hr;

            D3DDEVICE_CREATION_PARAMETERS d3dcp;
		    pDev->GetCreationParameters(&d3dcp);

		    gRenderingWindow = d3dcp.hFocusWindow;
            gPresentHookThreadId = GetCurrentThreadId();
            IGOLogInfo("RenderingWindow=%p, PresentThreadId=%d", gRenderingWindow, gPresentHookThreadId);

            int width = 0;
            int height = 0;
            if (pSourceRect)
            {
                // Right now, we're going to assume there isn't an offset to use!
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
		        IDirect3DSurface9 *bb;
                hr = pDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
                if (SUCCEEDED(hr))
                {
                    D3DSURFACE_DESC desc;
                    bb->GetDesc(&desc);
                    width = desc.Width;
                    height = desc.Height;
                    bb->Release();
                    gBackBufferUsable = true;

                    IGO_ASSERT(width >= 16);
                    IGO_ASSERT(height >= 16);
                }
            }

            if (pDestRect)
            {
                // Do not stretch the OIG desktop / keep the proper aspect ratio
                width = (pDestRect->right - pDestRect->left) > width ? (pDestRect->right - pDestRect->left) : width;
                height = (pDestRect->bottom - pDestRect->top) > height ? (pDestRect->bottom - pDestRect->top) : height;
            }
                    
            IGO_ASSERT(width > 0);
            IGO_ASSERT(height > 0);

            SAFE_CALL_LOCK_ENTER
            SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, width, height);
            SAFE_CALL_LOCK_LEAVE
		    buildProjectionMatrixDX9(width, height, 1, 10);
            if (gDX9ViewProjectionVariable)
                gDX9Effect->SetMatrix(gDX9ViewProjectionVariable, &gDX9MatOrtho);

		    RECT rect;
		    GetClientRect(gRenderingWindow, &rect);
		    width = rect.right - rect.left;
		    height = rect.bottom - rect.top;

		    D3DPRESENT_PARAMETERS pp={0};
		    hr = pSC->GetPresentParameters(&pp);
		    gWindowedMode = pp.Windowed==TRUE;
            SAFE_CALL_LOCK_ENTER
		    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowedMode, gWindowedMode);
		    SAFE_CALL(IGOApplication::instance(), &IGOApplication::setWindowScreenSize, width, height);
            SAFE_CALL_LOCK_LEAVE

            // hook windows message
            if (!gQuitting)
		        InputHook::HookWindow(gRenderingWindow);

		    // set up gamma ramp
		    for (WORD i = 0; i < 256; i++)
		    {
			    gDefaultGammaRamp.red[i] = (WORD)MulDiv(i, 65535, 255);
			    gDefaultGammaRamp.green[i] = (WORD)MulDiv(i, 65535, 255);
			    gDefaultGammaRamp.blue[i] = (WORD)MulDiv(i, 65535, 255);
		    }

		    // do not use pDev->Release() because we have hooked release !!!
		    IGO_ASSERT(ReleaseHookNext);
		    if (ReleaseHookNext)
			    ReleaseHookNext(pDev);
            else
                pDev->Release();

		    return hr;
	    }
	    else
	    {
		    // hook windows message
            if (!gQuitting)
		        InputHook::HookWindow(gRenderingWindow);

            IDirect3DDevice9 *pDev = NULL;
		    hr = pSC->GetDevice(&pDev);
            gRenderingDeviceDX9 = pDev;

		    if (SUCCEEDED(hr) && pDev)
		    {
				D3DPRESENT_PARAMETERS pp={0};
				pSC->GetPresentParameters(&pp);
				gWindowedMode = pp.Windowed==TRUE;

                HWND renderingWindow = pp.hDeviceWindow;

                if (IGOApplication::instance() != NULL /*IGOApplication::instance() maybe deleted in TestCooperativeLevel, so check them again!!!*/)
			    {
                    if (renderingWindow != gRenderingWindow)
                    {
                        // This piece of code is really to watch for a specific case particularly hard to reproduce - but I believe this is fixed now
                        // (the problem being that sometimes we wouldn't clean up the IGOApplication because we were not on the proper thread)
                        OriginIGO::IGOLogger::instance()->enableLogging(true);
                        IGOLogWarn("Different backbuffer rendering window: %p (%p) - Valid: %d (%d)", renderingWindow, gRenderingWindow, ::IsWindow(renderingWindow), ::IsWindow(gRenderingWindow));

                        ReleaseD3D9Objects();
    	                IGOApplication::deleteInstance();

                        OriginIGO::IGOLogger::instance()->enableLogging(getIGOConfigurationFlags().gEnabledLogging);
            
	                    HRESULT hr = SwapChainPresentHookNext(pSC, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
                        return hr;
                    }

					RECT rect;
					GetWindowRect(gRenderingWindow, &rect);
					if (((rect.right - rect.left) * (rect.bottom-rect.top)) != 0)
					{
						int width = 0;
						int height = 0;
                        if (pSourceRect)
                        {
                            // Right now, we're going to assume there isn't an offset to use!
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
						    IDirect3DSurface9 *bb;
						    hr = pDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb);
						    if (SUCCEEDED(hr))
						    {
							    D3DSURFACE_DESC desc;
							    bb->GetDesc(&desc);
							    width = desc.Width;
							    height = desc.Height;
							    bb->Release();
                                gBackBufferUsable = true;
                        
                                IGO_ASSERT(width >= 16);
                                IGO_ASSERT(height >= 16);
                            }
                        }

                        if ((uint32_t)width != pp.BackBufferWidth && (uint32_t)height != pp.BackBufferHeight)    // Realm of the Mad God uses a 16x16 back buffer, but a real swap chain
                        {
                            gBackBufferUsable = false;
				            width =  pp.BackBufferWidth;
				            height = pp.BackBufferHeight;
                        }
                        if (pDestRect)
                        { 
                            // Do not stretch the OIG desktop / keep the proper aspect ratio
                            width = (pDestRect->right - pDestRect->left) > (LONG)pp.BackBufferWidth ? (pDestRect->right - pDestRect->left) : pp.BackBufferWidth;
                            height = (pDestRect->bottom - pDestRect->top) > (LONG)pp.BackBufferHeight ? (pDestRect->bottom - pDestRect->top) : pp.BackBufferHeight;
                        }

                        IGO_ASSERT(width > 0);
                        IGO_ASSERT(height > 0);

						SAFE_CALL_LOCK_ENTER
						SAFE_CALL(IGOApplication::instance(), &IGOApplication::setScreenSize, width, height);
						SAFE_CALL_LOCK_LEAVE
						buildProjectionMatrixDX9(width, height, 1, 10);
                        if (gDX9ViewProjectionVariable)
                            gDX9Effect->SetMatrix(gDX9ViewProjectionVariable, &gDX9MatOrtho);

						updateWindowScreenSizeDX9();
						SAFE_CALL_LOCK_ENTER
					    SAFE_CALL(IGOApplication::instance(), &IGOApplication::startTime);
	                    SAFE_CALL_LOCK_LEAVE
						if (TwitchManager::IsBroadCastingInitiated())
    						CreateScreenCopyDX9(pDev, width, height);
						else
							DestroyScreenCopyDX9();

						if (TwitchManager::IsBroadCastingInitiated())
						{
							TwitchManager::TTV_PollTasks();
							if (TTV_SUCCEEDED(TwitchManager::IsReadyForStreaming()) && TwitchManager::IsBroadCastingRunning())
							{
								broadcastFrameReady = SAFE_CALL(gScreenCopy, &IScreenCopy::Render, pDev, NULL);
							}
						}

                        OIGRenderStateExtras extras;
						if (saveRenderState(pDev, &extras))
					    {
						    if (pDev->BeginScene() == D3D_OK)
						    {
								SAFE_CALL_LOCK_ENTER    						    
								SAFE_CALL(IGOApplication::instance(), &IGOApplication::render, pDev);
                                SAFE_CALL_LOCK_LEAVE
                                pDev->EndScene();
						    }
						    restoreRenderState(pDev, &extras);
					    }
                        SAFE_CALL_LOCK_ENTER
					    SAFE_CALL(IGOApplication::instance(), &IGOApplication::stopTime);
                        SAFE_CALL_LOCK_LEAVE
				    }
			    }
			    else
                {
                    ReleaseD3D9Objects();
                    IGOApplication::deleteInstance();// important, delete the IGOApplication::instance() here!!!                    
                }

                // do not use pDev->Release() because we have hooked release !!!
		        IGO_ASSERT(ReleaseHookNext);
		        if (ReleaseHookNext)
			        ReleaseHookNext(pDev);
                else
                    pDev->Release();
            }
	    }
	    hr = SwapChainPresentHookNext(pSC, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

        if (broadcastFrameReady)
        {
            IDirect3DDevice9 *pDev = NULL;
		    pSC->GetDevice(&pDev);
            if (pDev)
            {
                SAFE_CALL(gScreenCopy, &IScreenCopy::CopyRT, pDev, NULL);

                // do not use pDev->Release() because we have hooked release !!!
		        IGO_ASSERT(ReleaseHookNext);
		        if (ReleaseHookNext)
			        ReleaseHookNext(pDev);
                else
                    pDev->Release();
            }
        }
	    return hr;
    }

    // Reset Hook
    DEFINE_HOOK_SAFE(HRESULT, ResetHook, (LPDIRECT3DDEVICE9 pDev, D3DPRESENT_PARAMETERS *pPresentationParameters))
    
	    IGOLogInfo("ResetHook() called");
  
        // dead hook handling
        if (ResetHookNext == NULL || isResetHooked == false)
        {
            ULONG refCount = DX9AllocatedObjectsCount();

            if (refCount)
                IGOLogWarn("ResetHook() dead hook + resource leak!");
            else
                IGOLogWarn("ResetHook() dead hook!");
            return pDev->Reset(pPresentationParameters);
        }  


        IGO_ASSERT(pPresentationParameters);

        HRESULT hr = D3D_OK;
        gTestCooperativeLevelHook_or_ResetHook_Called = true;

        if (pPresentationParameters)
		    gWindowedMode = pPresentationParameters->Windowed==TRUE;

        IGOLogInfo("game is windowed = %d dev hwnd %p input hwnd %p rendering hwnd %p", gWindowedMode == true ? 1 : 0, pPresentationParameters != NULL ? pPresentationParameters->hDeviceWindow : 0, gHwnd, gRenderingWindow);
        
        if (gPresentHookThreadId != 0 && gPresentHookThreadId != GetCurrentThreadId())
            IGOLogWarn("ResetHook() called from the wrong thread (%i)!", gPresentHookThreadId);


        if (pPresentationParameters)
        {
            IGOLogInfo("BackBufferWidth %i\r\nBackBufferHeight %i\r\nBackBufferFormat %i\r\nBackBufferCount %i\r\nMultiSampleType %i\r\nMultiSampleQuality %i\r\nSwapEffect %i\r\nhDeviceWindow %p\r\nWindowed %i\r\nEnableAutoDepthStencil %i\r\nAutoDepthStencilFormat %i\r\nFlags %i\r\nFullScreen_RefreshRateInHz %i\r\nPresentationInterval %x",
            pPresentationParameters->BackBufferWidth,
            pPresentationParameters->BackBufferHeight,
            pPresentationParameters->BackBufferFormat,
            pPresentationParameters->BackBufferCount,
            pPresentationParameters->MultiSampleType,
            pPresentationParameters->MultiSampleQuality,
            pPresentationParameters->SwapEffect,
            pPresentationParameters->hDeviceWindow,
            pPresentationParameters->Windowed,
            pPresentationParameters->EnableAutoDepthStencil,
            pPresentationParameters->AutoDepthStencilFormat,
            pPresentationParameters->Flags,
            pPresentationParameters->FullScreen_RefreshRateInHz,
            pPresentationParameters->PresentationInterval);
        }

        // clear all objects
        ReleaseD3D9Objects();
        IGOApplication::deleteInstance();
        gPresentHookThreadId = 0;   // reset the present thread, just for safety!
        hr = ResetHookNext(pDev, pPresentationParameters);

        IGOLogWarn("ResetHook() %S", D3D9errorValueToString(hr));

        return hr;
    }

    DX9Hook::DX9Hook()
    {
        isHooked = false;
    }


    DX9Hook::~DX9Hook()
    {
    }

    PRE_DEFINE_HOOK_SAFE(IDirect3D9*, Direct3DCreate9Hook, (UINT SDK_VERSION));

    bool D3D9Init(HMODULE hD3D9, bool calculateOffsetsOnly)
    {
        IGOLogInfo("D3D9Init(%i) called", calculateOffsetsOnly);

        if (DX9Hook::IsBroken())
            return false;

        if (DX9Hook::mDX9Release && DX9Hook::mDX9TestCooperativeLevel && DX9Hook::mDX9Reset && DX9Hook::mDX9SwapChainPresent && DX9Hook::mDX9Present && DX9Hook::mDX9SetGammaRamp)
        {
            IGOLogWarn("Using precalculated offsets.");

            if (!isReleaseHooked)
                HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D9 + (LONG_PTR)DX9Hook::mDX9Release), ReleaseHook);

            if (!isTestCooperativeLevelHooked)
                HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D9 + (LONG_PTR)DX9Hook::mDX9TestCooperativeLevel), TestCooperativeLevelHook);

            if (!isResetHooked)
                HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D9 + (LONG_PTR)DX9Hook::mDX9Reset), ResetHook);

            if (!isSwapChainPresentHooked)
                HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D9 + (LONG_PTR)DX9Hook::mDX9SwapChainPresent), SwapChainPresentHook);

            if (!isPresentHooked)
                HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D9 + (LONG_PTR)DX9Hook::mDX9Present), PresentHook);

            if (!isSetGammaRampHooked)
                HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D9 + (LONG_PTR)DX9Hook::mDX9SetGammaRamp), SetGammaRampHook);

			if (!isSetGammaRampHooked || !isSwapChainPresentHooked || !isPresentHooked || !isReleaseHooked || !isResetHooked || !isTestCooperativeLevelHooked)
			{
				CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_MHOOK, TelemetryRenderer_DX9, "Hooking DX9 via precalculated offsets failed (3rd party tool running?)"));

				UNHOOK_SAFE(SwapChainPresentHook);
				UNHOOK_SAFE(PresentHook);
				UNHOOK_SAFE(TestCooperativeLevelHook);
				UNHOOK_SAFE(ResetHook);
				UNHOOK_SAFE(SetGammaRampHook);
				UNHOOK_SAFE(ReleaseHook);
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

        // just a safety precaution...
        UNHOOK_SAFE(SwapChainPresentHook);
        UNHOOK_SAFE(PresentHook);
        UNHOOK_SAFE(TestCooperativeLevelHook);
        UNHOOK_SAFE(ResetHook);
        UNHOOK_SAFE(SetGammaRampHook);
        UNHOOK_SAFE(ReleaseHook);

        if (!isDirect3DCreate9Hooked || !Direct3DCreate9HookNext)
        {
            // a special case, if we are loaded by the SDK and no offsets have been calculated, try this workaround
			// During dev, the game team may start the game outside Origin -> the injection won't happen immediately as when started from Origin -> the Direct3D Dlls may be loaded by the time the game loads OIG -> we wouldn't get the trigger we're waiting for to hook the CreateDevice methods and OIG would never work; so we use an external signal to let us know whether this is an SDK game and we should take the risk of not using our pre-computed offsets.            
            if (gSDKSignaled && !calculateOffsetsOnly)
            {
                HOOKAPI_SAFE("d3d9.dll", "Direct3DCreate9", Direct3DCreate9Hook);   // do D3D9Init(...) from the create call and calculate the offsets from there!!!
                if (!isDirect3DCreate9Hooked || !Direct3DCreate9HookNext)
                {
                    IGOLogWarn("Hooking Direct3DCreate9 failed. Aborting IGO.");
                    return false;
                }
            }
            else
                return false;
        }

        LPDIRECT3DDEVICE9 pDevice = NULL;

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
        wc.lpszClassName = L"IGO Window Class DX9";

        RegisterClassEx(&wc);

        // create the window and use the result as the handle
        HWND hWnd = CreateWindowEx(NULL,
            L"IGO Window Class DX9",    // name of the window class
            L"IGO Window Title DX9",   // title of the window
            WS_OVERLAPPEDWINDOW,    // window style
            300,    // x-position of the window
            300,    // y-position of the window
            500,    // width of the window
            400,    // height of the window
            NULL,    // we have no parent window, NULL
            NULL,    // we aren't using menus, NULL
            hInstance,    // application handle
            NULL);    // used with multiple windows, NULL
        IGO_ASSERT(hWnd);
        
        IDirect3D9 *d3d9 = NULL;
        d3d9 = Direct3DCreate9HookNext(D3D_SDK_VERSION);
        IGO_ASSERT(d3d9);
        
        if (d3d9)
        {
            HRESULT hr;

            D3DDISPLAYMODE Mode = { 0 };
            hr = d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &Mode);
            if (SUCCEEDED(hr))
            {
                // common back buffer formats
                D3DFORMAT bbFormats[] = {
                        Mode.Format,
                        D3DFMT_R5G6B5,
                        D3DFMT_X1R5G5B5,
                        D3DFMT_A1R5G5B5,
                        D3DFMT_A8R8G8B8,
                        D3DFMT_X8R8G8B8,
                        D3DFMT_A2R10G10B10,
                    };

                int formatIndex = -1;
                for (int i = 0; i < sizeof(bbFormats) / sizeof(bbFormats[0]); i++)
                {
                    hr = d3d9->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, Mode.Format, bbFormats[i], TRUE);
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
                    // fixed: https://developer.origin.com/support/browse/OPUB-4038
                    d3dpp.BackBufferFormat = bbFormats[formatIndex];

                    d3dpp.BackBufferCount = 1;
                    d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
                    d3dpp.MultiSampleQuality = 0;
                    d3dpp.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
                    d3dpp.Flags = 0;
                    d3dpp.Windowed = TRUE;
                    d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
                    d3dpp.hDeviceWindow = hWnd;

                    hr = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &pDevice);
                    
                    if (FAILED(hr) || !pDevice)
                    {
                        // fallback to the old software vertex processing – which may crash on top notch gfx hardware on a pretty old OS – like winxp+sp3
                        // http://forums.amd.com/forum/messageview.cfm?catid=203&threadid=144748
                        IGOLogWarn("D3DCREATE_HARDWARE_VERTEXPROCESSING failed, trying D3DCREATE_SOFTWARE_VERTEXPROCESSING %S.", D3D9errorValueToString(hr));
                        hr = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice);

                        if (FAILED(hr) || !pDevice)
                        {
                            IGOLogWarn("D3DCREATE_SOFTWARE_VERTEXPROCESSING failed, trying D3DCREATE_MIXED_VERTEXPROCESSING %S.", D3D9errorValueToString(hr));
                            hr = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &pDevice);

                            // fixed: https://developer.origin.com/support/browse/OPUB-4038
                            /*
                            if (FAILED(hr) || !pDevice)
                            {
                            IGOLogWarn("D3DCREATE_MIXED_VERTEXPROCESSING failed, trying D3DCREATE_PUREDEVICE %S.", D3D9errorValueToString(hr));
                            hr = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_PUREDEVICE, &d3dpp, &pDevice);
                            if (FAILED(hr) || !pDevice)
                            {
                            IGOLogWarn("D3DCREATE_PUREDEVICE failed %S.", D3D9errorValueToString(hr));
                            }
                            }*/
												
							if (FAILED(hr) || !pDevice)
							{
								IGOLogWarn("D3DCREATE_MIXED_VERTEXPROCESSING failed, trying precalculated offsets %S.", D3D9errorValueToString(hr));
								IGOLogWarn("mDX9Release %p mDX9TestCooperativeLevel %p mDX9Reset %p mDX9SwapChainPresent %p mDX9Present %p mDX9SetGammaRamp %p", DX9Hook::mDX9Release, DX9Hook::mDX9TestCooperativeLevel, DX9Hook::mDX9Reset, DX9Hook::mDX9SwapChainPresent, DX9Hook::mDX9Present, DX9Hook::mDX9SetGammaRamp);
								if (DX9Hook::mDX9Release && DX9Hook::mDX9TestCooperativeLevel && DX9Hook::mDX9Reset && DX9Hook::mDX9SwapChainPresent && DX9Hook::mDX9Present && DX9Hook::mDX9SetGammaRamp)
								{
									HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D9 + (LONG_PTR)DX9Hook::mDX9Release), ReleaseHook);
									HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D9 + (LONG_PTR)DX9Hook::mDX9TestCooperativeLevel), TestCooperativeLevelHook);
									HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D9 + (LONG_PTR)DX9Hook::mDX9Reset), ResetHook);
									HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D9 + (LONG_PTR)DX9Hook::mDX9SwapChainPresent), SwapChainPresentHook);
									HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D9 + (LONG_PTR)DX9Hook::mDX9Present), PresentHook);
									HOOKCODE_SAFE((LPVOID)((LONG_PTR)hD3D9 + (LONG_PTR)DX9Hook::mDX9SetGammaRamp), SetGammaRampHook);

									d3d9->Release();

									DestroyWindow(hWnd);
									UnregisterClass(L"IGO Window Class DX9", hInstance);
									if (!isSetGammaRampHooked || !isSwapChainPresentHooked || !isPresentHooked || !isReleaseHooked || !isResetHooked || !isTestCooperativeLevelHooked)
									{
										IGOLogWarn("Hooking DX9 via precalculated offsets failed. Quitting IGO.");

										UNHOOK_SAFE(SwapChainPresentHook);
										UNHOOK_SAFE(PresentHook);
										UNHOOK_SAFE(TestCooperativeLevelHook);
										UNHOOK_SAFE(ResetHook);
										UNHOOK_SAFE(SetGammaRampHook);
										UNHOOK_SAFE(ReleaseHook);
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
                            IDirect3DSwapChain9 *pSwapChain = NULL;
                            hr = pDevice->GetSwapChain(0, &pSwapChain);
                            if (SUCCEEDED(hr) && pSwapChain)
                            {
                                // swapchain present
                                if (calculateOffsetsOnly)
                                    DX9Hook::mDX9SwapChainPresent = (LONG_PTR)GetInterfaceMethod(pSwapChain, 3) - (LONG_PTR)hD3D9;
                                else
                                    HOOKCODE_SAFE((LPVOID)GetInterfaceMethod(pSwapChain, 3), SwapChainPresentHook);

                                pSwapChain->Release();
                            }
                            else
                            {
                                IGOLogWarn("GetSwapChain() failed.");
                            }

                            if (isSwapChainPresentHooked || calculateOffsetsOnly)
                            {
                                // present
                                if (calculateOffsetsOnly)
                                    DX9Hook::mDX9Present = (LONG_PTR)GetInterfaceMethod(pDevice, 17) - (LONG_PTR)hD3D9;
                                else
                                    HOOKCODE_SAFE((LPVOID)GetInterfaceMethod(pDevice, 17), PresentHook);
                                if (isPresentHooked || calculateOffsetsOnly)
                                {
                                    // TestCooperativeLevel
                                    if (calculateOffsetsOnly)
                                        DX9Hook::mDX9TestCooperativeLevel = (LONG_PTR)GetInterfaceMethod(pDevice, 3) - (LONG_PTR)hD3D9;
                                    else
                                        HOOKCODE_SAFE((LPVOID)GetInterfaceMethod(pDevice, 3), TestCooperativeLevelHook);
                                    if (isTestCooperativeLevelHooked || calculateOffsetsOnly)
                                    {
                                        // reset
                                        if (calculateOffsetsOnly)
                                            DX9Hook::mDX9Reset = (LONG_PTR)GetInterfaceMethod(pDevice, 16) - (LONG_PTR)hD3D9;
                                        else
                                            HOOKCODE_SAFE((LPVOID)GetInterfaceMethod(pDevice, 16), ResetHook);
                                        if (isResetHooked || calculateOffsetsOnly)
                                        {
                                            // release - DAO & DA2 will crash without this hook (req. for propper DX9 cleanup of IGO objects) 
                                            gCalledFromInsideIGO = 0;
                                            if (calculateOffsetsOnly)
                                                DX9Hook::mDX9Release = (LONG_PTR)GetInterfaceMethod(pDevice, 2) - (LONG_PTR)hD3D9;
                                            else
                                                HOOKCODE_SAFE((LPVOID)GetInterfaceMethod(pDevice, 2), ReleaseHook);
                                            gCalledFromInsideIGO = 0;
                                            if (isReleaseHooked || calculateOffsetsOnly)
                                            {
                                                // gamma 
                                                if (calculateOffsetsOnly)
                                                    DX9Hook::mDX9SetGammaRamp = (LONG_PTR)GetInterfaceMethod(pDevice, 21) - (LONG_PTR)hD3D9;
                                                else
                                                    HOOKCODE_SAFE(GetInterfaceMethod(pDevice, 21), SetGammaRampHook);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else
                            IGOLogWarn("TestCooperativeLevel() failed. hr = %S", D3D9errorValueToString(hr));

                        if (ReleaseHookNext)
                            ReleaseHookNext(pDevice);
                        else
                            pDevice->Release();
                    }
                    else
                    {
                        IGOLogWarn("CreateDevice() failed. hr = %S", D3D9errorValueToString(hr));
                    }

                }
                else
                {
                    IGOLogWarn("CheckDeviceType() failed. hr = %S", D3D9errorValueToString(hr));
                }
            }
            else
            {
                IGOLogWarn("GetAdapterDisplayMode() failed. hr = %S", D3D9errorValueToString(hr));
            }
                
            d3d9->Release();
        }
        else
        {
            IGOLogWarn("Direct3DCreate9() failed.");
        }

       DestroyWindow(hWnd);
        UnregisterClass(L"IGO Window Class DX9", hInstance);
        if (!isSetGammaRampHooked || !isSwapChainPresentHooked || !isPresentHooked || !isReleaseHooked || !isResetHooked || !isTestCooperativeLevelHooked)
        {
            if (!calculateOffsetsOnly)
                IGOLogWarn("Hooking DX9 failed. Quitting IGO.");
				
            UNHOOK_SAFE(SwapChainPresentHook);
            UNHOOK_SAFE(PresentHook);
            UNHOOK_SAFE(TestCooperativeLevelHook);
            UNHOOK_SAFE(ResetHook);
            UNHOOK_SAFE(SetGammaRampHook);
            UNHOOK_SAFE(ReleaseHook);
            return false;
        }

        return true;
    }

    PRE_DEFINED_HOOK_SAFE_NO_CHECK(IDirect3D9*, Direct3DCreate9Hook, (UINT SDK_VERSION))

        IGOLogWarn("Direct3DCreate9 called...");
        if (!OriginIGO::CheckHook((LPVOID *)&(Direct3DCreate9HookNext), Direct3DCreate9Hook))
        {
            char details[128] = {0};
            OriginIGO::GetLastHookErrorDetails(details, sizeof(details));
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_MHOOK, TelemetryRenderer_DX9, "Direct3DCreate9 hook check failed (nVidia/detoured.dll?) - %s", details));
        }
       
        EA::Thread::AutoFutex lock(DX9Hook::mInstanceHookMutex);
        DX9Hook::mDX9Initialized = true;

        if (!DX9Hook::IsPrecalculationDone())
        {
            HMODULE hD3D9 = GetModuleHandle(L"d3d9.dll");
            if (hD3D9)
            {
                D3D9Init(hD3D9, true);
            }
        }
        return Direct3DCreate9HookNext(SDK_VERSION);
    }

    bool DX9Hook::TryHook(bool calculateOffsetsOnly/* = false*/)
    {      
        if (DX9Hook::mInstanceHookMutex.TryLock())
        {
            // fix for a special recursive error, if we are called from Direct3DCreate9Hook, UNHOOK_SAFE(Direct3DCreate9Hook) will hang
            /*
            https://developer.origin.com/support/browse/EBIBUGS-28931

            IGO32.dll!EA::Thread::RWSpinLock::WriteLock() Line 281	C++
            IGO32.dll!OriginIGO::DX9Hook::TryHook(bool calculateOffsetsOnly) Line 1745	C++
            IGO32.dll!LdrLoadDll_Hook(wchar_t * * szcwPath, unsigned long * pdwLdrErr, _UNICODE_STRING * pUniModuleName, void * pResultInstance) Line 835	C++
            KERNELBASE.dll!_LoadLibraryExW@12()	Unknown
            KERNELBASE.dll!_LoadLibraryExA@12()	Unknown
            kernel32.dll!_LoadLibraryA@4()	Unknown
            d3d9.dll!_D3D9AddModuleRefCount()	Unknown
            d3d9.dll!CBatchFilterI::InitializeBatchFilter(void *, class CD3DBase *)	Unknown
            d3d9.dll!CreateBatchFilter(void *, class CD3DBase *, class CBatchFilter * *)	Unknown
            d3d9.dll!CD3DBase::InitDevice(void)	Unknown
            d3d9.dll!CD3DHal::InitDevice(void)	Unknown
            d3d9.dll!CD3DBase::Init(void)	Unknown
            d3d9.dll!CEnum::CreateDeviceImpl(unsigned int, enum _D3DDEVTYPE, struct HWND__ *, unsigned long, struct _D3DPRESENT_PARAMETERS_ *, struct D3DDISPLAYMODEEX const *, struct IDirect3DDevice9Ex * *)	Unknown
            d3d9.dll!CEnum::CreateDevice(unsigned int, enum _D3DDEVTYPE, struct HWND__ *, unsigned long, struct _D3DPRESENT_PARAMETERS_ *, struct IDirect3DDevice9 * *)	Unknown
            IGO32.dll!OriginIGO::D3D9Init(HINSTANCE__ * hD3D9, bool calculateOffsetsOnly) Line 1506	C++
            IGO32.dll!OriginIGO::Direct3DCreate9Hook(unsigned int SDK_VERSION) Line 1730	C++
            TS4.exe!0107d1b0()	Unknown
            */

            if (Direct3DCreate9HookMutex.IsReadLocked())
            {
                DX9Hook::mInstanceHookMutex.Unlock();
                return isHooked;
            }

            mLastHookTime = GetTickCount();

            if (calculateOffsetsOnly)
            {
                UNHOOK_SAFE(Direct3DCreate9Hook);
                HOOKAPI_SAFE("d3d9.dll", "Direct3DCreate9", Direct3DCreate9Hook);   // do D3D9Init(...) from the create call and calculate the offsets from there!!!
            }
            else
            {

                // do not hook another GFX api, if we already have IGO running in this process!!!
                SAFE_CALL_LOCK_AUTO
                    if (IGOApplication::instance() != NULL && (IGOApplication::instance()->rendererType() != RendererDX9))
                    {
                        UNHOOK_SAFE(Direct3DCreate9Hook);   // unhook this call, otherwise PunkBuster will kick us!
                        DX9Hook::mInstanceHookMutex.Unlock();
                        return false;
                    }

                HMODULE hD3D9 = GetModuleHandle(L"d3d9.dll");
                if (hD3D9)
                {
                    isHooked = D3D9Init(hD3D9, calculateOffsetsOnly) && !calculateOffsetsOnly;  // do not set isHooked, when we use calculateOffsetsOnly
                    if (isHooked)
                        CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_INFO, TelemetryContext_GENERIC, TelemetryRenderer_DX9, "Hooked at least once"))

                    UNHOOK_SAFE(Direct3DCreate9Hook);   // unhook this call, otherwise PunkBuster will kick us!
                }
            }

            DX9Hook::mInstanceHookMutex.Unlock();
        }
        return isHooked;
    }

    bool DX9Hook::IsReadyForRehook()
    {
        // limit re-hooking to have a 15 second timeout!
        EA::Thread::AutoFutex lock(DX9Hook::mInstanceHookMutex);
        if (
            (!DX9Hook::mDX9Initialized && !gSDKSignaled)    // if we have a signal from the SDK, don't rely on mDX9Initialized, because we may never reach Direct3DCreate9Hook
            || (GetTickCount() - mLastHookTime < REHOOKCHECK_DELAY))
            return false;

        return true;
    }

    bool DX9Hook::IsBroken()
    {
        bool broken = false;

        if (!isHooked)
            return broken;

        if (!OriginIGO::CheckHook((LPVOID *)&(SwapChainPresentHookNext), SwapChainPresentHook, true) ||
            !OriginIGO::CheckHook((LPVOID *)&(PresentHookNext), PresentHook, true) ||
            !OriginIGO::CheckHook((LPVOID *)&(ReleaseHookNext), ReleaseHook, true) ||
            !OriginIGO::CheckHook((LPVOID *)&(ResetHookNext), ResetHook, true) ||
            !OriginIGO::CheckHook((LPVOID *)&(TestCooperativeLevelHookNext), TestCooperativeLevelHook, true) ||
            !OriginIGO::CheckHook((LPVOID *)&(SetGammaRampHookNext), SetGammaRampHook, true))
        {
            IGOLogWarn("DX9Hook broken.");
            CALL_ONCE_ONLY(TelemetryDispatcher::instance()->Send(TelemetryFcn_IGO_HOOKING_FAIL, TelemetryContext_THIRD_PARTY, TelemetryRenderer_DX9, "DX9Hook is broken."));
            broken = true;

            // disable logging, if IGO is broken
            if (OriginIGO::IGOLogger::instance())
            {
                OriginIGO::IGOLogger::instance()->enableLogging(getIGOConfigurationFlags().gEnabledLogging);
            }

        }

        return broken;
    }

    void DX9Hook::Cleanup()
    {
        DX9Hook::mInstanceHookMutex.Lock();
        
	    IGOLogInfo("DX9Hook::Cleanup()");
        if (!IsBroken())
        {
            UNHOOK_SAFE(SwapChainPresentHook);
            UNHOOK_SAFE(PresentHook);
            UNHOOK_SAFE(TestCooperativeLevelHook);
            UNHOOK_SAFE(ResetHook);
            UNHOOK_SAFE(SetGammaRampHook);
            UNHOOK_SAFE(ReleaseHook);

            // only kill IGO if it was created by this render
            SAFE_CALL_LOCK_ENTER
                if (IGOApplication::instance() != NULL && (IGOApplication::instance()->rendererType() == RendererDX9))
                {
                bool safeToCall = SAFE_CALL(IGOApplication::instance(), &IGOApplication::isThreadSafe);
                SAFE_CALL_LOCK_LEAVE

                    if (safeToCall)
                    {
                        IGOApplication::deleteInstance();
                        ReleaseD3D9Objects();
                    }
                    else
                        IGOApplication::deleteInstanceLater();
                }
                else
                {
                    SAFE_CALL_LOCK_LEAVE
                }
            isHooked = false;
        }
        DX9Hook::mInstanceHookMutex.Unlock();
    }

    enum DX9Hook::ShaderSupport DX9Hook::ShaderSupport()
    {
        return gShaderSupport;
    }
}
#endif // ORIGIN_PC
