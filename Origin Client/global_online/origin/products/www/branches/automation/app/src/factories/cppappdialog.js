/**
* @file factories/cppappdialog.js
*/
(function() {
    'use strict';

    function CPPAppDialogFactory(DialogFactory, AppCommFactory, LogFactory, ReportUserFactory, InviteUsersFactory, ConversationFactory) {

        /**
        * Called from C++ function 'dialogOpen'. Takes in the dialog info passed to us from C++. It constructs a dialog
        * @param  {Object} dialog info
        * id - A unique id for the dialog.
        * dialogType - The directory name of the main body of the dialog. 
        *              Typically this will be origin-dialog-content-prompt
        * 
        * 
        */
        function handleDialogOpen(dialogInfo) {
            var content = DialogFactory.createDirective(dialogInfo.contentType, dialogInfo.content);
            if(dialogInfo.dialogType === 'origin-dialog-content-prompt') {
                DialogFactory.openPrompt({
                    id: dialogInfo.id,
                    title: dialogInfo.title,
                    description: dialogInfo.text,
                    acceptText: dialogInfo.acceptText,
                    acceptEnabled: dialogInfo.acceptEnabled,
                    rejectText: dialogInfo.rejectText,
                    defaultBtn: dialogInfo.defaultBtn,
                    blocking: dialogInfo.blocking,
                    contentDirective: content
                });
            } else {
                DialogFactory.openDirective({
                    id: dialogInfo.id,
                    name: dialogInfo.dialogType, 
                    data: dialogInfo.content
                });
            }
        }

        /**
        * Called from C++ signal 'dialogClosed'. Tells Javascript to close current dialog being shown.
        * @param  {Object} closed dialog info
        * id {String} - id of a specific dialog we want to close if it's open.
        */
        function handleDialogClosed(dialogInfo) {
            DialogFactory.close(dialogInfo.id);
        }
        
        /**
        * Called from C++ signal 'dialogChanged'. Tells Javascript to update current dialog being shown.
        * @param  {Object} dialog info
        * id {String} - id of a specific dialog we want to close if it's open.
        */
        function handleDialogChanged(dialogInfo) {
            DialogFactory.update(dialogInfo);
        }

        function handleDialogSystemError() {
            LogFactory.log('Origin.client.dialogs.show: client not available');
        }
        /**
        * Called from cppDialog:handleDialogSystemReady to tell the C++ know to show the first dialog if one is ready.
        */
        function handleDialogSystemReady() {
            // Show the current dialog being shown in the client. This is for the case if the page is refreshed, or a
            // new instance of the web browser client is created. We need to show the dialog that's being displayed in the client.
            Origin.client.dialogs.show({context:'cppappdialog:handleDialogSystemReady'}).catch(handleDialogSystemError);
        }

        function handleReportUser(nucleusId, EAID) {
            // This event came from the C++ so we can assume that we have OIG active
            ReportUserFactory.handleReportUser(nucleusId, EAID);
        }

        function handleInviteFriendsToGame(gameName) {
            // This event came from the C++ so we can assume that we have OIG active
            InviteUsersFactory.inviteUsersToGame(gameName);
        }

        function handleStartConversation(nucleusId) {
            ConversationFactory.openConversation(nucleusId);
        }

        return {
            init: function() {
                if (!Origin.client.oig.IGOIsActive()) {
                    Origin.events.on(Origin.events.CLIENT_DIALOGOPEN, handleDialogOpen);
                    Origin.events.on(Origin.events.CLIENT_DIALOGCLOSED, handleDialogClosed);
                    Origin.events.on(Origin.events.CLIENT_DIALOGCHANGED, handleDialogChanged);
                    Origin.events.on(Origin.events.CLIENT_GAME_REQUESTREPORTUSER, handleReportUser);
                    Origin.events.on(Origin.events.CLIENT_GAME_INVITEFRIENDSTOGAME, handleInviteFriendsToGame);
                    Origin.events.on(Origin.events.CLIENT_GAME_STARTCONVERSATION, handleStartConversation);
                    AppCommFactory.events.on('cppDialog:dialogSystemReady', handleDialogSystemReady);
                    AppCommFactory.events.on('cppDialog:finished', Origin.client.dialogs.finished);
                    AppCommFactory.events.on('cppDialog:showingDialog', Origin.client.dialogs.showingDialog);
                    AppCommFactory.events.on('cppDialog:linkReact', Origin.client.dialogs.linkReact);
                }
            }
        };


    }

    /**
     * @ngdoc service
     * @name originApp.factories.CPPAppDialogFactory
     * @description This factory is the middle man between dialog JSSDK and front end related to dialogs.
     *
     */
    angular.module('originApp').factory('CPPAppDialogFactory', CPPAppDialogFactory);

}());
