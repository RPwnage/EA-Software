#ifndef __ORIGIN_PROGRESSIVE_INSTALLATION_H__
#define __ORIGIN_PROGRESSIVE_INSTALLATION_H__

#include "OriginTypes.h"
#include "OriginEnumeration.h"

#ifdef __cplusplus
extern "C" 
{
#endif

    /// \defgroup progressive Progressive Installation
    /// \brief This module contains the functions for dealing with progressive installation.
    /// 
    /// There are sychronous and asynchronous versions for most of the functions in progressive installation. For an explanation of
    /// why, see \ref syncvsasync in the <em>Integrator's Guide</em>.
    /// 
    /// For more information on the integration of this feature, see \ref ga-intguide-profile in the <em>Integrator's Guide</em>.

	/// \ingroup progressive
	/// \brief Check whether Progressive Installation is available for this title.
	/// If this function returns false the Progressive Installation support in Origin is not available. The game title should expect all the base game data to be installed.
	/// \param[in] itemId The offerId allows for checking the install status of DLC content. For the main game, pass in NULL.
	/// \param[in] callback Game defined callback to receive the result of the request.
	/// \param[in] pContext A game defined context that will be passed back into the callback. This usually is a pointer to a handler class.
	/// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
	OriginErrorT ORIGIN_SDK_API OriginIsProgressiveInstallationAvailable(const OriginCharT *itemId, OriginIsProgressiveInstallationAvailableCallback callback, void * pContext, uint32_t timeout);

	/// \ingroup progressive
	/// \brief Check whether Progressive Installation is available for this title.
	/// If this function returns false the Progressive Installation support in Origin is not available. The game title should expect all the base game data to be installed.
	/// \param[in] itemId The offerId allows for checking the install status of DLC content. For the main game, pass in NULL.
	/// \param[in] bAvailable A boolean value indicating whether the Progressive Installation API is available.
	/// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
	OriginErrorT ORIGIN_SDK_API OriginIsProgressiveInstallationAvailableSync(const OriginCharT *itemId, bool *bAvailable, uint32_t timeout);

	/// \ingroup progressive
    /// \brief Check whether the chunks you are interested in are installed.
    /// This function will do a query to check whether all the chunks you are interested in are installed. All the chunks need to be 
    /// downloaded and installed in order for the result of this function to be true.
    /// \param[in] itemId The offerId allows for checking the install status of DLC content. For the main game, pass in NULL.
    /// \param[in] pChunkIds An array of chunk Ids you are interested in. The Ids should range from [1-N], as the 0 chunk is the base chunk which should be installed, otherwise your game will not run.
    /// \param[in] chunkCount The size of the pChunkIds array.
    /// \param[in] callback The function to call when this function completes.
    /// \param[in] pContext A game defined context that will be passed back into the callback. This usually is a pointer to a handler class.
    /// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
    /// \return Indication whether the command was submitted successfully to Origin.
    OriginErrorT ORIGIN_SDK_API OriginAreChunksInstalled(const OriginCharT *itemId, int32_t * pChunkIds, OriginSizeT chunkCount, OriginAreChunksInstalledCallback callback, void *pContext, uint32_t timeout);

	/// \ingroup progressive
    /// \brief Check whether the chunks you are interested in are installed.
    /// This function will do a query to check whether all the chunks you are interested in are installed. All the chunks need to be 
    /// downloaded and installed in order for the result of this function to be true.
    /// \param[in] itemId The offerId allows for checking install status of DLC content. For the main game, pass in NULL.
    /// \param[in] pChunkIds An array of chunk Ids you are interested in. The Ids should range from [1-N], as the 0 chunk is the base chunk which should be installed, otherwise your game will not run.
    /// \param[in] chunkCount The size of the pChunkIds array.
    /// \param[out] installed The variable that will receive the result of the query.
    /// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
    /// \return Returns whether the operation was succeeded successfully. If not it will indicate what went wrong. 
    OriginErrorT ORIGIN_SDK_API OriginAreChunksInstalledSync(const OriginCharT *itemId, int32_t * pChunkIds, OriginSizeT chunkCount, bool *installed, uint32_t timeout);

	/// \ingroup progressive
    /// \brief Get the status of a chunk.
    /// This function will do a query to check the status of the chunk.
    /// \param[in] itemId The offerId allows for checking the status of DLC content. For the main game, pass in NULL.
    /// \param[in] callback The function to call when this function completes.
    /// \param[in] pContext A game defined context that will be passed back into the callback. This usually is a pointer to a handler class.
    /// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
	/// \param[out] handle A handle to the enumerator object.    
    /// \return Indication whether the command was submitted successfully to Origin.
    OriginErrorT ORIGIN_SDK_API OriginQueryChunkStatus(const OriginCharT *itemId, OriginEnumerationCallbackFunc callback, void *pContext, uint32_t timeout, OriginHandleT *handle);

	/// \ingroup progressive
    /// \brief Get the status of a chunk.
    /// This function will do a query to check the status of the chunk.
    /// \param[in] itemId The offerId allows for checking the status of DLC content. For the main game, pass in NULL.
    /// \param[out] pTotalCount The total number of chunk records.
    /// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
	/// \param[out] handle A handle to the enumerator object.    
    /// \return Returns whether the operation was succeeded successfully. If not it will indicate what went wrong. The status will not be valid 
    OriginErrorT ORIGIN_SDK_API OriginQueryChunkStatusSync(const OriginCharT *itemId, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *handle);

	/// \ingroup progressive
    /// \brief Checks whether a file is downloaded.
    /// This function will check whether a file if downloaded. The file needs to be in one of the chunks belonging to the game.
	/// NOTE: This cannot be used to determine whether a file is downloaded before the chunk is installed. It just reports back whether the file is in a completed chunk or not.
    /// \param[in] itemId The offerId allows for checking the file download status for files in DLC content. For the main game, pass in NULL.
    /// \param[in] filePath The file to check. This should be an absolute path.
    /// \param[in] callback The function to call when this function completes.
    /// \param[in] pContext A game defined context that will be passed back into the callback. This usually is a pointer to a handler class.
    /// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
    /// \return Indication whether the command was submitted successfully to Origin.
    OriginErrorT ORIGIN_SDK_API OriginIsFileDownloaded(const OriginCharT *itemId, const char *filePath, OriginIsFileDownloadedCallback callback, void *pContext, uint32_t timeout);


	/// \ingroup progressive
    /// \brief Checks whether a file is downloaded.
    /// This function will check whether a file if downloaded. The file needs to be in one of the chunks belonging to the game.
	/// NOTE: This cannot be used to determine whether a file is downloaded before the chunk is installed. It just reports back whether the file is in a completed chunk or not.
    /// \param[in] itemId The offerId allows for checking the file download status for files in DLC content. For the main game, pass in NULL.
    /// \param[in] filePath The file to check. This should be an absolute path.
    /// \param[out] downloaded The variable that will receive the result of the query.
    /// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
    /// \return Indication whether the command was submitted successfully to Origin.
    OriginErrorT ORIGIN_SDK_API OriginIsFileDownloadedSync(const OriginCharT *itemId, const char *filePath, bool *downloaded, uint32_t timeout);

	/// \ingroup progressive
    /// \brief Override the download order of the chunks
    /// \param[in] itemId The offerId allows for reordering the priority of chunks for DLC content. For the main game, pass in NULL.
    /// \param[in] pChunckIds An array of chunkIds. The id in the beginning of the array will be downloaded first. Note: Only include ORIGIN_CHUNK_TYPE_ON_DEMAND chunks if you want them to start downloading.
    /// \param[in] chunkCount The number of elements in the chunk array.
    /// \param[in] callback The function to call when this function completes.
    /// \param[in] pContext A game defined context that will be passed back into the callback. This usually is a pointer to a handler class.
    /// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
    /// \return Indication whether the command was submitted successfully to Origin.
    OriginErrorT ORIGIN_SDK_API OriginSetChunkPriority(const OriginCharT *itemId, int32_t * pChunckIds, OriginSizeT chunkCount, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout);

	/// \ingroup progressive
    /// \brief Override the download order of the chunks
    /// \param[in] itemId The offerId allows for reordering the priority of chunks for DLC content. For the main game, pass in NULL.
    /// \param[in] pChunckIds An array of chunkIds. The id in the beginning of the array will be downloaded first.
    /// \param[in] chunkCount The number of elements in the chunk array.
    /// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
    /// \return Indication whether the command was submitted successfully to Origin.
    OriginErrorT ORIGIN_SDK_API OriginSetChunkPrioritySync(const OriginCharT *itemId, int32_t * pChunckIds, OriginSizeT chunkCount, uint32_t timeout);

	/// \ingroup progressive
    /// \brief Get the download order of the chunks
    /// \param[in] itemId The offerId specifies for what offerId you want the chunk priority. Pass in NULL if you asking for the main game.
    /// \param[in] callback The function to call when this function completes.
    /// \param[in] pContext A game defined context that will be passed back into the callback. This usually is a pointer to a handler class.
    /// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
    /// \return Indication whether the command was submitted successfully to Origin.
    OriginErrorT ORIGIN_SDK_API OriginGetChunkPriority(const OriginCharT *itemId, OriginChunkPriorityCallback callback, void *pContext, uint32_t timeout);

	/// \ingroup progressive
    /// \brief Get the download order of the chunks
    /// \param[in] itemId The offerId specifies for what offerId you want the chunk priority. Pass in NULL if you asking for the main game.
	/// \param[out] itemIdOut The actual offerId belonging to the chunks.
	/// \param[in,out] itemIdOutSize The allocated size of the buffer to receive the actual itemId. Will receive the size of the item.
    /// \param[out] pChunckIds receives the array of chunkIds. The id in the beginning of the array will be downloaded first.
    /// \param[in,out] chunkCount The allocated size of the buffer to receive the chunkIds. Will receive the number of elements in the chunk array.
    /// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
    /// \return Indication whether the command was submitted successfully to Origin.
    OriginErrorT ORIGIN_SDK_API OriginGetChunkPrioritySync(const OriginCharT *itemId, OriginCharT * itemIdOut, OriginSizeT * itemIdOutSize, int32_t * pChunckIds, OriginSizeT * chunkCount, uint32_t timeout);

	/// \ingroup progressive
    /// \brief Get all the files in a chunk.
    /// \param[in] itemId The offerId specifies for what offerId you want the chunk priority. Pass in NULL if you asking for the main game.
    /// \param[in] chunkId The chunk Id of the chunk you want information for.
    /// \param[in] callback The function to call when this function completes.
    /// \param[in] pContext A game defined context that will be passed back into the callback. This usually is a pointer to a handler class.
    /// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
    /// \param[out] handle A handle to the enumerator object. This can be NULL.
    OriginErrorT ORIGIN_SDK_API OriginQueryChunkFiles(const OriginCharT *itemId, int32_t chunkId, OriginEnumerationCallbackFunc callback, void *pContext, uint32_t timeout, OriginHandleT *handle);

	/// \ingroup progressive
    /// \brief Get all the files in a chunk.
    /// \param[in] itemId The offerId specifies for what offerId you want the chunk priority. Pass in NULL if you asking for the main game.
    /// \param[in] chunkId The chunk Id of the chunk you want information for.
    /// \param[in] pTotalCount The number of files in the response.
    /// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
    /// \param[out] handle A handle to the enumerator object.    
    OriginErrorT ORIGIN_SDK_API OriginQueryChunkFilesSync(const OriginCharT *itemId, int32_t chunkId, OriginSizeT *pTotalCount, uint32_t timeout, OriginHandleT *handle);

	/// \ingroup progressive
	/// \brief Create a new chunk and have it be prioritized over the existing chunks.
	/// \param[in] itemId The offerId specifies for what offerId you want the chunk priority. Pass in NULL if you asking for the main game.
	/// \param[in] files An array of files to put in the chunk.
	/// \param[in] filesCount The number of files in the files array.
	/// \param[in] callback The function to call when this function completes.
	/// \param[in] pContext A game defined context that will be passed back into the callback. This usually is a pointer to a handler class.
	/// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
	OriginErrorT ORIGIN_SDK_API OriginCreateChunk(const OriginCharT *itemId, const OriginCharT **files, OriginSizeT filesCount, OriginCreateChunkCallback callback, void *pContext, uint32_t timeout);

	/// \ingroup progressive
	/// \brief Create a new chunk and have it be prioritized over the existing chunks.
	/// \param[in] itemId The offerId specifies for what offerId you want the chunk priority. Pass in NULL if you asking for the main game.
	/// \param[in] files An array of files to put in the chunk.
	/// \param[in] filesCount The number of files in the files array.
	/// \param[in] pChunkId The chunkId assigned to this chunk
	/// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
	OriginErrorT ORIGIN_SDK_API OriginCreateChunkSync(const OriginCharT *itemId, const OriginCharT **files, OriginSizeT filesCount, int32_t *pChunkId, uint32_t timeout);

	/// \ingroup progressive
	/// \brief Instruct Origin to start downloading a DLC package.
	/// \param[in] itemId The DLC to download.
	/// \param[in] callback The function to call when this function completes.
	/// \param[in] pContext A game defined context that will be passed back into the callback. This usually is a pointer to a handler class.
	/// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
	OriginErrorT ORIGIN_SDK_API OriginStartDownload(const OriginCharT *itemId, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout);

	/// \ingroup progressive
	/// \brief Instruct Origin to start downloading a DLC package.
	/// \param[in] itemId The DLC to download.
	/// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
	OriginErrorT ORIGIN_SDK_API OriginStartDownloadSync(const OriginCharT *itemId, uint32_t timeout);

	/// \ingroup progressive
	/// \brief Instruct Origin to increase/reduce download utilization. If the game wants Origin to reduce its use of network bandwidth it can use this.
	/// Only utilization 1 and 0 will be guaranteed, while any intermediate value is 'best effort'. Any changes to the utilization will take some time to take effect.
	/// \param[in] utilization The amount of utilization the game wants Origin to adhere to: 1.0f == Full Speed, 0.5f = Moderate, 0.1f = Trickle, 0.0f = Paused.
	/// \param[in] callback The function to call when this function completes.
	/// \param[in] pContext A game defined context that will be passed back into the callback. This usually is a pointer to a handler class.
	/// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
	OriginErrorT ORIGIN_SDK_API OriginSetDownloadUtilization(float utilization, OriginErrorSuccessCallback callback, void *pContext, uint32_t timeout);

	/// \ingroup progressive
	/// \brief Instruct Origin to increase/reduce download utilization. If the game wants Origin to reduce its use of network bandwidth it can use this.
	/// Only utilization 1 and 0 will be guaranteed, while any intermediate value is 'best effort'. Any changes to the utilization will take some time to take effect.
	/// \param[in] utilization The amount of utilization the game wants Origin to adhere to: 1.0f == Full Speed, 0.5f = Moderate, 0.1f = Trickle, 0.0f = Paused.
	/// \param[in] timeout The time the OriginSDK will wait for a reply from Origin in milliseconds 
	OriginErrorT ORIGIN_SDK_API OriginSetDownloadUtilizationSync(float utilization, uint32_t timeout);

#ifdef __cplusplus
}
#endif



#endif //__ORIGIN_PROGRESSIVE_INSTALLATION_H__