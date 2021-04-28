/**
 * @file dialog/content/reportuser.js
 */
(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginDialogContentReportuserCtrl($scope, $state, UtilFactory, DialogFactory, AuthFactory) {

        var CONTEXT_NAMESPACE = 'origin-dialog-content-reportuser';

        $scope.selectOneLoc = UtilFactory.getLocalizedStr($scope.selectOneStr, CONTEXT_NAMESPACE, 'selectonestr');
        $scope.whyLoc = UtilFactory.getLocalizedStr($scope.whyStr, CONTEXT_NAMESPACE, 'whystr').replace('%username%', $scope.username);
        $scope.whereLoc = UtilFactory.getLocalizedStr($scope.whereStr, CONTEXT_NAMESPACE, 'wherestr');
        $scope.commentsLoc = UtilFactory.getLocalizedStr($scope.commentsStr, CONTEXT_NAMESPACE, 'commentsstr');
        $scope.enterNotesLoc = UtilFactory.getLocalizedStr($scope.enterNotesStr, CONTEXT_NAMESPACE, 'enternotesstr');
        $scope.cheatingLoc = UtilFactory.getLocalizedStr($scope.cheatingStr, CONTEXT_NAMESPACE, 'cheatingstr');
        $scope.solicitationLoc = UtilFactory.getLocalizedStr($scope.solicitationStr, CONTEXT_NAMESPACE, 'solicitationstr');
        $scope.harassmentLoc = UtilFactory.getLocalizedStr($scope.harassmentStr, CONTEXT_NAMESPACE, 'harassmentstr');
        $scope.offensiveLoc = UtilFactory.getLocalizedStr($scope.offensiveStr, CONTEXT_NAMESPACE, 'offensivestr');
        $scope.spamLoc = UtilFactory.getLocalizedStr($scope.spamStr, CONTEXT_NAMESPACE, 'spamstr');
        $scope.suicideLoc = UtilFactory.getLocalizedStr($scope.suicideStr, CONTEXT_NAMESPACE, 'suicidestr');
        $scope.terroristLoc = UtilFactory.getLocalizedStr($scope.terroristStr, CONTEXT_NAMESPACE, 'terroriststr');
        $scope.hackLoc = UtilFactory.getLocalizedStr($scope.hackStr, CONTEXT_NAMESPACE, 'hackstr');
        $scope.realNameLoc = UtilFactory.getLocalizedStr($scope.realNameStr, CONTEXT_NAMESPACE, 'realnamestr');
        $scope.avatarLoc = UtilFactory.getLocalizedStr($scope.avatarStr, CONTEXT_NAMESPACE, 'avatarstr');
        $scope.customGameLoc = UtilFactory.getLocalizedStr($scope.customGameStr, CONTEXT_NAMESPACE, 'customgamestr');
        $scope.inGameLoc = UtilFactory.getLocalizedStr($scope.inGameStr, CONTEXT_NAMESPACE, 'ingamestr');

        $scope.whySelection = '';
        $scope.whereSelection = '';
        $scope.reportSent = false;

        function onClientOnlineStateChanged(onlineObj) {
            if (onlineObj && !onlineObj.isOnline) {
                DialogFactory.close('origin-dialog-content-reportuser-id');
            }
        }

        function onDestroy() {
            AuthFactory.events.off('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
            Origin.events.off(Origin.events.CLIENT_ONLINESTATECHANGED, onClientOnlineStateChanged);
        }

        //listen for connection change (for embedded)
        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);
        $scope.$on('$destroy', onDestroy);

        function updateUI() {
            if ($scope.whySelection.length && $scope.whereSelection.length) {
                DialogFactory.update({ acceptEnabled: true });
            }
            else {
                DialogFactory.update({ acceptEnabled: false });
            }
        }

        this.setWhyValue = function (value, string) {
            $scope.whySelection = value;
            $scope.$whyLabel.text(string);
            updateUI();
        };

        this.setWhereValue = function (value, string) {
            $scope.whereSelection = value;
            $scope.$whereLabel.text(string);
            updateUI();
        };

    }

    /**
    * @ngdoc directive
    * @name origin-components.directives:originDialogContentReportuser
    * @restrict E
    * @element ANY
    * @scope
    * @param {string} userid the userId of the user to report
    * @param {string} username the username of the user to report
    * @param {string} mastertitle the master title of the game the user is playing (optional)
    * @param {LocalizedString} selectonestr "Select One"
    * @param {LocalizedString} whystr "Why are you reporting %username%?"
    * @param {LocalizedString} wherestr "Where did this happen?"
    * @param {LocalizedString} commentsstr "Additional Comments"
    * @param {LocalizedString} enternotesstr "Enter your notes..."
    * @param {LocalizedString} cheatingstr "Cheating"
    * @param {LocalizedString} solicitationstr "Child Solicitation"
    * @param {LocalizedString} harassmentstr "Harassment"
    * @param {LocalizedString} offensivestr "Offensive Content"
    * @param {LocalizedString} spamstr "Spam"
    * @param {LocalizedString} suicidestr "Suicide Threat"
    * @param {LocalizedString} terroriststr "Terrorist Threat"
    * @param {LocalizedString} hackstr "Client Hack"
    * @param {LocalizedString} realnamestr "Real Name and EA ID"
    * @param {LocalizedString} avatarstr "Avatar"
    * @param {LocalizedString} customgamestr "Custom Game Name"
    * @param {LocalizedString} ingamestr "In-Game"


    * @description
    *
    * Report user dialog
    *
    * @example
    * <example module="origin-components">
    *     <file name="index.html">
    *         <origin-dialog-content-reportuser
    *            userid="123456789"
    *            username="Joe Smith"
    *            mastertitle="Battlefield"
    *            selectonestr="Select One"
    *            whystr="Why are you reporting %username%?"
    *            wherestr="Where did this happen?"
    *            commentsstr="Additional Comments"
    *            enternotesstr="Enter your notes..."
    *            cheatingstr="Cheating"
    *            solicitationstr="Child Solicitation"
    *            harassmentstr="Harassment"
    *            offensivestr="Offensive Content"
    *            spamstr="Spam"
    *            suicidestr="Suicide Threat"
    *            terroriststr="Terrorist Threat"
    *            hackstr="Client Hack"
    *            realnamestr="Real Name and EA ID"
    *            avatarstr="Avatar"
    *            customgamestr="Custom Game Name"
    *            ingamestr="In-Game"
    *         ></origin-dialog-content-reportuser>
    *     </file>
    * </example>
    *
    */

    angular.module('origin-components')
        .controller('OriginDialogContentReportuserCtrl', OriginDialogContentReportuserCtrl)
        .directive('originDialogContentReportuser', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                require: ['^originDialogContentPrompt', 'originDialogContentReportuser'],
                scope: {
                    userId: '@userid',
                    username: '@username',
                    masterTitle: '@mastertitle',
                    selectOneStr: '@selectonestr',
                    whyStr: '@whystr',
                    whereStr: '@wherestr',
                    commentsStr: '@commentsstr',
                    enterNotesStr: '@enternotesstr',
                    cheatingStr: '@cheatingstr',
                    solicitationStr: '@solicitationstr',
                    harassmentStr: '@harassmentstr',
                    offensiveStr: '@offensivestr',
                    spamStr: '@spamstr',
                    suicideStr: '@suicidestr',
                    terroristStr: '@terroriststr',
                    hackStr: '@hackstr',
                    realNameStr: '@realnamestr',
                    avatarStr: '@avatarstr',
                    customGameStr: '@customgamestr',
                    inGameStr: '@ingamestr'
                },
                controller: 'OriginDialogContentReportuserCtrl',
                templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/reportuser.html'),
                link: function (scope, element, attrs, ctrl) {

                    scope = scope;
                    attrs = attrs;

                    var promptCtrl = ctrl[0];
                    var thisCtrl = ctrl[1];

                    var $whyLabel = $(element).find('.dialog-content-reportuser-section-form-why-area .otkselect-label'),
                        $whereLabel = $(element).find('.dialog-content-reportuser-section-form-where-area .otkselect-label');

                    scope.$element = $(element);
                    scope.$whyLabel = $whyLabel;
                    scope.$whereLabel = $whereLabel;

                    $(element).on('change', '#why-select', function () {
                        var value = $(this).val();
                        var string = $(this).find('option:selected').text();
                        thisCtrl.setWhyValue(value, string);
                    });

                    $(element).on('change', '#where-select', function () {
                        var value = $(this).val();
                        var string = $(this).find('option:selected').text();
                        thisCtrl.setWhereValue(value, string);
                    });

                    promptCtrl.setContentDataFunc(function () {
                        return {
                            userId: scope.userId,
                            whySelection: scope.whySelection,
                            whereSelection: scope.whereSelection,
                            comment: scope.comment,
                            masterTitle: scope.masterTitle
                        };
                    });

                }

            };

        });

} ());

