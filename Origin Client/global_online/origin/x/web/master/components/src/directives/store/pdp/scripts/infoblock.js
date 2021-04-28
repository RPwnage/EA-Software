/**
 * @file store/pdp/infoblock.js
 */
(function(){
    'use strict';

    function originStorePdpInfoblock(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                isVisible: '&',
                isDimmed: '&',
                header: '&',
                body: '&',
                readMoreText: '&',
                onReadMore: '&'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/infoblock.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpInfoblock
     * @restrict E
     * @element ANY
     * @scope
     * @param {boolean} is-visible directive visibility flag
     * @param {string} header the block title
     * @param {string} body the info text
     * @param {string} read-more-text the link caption
     * @param {Function} on-read-more the read more link click event handler function
     * @description
     *
     * Information block element containing header, body and a 'Read More' link
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-infoblock header="Important Info" body="Get this game for only $1.99!" is-visible="isGameAvailable()"></origin-store-pdp-infoblock>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpInfoblock', originStorePdpInfoblock);
}());
