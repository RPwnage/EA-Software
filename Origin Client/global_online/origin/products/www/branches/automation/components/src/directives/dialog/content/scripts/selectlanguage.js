/**
 * @file dialog/content/languageselection.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-selectlanguage';

    function OriginDialogContentSelectlanguageCtrl($scope, UtilFactory, DialogFactory, LanguageFactory) {

        var localizer = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE);

        $scope.strings = {
            title: localizer($scope.languagePreferencesStr, 'language-preferences-str'),
            save: localizer($scope.saveStr, 'save-str'),
            cancel: localizer($scope.cancelStr, 'cancel-str')
        };
        $scope.currentLocale = LanguageFactory.getDashedLocale();
        $scope.languageMap = {};

        /**
        * Store the languages
        * @param {Object} response
        * @method storeLanguages
        */
        function storeLanguages(response) {
            $scope.languageMap = response;
        }

        /**
        * Cancel the dialog selection
        * @method cancel
        */
        $scope.cancel = function() {
            DialogFactory.close('origin-dialog-content-selectlanguage');
        };

        LanguageFactory.getAllLanguages()
            .then(storeLanguages);
    }

    function originDialogContentSelectlanguage(ComponentsConfigFactory, LanguageFactory, DialogFactory) {

        function originDialogContentSelectlanguageLink(scope, elem) {

            /**
            * Determine if the element is checked or not
            * @param {Number} val
            * @param {HTMLRadioInput} elm
            * @return {Boolean}
            */
            function checked(val, elm) {
                return elm.checked;
            }

            /**
            * Save the selection by redirecting the user to the correct url
            * @method save
            */
            scope.save = function() {
                var inputs = elem.find('.origin-dialog-language input'),
                    selectedLocale = inputs.filter(checked).val();
                LanguageFactory.setLocale(selectedLocale);
                DialogFactory.close('origin-dialog-content-selectlanguage');
            };

        }

        return {
            restrict: 'E',
            scope: {
                languagePreferencesStr: '@',
                saveStr: '@',
                cancelStr: '@'
            },
            link: originDialogContentSelectlanguageLink,
            controller: 'OriginDialogContentSelectlanguageCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/selectlanguage.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentSelectlanguage
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} language-preferences-str Language Preferences
     * @param {LocalizedString} save-str Save
     * @param {LocalizedString} cancel-str Cancel
     * @description
     *
     * Language Selection Dialog
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-selectlanguage></origin-dialog-content-selectlanguage>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginDialogContentSelectlanguageCtrl', OriginDialogContentSelectlanguageCtrl)
        .directive('originDialogContentSelectlanguage', originDialogContentSelectlanguage);

}());
