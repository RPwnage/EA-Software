describe('Origin XMPP API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it("Origin.xmpp.connect(), Origin.events.XMPP_CONNECTED", function(done) {

        var asyncTicker = new AsyncTicker();

        var socialConnectedCB = function() {
            Origin.events.off(Origin.events.XMPP_CONNECTED, socialConnectedCB);
            expect(Origin.xmpp.isConnected()).toEqual(true, 'Origin.xmpp.isConnected() should return true');
            asyncTicker.clear(done);
        };

        Origin.events.on(Origin.events.XMPP_CONNECTED, socialConnectedCB);

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_CONNECTED should be triggered');

        Origin.xmpp.connect();

    });

    it("Origin.xmpp.sendMessage()", function(done) {

        Origin.xmpp.sendMessage('333333@localhost', '');

        // give the server time to work on message processing
        setTimeout(function() {
            expect().toPass();
            done();
        }, 5);

    });

    it("Origin.xmpp.requestRoster()", function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

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
                to : 'jssdkfunctionaltest@127.0.0.1/mockup'
            };

            expect(result.time).not.toBeLessThan(timeRangeStart, 'returned time (' + result.time + ') should after the time of message was sent (' + timeRangeStart + ')');
            expect(result.time).not.toBeGreaterThan(expected.time, 'returned time (' + result.time + ') should before the time of callback was triggered (' + expected.time + ')');
            
            // reset expected.time for comparing other properties.    
            expected.time = result.time;
            
            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_INCOMINGMSG, socialIncomingMsgCB);

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_INCOMINGMSG should be triggered');

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

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

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

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

        Origin.xmpp.friendRequestRevoke('333333@localhost');

    });

    it("Origin.events.XMPP_PRESENCECHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var socialPresenceChangedCB = function(result) {

            Origin.events.off(Origin.events.XMPP_PRESENCECHANGED, socialPresenceChangedCB);

            var expected = {
                jid : '333333@localhost',
                presenceType : 'SUBSCRIBE',
                show : 'ONLINE',
                gameActivity : {  },
                to : 'jssdkfunctionaltest@127.0.0.1/mockup',
                from : '',
                nick : ''
            };

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_PRESENCECHANGED, socialPresenceChangedCB);

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_PRESENCECHANGED should be triggered');

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

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

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

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

        Origin.xmpp.removeFriend('333333@localhost');

    });

    it("Origin.xmpp.disconnect(), Origin.xmpp.isConnected()", function() {

        Origin.xmpp.disconnect();
        expect(Origin.xmpp.isConnected()).toEqual(false, 'Origin.xmpp.isConnected() should return false');

    });

});
