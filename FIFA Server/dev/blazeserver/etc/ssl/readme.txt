Blaze Server:
    * The SSLContext for the secure endpoints in Blaze can be configured in ssl.cfg. The context includes the location of the cert (SSL_CERT) used by the endpoint. This is the cert that the server presents to the client when 
establishing secure connection.

Blaze Client:
    * A Blaze client needs the correct CA bundle loaded in order to establish the secure connection with blaze server. For DirtySDK based clients, no additional step is needed as the SDK bakes in the required certs. For more information,
please visit https://developer.ea.com/pages/viewpage.action?pageId=25133122#SecureSocketsLayer(SSL)-DirtySDKembeddedCAcertificates 
    * For a non-DirtySDK based client, the appropriate CA cert need to be loaded by the app. Check the issuer of the cert from the above location and load that cert in your application. For example, if the issuer is GOS 2015 Certificate Authority
    (Issuer: CN=GOS 2015 Certificate Authority), load the gos2015.pem cert from the blazeserver/etc/ssl/ca-certs/ directory.