module.exports = {

    run : function(proxy_port) {

        var connections = 0;
        var WebSocketServer = require('ws').Server;
        var wss = new WebSocketServer({
            port : proxy_port
        });

        console.log('Local dirtybit server running at port: %d', proxy_port);

        wss.on('connection', function(ws) {

            console.log('dirtybit connected count: %d', ++connections);

            ws.on('open', function() {
                console.log('dirtybit open');
            });

            ws.on('ping', function(data) {
                console.log('dirtybit ping: %s', data);
            });

            ws.on('pong', function(data) {
                console.log('dirtybit pong: %s', data);
            });

            ws.on('iq', function(data) {
                console.log('dirtybit pong: %s', data);
            });

            ws.on('message', function(message) {
                console.log('dirtybit on message: %s', message);
            });

            ws.on('close', function(code) {
                console.log('dirtybit close: %s', code);
            });

        });

    }

};

