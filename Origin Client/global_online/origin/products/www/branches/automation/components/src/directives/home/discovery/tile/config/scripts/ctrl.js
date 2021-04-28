/**
 * @file home/discovery/tile/config/ctrl.js
 */
(function() {
    'use strict';

    var BUCKETCONFIG = 'bucketConfig',
        PINRECOMMENDATION = 'pinRecommendation';

    function OriginHomeDiscoveryTileConfigCtrl($scope, tileConfig) {

        function getPropertyFromParent(property) {
            //if there is transclusion introduced by targeting there will be an extra nested scope, therefore we check an extra level up
            return _.get($scope, ['$parent', property]) ||
                   _.get($scope, ['$parent', '$parent', property]) ||
                   _.get($scope, ['$parent', '$parent', '$parent', property]) ||
                   _.get($scope, ['$parent', '$parent', '$parent', '$parent', property]);
        }

        var bucketConfig = getPropertyFromParent(BUCKETCONFIG),
            pinRecoWrapped = getPropertyFromParent(PINRECOMMENDATION);

        //check to see if we have a news article wrapper.
        //this logic will disappear once we integrate with pulse, but for now we need it to filter out
        //the manual entered tiles from GEO
        if (pinRecoWrapped) {
            tileConfig.usePinRec = true;
        }

        if (bucketConfig) {
            bucketConfig.push(tileConfig);
        }


    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileConfig
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-config></origin-home-discovery-tile-config>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileConfigCtrl', OriginHomeDiscoveryTileConfigCtrl);

}());