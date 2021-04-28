var xmppJssdkUserStub = require("./xmppjssdkuserstub.js");
var xmppFriendStub = require("./xmppfriendstub.js");
var xmppObjects = require("./xmppobjects.js");
var dataServer = require("./dataserver.js");
var express = require('express');
var bodyParser = require("body-parser");

// enable CORS
var allowCrossDomain = function(req, res, next) {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Methods', 'GET,PUT,POST,DELETE,OPTIONS');
    res.header('Access-Control-Allow-Headers', req.headers['access-control-request-headers']);

    // intercept OPTIONS method
    if ('OPTIONS' == req.method) {
        res.sendStatus(200);
    } else {
        next();
    }
};

var app = express();
app.use(allowCrossDomain);
app.use(bodyParser.json()); // support json encoded bodies
app.use(bodyParser.urlencoded({ extended: true })); // support encoded bodies

module.exports = {

    run : function(proxy_port, xmpp_server) {

        // For testing purpose, this helper func always return a friend even the given jid is not in friend list.
        // This newly created friend is NOT added in friend list
        function getFriend(jid, friends) {
            if (jid && friends) {
                var friend = friends.getFriendByJID(jid);
                if (!friend) {
                    var defaultName = jid.split('@')[0];
                    friend = new xmppObjects.Friend(jid, defaultName, defaultName, defaultName);
                }

                return friend;
            } else {
                return null;
            }
        };

        app.post('/response/:action', function(req, res) {

            console.log('receiving ' + req.protocol + '://' + req.get('host') + req.originalUrl);
            console.log('request.body: ' + JSON.stringify(req.body));

            var respHeaders = {};
            var resp = {};

            var action = req.params.action;
            if ('set' === action) {
                resp = dataServer.inject(req.body.url, req.body.statusCode, req.body.headers, req.body.data);
            } else if ('reset' === action) {
                dataServer.reset();

                resp.respData = 'ok';
                resp.statusCode = 200;
            } else {
                resp.respData = 'unknown action "' + action + '"';
                resp.statusCode = 400;
            }

            respHeaders['Content-Length'] = resp.respData.length;
            respHeaders['Access-Control-Allow-Origin'] = '*';

            res.writeHead(resp.statusCode, respHeaders);
            res.write(resp.respData);
            res.end();

        });

        app.post('/xmpp/:pid/:jid/:action', function(req, res) {

            console.log('receiving ' + req.protocol + '://' + req.get('host') + req.originalUrl);
            console.log('request.body: ' + JSON.stringify(req.body));

            var resp = {};
            resp['pid'] = req.params.pid;
            resp['jid'] = req.params.jid;
            resp['action'] = req.params.action;

            var pid = req.params.pid;
            var jid = req.params.jid;
            var action = req.params.action;

            // get connected jssdkuser's socket.
            var conn = xmpp_server.getConnByUsername(pid);
            if (conn) {
                var ws = conn.ws;
                var jssdkuser = conn.stub.jssdkuser;
                var friends = conn.friends;

                var friend = getFriend(jid, friends);
                if (friend) {
                    var stub = new xmppFriendStub();
                    if (action === 'sendMessage') {
                        var message = 'default mockup message';
                        if (req.body.hasOwnProperty('message')) {
                            message = req.body.message;
                            resp['message'] = message;
                        }
                        stub.sendMessage(ws, jssdkuser, friend, message);
                    } else if (action === 'sendAccept') {
                        stub.sendAccept(ws, jssdkuser, friend, conn.stub.getRsttigID());
                        friend.subscription = xmppObjects.subscriptions.BOTH;
                        friend.ask = xmppObjects.asks.NULL;
                        friends.add(friend);
                    } else if (action === 'sendReject') {
                        stub.sendReject(ws, jssdkuser, friend, conn.stub.getRsttigID());
                        friends.removeFriendByJID(jid);
                    } else if (action === 'sendFriendRequest') {
                        stub.sendFriendRequest(ws, jssdkuser, friend);
                        friends.add(friend);
                    } else if (action === 'sendFriendRequestRevoke') {
                        stub.sendFriendRequestRevoke(ws, jssdkuser, friend, conn.stub.getRsttigID());
                        friends.removeFriendByJID(jid);
                    } else if (action === 'sendFriendRemove') {
                        stub.sendFriendRemove(ws, jssdkuser, friend, conn.stub.getRsttigID());
                        friends.removeFriendByJID(jid);
                    } else if (action === 'sendPresence') {
                        if (req.body.hasOwnProperty('presence')) {
                            var presence = xmppObjects.presences[req.body.presence];
                            if (presence) {
                                friend.presence = presence;
                            } else {
                                resp['error'] = 'unknown presence: ' + req.body.presence;
                            }
                            resp['presence'] = friend.presence;
                        }
                        stub.sendPresence(ws, jssdkuser, friend);
                    } else {
                        res.status(400);
                        resp['error'] = 'unknown action: ' + action;
                    }
                } else {
                    res.status(400);
                    resp['error'] = 'unhandled jid: ' + jid;
                }
            } else {
                res.status(400);
                resp['error'] = 'no connection';
            }

            res.json(resp);
        });

        app.listen(process.env.PORT || proxy_port, function() {
            console.log('Local command server running at port: ' + proxy_port);
        });

    }

};
