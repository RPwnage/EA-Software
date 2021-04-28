describe('Origin Client Social API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.events.CLIENT_SOCIAL_CONNECTIONCHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_GAMES_PROGRESSCHANGED should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_CONNECTIONCHANGED, cb);

            expect(result).toEqual(true);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_CONNECTIONCHANGED, cb);

        window.OriginSocialManager.connectionChanged(true);

    });

    it('Origin.events.CLIENT_SOCIAL_MESSAGERECEIVED', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_MESSAGERECEIVED should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_MESSAGERECEIVED, cb);

            expect(result).toEqual('');
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_MESSAGERECEIVED, cb);

        window.OriginSocialManager.messageReceived('');

    });

    it('Origin.events.CLIENT_SOCIAL_CHATSTATERECEIVED', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_CHATSTATERECEIVED should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_CHATSTATERECEIVED, cb);

            expect(result).toEqual('');
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_CHATSTATERECEIVED, cb);

        window.OriginSocialManager.chatStateReceived('');

    });

    it('Origin.events.CLIENT_SOCIAL_PRESENCECHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_PRESENCECHANGED should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_PRESENCECHANGED, cb);

            expect(result).toEqual('');
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_PRESENCECHANGED, cb);

        window.OriginSocialManager.presenceChanged('');

    });

    it('Origin.events.CLIENT_SOCIAL_ROSTERLOADED', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_ROSTERLOADED should be triggered');

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_ROSTERLOADED, cb);

            expect(result).toEqual('');
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_ROSTERLOADED, cb);

        window.OriginSocialManager.rosterLoaded('');

    });


    it('Origin.client.social.isConnectionEstablished()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.social.isConnectionEstablished().then(function(result) {

            expect(result).toEqual(true);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.isRosterLoaded()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.social.isRosterLoaded().then(function(result) {

            expect(result).toEqual(true);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.roster()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.social.roster().then(function(result) {

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.sendMessage()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.social.sendMessage('id', 'msgBody', 'type').then(function(result) {

            expect(result).toEqual(undefined);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.setTypingState()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.social.setTypingState('state', 'userId').then(function(result) {

            expect(result).toEqual(undefined);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.approveSubscriptionRequest(), Origin.events.CLIENT_SOCIAL_ROSTERCHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_ROSTERCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_ROSTERCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_ROSTERCHANGED should be triggered');

        Origin.client.social.approveSubscriptionRequest('jid').then(function(result) {

            expect(result).toEqual({});

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.denySubscriptionRequest(), Origin.events.CLIENT_SOCIAL_ROSTERCHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_ROSTERCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_ROSTERCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_ROSTERCHANGED should be triggered');

        Origin.client.social.denySubscriptionRequest('jid').then(function(result) {

            expect(result).toEqual({});

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.subscriptionRequest(), Origin.events.CLIENT_SOCIAL_ROSTERCHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_ROSTERCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_ROSTERCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_ROSTERCHANGED should be triggered');

        Origin.client.social.subscriptionRequest('jid').then(function(result) {

            expect(result).toEqual({});

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.cancelSubscriptionRequest(), Origin.events.CLIENT_SOCIAL_ROSTERCHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_ROSTERCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_ROSTERCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_ROSTERCHANGED should be triggered');

        Origin.client.social.cancelSubscriptionRequest('jid').then(function(result) {

            expect(result).toEqual({});

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.removeFriend(), Origin.events.CLIENT_SOCIAL_ROSTERCHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_ROSTERCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_ROSTERCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_ROSTERCHANGED should be triggered');

        Origin.client.social.removeFriend('jid').then(function(result) {

            expect(result).toEqual({});

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.setInitialPresence()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.social.setInitialPresence('presence').then(function(result) {

            expect(result).toEqual(undefined);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.requestInitialPresenceForUserAndFriends()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.social.requestInitialPresenceForUserAndFriends().then(function(result) {

            expect(result).toEqual([{}]);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.requestPresenceChange()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.social.requestPresenceChange('presence').then(function(result) {

            expect(result).toEqual(undefined);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.joinRoom()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.social.joinRoom('jid', 'originId').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.leaveRoom()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.client.social.leaveRoom('jid', 'originId').then(function(result) {

            expect(result).toEqual({});
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.blockUser(), Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED should be triggered');

        Origin.client.social.blockUser('jid').then(function(result) {

            expect(result).toEqual({});

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.unblockUser(), Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_SOCIAL_BLOCKLISTCHANGED should be triggered');

        Origin.client.social.unblockUser('jid').then(function(result) {

            expect(result).toEqual({});

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.client.social.isBlocked()', function(done) {

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