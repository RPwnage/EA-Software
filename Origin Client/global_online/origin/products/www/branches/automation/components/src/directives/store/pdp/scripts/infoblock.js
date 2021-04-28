/**
 * @file store/pdp/infoblock.js
 */
(function() {
    'use strict';

    var DESCRIPTION_CHAR_LIMIT = 190;

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function originStorePdpInfoblock(ComponentsConfigFactory, $filter) {
        function originStorePdpInfoblockLink(scope) {
            var stopWatching = scope.$watch('body', function(newBody) {
                if (newBody) {
                    stopWatching();
                    scope.truncatedBody = $filter('limitHtml')(newBody, DESCRIPTION_CHAR_LIMIT, '...', true);
                }
            });
        }

        return {
            restrict: 'E',
            scope: {
                isVisible: '=',
                isDimmed: '=',
                header: '@', //html must remain a function
                body: '@',
                readMoreText: '@',
                onReadMore: '&'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/infoblock.html'),
            link: originStorePdpInfoblockLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpInfoblock
     * @restrict E
     * @element ANY
     * @scope
     * @param {BooleanEnumeration} is-visible directive visibility flag
     * @param {BooleanEnumeration} is-dimmed directive dim flag
     * @param {LocalizedString} header title of info block
     * @param {LocalizedString} body body of info block
     * @param {LocalizedString} read-more-text the link caption
     * @param {Function} on-read-more the read more link click event handler function
     * @description
     *
     * Information block element containing header, body and a 'Read More' link
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-infoblock header="Important Info" body="Get this game for only $1.99!" is-visible="true/false"></origin-store-pdp-infoblock>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpInfoblock', originStorePdpInfoblock);
}());
