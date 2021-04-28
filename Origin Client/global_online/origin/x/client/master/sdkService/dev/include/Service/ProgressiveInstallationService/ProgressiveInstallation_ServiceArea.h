///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2011 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: ProgressiveInstallation_ServiceArea.h
//	Progressive Installation Service Area
//	
//	Author: Hans van Veenendaal

#ifndef _PROGRESSIVE_INSTALLATION_SERVICE_AREA_
#define _PROGRESSIVE_INSTALLATION_SERVICE_AREA_

#include "Service/BaseService.h"
#include <QVector>
#include <QMap>
#include <QList>
#include <QTimer>
#include <lsx.h>
#include "engine/content/DynamicContentTypes.h"

#define INCLUDE_DEBUG_PI_SIMULATOR_CODE // Use the PI simulator instead of the real downloader

namespace Origin
{
	namespace SDK
	{
		class ProgressiveInstallation_ServiceArea :	public BaseService
		{
			Q_OBJECT

#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
        public:
                struct ChunkInfo
                {
					ChunkInfo(){}

                    ChunkInfo(const QString &offerId, const QString &chunkName, int id, double p, long long s, lsx::ChunkTypeT t)
                    {
                        itemId = offerId;
                        name = chunkName;
                        chunkId = id;
                        progress = p;
						type = t;
                        size = s;
                    }
                    QString itemId;
                    QString name;
					lsx::ChunkTypeT type;
                    int chunkId;
                    double progress;
                    long long size;

                    QList<QString> files;
                };

                typedef QVector<ChunkInfo> ChunkInfoCollection;
                typedef QList<QString> DownloadOrder;
                typedef QMap<QString, ChunkInfoCollection > ChunkCollectionMap;
#endif

		public:
			// Singleton functions
			static ProgressiveInstallation_ServiceArea* instance();
			static ProgressiveInstallation_ServiceArea* create(Lsx::LSX_Handler* handler);
			void destroy();

			virtual ~ProgressiveInstallation_ServiceArea();

		private slots:
#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
			// Place your Origin client event slots here.
			void chunkUpdateEvent(const QString &offerId, const QString &name, int chunkId, float progress, long long size, int type, int state);
            void updateChunks();
#endif
            void onDDC_DynamicChunkUpdate(QString offerId, QString name, int chunkId, Origin::Engine::Content::DynamicDownloadChunkRequirementT chunkRequirement, float chunkProgress, qint64 chunkSize, qint64 chunkEta, qint64 totalEta, Origin::Engine::Content::DynamicDownloadChunkStateT chunkState);

		private:
			//	Handlers
			void IsProgressiveInstallationAvailableHandler(Lsx::Request& request, Lsx::Response& response );
			void GetChunkPriorityHandler(Lsx::Request& request, Lsx::Response& response );
			void SetChunkPriorityHandler(Lsx::Request& request, Lsx::Response& response );
			void AreChunksInstalledHandler(Lsx::Request& request, Lsx::Response& response );
			void QueryChunkStatusHandler(Lsx::Request& request, Lsx::Response& response );
			void QueryChunkFilesHandler(Lsx::Request& request, Lsx::Response& response );
			void IsFileDownloadedHandler(Lsx::Request& request, Lsx::Response& response );
			void CreateChunkHandler(Lsx::Request& request, Lsx::Response& response );
			void StartDownloadHandler(Lsx::Request& request, Lsx::Response& response );
            void SetDownloadUtilizationHandler(Lsx::Request& request, Lsx::Response& response );

			void CreateErrorResponse(Lsx::Response &response, int code, const char *description );

			// block constructors
			ProgressiveInstallation_ServiceArea( Lsx::LSX_Handler* handler );
			ProgressiveInstallation_ServiceArea();

#ifdef INCLUDE_DEBUG_PI_SIMULATOR_CODE
		private:
			int chunkETA( QString ItemId, int ChunkId );
            int totalETA( QString ItemId );
			int newChunkId( QString itemId );
			void resetChunks(const QString &baseGameOfferId);
            bool StartDownload(const QString &r);

			bool bUseSimulator;
        
            float downloadSpeed;
			float downloadUtilization;

            DownloadOrder prototypeDownloadOrder;
            ChunkCollectionMap prototypeDownloadedChunkList;  // These chunks are already downloaded.
            ChunkCollectionMap prototypeInprocessChunkList;   // These chunks are available for reordering.

            QTimer prototypeProgressTimer;
#endif
		};
	}
}

#endif //_PROGRESSIVE_INSTALLATION_SERVICE_AREA_
