/**
 * @file downloadmanager/downloadmanagerqueue.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-download-manager-queue',
        CLS_DOWNLOAD_FLYOUT_WRAPPER = 'origin-downloadflyout-wrapper',
        CLS_DOWNLOAD_FLYOUT_OPEN = 'origin-downloadflyout-isopen';

    /**
    * The Controller
    */
    function originDownloadManagerQueueCtrl($scope, GamesDataFactory, UtilFactory, ComponentsLogFactory, EventsHelperFactory, ComponentsConfigHelper) {

        var handles = [];

        $scope.isOIGContext = !!ComponentsConfigHelper.getOIGContextConfig();

        $scope.queueLoc = UtilFactory.getLocalizedStr($scope.queuestr, CONTEXT_NAMESPACE, 'queuestr');
        $scope.completeLoc = UtilFactory.getLocalizedStr($scope.completestr, CONTEXT_NAMESPACE, 'completestr');
        $scope.downloadManagerLoc = UtilFactory.getLocalizedStr($scope.downloadmanagerstr, CONTEXT_NAMESPACE, 'downloadmanagerstr');
        $scope.minimizeLoc = UtilFactory.getLocalizedStr($scope.minimizestr, CONTEXT_NAMESPACE, 'minimizestr');
        $scope.emptyQueueLoc = UtilFactory.getLocalizedStr($scope.emptyqueuestr, CONTEXT_NAMESPACE, 'emptyqueuestr');
        $scope.clearLoc = UtilFactory.getLocalizedStr($scope.clearstr, CONTEXT_NAMESPACE, 'clearstr');

        /**
         * Handle an error if the promise is rejected
         * @param {Exception} err the error message
         */
        function handleError(err) {
            ComponentsLogFactory.error('Could not retrieve entitlement list from client proxy:', err);
        }

        /**
        * Set download and completed queue
        * @method setQueue
        */
        function setQueue(queue) {
            $scope.downloadQueue = queue[0];
            $scope.completedQueue = queue[1];

            if($scope.toggleFlyout){
                $scope.toggleDownloadFlyout();
            }
            $scope.$digest();
        }

        /**
        * Update the completed and download queue when client operation queue event fires
        * @method updateQueue
        */
        function updateQueue(toggleFlyout) {
            // we need this to stop opening flyout on page load.
            $scope.toggleFlyout = toggleFlyout;

            return Promise.all([
                Origin.client.contentOperationQueue.entitlementsQueued(),
                Origin.client.contentOperationQueue.entitlementsCompletedOfferIdList()
            ])
            .then(setQueue)
            .catch(handleError);
        }

        /**
         * Clears the completed queue
         * @method clearCompleteQueue
         */
        $scope.clearCompleteQueue = function(event) {
            event.stopPropagation();
            Origin.client.contentOperationQueue.clearCompleteList();
            updateQueue(true);
        };

        /**
        * Open the download flyout
        @method openFlyout
        */
        function openFlyout() {
            if($scope.openFlyout){
                $scope.openFlyout();
            }
        }

        /**
        * Called when $scope is destroyed, release event handlers
        * @method onDestroy
        */
        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
        }

        handles = [
            Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_HEADCHANGED, updateQueue),
            Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_ENQUEUED, updateQueue),
            Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_ADDEDTOCOMPLETE, updateQueue),
            Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_REMOVED, updateQueue),
            Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_COMPLETELISTCLEARED, updateQueue),
            Origin.events.on(Origin.events.CLIENT_NAV_OPEN_DOWNLOADQUEUE, openFlyout)
        ];

        //need to wait for supercat to get loaded
        GamesDataFactory.waitForGamesDataReady()
            .then(updateQueue(false));

        $scope.$on('$destroy', onDestroy);

    }

    function originDownloadManagerQueueLink(scope) {

        var flyout = angular.element(document.querySelectorAll('.'+CLS_DOWNLOAD_FLYOUT_WRAPPER)[0]);

        /**
         * Open the flyout
         * @method openFlyout 
         */
        scope.openFlyout = function() {
            if(!flyout.hasClass(CLS_DOWNLOAD_FLYOUT_OPEN)) {
                flyout.addClass(CLS_DOWNLOAD_FLYOUT_OPEN);
            }
        };

        /**
         * Close the flyout when clicked in full screen mode
         * @method closeFlyout 
         */
        scope.closeFlyout = function(event) {
            event.stopPropagation();
            if(flyout.hasClass(CLS_DOWNLOAD_FLYOUT_OPEN)) {
                flyout.removeClass(CLS_DOWNLOAD_FLYOUT_OPEN);
            }
        };

        /**
         * Hide/Show the flyout based on completed and download queue length
         * @method toggleDownloadFlyout
         */
        scope.toggleDownloadFlyout = function() {
            flyout.toggleClass(CLS_DOWNLOAD_FLYOUT_OPEN, (scope.downloadQueue.length > 0 || scope.completedQueue.length > 0));
        };

    }

    /**
    * The directive
    */
    function originDownloadManagerQueue(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                completestr: '@complete',
                queuestr: '@completed'
            },
            controller:originDownloadManagerQueueCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('downloadmanager/views/downloadmanagerqueue.html'),
            link: originDownloadManagerQueueLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originDownloadManagerQueue
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} completestr "Compelete"
     * @param {LocalizedString} queuestr "Queue"
     * @param {LocalizedString} downloadmanagerstr "Download Manager"
     * @param {LocalizedString} minimizestr "Minimize"
     * @param {LocalizedString} emptyqueuestr "There are no active tasks at this time."
     * @param {LocalizedString} clearstr "Clear"
     * @description
     *
     * Origin Download Manager Queue
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-download-manager-queue></origin-download-manager-queue>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originDownloadManagerQueueCtrl', originDownloadManagerQueueCtrl)
        .directive('originDownloadManagerQueue', originDownloadManagerQueue);
}());
