using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EnvironmentMonitor
{
    class BlazeModuleParam : ModuleParam
    {
        public System.String mRedirevtorURL;
        public System.String mPlatform;
        public System.String mGOSEnv;
        public System.String mGameTitle;
        public System.String mGameYear;
        public System.String mBlazeServerName {get;set;}
        public System.String mBlazeHostName;
        public System.String mBlazeServerIP;
        public System.String mBlazePort;
        public System.String mBlazeServerVersion;        
    }

    class NucleusModuleParam : ModuleParam
    {
        public System.String mNucleusConnect;
        public System.String mNucleusConnectTrusted;
        public System.String mNucleusProxy;
        public System.String mNucleusRedirect;    

        public System.String mBlazeSDKClientId = "FIFA-16-PS4-Client";
        public System.String mBlazeSDKClientSecret = "TTajH90hgvaSmcE7XwzEgjXS3EaAcc9TU72r7TwbcwTnxmkkIJMPQAvH2t82v5Ld3Cu0jPZl9GywSGFp";
        public System.String mBlazeServerClientId = "FIFA-16-PS4-Blaze-Server";
        
        public System.String mClientAuthCode = string.Empty;
        public System.String mClientAuthToken = string.Empty;
        public System.String mServerAuthCode = string.Empty;
        public System.String mLionAuthCode = string.Empty;
    }

    class BlazeUserParam : ModuleParam
    {
        public long mBlazeId = 0;
        public long mPersonaId = 0;
        public long mNucleusId = 0;
        public System.String mEmail = string.Empty;
        public System.String mSessionKey = string.Empty;
        public System.String mDisplayName = string.Empty;
        public System.String mClientPlatform = string.Empty;
    }

    class CMSModuleParam : ModuleParam
    {
        public System.String mCMS_BASE_URL;
        public System.String mCMS_API_KEY;
        public System.String mCMS_HOSP_SKU_KC;
        public System.String mCMS_SKU;
        public System.String mCMS_UPLOAD_PERMISSION_URL;
        public System.String mCMS_UPLOAD_PERMISSION_PARAMS;

        public System.String mCONTENT_URL;
        public System.String mROUTINGCFGFILE_URL;

        public System.String mCTSAuthCode;
        public System.String mOSDK_EASW_AUTH_URL; // Services URL
        public System.String mOSDK_EASW_GF_FILE_URL; // Content URL
        public System.String mGameFaceBinaryPath;
    }

    class EASFCModuleParam : ModuleParam
    {
        public System.String mFIFA_POW_URL;
        public System.String mFIFA_POW_CONTENT_SERVER_URL;
        public System.String mFIFA_POW_NUCLEUS_PROXY_URL;
        public System.String mFIFA_POW_MMM_URI;
        public System.String mFIFA_RS4_URL;

        public System.String mEasfcSessionId;

        public object mChallengeData;

    }    

    class FUTModuleParam : ModuleParam
    {
        public System.String mFUT_RS4_ADMIN_URL;
        public System.String mFUT_RS4_BASE_URL;
        public System.String mFUTBOOTCFGFILE_URL;

        public System.String mFutSessionId;
        public System.String mFutDBLoc;
        public System.String mFutDBVer;
        public System.String mFutDBCRC;
    }

    class EnvironmentConfigParam
    {
        public BlazeModuleParam     mBlazeParam;
        public BlazeUserParam       mBlazeUserParam;
        public NucleusModuleParam   mNucleusParam;
        public CMSModuleParam       mCMSParam;
        public EASFCModuleParam     mEASFCParam;
        public FUTModuleParam       mFUTParam;

        private static EnvironmentConfigParam sInstance = null;

        public EnvironmentConfigParam()
        {
            mBlazeParam     = new BlazeModuleParam();
            mNucleusParam   = new NucleusModuleParam();
            mCMSParam       = new CMSModuleParam();
            mEASFCParam     = new EASFCModuleParam();
            mFUTParam       = new FUTModuleParam();
            mBlazeUserParam = new BlazeUserParam();
        }

        static public EnvironmentConfigParam getInstance()
        {
            if (sInstance == null)
            {
                sInstance = new EnvironmentConfigParam();
            }

            return sInstance;
        }
    }
}
