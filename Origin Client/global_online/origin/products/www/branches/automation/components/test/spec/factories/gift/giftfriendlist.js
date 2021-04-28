/**
 * Jasmine functional test
 */

'use strict';

describe('GiftFriendListFactory', function () {
    var giftFriendListFactory;

    var friendsListRoster = {
        '12302387261': {
            'originId': 'Jbar-Ebisu',
            'jabberId': '12302387261@integration.chat.dm.origin.com',
            'nucleusId': '12302387261',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false

        },
        '1000109752570': {
            'originId': 'drogoqatest',
            'jabberId': '1000109752570@integration.chat.dm.origin.com',
            'nucleusId': '1000109752570',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000207715586': {
            'originId': 'TesterJarret001',
            'jabberId': '1000207715586@integration.chat.dm.origin.com',
            'nucleusId': '1000207715586',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000130562326': {
            'originId': 'huggiebear2202',
            'jabberId': '1000130562326@integration.chat.dm.origin.com',
            'nucleusId': '1000130562326',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false

        },
        '1000136789433': {
            'originId': 'bckwardpass',
            'jabberId': '1000136789433@integration.chat.dm.origin.com',
            'nucleusId': '1000136789433',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false

        },
        '1000167324636': {
            'originId': 'imoldaffam',
            'jabberId': '1000167324636@integration.chat.dm.origin.com',
            'nucleusId': '1000167324636',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000228300134': {
            'originId': 'TestWiggen01',
            'jabberId': '1000228300134@integration.chat.dm.origin.com',
            'nucleusId': '1000228300134',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000217761522': {
            'originId': 'BelgiumTest',
            'jabberId': '1000217761522@integration.chat.dm.origin.com',
            'nucleusId': '1000217761522',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '12302466806': {
            'originId': 'mdulay4',
            'jabberId': '12302466806@integration.chat.dm.origin.com',
            'nucleusId': '12302466806',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000186144220': {
            'originId': 'huggiebear221',
            'jabberId': '1000186144220@integration.chat.dm.origin.com',
            'nucleusId': '1000186144220',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '12303535501': {
            'originId': 'KnightQA',
            'jabberId': '12303535501@integration.chat.dm.origin.com',
            'nucleusId': '12303535501',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000187328381': {
            'originId': 'huggiebear223',
            'jabberId': '1000187328381@integration.chat.dm.origin.com',
            'nucleusId': '1000187328381',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000264915669': {
            'originId': 'estoneabr',
            'jabberId': '1000264915669@integration.chat.dm.origin.com',
            'nucleusId': '1000264915669',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '12303780445': {
            'originId': 'CaptainAmericah',
            'jabberId': '12303780445@integration.chat.dm.origin.com',
            'nucleusId': '12303780445',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '12302835153': {
            'originId': 'NATC-DEP',
            'jabberId': '12302835153@integration.chat.dm.origin.com',
            'nucleusId': '12302835153',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000176199881': {
            'originId': 'Verification-Jay',
            'jabberId': '1000176199881@integration.chat.dm.origin.com',
            'nucleusId': '1000176199881',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000194312544': {
            'originId': 'Phyco02',
            'jabberId': '1000194312544@integration.chat.dm.origin.com',
            'nucleusId': '1000194312544',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000172728739': {
            'originId': 'testthrowaway2',
            'jabberId': '1000172728739@integration.chat.dm.origin.com',
            'nucleusId': '1000172728739',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000187527293': {
            'originId': 'testthrowaway1',
            'jabberId': '1000187527293@integration.chat.dm.origin.com',
            'nucleusId': '1000187527293',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000187527294': {
            'originId': 'testthrowaway10',
            'jabberId': '1000187527294@integration.chat.dm.origin.com',
            'nucleusId': '1000187527294',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000187527295': {
            'originId': 'testthrowaway111',
            'jabberId': '1000187527295@integration.chat.dm.origin.com',
            'nucleusId': '1000187527295',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        },
        '1000213588501': {
            'originId': 'BlockTester1',
            'jabberId': '1000213588501@integration.chat.dm.origin.com',
            'nucleusId': '1000213588501',
            'firstName': '',
            'lastName': '',
            'subState': 'BOTH',
            'subReqSent': false
        }
    };

    var expectedSortedFriendList = ['1000136789433', '1000217761522', '1000213588501', '12303780445', '1000109752570', '1000264915669', '1000130562326', '1000186144220', '1000187328381', '1000167324636', '12302387261', '12303535501', '12302466806', '12302835153', '1000194312544', '1000207715586', '1000187527293', '1000187527294', '1000187527295', '1000172728739', '1000228300134', '1000176199881'];

    var pageSize = 10;

    beforeEach(function () {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        window.Origin = {
            log: {
                message: angular.noop
            },
            utils: {
                Communicator: angular.noop
            },
            events: {
                on: angular.noop,
                off: angular.noop
            }
        };

        module(function ($provide) {

            $provide.factory('SearchFactory', function () {
                return {};
            });

            $provide.factory('AppCommFactory', function () {
                return {
                    events: {
                        on: angular.noop,
                        off: angular.noop
                    }
                };
            });


            $provide.factory('RosterDataFactory', function () {
                return {
                    getRoster: function () {
                        return Promise.resolve(friendsListRoster);
                    }
                };
            });
        });
    });

    beforeEach(inject(function (_GiftFriendListFactory_) {
        giftFriendListFactory = _GiftFriendListFactory_;
    }));

    describe('getFriendList', function () {

        it('will return friends sorted by originId for current page', function () {
            var expectedSortedFriendListPage1 = _.slice(expectedSortedFriendList, 0, pageSize);
            giftFriendListFactory.getFriendList()
                .then(function (friendslist) {
                    friendslist.getCurrentPage()
                        .then(function (friendsPage1) {
                            expect(friendsPage1).toEqual(expectedSortedFriendListPage1);
                        }).catch(function () {
                        fail('Unexpected exception');
                    });
                });
        });


        it('will return friends sorted by originId for subsequent page', function () {


            giftFriendListFactory.getFriendList()
                .then(function (friendslist) {
                    friendslist.getCurrentPage()
                        .then(friendslist.getNextPage)
                        .then(friendslist.getNextPage)
                        .then(function (friendsAllPages) {
                            expect(friendsAllPages).toEqual(expectedSortedFriendList);
                        }).catch(function () {
                        fail('Unexpected exception');
                    });
                });

        });

    });

});