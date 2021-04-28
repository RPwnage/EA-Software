/**
 * @file store/carousel/media/legaldisclaimer.js
 */
(function() {
    'use strict';
    function originDialogContentLegaldisclaimer(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                header: '@',
                body: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/legaldisclaimer.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentLegaldisclaimer
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} header header of the dialog
     * @param {LocalizedString} body body of the dialog
     *
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-legaldisclaimer>
     *         </origin-dialog-content-legaldisclaimer>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originDialogContentLegaldisclaimer', originDialogContentLegaldisclaimer);

}());
