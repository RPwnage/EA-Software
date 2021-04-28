var states = {
    ACTIVE : 'active',
    INACTIVE : 'inactive',
    GONE : 'gone',
    COMPOSING : 'composing',
    PAUSED : 'paused'
};

module.exports.states = states;

var presences = {
    ONLINE : {
        type : null,
        show : null
    },
    AWAY : {
        type : null,
        show : 'away'
    },
    OFFLINE : {
        type : 'unavailable',
        show : 'away'
    }
};

module.exports.presences = presences;

var Person = function(jid, name, personaid, eaid) {

    this.jid = jid;
    this.name = name;
    this.personaid = personaid;
    this.eaid = eaid;
    this.presence = presences.ONLINE;
    this.state = states.ACTIVE;

};

module.exports.Person = Person;

// http://xmpp.org/rfcs/rfc3921.html#substates
var subscriptions = {
    NONE : 'none', // contact and user are not subscribed to each other, and neither has requested a subscription from the other
    NONE_PENDING_OUT : 'none + pending out', // contact and user are not subscribed to each other, and user has sent contact a subscription request but contact has not replied yet
    NONE_PENDING_IN : 'none + pending in', // contact and user are not subscribed to each other, and contact has sent user a subscription request but user has not replied yet (note: contact's server SHOULD NOT push or deliver roster items in this state, but instead SHOULD wait until contact has approved subscription request from user)
    NONE_PENDING_OUT_IN : 'none + pending out/in', // contact and user are not subscribed to each other, contact has sent user a subscription request but user has not replied yet, and user has sent contact a subscription request but contact has not replied yet
    TO : 'To', // user is subscribed to contact (one-way)
    TO_PENDING_IN : 'To + Pending In', // user is subscribed to contact, and contact has sent user a subscription request but user has not replied yet
    FROM : 'From', // contact is subscribed to user (one-way)
    FROM_PENDING_OUT : 'From + Pending Out', // contact is subscribed to user, and user has sent contact a subscription request but contact has not replied yet
    BOTH : 'both' // user and contact are subscribed to each other (two-way)
};

module.exports.subscriptions = subscriptions;

var groups = {
    GLOBAL : 'global',
    BUDDY : 'buddy'
};

module.exports.groups = groups;

var asks = {
    NULL : null,
    SUBSCRIBE : 'subscribe'
};

module.exports.asks = asks;

// Define the Friend constructor
var Friend = function(jid, name, personaid, eaid) {
    // Call the parent constructor, making sure (using Function#call)
    // that "this" is set correctly during the call
    Person.apply(this, Array.prototype.slice.call(arguments));

    this.subscription = subscriptions.NONE;
    this.ask = asks.NULL;
    this.group = groups.GLOBAL;
    this.recvMsg = '';

};

// Create a Friend.prototype object that inherits from Person.prototype.
// Note: A common error here is to use "new Person()" to create the
// Friend.prototype. That's incorrect for several reasons, not least
// that we don't have anything to give Person for the "name" argument.
// The correct place to call Person is above, where we call it from Friend.
Friend.prototype = Object.create(Person.prototype);

// Set the "constructor" property to refer to Friend
Friend.prototype.constructor = Friend;

module.exports.Friend = Friend;

var Friends = function() {

    var list = {};

    this.add = function(friend) {
        list[friend.jid] = friend;
    };

    this.getFriendJIDs = function() {
        return Object.keys(list);
    };

    this.getFriendByJID = function(jid) {
        return list[jid];
    };

    this.length = function() {
        return Object.keys(list).length;
    };
    
    this.removeFriendByJID = function(jid) {
        delete list[jid];
    };

};

module.exports.Friends = Friends;
