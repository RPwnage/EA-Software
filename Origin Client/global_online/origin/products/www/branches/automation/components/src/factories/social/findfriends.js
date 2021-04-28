(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-dialog-content-searchforfriends';

    function FindFriendsFactory($state, DialogFactory, AuthFactory, NavigationFactory, UtilFactory) {

        function onClientOnlineStateChanged(onlineObj) {
            if (onlineObj && !onlineObj.isOnline) {
                DialogFactory.close('origin-dialog-content-searchforfriends-id');
            }
        }

        function onSearchTextFinished(result) {
            if (result.accepted) {
                var searchTerm = result.content.searchText;
                NavigationFactory.goToState('app.search', {
                    searchString: decodeURI(searchTerm),
                    category: 'people'
                });
            }
        }

        function findFriends() {
            var localizer = UtilFactory.getLocalizedStrGenerator(CONTEXT_NAMESPACE);

            // Show the Find Friends dialog
            var content = DialogFactory.createDirective('origin-dialog-content-searchforfriends', { placeholderstr: localizer(null, 'searchfriendsinputplaceholdertext') });
            DialogFactory.openPrompt({
                id: 'origin-dialog-content-searchforfriends-id',
                title: localizer(null, 'searchfriendstitle'),
                description: localizer(null, 'searchfriendsdescription'),
                acceptText: localizer(null, 'searchfriendssearchbuttontext'),
                acceptEnabled: false,
                rejectText: localizer(null, 'searchfriendscancelbuttontext'),
                defaultBtn: 'yes',
                contentDirective: content,
                callback: onSearchTextFinished
            });
        }
        //listen for client emit to open this modal (embedded only)    
        Origin.events.on(Origin.events.CLIENT_NAV_OPENMODAL_FINDFRIENDS, findFriends);
        //listen for connection change (for embedded)
        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientOnlineStateChanged);


        return {
            findFriends: findFriends
        };
    }

     /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function FindFriendsFactorySingleton($state, DialogFactory, AuthFactory, NavigationFactory, UtilFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('FindFriendsFactory', FindFriendsFactory, this, arguments);
    }
    /**
     * @ngdoc service
     * @name origin-components.factories.FindFriendsFactory
     
     * @description
     *
     * FindFriendsFactory
     */
    angular.module('origin-components')
        .factory('FindFriendsFactory', FindFriendsFactorySingleton);
}());
