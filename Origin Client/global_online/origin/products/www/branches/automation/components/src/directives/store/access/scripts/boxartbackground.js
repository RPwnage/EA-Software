/**
 * @file store/access/scripts/boxartbackground.js
 */
(function(){
    'use strict';

    function OriginStoreAccessBoxartbackgroundCtrl($scope, $element, ObjectHelperFactory, OcdPathFactory, SearchFactory, ComponentsLogFactory, AnimateFactory) {
        $scope.offers = [];
        $scope.models = [];
        $scope.counter = 0;

        // scope function for getting loop counter
        $scope.getCounter = function(num) {
            return new Array(num);
        };

        // extract offer ids from solr response
        function extractPropertyOrDefault(data, propertyArray, defaultValue) {
            return ObjectHelperFactory.defaultTo(defaultValue)(ObjectHelperFactory.getProperty(propertyArray)(data));
        }

        function triggerLoaded() {
            $scope.$emit('storeBoxartBackground:loaded');
        }

        // figure out how wide the module is and how many packarts will be needed to cover it
        function countPackartRows() {
            if(ObjectHelperFactory.length($scope.models) > 0) {
               $scope.counter =  Math.ceil((($element.find('.origin-store-access-boxartbackground').outerWidth()/92)*6)/ObjectHelperFactory.length($scope.models));
            }

            triggerLoaded();
        }

        // apply product data to scope models
        function applyModels(data) {
            $scope.models = data;
            countPackartRows();
        }

        // handle serchFactory results
        function handleSearchGamesResults(data) {
            var games = extractPropertyOrDefault(data, ['games', 'game'], []);

            if(games.length) {
                $scope.offers = ObjectHelperFactory.toArray(ObjectHelperFactory.map(ObjectHelperFactory.getProperty('path'), games));

                OcdPathFactory
                    .get($scope.offers)
                    .attachToScope($scope, applyModels);
            } else {
                triggerLoaded();
            }
        }

        // generate search request
        function getPackart() {
            var searchParams = {
                filterQuery: 'subscriptionGroup:vault-games',
                facetField: '',
                sort: 'rank desc',
                start: 0,
                rows: 17
            };

            SearchFactory.searchStore('', searchParams)
                .then(handleSearchGamesResults)
                .catch(function(error) {
                    ComponentsLogFactory.error('[StoreSearchFactory: SearchFactory.searchStore] failed', error);
                    triggerLoaded();
                });
        }

        AnimateFactory.addResizeEventHandler($scope, countPackartRows);

        // trigger the offer process
        getPackart();
    }
    function originStoreAccessBoxartbackground(ComponentsConfigFactory, AnimateFactory, ScreenFactory, CSSUtilFactory, FeatureDetectionFactory) {
        function originStoreAccessBoxartbackgroundLink(scope, element) {
            var parallaxContainer = element.find('.origin-store-access-boxartbackground');
            scope.supportsCssFilter = FeatureDetectionFactory.browserSupports('css-filters');

            function updatePosition() {
                if (ScreenFactory.isPdpLarge()) {
                    var top = element.offset().top - 0,
                        parallaxPosition = ((window.pageYOffset - top) / 8).toFixed(2),
                        transition = 'translate3d(0,' + parallaxPosition + 'px, 0)';
                    parallaxContainer.css(CSSUtilFactory.addVendorPrefixes('transform', transition));
                } else {
                    parallaxContainer.removeAttr('style');
                }
            }

            AnimateFactory.addScrollEventHandler(scope, updatePosition);

            updatePosition();

        }
        return {
            restrict: 'E',
            scope: {},
            controller: 'OriginStoreAccessBoxartbackgroundCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/access/views/boxartbackground.html'),
            link: originStoreAccessBoxartbackgroundLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAccessBoxartbackground
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-access-boxartbackground />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreAccessBoxartbackgroundCtrl', OriginStoreAccessBoxartbackgroundCtrl)
        .directive('originStoreAccessBoxartbackground', originStoreAccessBoxartbackground);
}());
