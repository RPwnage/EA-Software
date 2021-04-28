/**
 * @file dialog/content/reportuser.js
 */
(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginDialogContentReportuserCtrl($scope, $state, UtilFactory, DialogFactory, AuthFactory, ComponentsLogFactory) {

        var CONTEXT_NAMESPACE = 'origin-dialog-content-reportuser';

        $scope.titleLoc = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titlestr');
        $scope.submitLoc = UtilFactory.getLocalizedStr($scope.submitStr, CONTEXT_NAMESPACE, 'submitstr');
        $scope.cancelLoc = UtilFactory.getLocalizedStr($scope.cancelStr, CONTEXT_NAMESPACE, 'cancelstr');
        $scope.closeLoc = UtilFactory.getLocalizedStr($scope.closeStr, CONTEXT_NAMESPACE, 'closestr');
        $scope.thankYouLoc = UtilFactory.getLocalizedStr($scope.thankYouStr, CONTEXT_NAMESPACE, 'thankyoustr');
        $scope.selectOneLoc = UtilFactory.getLocalizedStr($scope.selectOneStr, CONTEXT_NAMESPACE, 'selectonestr');
        $scope.confidentialLoc = UtilFactory.getLocalizedStr($scope.confidentialStr, CONTEXT_NAMESPACE, 'confidentialstr');
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
                $scope.$submitButton.removeClass("otkbtn-disabled");
            }
            else {
                $scope.$submitButton.addClass("otkbtn-disabled");
            }
        }


        function processReportUser(response) {
            response = response;
            $scope.reportSent = true;
            $scope.$digest();
        }

        function handleReportUserError(error) {
            // 9.x doesn't care if the report failed
            $scope.reportSent = true;
            $scope.$digest();
            ComponentsLogFactory.error('origin-dialog-content-reportuser atomReportUser:', error.message);
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

        this.onSubmit = function () {
            var comment = $scope.$textArea.val();

            Origin.atom.atomReportUser($scope.userId, $scope.whereSelection, $scope.whySelection, comment, $scope.masterTitle)
                .then(processReportUser)
                .catch(handleReportUserError);

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
    * @param {LocalizedString} titlestr "Report User"
    * @param {LocalizedString} submitstr "Submit"
    * @param {LocalizedString} cancelstr "Cancel"
    * @param {LocalizedString} closestr "Close"
    * @param {LocalizedString} thankyoustr "Thank you. Your report has been submitted to Customer Support."
    * @param {LocalizedString} selectonestr "Select One"
    * @param {LocalizedString} confidentialstr "All Reports are confidential."
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
    *            titlestr="Report User"
    *            submitstr="Submit"
    *            cancelstr="Cancel"
    *            closestr="Close"
    *            thankyoustr="Thank you. Your report has been submitted to Customer Support."
    *            selectonestr="Select One"
    *            confidentialstr="All Reports are confidential."
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
        .directive('originDialogContentReportuser', function (ComponentsConfigFactory, DialogFactory) {

            return {
                restrict: 'E',
                scope: {
                    userId: '@userid',
                    username: '@username',
                    masterTitle: '@mastertitle',
                    titleStr: '@titlestr',
                    submitStr: '@submitstr',
                    cancelStr: '@cancelstr',
                    closeStr: '@closestr',
                    thankYouStr: '@thankyoustr',
                    selectOneStr: '@selectonestr',
                    confidentialStr: '@confidentialstr',
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

                    var $submitButton = $(element).find('.dialog-content-reportuser-section-footer .otkbtn-primary'),
                        $cancelButton = $(element).find('.dialog-content-reportuser-section-footer .otkbtn-light'),
                        $whyLabel = $(element).find('.dialog-content-reportuser-section-form-why-area .otkselect-label'),
                        $whereLabel = $(element).find('.dialog-content-reportuser-section-form-where-area .otkselect-label'),
                        $textArea = $(element).find('#textarea');

                    scope.$element = $(element);
                    scope.$submitButton = $submitButton;
                    scope.$textArea = $textArea;
                    scope.$whyLabel = $whyLabel;
                    scope.$whereLabel = $whereLabel;

                    $('#why-select').change(function () {
                        var value = $(this).val();
                        var string = $(this).find('option:selected').text();
                        ctrl.setWhyValue(value, string);
                    });

                    $('#where-select').change(function () {
                        var value = $(this).val();
                        var string = $(this).find('option:selected').text();
                        ctrl.setWhereValue(value, string);
                    });

                    $submitButton.click(function () {
                        ctrl.onSubmit();
                    });

                    $cancelButton.click(function () {
                        DialogFactory.close('origin-dialog-content-reportuser-id');
                    });

                }

            };

        });

} ());

