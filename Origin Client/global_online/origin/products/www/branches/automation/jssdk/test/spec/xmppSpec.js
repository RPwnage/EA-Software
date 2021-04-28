describe('Origin XMPP API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it("Origin.xmpp.nucleusIdToJid()", function() {

        expect(Origin.xmpp.nucleusIdToJid('jid')).toEqual('jid@127.0.0.1');

    });

    it("Origin.xmpp.connect(), Origin.events.XMPP_CONNECTED", function(done) {

        var asyncTicker = new AsyncTicker();

        var socialConnectedCB = function() {

            Origin.events.off(Origin.events.XMPP_CONNECTED, socialConnectedCB);

            expect(Origin.xmpp.isConnected()).toEqual(true, 'Origin.xmpp.isConnected() should return true');
            asyncTicker.clear(done);
        };

        Origin.events.on(Origin.events.XMPP_CONNECTED, socialConnectedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_CONNECTED should be triggered');

        Origin.xmpp.connect();

    });

    it("Origin.xmpp.sendMessage()", function() {

        expect(Origin.xmpp.sendMessage('333333@localhost', '')).toEqual(undefined);

    });

    it("Origin.xmpp.sendTypingState()", function() {

        expect(Origin.xmpp.sendTypingState('state', 'userId')).toEqual(undefined);

    });

    it("Origin.xmpp.requestRoster()", function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(3000, done, 'Expect onResolve() to be executed');

        Origin.xmpp.requestRoster().then(function(result) {

            var expected = [{
                subState : 'BOTH',
                jid : '1000120382991@127.0.0.1',
                originId : '1000120382991_e',
                subReqSent : false
            }, {
                subState : 'BOTH',
                jid : '12306939085@127.0.0.1',
                originId : '12306939085_e',
                subReqSent : false
            }];

            for (var i = 0; i < expected.length; i++) {
                expect(result).toContain(expected[i]);
            }

            expect(result.length).toEqual(expected.length, 'returned result should have ' + expected.length + ' entries');
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it("Origin.events.XMPP_INCOMINGMSG", function(done) {

        var asyncTicker = new AsyncTicker();

        var timeRangeStart = Date.now();

        var socialIncomingMsgCB = function(result) {

            Origin.events.off(Origin.events.XMPP_INCOMINGMSG, socialIncomingMsgCB);

            var expected = {
                jid : '1000120382991@127.0.0.1',
                msgBody : 'message from command server',
                chatState : 'ACTIVE',
                time : Date.now(),
                from : '1000120382991@127.0.0.1/mockup',
                to : 'jssdkfunctionaltest@127.0.0.1'
            };

            expect(result.time).not.toBeLessThan(timeRangeStart, 'returned time (' + result.time + ') should after the time of message was sent (' + timeRangeStart + ')');
            expect(result.time).not.toBeGreaterThan(expected.time, 'returned time (' + result.time + ') should before the time of callback was triggered (' + expected.time + ')');

            // reset expected.time for comparing other properties.
            expected.time = result.time;

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_INCOMINGMSG, socialIncomingMsgCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_INCOMINGMSG should be triggered');

        var endPoint = 'http://localhost:1402/xmpp/jssdkfunctionaltest/1000120382991@127.0.0.1/sendMessage';
        var req = new XMLHttpRequest();
        req.open('POST', endPoint, true);
        req.setRequestHeader('Content-Type', 'application/json; charset=utf-8');
        req.send('{"message":"message from command server"}');

    });

    it("Origin.xmpp.friendRequestSend(), Origin.events.XMPP_ROSTERCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var socialRosterChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);

            var expected = {
                jid : '333333@localhost',
                subState : 'NONE',
                subReqSent: true

            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

        Origin.xmpp.friendRequestSend('333333@localhost');

    });

    it("Origin.xmpp.friendRequestRevoke(), Origin.events.XMPP_ROSTERCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var socialRosterChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);

            var expected = {
                jid : '333333@localhost',
                subState : 'REMOVE',
                subReqSent: false
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);
        };

        Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

        Origin.xmpp.friendRequestRevoke('333333@localhost');

    });

    it("Origin.events.XMPP_PRESENCECHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var socialPresenceChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_PRESENCECHANGED, socialPresenceChangedCB);

            var expected = {
                jid : '333333@localhost/mockup',
                presenceType : 'SUBSCRIBE',
                show : 'ONLINE',
                gameActivity : {  },
                groupActivity: { groupId: undefined, groupName: undefined },
                to : 'jssdkfunctionaltest@127.0.0.1',
                from : '',
                nick : ''
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_PRESENCECHANGED, socialPresenceChangedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_PRESENCECHANGED should be triggered');

        var endPoint = 'http://localhost:1402/xmpp/jssdkfunctionaltest/333333@localhost/sendFriendRequest';
        var req = new XMLHttpRequest();
        req.open('POST', endPoint, true);
        req.setRequestHeader('Content-Type', 'application/json; charset=utf-8');
        req.send('');

    });

    it("Origin.xmpp.friendRequestAccept(), Origin.events.XMPP_ROSTERCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var socialRosterChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);

            var expected = {
                jid : '333333@localhost',
                subState : 'BOTH',
                subReqSent: false
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);
        };

        Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

        Origin.xmpp.friendRequestAccept('333333@localhost');

    });

    it("Origin.xmpp.removeFriend(), Origin.events.XMPP_ROSTERCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var socialRosterChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);

            var expected = {
                jid : '333333@localhost',
                subState : 'REMOVE',
                subReqSent: false
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

        Origin.xmpp.removeFriend('333333@localhost');

    });

    it("Origin.xmpp.friendRequestReject(), Origin.events.XMPP_ROSTERCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var socialRosterChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);

            var expected = {
                jid : '444444@localhost',
                subState : 'REMOVE',
                subReqSent: false
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        };

        var socialPresenceChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_PRESENCECHANGED, socialPresenceChangedCB);

            Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);

            Origin.xmpp.friendRequestReject('444444@localhost');

        };

        Origin.events.on(Origin.events.XMPP_PRESENCECHANGED, socialPresenceChangedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

        var endPoint = 'http://localhost:1402/xmpp/jssdkfunctionaltest/444444@localhost/sendFriendRequest';
        var req = new XMLHttpRequest();
        req.open('POST', endPoint, true);
        req.setRequestHeader('Content-Type', 'application/json; charset=utf-8');
        req.send('');

    });

    it("Origin.xmpp.loadBlockList(), Origin.events.XMPP_BLOCKLISTCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var blockListChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_BLOCKLISTCHANGED, blockListChangedCB);

            expect(result).toEqual();
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_BLOCKLISTCHANGED, blockListChangedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_BLOCKLISTCHANGED should be triggered');

        expect(Origin.xmpp.loadBlockList()).toEqual(undefined);

    });

    it("Origin.xmpp.blockUser(), Origin.xmpp.isBlocked(), Origin.events.XMPP_BLOCKLISTCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var userBlockedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_BLOCKLISTCHANGED, userBlockedCB);

            expect(result).toEqual();

            Origin.events.on(Origin.events.XMPP_BLOCKLISTCHANGED, blockListChangedCB);

            expect(Origin.xmpp.loadBlockList()).toEqual(undefined);

        };

        var blockListChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_BLOCKLISTCHANGED, blockListChangedCB);

            Origin.xmpp.isBlocked('1000120382991').then(function(response) {

                expect(response).toEqual(true);
                asyncTicker.clear(done);

            }).catch(function(error) {
                expect().toFail(error.message);
                asyncTicker.clear(done);
            });

        };

        Origin.events.on(Origin.events.XMPP_BLOCKLISTCHANGED, userBlockedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_BLOCKLISTCHANGED should be triggered');

        Origin.xmpp.blockUser('1000120382991');

    });

    it("Origin.xmpp.unblockUser(), Origin.xmpp.isBlocked(), Origin.events.XMPP_BLOCKLISTCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var userUnblockedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_BLOCKLISTCHANGED, userUnblockedCB);

            expect(result).toEqual();

            Origin.events.on(Origin.events.XMPP_BLOCKLISTCHANGED, blockListChangedCB);

            expect(Origin.xmpp.loadBlockList()).toEqual(undefined);

        };

        var blockListChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_BLOCKLISTCHANGED, blockListChangedCB);

            Origin.xmpp.isBlocked('1000120382991').then(function(response) {

                expect(response).toEqual(false);
                asyncTicker.clear(done);

            }).catch(function(error) {
                expect().toFail(error.message);
                asyncTicker.clear(done);
            });

        };

        Origin.events.on(Origin.events.XMPP_BLOCKLISTCHANGED, userUnblockedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_BLOCKLISTCHANGED should be triggered');

        Origin.xmpp.unblockUser('1000120382991');

    });

    it("Origin.xmpp.cancelAndBlockUser()", function(done) {

        var asyncTicker = new AsyncTicker();

        var userBlockedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_BLOCKLISTCHANGED, userBlockedCB);

            expect(result).toEqual();

            Origin.events.on(Origin.events.XMPP_BLOCKLISTCHANGED, blockListChangedCB);

            expect(Origin.xmpp.loadBlockList()).toEqual(undefined);

        };

        var blockListChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_BLOCKLISTCHANGED, blockListChangedCB);

            Origin.xmpp.isBlocked('555555').then(function(response) {

                expect(response).toEqual(true);
                asyncTicker.clear(done);

            }).catch(function(error) {
                expect().toFail(error.message);
                asyncTicker.clear(done);
            });

        };

        var socialRosterChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);

            var expected = {
                jid : '555555@127.0.0.1',
                subState : 'NONE',
                subReqSent: true
            };

            expect(result).toEqual(expected);

            Origin.events.on(Origin.events.XMPP_BLOCKLISTCHANGED, userBlockedCB);

            Origin.xmpp.cancelAndBlockUser('555555');

        };

        Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_BLOCKLISTCHANGED, and Origin.events.XMPP_ROSTERCHANGED should be triggered');

        Origin.xmpp.friendRequestSend('555555@127.0.0.1');

    });

    it("Origin.xmpp.ignoreAndBlockUser()", function(done) {

        var asyncTicker = new AsyncTicker();

        var userBlockedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_BLOCKLISTCHANGED, userBlockedCB);

            expect(result).toEqual();

            Origin.events.on(Origin.events.XMPP_BLOCKLISTCHANGED, blockListChangedCB);

            expect(Origin.xmpp.loadBlockList()).toEqual(undefined);

        };

        var blockListChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_BLOCKLISTCHANGED, blockListChangedCB);

            Origin.xmpp.isBlocked('666666').then(function(response) {

                expect(response).toEqual(true);
                asyncTicker.clear(done);

            }).catch(function(error) {
                expect().toFail(error.message);
                asyncTicker.clear(done);
            });

        };

        var socialRosterChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);

            var expected = {
                jid : '666666@127.0.0.1',
                subState : 'REMOVE',
                subReqSent: false
            };

            expect(result).toEqual(expected);

        };

        var socialPresenceChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_PRESENCECHANGED, socialPresenceChangedCB);

            var expected = {
                jid : '666666@127.0.0.1/mockup',
                presenceType : 'SUBSCRIBE',
                show : 'ONLINE',
                gameActivity : {},
                groupActivity: { groupId: undefined, groupName: undefined },
                to : 'jssdkfunctionaltest@127.0.0.1',
                from : '',
                nick : ''
            };

            expect(result).toEqual(expected);

            Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, socialRosterChangedCB);
            Origin.events.on(Origin.events.XMPP_BLOCKLISTCHANGED, userBlockedCB);

            Origin.xmpp.ignoreAndBlockUser('666666');

        };

        Origin.events.on(Origin.events.XMPP_PRESENCECHANGED, socialPresenceChangedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_BLOCKLISTCHANGED, and Origin.events.XMPP_ROSTERCHANGED should be triggered');

        var endPoint = 'http://localhost:1402/xmpp/jssdkfunctionaltest/666666@127.0.0.1/sendFriendRequest';
        var req = new XMLHttpRequest();
        req.open('POST', endPoint, true);
        req.setRequestHeader('Content-Type', 'application/json; charset=utf-8');
        req.send('');

    });

    it("Origin.xmpp.removeFriendAndBlock(), Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var userBlockedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_BLOCKLISTCHANGED, userBlockedCB);

            expect(result).toEqual();

            Origin.events.on(Origin.events.XMPP_BLOCKLISTCHANGED, blockListChangedCB);

            expect(Origin.xmpp.loadBlockList()).toEqual(undefined);

        };

        var blockListChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_BLOCKLISTCHANGED, blockListChangedCB);

            Origin.xmpp.isBlocked('1000120382991').then(function(response) {

                expect(response).toEqual(true);
                asyncTicker.clear(done);

            }).catch(function(error) {
                expect().toFail(error.message);
                asyncTicker.clear(done);
            });

        };

        Origin.events.on(Origin.events.XMPP_BLOCKLISTCHANGED, userBlockedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_BLOCKLISTCHANGED should be triggered');

        Origin.xmpp.removeFriendAndBlock('1000120382991');

    });

    it("Origin.xmpp.updatePresence()", function() {

        expect(Origin.xmpp.updatePresence()).toEqual(undefined);

    });

    it("Origin.xmpp.requestPresence()", function(done) {

        var asyncTicker = new AsyncTicker();

        var socialPresenceChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_PRESENCECHANGED, socialPresenceChangedCB);

            var expected = {
                jid : 'jssdkfunctionaltest@127.0.0.1/mockup',
                presenceType : undefined,
                show : 'AWAY',
                gameActivity : {},
                groupActivity: { groupId: undefined, groupName: undefined },
                to : 'jssdkfunctionaltest@127.0.0.1',
                from : '',
                nick : ''
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_PRESENCECHANGED, socialPresenceChangedCB);

        asyncTicker.tick(3000, done, 'event Origin.events.XMPP_PRESENCECHANGED should be triggered');

        expect(Origin.xmpp.requestPresence('AWAY')).toEqual(undefined);

    });

    it("Origin.xmpp.joinRoom()", function() {

        // chatroom feature is not actually implemented in JSSDK yet
        expect(Origin.xmpp.joinRoom('jssdkfunctionaltest@127.0.0.1', '5620')).toEqual(undefined);

    });

    it("Origin.xmpp.leaveRoom()", function() {

        // chatroom feature is not actually implemented in JSSDK yet
        expect(Origin.xmpp.leaveRoom('jssdkfunctionaltest@127.0.0.1', '5620')).toEqual(undefined);

    });

    it("Origin.xmpp.disconnect(), Origin.xmpp.isConnected()", function() {

        Origin.xmpp.disconnect();
        expect(Origin.xmpp.isConnected()).toEqual(false, 'Origin.xmpp.isConnected() should return false');

    });

});
