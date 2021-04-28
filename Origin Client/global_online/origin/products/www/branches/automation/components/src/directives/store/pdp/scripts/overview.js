/**
 * @file store/pdp/scripts/overview.js
 */
/* global moment */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-overview';
    var FACETS_CONTEXT_NAMESPACE = 'origin-store-facet-option';
    var SCHEMA_ITEM_TYPE = 'https://schema.org/Product'; //Schema.org item type Product. This is for SEO purpose
    var EXTERNAL_URL = '<a class="otka external-link" href="{url}">{label}</a>';
    var LABEL_SUFFIX = '-label';

    function originStorePdpOverview(ComponentsConfigFactory, PdpUrlFactory, DirectiveScope, LocaleFactory, UtilFactory, NavigationFactory) {


        function originStorePdpOverviewLink(scope, element, attributes, originStorePdpSectionWrapperCtrl) {
            scope.schemaItemType = SCHEMA_ITEM_TYPE;

            function buildExternalUrl(url, label) {
                return EXTERNAL_URL.replace('{url}', url).replace('{label}', label);
            }

            function buildBrowseUrl(category, facetKey){
                return PdpUrlFactory.getBrowsePageUrl(category + ':' + facetKey);
            }

            function sanitize(value) {
                //do not change the logic in this function. This code is shared between CMS and SPA
                return value ? value.trim().replace(/\s/g, '-')
                    .replace(/(™|®|©|&trade;|&reg;|&copy;|&#8482;|&#174;|&#169;|&|\/|\\|\?)/g, '') // strip (TM), (R) & (C)
                    .toLowerCase() : '';
            }

            function setDeveloperHtml(model) {
                if (model.developer) {
                    var sanitizedDeveloper = sanitize(model.developer);
                    scope.developerLink = {
                        label: UtilFactory.getLocalizedStr(false, FACETS_CONTEXT_NAMESPACE, sanitizedDeveloper + LABEL_SUFFIX),
                        url: buildBrowseUrl('developer', sanitizedDeveloper),
                        absUrl: NavigationFactory.getAbsoluteUrl(buildBrowseUrl('developer', sanitizedDeveloper))
                    };
                }
            }

            function setGameLinksHtml(model) {
                var html = [];
                if (scope.officialSite && model.officialSite) {
                    html.push(buildExternalUrl(model.officialSite, scope.officialSite));
                }
                if (scope.forumSite && model.forumSite) {
                    html.push(buildExternalUrl(model.forumSite, scope.forumSite));
                }
                scope.gameLinksHtml = html.join('<br/>');
            }

            function setGenreHtml(model) {
                if (model.genre) {
                    var genreList = model.genre.split(',');
                    scope.genreLinks = [];
                    _.forEach(genreList, function(genre){
                        var sanitizedGenre = sanitize(genre);
                        var label = UtilFactory.getLocalizedStr(false, FACETS_CONTEXT_NAMESPACE, sanitizedGenre + LABEL_SUFFIX);
                        scope.genreLinks.push({
                            label: label,
                            url: buildBrowseUrl('genre', sanitizedGenre),
                            absUrl: NavigationFactory.getAbsoluteUrl(buildBrowseUrl('genre', sanitizedGenre))
                        });
                    });
                }
            }

            function setSupportedLanguagesHtml(model) {
                if (_.isArray(model.softwareLocales)) {
                    scope.softwareLocales = LocaleFactory.getLanguages(model.softwareLocales).join(', ');
                }
            }

            function setReleaseDate(model) {
                var releaseDateOverride = _.get(scope, ['model', 'i18n', 'preAnnouncementDisplayDate']);

                if(releaseDateOverride) {
                    scope.formattedReleaseDate = releaseDateOverride;
                } else if (model.releaseDate) {
                    scope.formattedReleaseDate = moment(model.releaseDate).format('LL');
                }
            }


            function applyModel(model) {
                if (model) {
                    scope.model = model;
                    setReleaseDate(model);
                    setSupportedLanguagesHtml(model);
                    setGenreHtml(model);
                    setDeveloperHtml(model);
                    setGameLinksHtml(model);
                    originStorePdpSectionWrapperCtrl.setVisibility(true);
                }
            }

            scope.openLink = function(href, $event){
                $event.preventDefault();
                NavigationFactory.openLink(href);
            };

            DirectiveScope.populateScopeWithOcdAndCatalog(scope, CONTEXT_NAMESPACE, PdpUrlFactory.buildPathFromUrl(), applyModel);
        }

        return {
            restrict: 'E',
            require: '^originStorePdpSectionWrapper',
            scope: {
                genre: '@',
                releasedate: '@',
                rating: '@',
                developer: '@',
                publisher: '@',
                supportedLanguages: '@',
                gameLinks: '@',
                officialSite: '@',
                forumSite: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/overview.html'),
            link: originStorePdpOverviewLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpOverview
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {LocalizedString} genre Label for the genre overview item
     * @param {LocalizedString} releasedate Label for the release date overview item
     * @param {LocalizedString} rating Label for the rating overview item
     * @param {LocalizedString} developer Label for the developer overview item
     * @param {LocalizedString} publisher Label for the publisher overview item
     * @param {LocalizedString} supported-languages Label for the supported languages overview item
     * @param {LocalizedString} game-links Label for the game links overview item
     * @param {LocalizedString} official-site Label for the official site link
     * @param {LocalizedString} forum-site Label for the forum site link
     *
     * PDP overview blocks, retrieved from catalog
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-overview></origin-store-pdp-overview>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpOverview', originStorePdpOverview);
}());
