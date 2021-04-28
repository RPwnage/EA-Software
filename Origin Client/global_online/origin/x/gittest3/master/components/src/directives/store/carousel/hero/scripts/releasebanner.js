/**
 * @file store/carousel/hero/scripts/releasebanner.js
 */
(function () {
    'use strict';
    /* jshint ignore:start */
    var ExpiredDateTime = {
        "format" : "F j Y H:i:s T"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-carousel';


    function originStoreCarouselHeroReleasebanner(ComponentsConfigFactory, UtilFactory) {

        function transfromDate(dateString) {
            if (dateString === undefined) {
                return undefined;
            }

            var date = new Date(dateString);
            var currDate = date.getDate();
            var currMonth = date.getMonth() + 1;
            var currYear = date.getFullYear();
            var fDate = currMonth + "/" + currDate + "/" + currYear;
            return fDate;
        }

        function originStoreCarouselHeroReleasebannerLink(scope) {
            scope.sDate = transfromDate(scope.startDate);
            scope.eDate = transfromDate(scope.endDate);
            scope.aText = UtilFactory.getLocalizedStr(scope.availableText, CONTEXT_NAMESPACE, 'releaseString', {'%startDate%': scope.sDate, '%endDate%': scope.eDate});
        }

        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            scope: {
                title: '@',
                subtitle: '@',
                image: '@',
                href: '@',
                startDate: '@',
                endDate: '@',
                availableText: '@',
                videoid: '@',
                restrictedage: '@',
                videoDescription: '@'
            },
            link: originStoreCarouselHeroReleasebannerLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/hero/views/releasebanner.html'),
            controller: 'OriginStoreCarouselHeroBannerCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselHeroReleasebanner
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString } title The title of the module
     * @param {LocalizedString} subtitle The subtitle of the module
     * @param {ImageUrl} image The banner image
     * @param {string} href Url to something
     * @param {ExpiredDateTime} startDate The start date, may be release or sale date
     * @param {ExpiredDateTime} endDate offer may be available till date or sale date
     * @param {LocalizedString} availableText The string that holds the dates. The dates will subbed in the place of %startDate% and %endDate%. EG: "Available %startDate% - %endDate% would yeild "Available 6/10/2015 - 6/15/2015"
     * Standard Store Hero Banner
     * @param {Number} restrictedage The Restricted Age of the youtube video for this promo (Optional).
     * @param {LocalizedString} video-description The text for the video cta
     *
     */
    angular.module('origin-components')
        .directive('originStoreCarouselHeroReleasebanner', originStoreCarouselHeroReleasebanner);
}());
