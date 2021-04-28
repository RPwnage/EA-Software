/**
 * @file home/discovery/bucket.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-bucket';

    function OriginHomeDiscoveryBucketCtrl($scope, DiscoveryStoryFactory, UtilFactory) {

        var id = DiscoveryStoryFactory.getNumBuckets();

        $scope.isLoading = true;
        $scope.isLoadingMore = false;
        $scope.visible = true;
        $scope.moreTilesAvailable = false;


        // Localized Title
        $scope.titlestr = UtilFactory.getLocalizedStr($scope.titlestr, CONTEXT_NAMESPACE, 'title');

        /**
         * Return the bucket id
         * @return {String} bucketId
         * @method
         */
        this.bucketId = function() {
            return id;
        };

    }

    function originHomeDiscoveryBucket($compile, ComponentsConfigFactory, DiscoveryStoryFactory, ComponentsLogFactory, $window) {

        function originHomeDiscoveryBucketLink(scope, element, attr, ctrl) {

            var nextTileIndexToShow = -1,
                sidebarWidth = 250;

            /**
             * Returns the number of tiles that the bucket should be initially
             * populated with based on the given screen width.
             * @param {integer} screenSize screen width
             * @return {integer}
             */
            function getNumberOfColumnsForScreenSize(screenSize) {
                if (screenSize <= 768) {
                    return 2;
                } else if (screenSize <= 1200) {
                    return 3;
                } else {
                    return 4;
                }
            }

            /**
             * Returns the number of columns for the current window size
             * @return {integer}
             */
            function getCurrentNumberOfColumns() {
                return getNumberOfColumnsForScreenSize($window.innerWidth - sidebarWidth);
            }

            /**
             * Add the tiles to the bucket after we get the content
             * @param {Object} response
             * @param {Object} clear - to clear the bucket area or not
             * @method
             */
            function addTilesToBucket(response, clearBucket) {

                var bucketArea = element.find('.origin-home-discovery-tiles'),
                    tileContainer = element.find('.l-origin-home-discovery-tile'),
                    newTileContainer;

                if (!response.length) {
                    scope.visible = false;
                }

                //clear it out
                if (clearBucket) {
                    bucketArea.html('');
                }

                scope.isLoading = false;

                for (var i = 0; i < response.length; i++) {
                    newTileContainer = angular.element(tileContainer[0].cloneNode(false));
                    newTileContainer.html($compile(response[i])(scope));
                    bucketArea.append(newTileContainer);
                }
            }

           /**
            * Toggle if we should display the view more button
            * @method updateViewMore
            */
            function updateViewMore() {
                scope.moreTilesAvailable = (nextTileIndexToShow > -1);
            }

            /**
             * Update the next tile index
             * @param {Object} response
             * @method updateNextTileIndex
             */
            function updateNextTileIndex(response) {
                nextTileIndexToShow = response.nextToShow;
            }

            /**
             * Display the new tiles by adding them to the bucket
             * @param {Object} response
             * @method displayNewTIles
             */
            function displayNewTiles(response) {
                scope.isLoadingMore = false;
                addTilesToBucket(response.directives, false);
            }

            /**
             * error handler
             * @param {Object} error
             * @method errorHandler
             */
            function errorHandler(error) {
                ComponentsLogFactory.error('[origin-home-discovery-bucket DiscoveryStoryFactory.getTileDirectivesForBucket] error', error.stack);
            }

            /**
             * Add more tiles to the bucket
             * @return {void}
             * @method
             */
            scope.showMoreTiles = function() {
                if (nextTileIndexToShow > -1) {
                    scope.isLoadingMore = true;
                    DiscoveryStoryFactory.getTileDirectivesForBucket(ctrl.bucketId(), nextTileIndexToShow, getCurrentNumberOfColumns())
                        .then(function(response) {
                            displayNewTiles(response);
                            updateNextTileIndex(response);
                        })
                        .then(updateViewMore)
                        .catch(errorHandler);

                }
            };

            DiscoveryStoryFactory.getTileDirectivesForBucket(ctrl.bucketId(), 0, getCurrentNumberOfColumns())
                .then(function(response) {
                    addTilesToBucket(response.directives, true);
                    updateNextTileIndex(response);
                })
                .then(updateViewMore)
                .catch(errorHandler);

        }

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                titlestr: '@title'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/views/bucket.html'),
            controller: 'OriginHomeDiscoveryBucketCtrl',
            link: originHomeDiscoveryBucketLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryBucket
     * @restrict E
     * @element ANY
     * @scope
     * @param {Number} numinitialtiles the number of initial tiles to show in the bucket
     * @param {Number} numadditionaltiles the number of additional tiles to show on view more action
     * @param {LocalizedString} title the title show in the bucket
     * @description
     *
     * home buckets
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-bucket titlestr="Discover Something New!">
     *             <origin-home-discovery-tile-config-recfriends limit="2" diminish="500" priority="2500"></origin-home-discovery-tile-config-recfriends>
     *             <origin-home-discovery-tile-config-newdlc limit="2" diminish="500" priority="2100"></origin-home-discovery-tile-config-newdlc>
     *         </origin-home-discovery-bucket>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryBucketCtrl', OriginHomeDiscoveryBucketCtrl)
        .directive('originHomeDiscoveryBucket', originHomeDiscoveryBucket);

}());