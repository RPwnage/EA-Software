/**
 * @file downloadmanager/downloadmanagermini.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-download-manager-mini',
        CLS_DOWNLOAD_FLYOUT_WRAPPER = 'origin-downloadflyout-wrapper',
        CLS_DOWNLOAD_FLYOUT_OPEN = 'origin-downloadflyout-isopen';

    /**
    * The Controller
    */
    function OriginDownloadManagerMiniCtrl($scope, UtilFactory, GamesDataFactory, ComponentsLogFactory, EventsHelperFactory, ComponentsConfigHelper) {

        var handles = [];

        $scope.inClient = Origin.client.isEmbeddedBrowser();
        $scope.isOIGContext = !!ComponentsConfigHelper.getOIGContextConfig();
        $scope.downloadManagerLoc = UtilFactory.getLocalizedStr($scope.downloadmanagerstr, CONTEXT_NAMESPACE, 'downloadmanagerstr');

        /**
         * Handle an error if the promise is rejected
         * @param {Exception} err the error message
         */
        function handleError(err) {
            ComponentsLogFactory.error('Could not retrieve head offerid from client proxy:', err);
        }

        /**
        * Update the downloaded notification count
        * @method updateNotificationCount
        */
        function updateNotificationCount() {
            Origin.client.contentOperationQueue.entitlementsCompletedOfferIdList()
                .then(function(offerIdList) {
                    $scope.completeCount = offerIdList.length;
                    $scope.$digest();
                });
        }

        /**
         * Notify the rest of the nav of a height change
         */
        function notifyHeightChange() {
            $scope.$emit('navigation:heightchange');
        }

        /**
        * Update the offerId with new value every time head changes
        * @param {string} offerId
        * @method updateHead
        */
        function updateHead(offerId) {
            //if we are showing/hiding the download manager let the nav know
            if (!$scope.offerId || !offerId) {
                setTimeout(notifyHeightChange, 0);
            }            
            
            $scope.offerId = offerId;
            $scope.$digest();
        }

        /**
        * Get the head offerId from the download queue or anytime the head offerid changes
        * @method getHeadOfferId
        */
        function getHeadOfferId() {
            Origin.client.contentOperationQueue.headOfferId()
            .then(updateHead)
            .catch(handleError);
        }

        /**
        * Called when $scope is destroyed, release event handlers
        * @method onDestroy
        */
        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
        }

        handles = [
            Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_HEADCHANGED, updateHead),
            Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_ADDEDTOCOMPLETE, updateNotificationCount),
            Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_REMOVED, updateNotificationCount),
            Origin.events.on(Origin.events.CLIENT_OPERATIONQUEUE_COMPLETELISTCLEARED, updateNotificationCount)
        ];

        if($scope.inClient) {
            //need to wait for supercat to get loaded
            GamesDataFactory.waitForGamesDataReady()
                .then(getHeadOfferId);

            // need to build the complete list if the SPA reloads without quitting client
            updateNotificationCount();
        }

        $scope.$on('$destroy', onDestroy);
    }

    /**
    * The directive
    */
    function originDownloadManagerMini(ComponentsConfigFactory, $timeout, LayoutStatesFactory) {

        function originDownloadManagerMiniLink(scope, element) {

            $timeout(function(){
                var flyout = angular.element(document.querySelectorAll('.'+CLS_DOWNLOAD_FLYOUT_WRAPPER)[0]);

                scope.showFlyout = function(event) {
                    event.stopPropagation();
                    event.preventDefault();
                    flyout.addClass(CLS_DOWNLOAD_FLYOUT_OPEN);

                    // hide the main menu in <1000px mode and show flyout in fullscreen mode.
                    LayoutStatesFactory.handleMenuClick();
                };

                function closeFlyout(event) {
                    event.stopPropagation();
                    if (element !== event.target && !element[0].contains(event.target)) {
                        flyout.removeClass(CLS_DOWNLOAD_FLYOUT_OPEN);
                    }
                }

                angular.element(document).on('mousedown', closeFlyout);

                scope.$on('$destroy', function() {
                    angular.element(document).off('mousedown', closeFlyout);
                });
            });

        }

        return {
            restrict: 'E',
            controller: OriginDownloadManagerMiniCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('downloadmanager/views/downloadmanagermini.html'),
            link: originDownloadManagerMiniLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDownloadManagerMini
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offerid of the game
     * @param {LocalizedString} downloadmanagerstr "Download Manager"
     * @description
     *
     * Origin Download Manager Mini
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-download-manager-mini></origin-download-manager-mini>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginDownloadManagerMiniCtrl', OriginDownloadManagerMiniCtrl)
        .directive('originDownloadManagerMini', originDownloadManagerMini);
}());
