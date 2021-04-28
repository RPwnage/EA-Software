
/**
 * @file dialog/content/scripts/dlcpurchaseprevention.js
 */
(function(){
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-dialog-dlcpurchaseprevention';
    var BTN_GET_IT_ANYWAY = "btngetitanyway";
    var BTN_GET_IT_NOW = "btngetitnow";
    var BTN_CANCEL = "btncancel";
    var TITLE = "title";
    var DESCRIPTION = "description";
    var DESCRIPTION_FREE = "descriptionfree";
    var CURRENCY_TITLE = 'currency-title';
    var CURRENCY_DESCRIPTION = 'currency-description';
    var BASEGAME_TITLE = 'basegamenotfoundtitle';
    var BASEGAME_DESCRIPTION = 'basegamenotfounddesc';
    var OWNS_TRIAL_BASEGAME_WARNING = 'ownstrialbasegamewarning';

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    function originDialogDlcpurchasepreventionCtrl($scope, DialogFactory) {
        /**
         * Close the dialog and set the response values
         * @param  {boolean} success If the user selected true or false
         */
        $scope.closeResponse = function(success) {
            DialogFactory.setReturnValues(success, {id: $scope.dialogid});
            DialogFactory.close($scope.dialogid);
        };
    }


    function originDialogDlcpurchaseprevention(ComponentsConfigFactory, UtilFactory, DialogFactory, NavigationFactory) {
        function originDialogDlcpurchasepreventionLink(scope, elem) {

            /**
             * Build anchor markup for insertion into dialog strings
             * @param  {string} text anchor tag text
             * @param  {string} href anchor tag URL
             * @return {string}      complete anchor tag
             */
            function buildAnchorMarkup(text, href) {
                return '<a class="otka baseGame" href="' +  href + '">' + text + '</a>';
            }

            /**
            * Create the strings that will be shown in the UI
            * @param  {Object} bundleProducts   Products the user is trying to buy
            * @param  {Array} ownedProducts  The set of products in the bundleProducts the user already owns
            */
            scope.buildStrings = function() {
                var dlcgame = UtilFactory.escapeAndDecodeURIComponent(scope.dlcgame);
                var basegame = UtilFactory.escapeAndDecodeURIComponent(scope.basegame);
                var basehref = UtilFactory.escapeAndDecodeURIComponent(scope.basehref);
                var basegameLink = buildAnchorMarkup(basegame, basehref);

                // localization string replacements
                var params = {
                    '%dlcgame%': dlcgame,
                    '%basegame%': basegame,
                    '%basegameLink%': basegameLink
                };

                scope.strings = scope.strings || {};
                scope.strings.btnContinue = '';
                scope.strings.btnCancel = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, BTN_CANCEL);

                if(scope.isCurrency === BooleanEnumeration.true) { // is a virtual currency
                    scope.strings.title = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, CURRENCY_TITLE, params);
                    scope.strings.description = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, CURRENCY_DESCRIPTION, params);
                } else { // is anything other than virtual currency
                    scope.strings.title = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, TITLE, params);
                    scope.strings.description = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, DESCRIPTION, params);
                    scope.strings.btnContinue = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, BTN_GET_IT_ANYWAY);
                }

                if (scope.isFree === BooleanEnumeration.true) { // is a free offer of any kind
                    scope.strings.description = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, DESCRIPTION_FREE, params);
                    scope.strings.btnContinue = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, BTN_GET_IT_NOW);
                }

                if (!scope.basegame) {  // the base game is not available in the store
                    scope.strings.title = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, BASEGAME_TITLE);
                    scope.strings.description = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, BASEGAME_DESCRIPTION);
                }

                if (scope.ownsTrialOnly === BooleanEnumeration.true) { // owns trial basegame of DLC
                    scope.strings.description = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, OWNS_TRIAL_BASEGAME_WARNING);
                    scope.strings.btnContinue = UtilFactory.getLocalizedStr(false, CONTEXT_NAMESPACE, BTN_GET_IT_ANYWAY);
                }
            };

            scope.buildStrings();

            setTimeout(function() {
                elem.find('.baseGame').click(function() {
                    DialogFactory.close(scope.dialogid);
                    NavigationFactory.internalUrl(decodeURIComponent(scope.basehref));
                });
            }, 0);
        }

        return {
            restrict: 'E',
            scope: {
                dialogid: '@',
                isFree: '@',
                dlcgame: '@',
                basegame: '@',
                basehref: '@',
                isCurrency: '@',
                thirdParty: '@',
                ownsTrialOnly: '@'
            },
            link: originDialogDlcpurchasepreventionLink,
            controller: originDialogDlcpurchasepreventionCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/dlcpurchaseprevention.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogDlcpurchaseprevention
     * @restrict E
     * @element ANY
     * @scope
     * @description DLC purchase prevention / warning dialog
     * @param {string} dialogid The ID of the dialog
     * @param {BooleanEnumeration} is-free is this DLC offer free
     * @param {string} dlcgame URIComponent encoded DLC name
     * @param {string} basegame URIComponent encoded base game name
     * @param {string} basehref URIComponent encoded URL of base game PDP
     * @param {LocalizedString} btncancel * Cancel text - merchandized in defaults
     * @param {LocalizedString} btngetitanyway * Get it anyway text - merchandized in defaults
     * @param {LocalizedString} btngetitnow * Get it now text - merchandized in defaults
     * @param {LocalizedString} descriptionfree * Continue to purchase anyways text - merchandized in defaults
     * @param {LocalizedString} title * Title text - merchandized in defaults
     * @param {LocalizedString} description * Description text - merchandized in defaults
     * @param {LocalizedString} currency-title * Currency Title text - merchandized in defaults
     * @param {LocalizedString} currency-description * Currency Description text - merchandized in defaults
     * @param {LocalizedString} basegamenotfoundtitle * base game not found title - merchandized in defaults
     * @param {LocalizedString} basegamenotfounddesc * basegame not found description - merchandized in defaults
     * @param {LocalizedString} ownstrialbasegamewarning * user only owns trial and is buying DLC warning message - merchandized in defaults
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-dialog-dlcpurchaseprevention dialogid="web-pdp-dlc-purchase-prevention-flow" price="0" dlcgame="100%20FIFA%2016%20Points" basegame="FIFA%2016" basehref="%23%2Fstore%2Ffifa%2Ffifa-16%2Fstandard-edition" />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originDialogDlcpurchasepreventionCtrl', originDialogDlcpurchasepreventionCtrl)
        .directive('originDialogDlcpurchaseprevention', originDialogDlcpurchaseprevention);
}());
