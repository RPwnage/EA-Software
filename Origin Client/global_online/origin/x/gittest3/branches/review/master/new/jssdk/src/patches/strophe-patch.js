/* jshint ignore:start */
Strophe.Connection.prototype.authenticate = function(matched) {
    // if none of the mechanism worked
    if (Strophe.getNodeFromJid(this.jid) === null) {
        // we don't have a node, which is required for non-anonymous
        // client connections
        this._changeConnectStatus(Strophe.Status.CONNFAIL, 'x-strophe-bad-non-anon-jid');
        this.disconnect('x-strophe-bad-non-anon-jid');
    } else {
        // fall back to legacy authentication
        this._changeConnectStatus(Strophe.Status.AUTHENTICATING, null);
        this._addSysHandler(this._auth1_cb.bind(this), null, null, null, "_auth_1");
        this.send($iq({
            type: "get",
            to: this.domain,
            id: "_auth_1"
        }).c("query", {
            xmlns: Strophe.NS.AUTH
        }).c("username", {}).t(Strophe.getNodeFromJid(this.jid)).tree());
    }
}

Strophe.Connection.prototype._auth1_cb = function(elem) {
    // build plaintext auth iq
    var iq = $iq({
            type: "set",
            id: "_auth_2"
        })
        .c('query', {
            xmlns: Strophe.NS.AUTH
        })
        .c('username', {}).t(Strophe.getNodeFromJid(this.jid))
        .up()
        .c('token').t(this.pass);

    if (!Strophe.getResourceFromJid(this.jid)) {
        // since the user has not supplied a resource, we pick
        // a default one here.  unlike other auth methods, the server
        // cannot do this for us.
        this.jid = Strophe.getBareJidFromJid(this.jid) + '/strophe';
    }
    iq.up().c('resource', {}).t(Strophe.getResourceFromJid(this.jid));
    this._addSysHandler(this._auth2_cb.bind(this), null, null, null, "_auth_2");
    this.send(iq.tree());
    return false;
}
/* jshint ignore:end */