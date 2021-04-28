/**
 * @file seo/scripts/seo.js
 */
(function () {
    'use strict';

    var ACCEPTABLE_SCOPE_PROPERTY = {
        titleStr: '@',
        canonical: '@',
        author: '@',
        subject: '@',
        description: '@',
        classification: '@',
        keywords: '@',
        language: '@',
        expires: '@',
        cacheControl: '@',
        robots: '@',
        country: '@',
        generator: '@',
        shortlink: '@'
    };
    var CONTEXT_NAMESPACE = 'origin-common-seo';

    function OriginSeoCommonCtrl($scope, OriginSeoFactory, PdpUrlFactory, UtilFactory) {
        function setSeoData(){
            _.forEach(ACCEPTABLE_SCOPE_PROPERTY, function(value, key){
                if ($scope[key]){
                    if (key === 'titleStr'){
                        // don't set title if on a PDP, pdpseo component handles this
                        if(!PdpUrlFactory.isPdpRoute()) {
                            var title = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'title-str', {
                               "%title%": $scope[key]
                            });
                            OriginSeoFactory.setSeoData('title', [], title);
                        }
                    }else{
                        var attrs = {
                            name: key,
                            content: $scope[key]
                        };
                        OriginSeoFactory.setSeoData('meta', attrs);
                    }
                }
            });
        }

        function init(){
            setSeoData();
        }

        this.init = init;
    }

    function originCommonSeo() {
        function originSeoCommonLink(scope, ele, attrs, ctrl){
            ctrl.init();
        }
        return {
            restrict: 'E',
            scope: ACCEPTABLE_SCOPE_PROPERTY,
            controller: OriginSeoCommonCtrl,
            link: originSeoCommonLink
        };
    }

    angular.module('origin-components')
        /**
         * @ngdoc directive
         * @name origin-components.directives:originCommonSeo
         * @restrict E
         * @element ANY
         * @scope
         *
         * @param {LocalizedString} title-str Title of the page as recorded by search engines (Optional)
         * @param {String} canonical Canonical URL of this page (Optional)
         * @param {String} author Name of whomever created this page (Optional)
         * @param {String} subject The subject of this page (Optional)
         * @param {LocalizedString} description Description of this page and its content (Optional)
         * @param {String} classification Five-digit classification code or text description (Optional)
         * @param {LocalizedString} keywords SEO Keywords used to tag this page (Optional)
         * @param {String} expires Set when this page should expire from search engine caches (Optional)
         * @param {String} cache-control Set browser caching settings for this page (Optional)
         * @param {String} robots Set access privileges for search engine robots (Optional)
         * @param {String} country Country specific to this page (Optional)
         * @param {String} generator Software/service used to generate this page (Optional)
         * @param {String} shortlink Shortened URL for this page (Optional)
         *
         * @description
         *
         * Interface to interact with SEO tags
         *
         * @example
         *
         * <origin-common-seo title-str="asdasd"></origin-common-seo>
         */
        .controller('OriginSeoCommonCtrl', OriginSeoCommonCtrl)
        .directive('originCommonSeo', originCommonSeo);
}());
