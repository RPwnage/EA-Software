
/**
 * @file seo/scripts/schema.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-seo-schema';

    /* global moment */
    function originSeoSchema(ComponentsConfigFactory, $state, $stateParams, AppCommFactory, OcdPathFactory, UtilFactory) {
        function originSeoSchemaLink(scope, element) {
            // set the home page schema
            function addHomeSchema() {
                var script = document.createElement('script'),
                    schemaObject = {
                        '@context': 'http://schema.org',
                        '@type': 'webSite',
                        'url': UtilFactory.generateBaseUrl() + '/store',
                        'potentialAction': {
                            '@type': 'SearchAction',
                            'target': UtilFactory.generateBaseUrl() + '/search?searchString={search_term_string}',
                            'query-input': 'required name=search_term_string'
                        }
                    };

                script.type = 'application/ld+json';
                script.innerHTML = JSON.stringify(schemaObject);
                angular.element(element).append(script);
                addGenericSchema();
            }

            // set the browse page schema
            function addBrowseSchema() {
                var tag =   '<div itemscope itemtype="http://schema.org/CollectionPage">' +
                                '<span itemprop="url">' + UtilFactory.generateBaseUrl() + '/store/browse</span>' +
                                '<span itemprop="name">' + UtilFactory.getLocalizedStr(scope.browse, CONTEXT_NAMESPACE, 'browse') +'</span>' +
                                '<span itemprop="description">' + UtilFactory.getLocalizedStr(scope.description, CONTEXT_NAMESPACE, 'description') +'</span>' +
                            '</div>';

                angular.element(element).append(tag);

                addGenericSchema();
            }

            // set PDP schema
            function populatePdpSchema(data) {
                var model = _.first(_.values(data)),
                    script = document.createElement('script'),
                    schemaObject = {
                        '@context': 'http://schema.org/',
                        '@type': 'Product',
                        'name': model.displayName,
                        'image': model.packArt,
                        'description': model.shortDescription,
                        'brand': {
                            '@type': 'Thing',
                            'name': 'Origin'
                        },
                        'offers': {
                            '@type': 'Offer',
                            'availability': 'http://schema.org/InStock',
                            'price': model.price,
                            'priceCurrency': Origin.locale.currencyCode(),
                            'priceValidUntil': moment().add(1, 'day').format(),
                        }
                    };

                script.type = 'application/ld+json';
                script.innerHTML = JSON.stringify(schemaObject);
                angular.element(element).append(script);
                addGenericSchema();
            }

            // get model for PDP
            function addPdpSchema(toParams) {
                var franchise = toParams.franchise ? '/' + toParams.franchise : '',
                    game = toParams.game ? '/' + toParams.game : '',
                    type = toParams.type ? '/' + toParams.type : '',
                    edition = toParams.edition ? '/' + toParams.edition : '',
                    path = franchise + game + type + edition;

                OcdPathFactory
                    .promiseGet(path)
                    .then(populatePdpSchema);
            }

            // add generic schema
            function addGenericSchema() {
                var script = document.createElement('script'),
                    schemaObject = {
                        '@context': 'http://schema.org',
                        '@type': 'Organization',
                        'url': UtilFactory.generateBaseUrl() + '/store/',
                        'logo': UtilFactory.getLocalizedStr(scope.logo, CONTEXT_NAMESPACE, 'logo'),
                        'sameAs': [
                            'https://www.facebook.com/OriginInsider',
                            'https://twitter.com/originInsider',
                            'https://www.youtube.com/user/OriginInsider',
                            'https://plus.google.com/+OriginInsider'
                        ]
                    };

                script.type = 'application/ld+json';
                script.innerHTML = JSON.stringify(schemaObject);
                angular.element(element).append(script);
            }

            // select proper schema on load and state change
            function setSchemaTags(event, toState, toParams) {
                // reset the content
                angular.element(element).html('');

                if(toState.name === 'app.store.wrapper.addon' || toState.name === 'app.store.wrapper.pdp') {
                    addPdpSchema(toParams);
                } else if(toState.name === 'app.store.wrapper.browse') {
                    addBrowseSchema();
                } else if(toState.name === 'app.store.wrapper.home') {
                    addHomeSchema();
                } else {
                    addGenericSchema();
                }
            }

            // init
            setSchemaTags(null, $state.current, $stateParams);

            AppCommFactory.events.on('uiRouter:stateChangeStart', setSchemaTags);

            function onDestroy() {
                AppCommFactory.events.off('uiRouter:stateChangeStart', setSchemaTags);
            }

            scope.$on('$destroy', onDestroy);
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                logo: '@',
                browse: '@',
                description: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('seo/views/schema.html'),
            link: originSeoSchemaLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originSeoSchema
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} browse Browse page schema title
     * @param {LocalizedString} description Browse page schema description
     * @param {ImageUrl} logo The Origin logo
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-seo-schema />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originSeoSchema', originSeoSchema);
}());
