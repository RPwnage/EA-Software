#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

#include <QMap>
#include <QSet>
#include <QLinkedList>
#include <QString>

#include "services/downloader/DownloadService.h"

namespace Origin
{
namespace Downloader
{
    class DownloadPriorityGroup
    {
    public:
        DownloadPriorityGroup() : mPriorityGroupId(0), mEnabled(true) { }
        int mPriorityGroupId;
        bool mEnabled;
        QList<RequestId> mRequests;
    };

    class DownloadPriorityQueue
    {
       public:
        class DownloadPriorityQueueIterator
        {
            friend class DownloadPriorityQueue;

            DownloadPriorityQueueIterator(DownloadPriorityQueue& queue);
        public:

            // Bare minimum set of iterator methods
            bool valid() const;
            RequestId current() const;
            int currentGroupId() const;
            void next();

        private:
            bool inner_next();

        private:
            DownloadPriorityQueue& mQueue;
            bool mIsValid;
            QSet<RequestId> mAlreadyIterated;
            QLinkedList<DownloadPriorityGroup>::const_iterator mPriorityGroupsIter;
            QList<RequestId>::const_iterator mSingleGroupIter;
        };
    public:
        DownloadPriorityQueueIterator constIterator();
        void Insert(QMap<int, QList<RequestId> > requestsByGroup);
        void Insert(RequestId id, int groupId, bool enabled);
        void Insert(QVector<RequestId> requests, int groupId, bool enabled);
        void MoveToTop(int priorityGroupId);
        void SetQueueOrder(QVector<int> priorityGroupIds);
        void SetGroupEnabledStatus(int groupId, bool enabled);
        QLinkedList<DownloadPriorityGroup>& GetQueue();

    private:
        QLinkedList<DownloadPriorityGroup> mPriorityGroups;
    };
} // Downloader
} // Origin
#endif // PRIORITYQUEUE_H
