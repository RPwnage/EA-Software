#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginSDKimpl.h"
#include <assert.h>
#include "OriginLSXRequest.h"
#include "OriginLSXEnumeration.h"

namespace Origin
{
	OriginErrorT OriginSDK::IsProgressiveInstallationAvailable(const OriginCharT *itemId, OriginIsProgressiveInstallationAvailableCallback callback, void * pContext, uint32_t timeout)
	{
		if(callback == NULL)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

		IObjectPtr< LSXRequest<lsx::IsProgressiveInstallationAvailableT, lsx::IsProgressiveInstallationAvailableResponseT, OriginIsProgressiveInstallationAvailableT, OriginIsProgressiveInstallationAvailableCallback>> req(
			new LSXRequest<lsx::IsProgressiveInstallationAvailableT, lsx::IsProgressiveInstallationAvailableResponseT, OriginIsProgressiveInstallationAvailableT, OriginIsProgressiveInstallationAvailableCallback>
			    (GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this, &OriginSDK::IsProgressiveInstallationAvailableConvertData, callback, pContext));

		lsx::IsProgressiveInstallationAvailableT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId ;
		
		if(req->ExecuteAsync(timeout))
		{
			return REPORTERROR(ORIGIN_SUCCESS);
		}
		else
		{
			OriginErrorT err = req->GetErrorCode();
			return REPORTERROR(err);
		}
	}

	OriginErrorT OriginSDK::IsProgressiveInstallationAvailableSync(const OriginCharT *itemId, bool *bAvailable, uint32_t timeout)
	{
		if(bAvailable == NULL)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

		IObjectPtr<LSXRequest<lsx::IsProgressiveInstallationAvailableT, lsx::IsProgressiveInstallationAvailableResponseT> > req( 
            new LSXRequest<lsx::IsProgressiveInstallationAvailableT, lsx::IsProgressiveInstallationAvailableResponseT>
                (GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this));

		lsx::IsProgressiveInstallationAvailableT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId ;

		if(req->Execute(timeout))
		{
			*bAvailable = req->GetResponse().Available;

			return REPORTERROR(ORIGIN_SUCCESS);
		}
		return REPORTERROR(req->GetErrorCode());
	}

	OriginErrorT OriginSDK::IsProgressiveInstallationAvailableConvertData(IXmlMessageHandler * /*pHandler*/, OriginIsProgressiveInstallationAvailableT &info, OriginSizeT &/*size*/, lsx::IsProgressiveInstallationAvailableResponseT &response)
	{
		info.ItemId = response.ItemId.c_str();
		info.bAvailable = response.Available;

		return REPORTERROR(ORIGIN_SUCCESS);
	}
	
	OriginErrorT OriginSDK::AreChunksInstalled(const OriginCharT *itemId, int32_t * pChunkIds, OriginSizeT chunkCount, OriginAreChunksInstalledCallback callback, void *pContext, uint32_t timeout)
    {
        if(chunkCount == 0 || pChunkIds == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::AreChunksInstalledT, lsx::AreChunksInstalledResponseT, OriginAreChunksInstalledInfoT, OriginAreChunksInstalledCallback> > req(
            new LSXRequest<lsx::AreChunksInstalledT, lsx::AreChunksInstalledResponseT, OriginAreChunksInstalledInfoT, OriginAreChunksInstalledCallback>
                (GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this, &OriginSDK::AreChunksInstalledConvertData, callback, pContext));

        lsx::AreChunksInstalledT &r = req->GetRequest();

        r.ItemId = itemId == NULL ? "" : itemId ;
        r.ChunkIds.assign(pChunkIds, pChunkIds + chunkCount);

        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
			OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }

    OriginErrorT OriginSDK::AreChunksInstalledSync(const OriginCharT *itemId, int32_t * pChunkIds, OriginSizeT chunkCount, bool *installed, uint32_t timeout)
    {
        if(chunkCount == 0 || pChunkIds == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        if(installed == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::AreChunksInstalledT, lsx::AreChunksInstalledResponseT> > req(
            new LSXRequest<lsx::AreChunksInstalledT, lsx::AreChunksInstalledResponseT>
                (GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this));

        lsx::AreChunksInstalledT &r = req->GetRequest();

        r.ItemId = itemId == NULL ? "" : itemId ;
        r.ChunkIds.assign(pChunkIds, pChunkIds + chunkCount);

        if(req->Execute(timeout))
        {
            *installed = req->GetResponse().Installed;

            return REPORTERROR(ORIGIN_SUCCESS);
        }
        return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::AreChunksInstalledConvertData(IXmlMessageHandler * /*pHandler*/, OriginAreChunksInstalledInfoT &installInfo, OriginSizeT &/*size*/, lsx::AreChunksInstalledResponseT &response)
    {
        installInfo.bInstalled = response.Installed;
        installInfo.ItemId = response.ItemId.c_str();
        installInfo.ChunkIds = response.ChunkIds.size()>0 ? &response.ChunkIds[0] : NULL;
        installInfo.ChunkIdsCount = response.ChunkIds.size();
        
        return REPORTERROR(ORIGIN_SUCCESS);
    }


	OriginErrorT OriginSDK::QueryChunkStatus(const OriginCharT *itemId, OriginEnumerationCallbackFunc callback, void *pContext, uint32_t timeout, OriginHandleT *pHandle)
	{
		if(callback == NULL)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        LSXEnumeration<lsx::QueryChunkStatusT, lsx::QueryChunkStatusResponseT, OriginChunkStatusT> * req = 
			new LSXEnumeration<lsx::QueryChunkStatusT, lsx::QueryChunkStatusResponseT, OriginChunkStatusT>
				(GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this, &OriginSDK::QueryChunkStatusConvertData, callback, pContext);

		OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(pHandle != NULL) *pHandle = h;

		lsx::QueryChunkStatusT &request = req->GetRequest();

		request.ItemId = itemId == NULL ? "" : itemId;

		if(req->ExecuteAsync(timeout))
		{
			return REPORTERROR(ORIGIN_SUCCESS);
		}
		else
		{
			OriginErrorT err = req->GetErrorCode();
            DestroyHandle(h);
            if (pHandle != NULL) *pHandle = NULL;
			return REPORTERROR(err);
		}

	}

	OriginErrorT OriginSDK::QueryChunkStatusSync(const OriginCharT *itemId, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
	{
		if(pTotalCount == NULL || pHandle == NULL)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);


		LSXEnumeration<lsx::QueryChunkStatusT, lsx::QueryChunkStatusResponseT, OriginChunkStatusT> * req =
			new LSXEnumeration<lsx::QueryChunkStatusT, lsx::QueryChunkStatusResponseT, OriginChunkStatusT>
				(GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this, &OriginSDK::QueryChunkStatusConvertData);

		lsx::QueryChunkStatusT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId;

		*pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

		if(req->Execute(timeout, pTotalCount))
		{
			return REPORTERROR(ORIGIN_SUCCESS);
		}
		return REPORTERROR(req->GetErrorCode());
	}

	OriginErrorT OriginSDK::QueryChunkStatusConvertData(IEnumerator * /*pEnumerator*/, OriginChunkStatusT *chunkStatus, size_t index, size_t count, lsx::QueryChunkStatusResponseT &response)
	{
		for(size_t i=index; i<index + count; i++)
		{
			OriginChunkStatusT &dest = chunkStatus[i];
			lsx::ChunkStatusT &src = response.ChunkStatus[i];

			dest.ItemId = src.ItemId.c_str();
			dest.Name = src.Name.c_str();
			dest.ChunkId = src.ChunkId;
			dest.Progress = src.Progress;
			dest.Size = src.Size;
			dest.Type = (enumChunkType)src.Type;
			dest.State = (enumChunkState)src.State;
			dest.ChunkETA = src.ChunkETA;
			dest.TotalETA = src.TotalETA;
		}
		return REPORTERROR(ORIGIN_SUCCESS);
	}

    OriginErrorT OriginSDK::IsFileDownloaded(const OriginCharT *itemId, const char *filePath, OriginIsFileDownloadedCallback callback, void *pContext, uint32_t timeout)
    {
        if(filePath == NULL || callback == NULL)
            return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::IsFileDownloadedT, lsx::IsFileDownloadedResponseT, OriginDownloadedInfoT, OriginIsFileDownloadedCallback> > req(
            new LSXRequest<lsx::IsFileDownloadedT, lsx::IsFileDownloadedResponseT, OriginDownloadedInfoT, OriginIsFileDownloadedCallback>
				(GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this, &OriginSDK::IsFileDownloadedConvertData, callback, pContext));

        lsx::IsFileDownloadedT &r = req->GetRequest();

        r.ItemId = itemId == NULL ? "" : itemId ;
        r.Filepath = filePath;

        if(req->ExecuteAsync(timeout))
        {
            return REPORTERROR(ORIGIN_SUCCESS);
        }
        else
        {
			OriginErrorT err = req->GetErrorCode();
            return REPORTERROR(err);
        }
    }
    OriginErrorT OriginSDK::IsFileDownloadedSync(const OriginCharT *itemId, const char *filePath, bool *downloaded, uint32_t timeout)
    {
		if(filePath == NULL || downloaded == NULL)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::IsFileDownloadedT, lsx::IsFileDownloadedResponseT> > req(
            new LSXRequest<lsx::IsFileDownloadedT, lsx::IsFileDownloadedResponseT>
                (GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this));

		lsx::IsFileDownloadedT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId ;
		r.Filepath = filePath;

		if(req->Execute(timeout))
		{
			const lsx::IsFileDownloadedResponseT &response = req->GetResponse();

			*downloaded = response.Downloaded;
			
			return REPORTERROR(ORIGIN_SUCCESS);
		}
		return REPORTERROR(req->GetErrorCode());
    }
    OriginErrorT OriginSDK::IsFileDownloadedConvertData(IXmlMessageHandler * /*pHandler*/, OriginDownloadedInfoT &fileStatus, OriginSizeT &/*size*/, lsx::IsFileDownloadedResponseT &response)
    {
		fileStatus.bDownloaded = response.Downloaded;
		fileStatus.ItemId = response.ItemId.c_str();
		fileStatus.FilePath = response.Filepath.c_str();

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::SetChunkPriority(const OriginCharT *itemId, int32_t * pChunckIds, OriginSizeT chunkCount, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout)
    {
		if(pChunckIds == NULL || chunkCount == 0)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::SetChunkPriorityT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback> > req(
			new LSXRequest<lsx::SetChunkPriorityT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback>
				(GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this, &OriginSDK::SetChunkPriorityConvertData, callback, pContext));

		lsx::SetChunkPriorityT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId ;
		r.ChunkIds.assign(pChunckIds, pChunckIds + chunkCount);

		if(req->ExecuteAsync(timeout))
		{
			return REPORTERROR(ORIGIN_SUCCESS);
		}
		else
		{
			OriginErrorT err = req->GetErrorCode();
			return REPORTERROR(err);
		}
    }
    OriginErrorT OriginSDK::SetChunkPrioritySync(const OriginCharT *itemId, int32_t * pChunckIds, OriginSizeT chunkCount, uint32_t timeout)
    {
		if(itemId == NULL || pChunckIds == NULL || chunkCount == 0)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::SetChunkPriorityT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::SetChunkPriorityT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this));

		lsx::SetChunkPriorityT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId ;
		r.ChunkIds.assign(pChunckIds, pChunckIds + chunkCount);

		if(req->Execute(timeout))
		{
			const lsx::ErrorSuccessT &response = req->GetResponse();

			return REPORTERROR(response.Code);
		}
		return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::SetChunkPriorityConvertData(IXmlMessageHandler * /*pHandler*/, int &/*dummy*/, OriginSizeT &/*size*/, lsx::ErrorSuccessT &response)
    {
        return response.Code;
    }

    OriginErrorT OriginSDK::GetChunkPriority(const OriginCharT *itemId, OriginChunkPriorityCallback callback, void *pContext, uint32_t timeout)
    {
		if(callback == NULL)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetChunkPriorityT, lsx::GetChunkPriorityResponseT, OriginChunkPriorityInfoT, OriginChunkPriorityCallback> > req(
			new LSXRequest<lsx::GetChunkPriorityT, lsx::GetChunkPriorityResponseT, OriginChunkPriorityInfoT, OriginChunkPriorityCallback>
				(GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this, &OriginSDK::GetChunkPriorityConvertData, callback, pContext));

		lsx::GetChunkPriorityT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId ;
		
		if(req->ExecuteAsync(timeout))
		{
			return REPORTERROR(ORIGIN_SUCCESS);
		}
		else
		{
			OriginErrorT err = req->GetErrorCode();
			return REPORTERROR(err);
		}
    }

    OriginErrorT OriginSDK::GetChunkPrioritySync(const OriginCharT *itemId, OriginCharT * itemIdOut, OriginSizeT * itemIdOutSize, int32_t * pChunckIds, OriginSizeT * chunkCount, uint32_t timeout)
    {
		if(itemId == NULL || pChunckIds == NULL || chunkCount == 0)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::GetChunkPriorityT, lsx::GetChunkPriorityResponseT> > req(
            new LSXRequest<lsx::GetChunkPriorityT, lsx::GetChunkPriorityResponseT>
                (GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this));

		lsx::GetChunkPriorityT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId ;

		if(req->Execute(timeout))
		{
			const lsx::GetChunkPriorityResponseT &response = req->GetResponse();

			if(*chunkCount<response.ChunkIds.size())
			{
				*chunkCount = response.ChunkIds.size();
				return REPORTERROR(ORIGIN_ERROR_BUFFER_TOO_SMALL);
			}

			if(*itemIdOutSize<response.ItemId.length() + 1)
			{
				*itemIdOutSize = response.ItemId.length() + 1;
				return REPORTERROR(ORIGIN_ERROR_BUFFER_TOO_SMALL);
			}

			memcpy(pChunckIds, &response.ChunkIds[0], sizeof(int32_t)*response.ChunkIds.size());
            *chunkCount = response.ChunkIds.size();

			memcpy(itemIdOut, response.ItemId.c_str(), response.ItemId.length() + 1);
			*itemIdOutSize = response.ItemId.length();

			return REPORTERROR(ORIGIN_SUCCESS);
		}
		return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::GetChunkPriorityConvertData(IXmlMessageHandler * /*pHandler*/, OriginChunkPriorityInfoT &chunkInfo, OriginSizeT &/*size*/, lsx::GetChunkPriorityResponseT &response)
    {
        chunkInfo.ItemId = response.ItemId.c_str();
		chunkInfo.ChunkIds = &response.ChunkIds[0];
		chunkInfo.ChunkIdsCount = response.ChunkIds.size();

        return REPORTERROR(ORIGIN_SUCCESS);
    }

    OriginErrorT OriginSDK::QueryChunkFiles(const OriginCharT *itemId, int32_t chunkId, OriginEnumerationCallbackFunc callback, void *pContext, uint32_t timeout, OriginHandleT *pHandle)
    {
        if(callback == NULL)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

		LSXEnumeration<lsx::QueryChunkFilesT, lsx::QueryChunkFilesResponseT, const char *> * req = 
			new LSXEnumeration<lsx::QueryChunkFilesT, lsx::QueryChunkFilesResponseT, const char *>
					(GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this, &OriginSDK::QueryChunkFilesConvertData, callback, pContext);

		OriginHandleT h = RegisterResource(OriginResourceContainerT::Enumerator, req);

        if(pHandle != NULL) *pHandle = h;

		lsx::QueryChunkFilesT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId;
		r.ChunkId = chunkId;

		if(req->ExecuteAsync(timeout))
		{
			return REPORTERROR(ORIGIN_SUCCESS);
		}
		else
		{
			OriginErrorT err = req->GetErrorCode();
            DestroyHandle(h);
            if (pHandle != NULL) *pHandle = NULL;
            return REPORTERROR(err);
		}
    }

    OriginErrorT OriginSDK::QueryChunkFilesSync(const OriginCharT *itemId, int32_t chunkId, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *pHandle)
    {
		if(pTotalCount == NULL || pHandle == NULL || chunkId<0)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

		
		LSXEnumeration<lsx::QueryChunkFilesT, lsx::QueryChunkFilesResponseT, const char *> * req =
			new LSXEnumeration<lsx::QueryChunkFilesT, lsx::QueryChunkFilesResponseT, const char *>
				(GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this, &OriginSDK::QueryChunkFilesConvertData);

		lsx::QueryChunkFilesT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId;
		r.ChunkId = chunkId;

		*pHandle = RegisterResource(OriginResourceContainerT::Enumerator, req);

		if(req->Execute(timeout, pTotalCount))
		{
			return REPORTERROR(ORIGIN_SUCCESS);
		}
		return REPORTERROR(req->GetErrorCode());
    }

    OriginErrorT OriginSDK::QueryChunkFilesConvertData(IEnumerator * /*pEnumerator*/, const char **filenames, size_t index, size_t count, lsx::QueryChunkFilesResponseT &response)
    {
		for(size_t i=index; i<index + count; i++)
		{
			filenames[i] = response.Files[i].c_str();
		}
        return REPORTERROR(ORIGIN_SUCCESS);
    }



	OriginErrorT OriginSDK::CreateChunk(const OriginCharT *itemId, const OriginCharT **files, OriginSizeT filesCount, OriginCreateChunkCallback callback, void *pContext, uint32_t timeout)
	{
		if(files == NULL || filesCount == 0)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::CreateChunkT, lsx::CreateChunkResponseT, int, OriginCreateChunkCallback> > req(
			new LSXRequest<lsx::CreateChunkT, lsx::CreateChunkResponseT, int, OriginCreateChunkCallback>
			(GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this, &OriginSDK::CreateChunkConvertData, callback, pContext));

		lsx::CreateChunkT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId;
		r.Files.resize(filesCount);

		for(unsigned int i=0; i<filesCount; i++)
		{
			r.Files[i] = files[i];
		}

		if(req->ExecuteAsync(timeout))
		{
			return REPORTERROR(ORIGIN_SUCCESS);
		}
		else
		{
			OriginErrorT err = req->GetErrorCode();
			return REPORTERROR(err);
		}
	}

	OriginErrorT OriginSDK::CreateChunkSync(const OriginCharT *itemId, const OriginCharT **files, OriginSizeT filesCount, int32_t *pChunkId, uint32_t timeout)
	{
		if(files == NULL || filesCount == 0)
			return REPORTERROR(ORIGIN_ERROR_INVALID_ARGUMENT);

        IObjectPtr< LSXRequest<lsx::CreateChunkT, lsx::CreateChunkResponseT> > req(
            new LSXRequest<lsx::CreateChunkT, lsx::CreateChunkResponseT>
                (GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this));

		lsx::CreateChunkT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId;
		r.Files.resize(filesCount);

		for(unsigned int i=0; i<filesCount; i++)
		{
			r.Files[i] = files[i];
		}

		if(req->Execute(timeout))
		{
			if(pChunkId)
			{
				*pChunkId = req->GetResponse().ChunkId;
			}
			return REPORTERROR(req->GetErrorCode());
		}
		return REPORTERROR(req->GetErrorCode());
	}

	OriginErrorT OriginSDK::CreateChunkConvertData(IXmlMessageHandler * /*pHandler*/, int &chunkId, OriginSizeT &size, lsx::CreateChunkResponseT &response)
	{
		chunkId = response.ChunkId;
		size = 1;

		return REPORTERROR(ORIGIN_SUCCESS);
	}



	OriginErrorT OriginSDK::StartDownload(const OriginCharT *itemId, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout)
	{
        IObjectPtr< LSXRequest<lsx::StartDownloadT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback> > req(
			new LSXRequest<lsx::StartDownloadT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback>
				(GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this, &OriginSDK::StartDownloadConvertData, callback, pContext));

		lsx::StartDownloadT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId;

		if(req->ExecuteAsync(timeout))
		{
			return REPORTERROR(ORIGIN_SUCCESS);
		}
		else
		{
			OriginErrorT err = req->GetErrorCode();
			return REPORTERROR(err);
		}
	}

	OriginErrorT OriginSDK::StartDownloadSync(const OriginCharT *itemId, uint32_t timeout)
	{
        IObjectPtr< LSXRequest<lsx::StartDownloadT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::StartDownloadT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this));

		lsx::StartDownloadT &r = req->GetRequest();

		r.ItemId = itemId == NULL ? "" : itemId;

		if(req->Execute(timeout))
		{
			return REPORTERROR(req->GetResponse().Code);
		}
		return REPORTERROR(req->GetErrorCode());
	}

	OriginErrorT OriginSDK::StartDownloadConvertData(IXmlMessageHandler * /*pHandler*/, int &/*dummy*/, OriginSizeT &/*size*/, lsx::ErrorSuccessT &response)
	{
		return response.Code;
	}

	OriginErrorT OriginSDK::SetDownloadUtilization(float utilization, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout)
	{
        IObjectPtr< LSXRequest<lsx::SetDownloaderUtilizationT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback> > req(
			new LSXRequest<lsx::SetDownloaderUtilizationT, lsx::ErrorSuccessT, int, OriginErrorSuccessCallback>
			    (GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this, &OriginSDK::SetDownloadUtilizationConvertData, callback, pContext));

		lsx::SetDownloaderUtilizationT &r = req->GetRequest();

		r.Utilization = utilization;

		if(req->ExecuteAsync(timeout))
		{
			return REPORTERROR(ORIGIN_SUCCESS);
		}
		else
		{
			OriginErrorT err = req->GetErrorCode();
			return REPORTERROR(err);
		}
	}

	OriginErrorT OriginSDK::SetDownloadUtilizationSync(float utilization, uint32_t timeout)
	{
        IObjectPtr< LSXRequest<lsx::SetDownloaderUtilizationT, lsx::ErrorSuccessT> > req(
            new LSXRequest<lsx::SetDownloaderUtilizationT, lsx::ErrorSuccessT>
                (GetService(lsx::FACILITY_PROGRESSIVE_INSTALLATION), this));

		lsx::SetDownloaderUtilizationT &r = req->GetRequest();

		r.Utilization = utilization;

		if(req->Execute(timeout))
		{
			return REPORTERROR(req->GetResponse().Code);
		}
		return REPORTERROR(req->GetErrorCode());
	}

	OriginErrorT OriginSDK::SetDownloadUtilizationConvertData(IXmlMessageHandler * /*pHandler*/, int &/*dummy*/, OriginSizeT &/*size*/, lsx::ErrorSuccessT &response)
	{
		return response.Code;
	}



}    


