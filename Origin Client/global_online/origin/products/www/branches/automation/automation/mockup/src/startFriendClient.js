var xmppsocketserver = require("./xmppsocketserver.js");

var dataServer = require("./dataserver.js");
dataServer.run(1400, 1401);

var commandServer = require("./xmppcommandserver.js");
commandServer.run(1402, xmppsocketserver);
