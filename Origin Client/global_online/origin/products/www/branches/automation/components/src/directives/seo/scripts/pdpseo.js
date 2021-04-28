/**
 * @file seo/scripts/pdpseo.js
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
            shortlink: '@',
            shortDescription: '@'
        },
        MAX_META_DESCRIPTION_LENGTH = 150,
        CONTEXT_NAMESPACE = 'origin-pdp-seo',
        PLATFORM_MAP = {
            pcwin: 'PC',
            mac: 'Mac'
        },
        GAME_TITLE_KEY = '%gameTitle%',
        PLATFORM_KEY = '%platform%';

    function originPdpSeo(OriginSeoFactory, OcdPathFactory, PdpUrlFactory, UtilFactory, DirectiveScope) {
        function originPdpSeoLink(scope, element, attrs, OriginSeoCommonCtrl){
            var ocdPath = PdpUrlFactory.buildPathFromUrl();
            var seoDataSet = false;
            /**
             * Extract platform information from game catalog data
             * @param gameData
             * @returns {string} PC, Mac or PC/Mac
             */
            function getPlatformInfo(gameData){
                var platforms = [],
                    platformString = '';
                if (_.isObject(gameData.platforms)){
                    platforms = _.compact(_.map(gameData.platforms, getValidPlatform)).sort(sortFunction);
                    platformString = _.toArray(_.pick(PLATFORM_MAP, platforms)).join('/');
                }
                return platformString;
            }

            /**
             * Sorts in descending alphabetical order
             * @param a
             * @param b
             * @returns {number}
             */
            function sortFunction(a, b){
                if (b > a){
                    return 1;
                }else if (b < a){
                    return -1;
                }
                return 0;
            }

            /**
             * Check platform validity by verifying its release date
             * Converts platform key to lower case
             */
            function getValidPlatform(platformData, key){
                if (platformData && platformData.releaseDate){
                    return key.toLowerCase();
                }
            }
            /**
             * Sets pdp page title
             * @param {Object} model game catalog data
             */
            function setPdpPageTitle(model){
                var tokenObj = {},
                    title = '';

                tokenObj[GAME_TITLE_KEY] = model.displayName || '';
                tokenObj[PLATFORM_KEY] = getPlatformInfo(model);
                
                title = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'title-str', tokenObj);
                OriginSeoFactory.setSeoData('title', [], title);
            }

            /**
             * Replace placeholders in strings
             * @param {Object} model game catalog data
             */
            function interpolatePageTitle(title, model){
                title = title.replace(GAME_TITLE_KEY, model.displayName || '').replace(PLATFORM_KEY, getPlatformInfo(model));
                OriginSeoFactory.setSeoData('title', {}, title);
            }

            /**
             * Sets SEO meta description tag base on game catalog short description
             * @param {Object} model game catalog data
             */
            function setSeoMetaDescription(model){
                if (scope.shortDescription || model.shortDescription) {
                    var attrs = {
                        name: 'description',
                        content: (scope.shortDescription || model.shortDescription).substring(0, MAX_META_DESCRIPTION_LENGTH)
                    };
                    OriginSeoFactory.setSeoData('meta', attrs);
                }
            }

            function setOpenGraphTags(model){
                var attrs;
                if (scope.description || scope.shortDescription || model.shortDescription){
                    attrs = {
                        property: 'og:description',
                        content: (scope.shortDescription || model.shortDescription).substring(0, MAX_META_DESCRIPTION_LENGTH)
                    };
                    OriginSeoFactory.setSeoData('meta', attrs);
                }

                if (model.packArt){
                    attrs = {
                        property: 'og:image',
                        content: model.packArt
                    };
                    OriginSeoFactory.setSeoData('meta', attrs);
                }

                if (model.displayName){
                    attrs = {
                        property: 'og:title',
                        content: model.displayName
                    };
                    OriginSeoFactory.setSeoData('meta', attrs);
                }
            }

            /**
             * Update SEO data for the page
             *
             * @param {Object} gameData game catalog data
             */
            function setSeoData(gameData){
                if (_.isEmpty(gameData) || seoDataSet){
                    return;
                }

                if (!scope.titleStr){
                    setPdpPageTitle(gameData);
                }else{
                    interpolatePageTitle(scope.titleStr, gameData);
                }

                if (!scope.description){
                    setSeoMetaDescription(gameData);
                }

                setOpenGraphTags(gameData);

                seoDataSet = true;
            }

            function getGameData(){
                OriginSeoCommonCtrl.init();
                OcdPathFactory.get(ocdPath).attachToScope(scope, setSeoData);
            }

            DirectiveScope.populateScope(scope, CONTEXT_NAMESPACE, ocdPath).then(getGameData);

        }
        return {
            restrict: 'E',
            scope: ACCEPTABLE_SCOPE_PROPERTY,
            controller: 'OriginSeoCommonCtrl',
            link: originPdpSeoLink
        };
    }

    angular.module('origin-components')
        /**
         * @ngdoc directive
         * @name origin-components.directives:originPdpSeo
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
         * @param {LocalizedString} short-description Shortened description for this page (Optional)
         *
         * @description
         *
         * Interface to interact with SEO tags with PDP specific logic
         *
         * @example
         *
         * <origin-pdp-seo title-str="asdasd"></origin-pdp-seo>
         */
        .directive('originPdpSeo', originPdpSeo);

}());
