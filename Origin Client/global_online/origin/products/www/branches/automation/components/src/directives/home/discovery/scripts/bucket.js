/**
 * @file home/discovery/bucket.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-discovery-bucket';

    function OriginHomeDiscoveryBucketCtrl($scope, DiscoveryStoryFactory, UtilFactory) {
        $scope.isLoadingMore = false;
        $scope.visible = false;
        $scope.moreTilesAvailable = false;
        $scope.bucketConfig = [];

        // Localized Title
        $scope.titleStr = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'title-str');
        $scope.subtitleStr = UtilFactory.getLocalizedStr($scope.subtitleStr, CONTEXT_NAMESPACE, 'subtitle-str');
        $scope.viewmoreStr = UtilFactory.getLocalizedStr($scope.viewmoreStr, CONTEXT_NAMESPACE, 'viewmore-str');

        $scope.$on('bucketFilter', function(event, filterObject) {
            $scope.filterFunction = filterObject.filterFunction;
        });
    }

    function originHomeDiscoveryBucket($compile, ComponentsConfigFactory, DiscoveryStoryFactory, ComponentsLogFactory, $window) {

        function originHomeDiscoveryBucketLink(scope, element) {

            var nextTileIndexToShow = -1,
                SIDEBAR_WIDTH = 280,
                ROWS_TO_SHOW = 2,
                generatedStories = null,
                generateXML = DiscoveryStoryFactory.generateXML;

            /**
             * Returns the number of tiles that the bucket should be initially
             * populated with based on the given screen width.
             * @param {integer} screenSize screen width
             * @return {integer}
             */
            function getNumberOfColumnsForScreenSize(screenSize) {
                if (screenSize <= 768) {
                    return 2;
                } else {
                    return 4;
                }
            }

            /**
             * Returns the number of columns for the current window size
             * @return {integer}
             */
            function getCurrentNumberOfColumns() {
                return getNumberOfColumnsForScreenSize($window.innerWidth - SIDEBAR_WIDTH);
            }

            /**
             * Add the tiles to the bucket after we get the content
             * @param {Object} response
             * @param {Object} clear - to clear the bucket area or not
             * @method
             */
            function addTilesToBucket(response) {
                if (response.length) {
                    scope.visible = true;
                    //need this digest here to trigger the effects of setting visible to true
                    scope.$digest();

                    //emit the event after we've updated the DOM
                    setTimeout(function() {
                        scope.$emit('myhome:dirty');

                        var bucketArea = element.find('.origin-home-discovery-tiles');
                        scope.isLoading = false;

                        _.forEach(response, function(item) {
                            if(item.outerHTML.length) {
                                var wrappedXMLString = '<li class=\'l-origin-home-discovery-tile l-origin-hometile\'>' + item.outerHTML + '</li>';
                                bucketArea.append($compile(wrappedXMLString)(scope));
                                Origin.telemetry.events.fire('origin-tealium-discovery-tile-loaded', wrappedXMLString);
                            }
                        });
                        scope.$digest();
                    }, 0);
                }

            }

            /**
             * kick off a digest cycle at the end of the promise chain
             */
            function runDigest() {
                scope.$digest();
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
            function updateNextTileIndex(newIndex) {
                return function() {
                    nextTileIndexToShow = newIndex;
                    if (nextTileIndexToShow >= generatedStories.length) {
                        nextTileIndexToShow = -1;
                    }
                };
            }

            /**
             * Display the new tiles by adding them to the bucket
             * @param {Object} response
             * @method displayNewTIles
             */
            function displayNewTiles(response) {
                scope.isLoadingMore = false;
                addTilesToBucket(response, false);
            }

            /**
             * save off the generated stories so we can use it to show more tiles
             * @param  {object[]} response an array of generated stories
             * @return {object[]} an array of generated stories
             */
            function storeGeneratedStories(response) {
                var limit = _.get(scope, 'tileLimit', -1);

                if(!_.isArray([response])) {
                    response = [];
                }

                if (limit >= 0) {
                    generatedStories = response.slice(0, limit);
                } else {
                    generatedStories = response;

                }

                return generatedStories;
            }
            /**
             * error handler
             * @param {Object} error
             * @method errorHandler
             */
            function errorHandler(error) {
                ComponentsLogFactory.error('[origin-home-discovery-bucket DiscoveryStoryFactory.getTileDirectivesForBucket] error', error);
            }

            /**
             * Add more tiles to the bucket
             * @return {void}
             * @method
             */
            scope.showMoreTiles = function() {
                if (nextTileIndexToShow > -1) {
                    scope.isLoadingMore = true;
                    DiscoveryStoryFactory.generateXML(nextTileIndexToShow, getCurrentNumberOfColumns() * ROWS_TO_SHOW)(generatedStories)
                        .then(displayNewTiles)
                        .then(updateNextTileIndex(nextTileIndexToShow + (getCurrentNumberOfColumns() * ROWS_TO_SHOW)))
                        .then(updateViewMore)
                        .catch(errorHandler);

                }
            };

            scope.smuggleNextTile = function() {
                if (nextTileIndexToShow > -1) {
                    var story = _.get(generatedStories[nextTileIndexToShow],
                        ['feedTypeConfig', 'feedData', nextTileIndexToShow]);
                    updateNextTileIndex(++nextTileIndexToShow)();
                    updateViewMore();
                    return story;
                }
            };

            function onDestroy() {
                scope.bucketConfig = null;
                generatedStories = null;
            }

            scope.$on('$destroy', onDestroy);

            //clear out the config directives
            element.find('.origin-home-discovery-configs').remove();

            DiscoveryStoryFactory.generateStories(scope.bucketConfig, scope.filterFunction)
                .then(storeGeneratedStories)
                .then(generateXML(0, getCurrentNumberOfColumns()))
                .then(addTilesToBucket)
                .then(updateNextTileIndex(getCurrentNumberOfColumns()))
                .then(updateViewMore)
                .then(runDigest)
                .catch(errorHandler);
        }

        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                subtitleStr: '@',
                viewmoreStr: '@',
                tileLimit: '@'
            },
            transclude: true,
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
     * @param {Number} tile-limit the max number of tiles that will show in this bucket, leave it blank for no limit
     * @param {LocalizedString} title-str the title show in the bucket
     * @param {LocalizedString} subtitle-str the subtitle show in the bucket
     * @param {LocalizedString} viewmore-str the text used in the view more button
     * @description
     *
     * home buckets
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-bucket title-str="Discover Something New!" subtitle-str="<a href='store/browse' class='otka'>Browse All</a>">
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
