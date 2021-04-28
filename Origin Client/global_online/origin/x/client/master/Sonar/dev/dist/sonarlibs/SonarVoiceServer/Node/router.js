function route(handle, method, pathname, response) {
  if (typeof handle[pathname] === 'function') {
    handle[pathname](method, response);
  }
  else if( pathname === "/sonar-voiceserver/health_check.html" ||
           pathname === "/sonar-voiceserver/health_check.xml" ) {
    handle["getFile"](pathname, response);
  }
  else if( pathname === "/sonar-voiceserver/worst_channel_0.json" ||
           pathname === "/sonar-voiceserver/worst_channel_1.json" ||
           pathname === "/sonar-voiceserver/worst_channel_2.json" ||
           pathname === "/sonar-voiceserver/worst_channel_3.json" ||
           pathname === "/sonar-voiceserver/worst_channel_4.json" ||
           pathname === "/sonar-voiceserver/worst_channel_5.json" ||
           pathname === "/sonar-voiceserver/worst_channel_6.json" ||
           pathname === "/sonar-voiceserver/worst_channel_7.json" ||
           pathname === "/sonar-voiceserver/worst_channel_8.json" ||
           pathname === "/sonar-voiceserver/worst_channel_9.json" ) {
    handle["getFileJson"](pathname, response);
  }
  else if( pathname === "/sonar-voiceserver/worst_client_0.json" ||
           pathname === "/sonar-voiceserver/worst_client_1.json" ||
           pathname === "/sonar-voiceserver/worst_client_2.json" ||
           pathname === "/sonar-voiceserver/worst_client_3.json" ||
           pathname === "/sonar-voiceserver/worst_client_4.json" ||
           pathname === "/sonar-voiceserver/worst_client_5.json" ||
           pathname === "/sonar-voiceserver/worst_client_6.json" ||
           pathname === "/sonar-voiceserver/worst_client_7.json" ||
           pathname === "/sonar-voiceserver/worst_client_8.json" ||
           pathname === "/sonar-voiceserver/worst_client_9.json" ) {
    handle["getFileJson"](pathname, response);
  }
  else {
    response.writeHead(404, {"Content-Type": "text/plain"});
    response.write("404 Not Found\n");
    response.end();
  }
}

exports.route = route;
