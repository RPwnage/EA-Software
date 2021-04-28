/**
 * @file store/pdp/announcement/sections/scripts/main.js
 */
(function(){
    'use strict';

    var DESCRIPTION_CHAR_LIMIT = 190;

    function originStoreAnnouncementSectionsMain(ComponentsConfigFactory, $filter) {

        function originStoreAnnouncementSectionsMainLink(scope) {

            scope.isVisible = function() {
                return !!scope.retireMessage;
            };
            scope.truncatedLongDescription = $filter('limitHtml')(scope.longDescription, DESCRIPTION_CHAR_LIMIT, '...');
        }

        return {
            restrict: 'E',
            link: originStoreAnnouncementSectionsMainLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/announcement/sections/views/main.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAnnouncementSectionsMain
     * @restrict E
     * @element ANY
     * @scope
     * @description Similar to pdp section main (for dev use only)
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-announcement-sections-main></origin-store-announcement-sections-main>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAnnouncementSectionsMain', originStoreAnnouncementSectionsMain);
}());
