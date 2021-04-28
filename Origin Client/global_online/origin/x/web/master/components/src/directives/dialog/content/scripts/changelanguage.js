/**
 * @file dialog/content/changelanguage.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-changelanguage';

    /**
     * Change Language Dialog Controller
     * @controller originDialogContentChangelanguageCtrl
     */
    function OriginDialogContentChangelanguageCtrl($scope, UtilFactory, AppCommFactory, ComponentsConfigFactory) {
        $scope.setLanguageString = UtilFactory.getLocalizedStr($scope.setLanguageStr, CONTEXT_NAMESPACE, 'setlanguage');
        $scope.languages = [{
            'key': 'en_US',
            'name': UtilFactory.getLocalizedStr($scope.englishStr, CONTEXT_NAMESPACE, 'english'),
            image: ComponentsConfigFactory.getImagePath('canada.png')
        }, {
            'key': 'pl_PL',
            'name': UtilFactory.getLocalizedStr($scope.polishStr, CONTEXT_NAMESPACE, 'polish'),
            image: ComponentsConfigFactory.getImagePath('poland.png')
        }];

        /**
         * Set the locale to teh new locale
         * @memberof originDialogContentChangelanguageCtrl
         * @method
         */
        $scope.setLocale = function(locale) {
            AppCommFactory.events.fire('locale:change', locale);
        };

    }

    /**
     * Change Language Dialog Directive
     * @directive originDialogContentChangelanguage
     */
    function originDialogContentChangelanguage(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                setLanguageStr: '@setlanguage',
                englishStr: '@english',
                polishStr: '@polish'
            },
            controller: 'OriginDialogContentChangelanguageCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/changelanguage.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentChangelanguage
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} setlanguage the default message for the set language dropdown
     * @param {LocalizedString} english the label for English language selection
     * @param {LocalizedString} polish the label for the Polish language selection
     * Change language dialog
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-changelanguage></origin-dialog-content-changelanguage>
     *     </file>
     * </example>
     */

    // register to angular
    angular.module('origin-components')
        .controller('OriginDialogContentChangelanguageCtrl', OriginDialogContentChangelanguageCtrl)
        .directive('originDialogContentChangelanguage', originDialogContentChangelanguage);

}());