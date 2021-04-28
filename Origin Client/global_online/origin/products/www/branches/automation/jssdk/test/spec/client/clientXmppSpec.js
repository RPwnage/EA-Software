describe('Origin XMPP API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it("Origin.xmpp.nucleusIdToJid()", function() {

        expect(Origin.xmpp.nucleusIdToJid('jid')).toEqual('jid@127.0.0.1');

    });

    it("Origin.xmpp.connect(), Origin.events.XMPP_CONNECTED, Origin.xmpp.isConnected()", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function() {
            Origin.events.off(Origin.events.XMPP_CONNECTED, cb);

            expect(Origin.xmpp.isConnected()).toEqual(true, 'Origin.xmpp.isConnected() should return true');
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_CONNECTED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_CONNECTED should be triggered');

        Origin.xmpp.connect();

    });

    it("Origin.events.XMPP_PRESENCECHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.XMPP_PRESENCECHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_PRESENCECHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_PRESENCECHANGED should be triggered');

        window.OriginSocialManager.presenceChanged([{}]);

    });

    it("Origin.events.XMPP_INCOMINGMSG", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.XMPP_INCOMINGMSG, cb);

            expect(result.type).toEqual('chat');
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_INCOMINGMSG, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_INCOMINGMSG should be triggered');

        window.OriginSocialManager.messageReceived({type:'chat'});

    });

    it("Origin.xmpp.sendMessage()", function() {

        expect(Origin.xmpp.sendMessage('333333@localhost', '')).toEqual(undefined);

    });

    it("Origin.xmpp.sendTypingState()", function() {

        expect(Origin.xmpp.sendTypingState('state', 'userId')).toEqual(undefined);

    });

    it("Origin.xmpp.requestRoster()", function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.xmpp.requestRoster().then(function(result) {

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it("Origin.xmpp.friendRequestSend(), Origin.events.XMPP_ROSTERCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

        expect(Origin.xmpp.friendRequestSend('333333@localhost')).toEqual(undefined);

    });

    it("Origin.xmpp.friendRequestRevoke(), Origin.events.XMPP_ROSTERCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

        expect(Origin.xmpp.friendRequestRevoke('333333@localhost')).toEqual(undefined);

    });

    it("Origin.xmpp.friendRequestAccept(), Origin.events.XMPP_ROSTERCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

        expect(Origin.xmpp.friendRequestAccept('333333@localhost')).toEqual(undefined);

    });

    it("Origin.xmpp.removeFriend(), Origin.events.XMPP_ROSTERCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

        expect(Origin.xmpp.removeFriend('333333@localhost')).toEqual(undefined);

    });

    it("Origin.xmpp.friendRequestReject(), Origin.events.XMPP_ROSTERCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.XMPP_ROSTERCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_ROSTERCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_ROSTERCHANGED should be triggered');

        expect(Origin.xmpp.friendRequestReject('333333@localhost')).toEqual(undefined);

    });

    it("Origin.xmpp.loadBlockList()", function() {

        expect(Origin.xmpp.loadBlockList()).toEqual(undefined);

    });

    it("Origin.xmpp.blockUser(), Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED should be triggered');

        expect(Origin.xmpp.blockUser('333333@localhost')).toEqual(undefined);

    });

    it("Origin.xmpp.unblockUser(), Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED should be triggered');

        expect(Origin.xmpp.unblockUser('333333@localhost')).toEqual(undefined);

    });

    it("Origin.xmpp.cancelAndBlockUser(), Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED should be triggered');

        expect(Origin.xmpp.cancelAndBlockUser('333333@localhost')).toEqual(undefined);

    });

    it("Origin.xmpp.ignoreAndBlockUser(), Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED should be triggered');

        expect(Origin.xmpp.ignoreAndBlockUser('333333@localhost')).toEqual(undefined);

    });

    it("Origin.xmpp.removeFriendAndBlock(), Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED should be triggered');

        expect(Origin.xmpp.removeFriendAndBlock('333333@localhost')).toEqual(undefined);

    });

    it("Origin.xmpp.updatePresence(), Origin.events.XMPP_PRESENCECHANGED", function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.XMPP_PRESENCECHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.XMPP_PRESENCECHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.XMPP_PRESENCECHANGED should be triggered');

        expect(Origin.xmpp.updatePresence()).toEqual(undefined);

    });

    it("Origin.xmpp.requestPresence()", function() {

        expect(Origin.xmpp.requestPresence()).toEqual(undefined);

    });

    it("Origin.xmpp.joinRoom()", function() {

        expect(Origin.xmpp.joinRoom('jid', 'originId')).toEqual(undefined);

    });

    it("Origin.xmpp.leaveRoom()", function() {

        expect(Origin.xmpp.leaveRoom('jid', 'originId')).toEqual(undefined);

    });

    it("Origin.xmpp.isBlocked()", function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.social.isBlocked('jid').then(function(result) {

            expect(result).toEqual(true);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

});
