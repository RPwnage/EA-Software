#ifndef _CLOUDSAVES_CLOUDRESTOREFLOW_H
#define _CLOUDSAVES_CLOUDRESTOREFLOW_H

#include <QObject>
#include <QString>
#include <QFutureWatcher>
#include <QPointer>

#include "engine/cloudsaves/SaveFileCrawler.h"
#include "engine/cloudsaves/LocalStateBackup.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit
{
class OriginWindow;
}
}

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
	class LocalStateSnapshot;
	class LocalStateCalculator;
}
}
}

namespace Origin
{
namespace Client
{
	class ORIGIN_PLUGIN_API CloudRestoreFlow : public QObject
	{
		Q_OBJECT

		public:
			CloudRestoreFlow(const Engine::CloudSaves::LocalStateBackup &backup);
			~CloudRestoreFlow();

            void start();
			Origin::UIToolkit::OriginWindow* currentWindow() const {return mCurrentWindow;}

		signals:
			void finished(bool);

		private slots:
			void localStateCalculated(Origin::Engine::CloudSaves::LocalStateSnapshot*);
			void restoreConfirmed();
			void restoreComplete();
            void flowStop();
            void flowComplete();
			void closeCurrentWindow();

		private:
			void cleanLocalStateCalculator();

			QString mBackupFileName;

			const Engine::CloudSaves::LocalStateSnapshot *mCurrentState;
            Engine::CloudSaves::LocalStateCalculator *mLocalStateCalculator;

            Engine::CloudSaves::LocalStateBackup mBackup;

			QFutureWatcher<bool> mRestoreWatcher;
            Origin::UIToolkit::OriginWindow* mCurrentWindow;
	};

} 
}

#endif
