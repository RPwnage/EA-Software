/**
 * @file gamelibrary/ogdheader.js
 */
(function() {

    'use strict';

    function OriginGamelibraryOgdHeaderCtrl($scope, $sce, GamesDataFactory, ComponentsLogFactory) {

        function extractSingleResponse(response) {
            return response[$scope.offerId];
        }
        function assignCatalogInfoToScope(catalogInfo) {
            $scope.packartLarge = catalogInfo.i18n.packArtLarge;
            $scope.gameName = catalogInfo.i18n.displayName;
            $scope.$digest();
        }

        function onDestroy() {
            GamesDataFactory.events.off('games:catalogUpdated:' + $scope.offerId, assignCatalogInfoToScope);
        }

        GamesDataFactory.events.on('games:catalogUpdated:' + $scope.offerId, assignCatalogInfoToScope);
        $scope.$on('$destroy', onDestroy);

        GamesDataFactory.getCatalogInfo([$scope.offerId])
            .then(extractSingleResponse)
            .then(assignCatalogInfoToScope)
            .catch(function(error) {
                ComponentsLogFactory.error('[origin-ogd-header] GamesDataFactory.getCatalogInfo', error.stack);
            });
    }

    function originGamelibraryOgdHeader(ComponentsConfigFactory, ObjectHelperFactory, GameViolatorFactory, OcdHelperFactory, ComponentsLogFactory, $compile) {
        function originGamelibraryOgdHeaderLink(scope, element) {
            var buildTag = OcdHelperFactory.buildTag;

            /**
             * Compile the directive for the game tile violator
             * @param {Object[]} violatorData the violator data
             */
            function showViolators(violatorData) {
                var violatorContainer = element.find('.important-info-container');

                if(!violatorData || !violatorData.length) {
                    return;
                }

                for(var i = 0; i < violatorData.length; i++) {
                    var tag;

                    if(violatorData[i].violatorType && violatorData[i].violatorType === 'automatic') {
                        tag = buildTag('origin-gamelibrary-ogd-violator', {
                            'violatortype': violatorData[i].label,
                            'offerid': scope.offerId,
                            'enddate': violatorData[i].endDate || ''
                        });
                    } else {
                        tag = buildTag('origin-gamelibrary-ogd-important-info', violatorData[i]);
                    }

                    violatorContainer.append($compile(tag)(scope));
                }

                scope.$digest();
            }

            GameViolatorFactory.getViolators(scope.offerId, 'importantinfo')
                .then(showViolators)
                .catch(function(error) {
                    ComponentsLogFactory.error('[origin-gamelibrary-ogd-header] error', error.stack);
                });
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@offerid'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdheader.html'),
            controller: OriginGamelibraryOgdHeaderCtrl,
            link: originGamelibraryOgdHeaderLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdHeader
     * @restrict E
     * @element ANY
     * @scope
     * @param {OfferId} offerid the offer id
     * @description
     *
     * Owned game details slide out header section
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-ogd>
     *             <origin-gamelibrary-ogd-header offerid="OFB-EAST:50885"></origin-gamelibrary-ogd-header>
     *         </origin-gamelibrary-ogd>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdHeaderCtrl', OriginGamelibraryOgdHeaderCtrl)
        .directive('originGamelibraryOgdHeader', originGamelibraryOgdHeader);
}());