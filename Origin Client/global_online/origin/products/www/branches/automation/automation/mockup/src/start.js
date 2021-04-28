var dataServer = require("./dataserver.js");
dataServer.run(1400, 1401);

var xmppServer = require("./xmppserver.js");
xmppServer.run(5291);

var commandServer = require("./commandserver.js");
commandServer.run(1402, xmppServer);

var dirtybitServer = require("./dirtybitserver.js");
dirtybitServer.run(5292);