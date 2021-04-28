(function() {
    'use strict';

    function ReportUserFactory(DialogFactory, AppCommFactory, AuthFactory, ComponentsLogFactory, UtilFactory) {

        function onClientOnlineStateChanged(onlineObj) {
            if (onlineObj && !onlineObj.isOnline) {
                DialogFactory.close('origin-dialog-content-reportuser-id');
                DialogFactory.close('origin-dialog-content-reportuser-thankyou-id');
            }
        }

        //listen for connection change (for embedded)
        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);

        function showReportUserThankyouDialog(dialogStrings) {
            DialogFactory.openAlert({
                id: 'origin-dialog-content-reportuser-thankyou-id',
                title: dialogStrings.title,
                description: dialogStrings.thankYouText,
                rejectText: dialogStrings.closeText
            });
        }

        function processReportUser(response, dialogStrings) {
            response = response;
            showReportUserThankyouDialog(dialogStrings);
        }

        function handleReportUserError(error) {
            // 9.x doesn't care if the report failed
            ComponentsLogFactory.error('origin-dialog-content-reportuser atomReportUser:', error);
            showReportUserThankyouDialog();
        }

        function reportUser(conf, spalocation) {
            // Show the report user dialog
            var content = DialogFactory.createDirective('origin-dialog-content-reportuser',
                {
                    userid: conf.userid,
                    username: conf.username,
                    mastertitle: conf.mastertitle
                });

            var dialogStrings = _.get(conf, 'dialogStrings', {});
            
            // These are copied here from the Profile header.js file in a quick attempt to fix
            // https://developer.origin.com/support/browse/ORPS-4835 
            // Created a tech debt ticket:
            // https://developer.origin.com/support/browse/ORPS-4839
            // So that we remember to move these to a better location later.
            var CONTEXT_NAMESPACE = 'origin-profile-header';
            dialogStrings.title = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'report-user-dialog-title-text');
            dialogStrings.submitText = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'report-user-dialog-submit-text');
            dialogStrings.cancelText = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'report-user-dialog-cancel-text');
            dialogStrings.closeText = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'report-user-dialog-close-text');
            dialogStrings.thankYouText = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'report-user-dialog-thank-you-text');
            dialogStrings.confidentialText = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'report-user-dialog-confidential-text');
            
            DialogFactory.openPrompt({
                id: 'origin-dialog-content-reportuser-id',
                size: 'large',
                title:  dialogStrings.title,
                description: dialogStrings.confidentialText,
                acceptText: dialogStrings.submitText,
                acceptEnabled: false,
                rejectText: dialogStrings.closeText,
                defaultBtn: 'yes',
                contentDirective: content,
                callback: _.partialRight(onReportUserFinished, dialogStrings),
                spalocation: spalocation,
                sublocation: conf.userid
            });
        }

        function handleReportUser(nucleusId, masterTitle) {
            // This event came from the C++ so we can assume that we have OIG active
            console.log('We got the reportUser event from the C++');
            Origin.atom.atomUserInfoByUserIds([nucleusId])
                .then(function (response) {
                    var userName = response[0].EAID;
                    console.log('We got back the user name: ', userName);
                    reportUser({
                        userid: nucleusId,
                        username: userName,
                        mastertitle: masterTitle
                    }, 'PROFILE');
                }).catch(function (error) {
                    ComponentsLogFactory.error('OriginProfileHeaderCtrl: Origin.atom.atomUserInfoByUserIds failed', error);
                }); 
        }

        function onReportUserFinished(result, dialogStrings) {
            AppCommFactory.events.off('cppDialog:finished', onReportUserFinished);
            if (result.accepted) {
                Origin.atom.atomReportUser(result.content.userId, result.content.whereSelection, result.content.whySelection, result.content.comment, result.content.masterTitle)
                    .then(_.partialRight(processReportUser, dialogStrings))
                    .catch(handleReportUserError);
            }
        }

        return {
            reportUser: reportUser,
            handleReportUser: handleReportUser
        };
    }

     /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ReportUserFactorySingleton(DialogFactory, AppCommFactory, AuthFactory, ComponentsLogFactory, UtilFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ReportUserFactory', ReportUserFactory, this, arguments);
    }
    /**
     * @ngdoc service
     * @name origin-components.factories.ReportUserFactory
     
     * @description
     *
     * ReportUserFactory
     */
    angular.module('origin-components')
        .factory('ReportUserFactory', ReportUserFactorySingleton);
}());
