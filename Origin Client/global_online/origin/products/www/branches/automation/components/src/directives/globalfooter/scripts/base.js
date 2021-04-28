
/** 
 * @file globalfooter/scripts/base.js
 */ 
(function(){
    'use strict';
    var APP_VERSION_PREFIX = 'Origin v',
        CONTEXT_NAMESPACE = 'origin-globalfooter-base';

    function originGlobalfooterBase(ComponentsConfigFactory, APP_VERSION, LanguageFactory, UtilFactory) {
        function originGlobalfooterBaseLink (scope, element, attrs, ctrl, transclude) {
            // Need to override transclude function so that the transcluded 'columns' replace the 
            // <div ng-transclude> element taht is inserted by angular. The 'columns' need to be siblings 
            // to the 'copyright/currency' column inside the footer element in this view in order to get the 
            // proper layout styles. 
            transclude(scope, function(clone) {
                element.find('footer').prepend(clone);
            });

            scope.versionText = APP_VERSION_PREFIX + APP_VERSION;

            function storeLanguages(response) {
                scope.languageMap = response;
            }

            function initLanguageSelector(){
                LanguageFactory.getAllLanguages()
                    .then(storeLanguages);
                scope.currentLocale = LanguageFactory.getDashedLocale();
                scope.setLocale = LanguageFactory.setLocale;
                scope.languageStr = UtilFactory.getLocalizedStr(scope.languageString, CONTEXT_NAMESPACE, 'language-string');
            }
            
            initLanguageSelector();
        }
        
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                copyrightText: '@',
                legalContent: '@',
                languageString: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('globalfooter/views/base.html'),
            link: originGlobalfooterBaseLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originGlobalfooterBase
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} copyright-text The text for the copyright statement.
     * @param {LocalizedText} legal-content Text and/or HTML for localized currency/legal text & links ('Prices are in ..." / PEGI etc.).
     * @param {LocalizedString} language-string the word 'language'
     *
     * @description Reusable global footer directive. Not meant for direct merchandizing (meant to be transcluded
     * into the Client or Web footer.
     * Transcludes origin-globalfooter-column. Is transcluded into the Client or Web 
     *
     *
     * @example
     * <example module="origin-globalfooter-base">
     *     <file name="index.html">
     *         <origin-globalfooter-base ng-transclude/>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originGlobalfooterBase', originGlobalfooterBase);
}()); 
