package com.ea.tool.origin.gridnodeagent;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.invoke.MethodHandles;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.X509Certificate;
import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSession;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

public class OAAgentRestfulHelper {

    private static final Logger _log = LogManager.getLogger(MethodHandles.lookup().lookupClass());

    //Following code to trust all certificates that are given by server
    static {
        TrustManager[] trustAllCerts = new TrustManager[]{new X509TrustManager() {
            @Override
            public java.security.cert.X509Certificate[] getAcceptedIssuers() {
                return null;
            }

            @Override
            public void checkClientTrusted(X509Certificate[] certs, String authType) {
            }

            @Override
            public void checkServerTrusted(X509Certificate[] certs, String authType) {
            }
        }};

        try {
            // Install the all-trusting trust manager
            final SSLContext sc = SSLContext.getInstance("SSL");
            sc.init(null, trustAllCerts, new java.security.SecureRandom());
            HttpsURLConnection.setDefaultSSLSocketFactory(sc.getSocketFactory());
        } catch (NoSuchAlgorithmException | KeyManagementException e) {
            _log.error(e, e);
        }

        // Create all-trusting host name verifier
        HostnameVerifier allHostsValid = new HostnameVerifier() {
            @Override
            public boolean verify(String hostname, SSLSession session) {
                return true;
            }
        };

        // Install the all-trusting host verifier
        HttpsURLConnection.setDefaultHostnameVerifier(allHostsValid);
    }

    /**
     * Generic get request
     *
     * @param url - Url to which the request has to be made
     * @return response of the request
     * @throws java.io.IOException
     * @throws java.net.URISyntaxException
     */
    public static String get(String url) throws IOException, URISyntaxException {
        _log.trace(">> get: " + url);
        final HttpURLConnection connection = (HttpURLConnection) new URL(url).openConnection();
        connection.setReadTimeout(5000);
        connection.setConnectTimeout(1000);
        connection.setRequestMethod("GET");
        _log.debug(connection.getRequestMethod() + ": " + connection.getURL().toString());
        _log.trace("<< get: " + url);

        return output(connection);
    }

    /**
     * Generic get request
     *
     * @param url - Url to which the request has to be made
     * @param output - file for storing binary data
     * @throws java.io.IOException
     * @throws java.net.URISyntaxException
     */
    public static void get(String url, File output) throws IOException, URISyntaxException {
        _log.trace(">> get: " + url);
        final HttpURLConnection connection = (HttpURLConnection) new URL(url).openConnection();
        connection.setReadTimeout(5000);
        connection.setConnectTimeout(1000);
        connection.setRequestMethod("GET");
        _log.debug(connection.getRequestMethod() + ": " + connection.getURL().toString());
        _log.trace("<< get: " + url);

        output(connection, output);
    }

    /**
     * Generic Put request
     *
     * @param url - Url to which the request has to be made
     * @param data - data or parameters that needs to be sent with request
     * @return response of the request
     * @throws java.io.IOException
     * @throws java.net.URISyntaxException
     */
    public static String post(String url, String data) throws IOException, URISyntaxException {
        _log.trace(">> post: " + url);
        final URI uri = new URI(url.replace(" ", "%20"));
        final HttpURLConnection connection = (HttpURLConnection) uri.toURL().openConnection();
        connection.setReadTimeout(5000);
        connection.setConnectTimeout(1000);
        connection.setDoOutput(true);
        connection.setDoInput(true);
        connection.setInstanceFollowRedirects(true);

        // Specify what kind of request we want
        connection.setRequestMethod("POST");
        connection.setRequestProperty("Accept-Charset", "UTF-8");
        connection.setRequestProperty("Content-Type", "application/x-www-form-urlencoded;charset=" + "UTF-8");
        connection.setRequestProperty("Content-type", "text/json");
        connection.setUseCaches(false);
        _log.debug(connection.getRequestMethod() + ": " + connection.getURL().toString());
        _log.debug("data: " + data);

        try (OutputStream wr = connection.getOutputStream()) {
            wr.write((data != null && !data.isEmpty()) ? data.getBytes("UTF-8") : "".getBytes("UTF-8"));
            wr.flush();
        }

        _log.trace("<< post: " + url);
        return output(connection);
    }

    /**
     * Generic Put request
     *
     * @param url - Url to which the request has to be made
     * @param data - data or parameters that needs to be sent with request
     * @return response of the request
     * @throws java.io.IOException
     */
    protected static String put(String url, String data) throws IOException {
        _log.trace(">> put: " + url);

        HttpURLConnection connection = (HttpURLConnection) new URL(url).openConnection();
        connection.setReadTimeout(5000);
        connection.setConnectTimeout(1000);
        connection.setDoOutput(true);

        // Specify what kind of request we want
        connection.setRequestMethod("PUT");
        connection.setRequestProperty("Content-type", "text/json");
        _log.debug(connection.getRequestMethod() + ": " + connection.getURL().toString());

        try (OutputStream wr = connection.getOutputStream()) {
            wr.write((data != null && !data.isEmpty()) ? data.getBytes("UTF-8") : "".getBytes("UTF-8"));
            wr.flush();
        }

        _log.trace("<< put: " + url);
        return output(connection);
    }

    /**
     * Generic delete request
     *
     * @param url - Url to which the request has to be made
     * @return response of the request
     * @throws java.net.MalformedURLException
     */
    public static String delete(String url) throws MalformedURLException, IOException {
        _log.trace(">> delete: " + url);

        HttpURLConnection connection = (HttpURLConnection) new URL(url).openConnection();
        connection.setReadTimeout(5000);
        connection.setConnectTimeout(1000);
        connection.setDoOutput(true);

        // Specify what kind of request we want
        connection.setDoOutput(true);
        connection.setRequestMethod("DELETE");
        connection.connect();

        _log.trace("<< delete: " + url);
        return output(connection);
    }

    /**
     * Generic Response parsing for any Rest call
     *
     * @param connection - associated with the REST request to be made
     * @return response of the connection
     * @throws IOException
     */
    public static String output(HttpURLConnection connection) throws IOException {
        _log.debug("Response code: " + connection.getResponseCode());
        checkResponseStatus(connection);

        try (InputStream in = connection.getInputStream(); ByteArrayOutputStream resultOS = new ByteArrayOutputStream();) {
            byte[] buffer = new byte[1024];
            int length;
            while ((length = in.read(buffer)) > 0) {
                resultOS.write(buffer, 0, length);
            }
            final String result = resultOS.toString();
            _log.debug("result: " + result);

            return result;
        } catch (Exception e) {
            _log.error(e, e);
            throw e;
        }
    }

    /**
     * Generic binary Response handler
     *
     * @param connection - associated with the REST request to be made
     * @param file - file for storing binary data
     * @throws IOException
     */
    public static void output(HttpURLConnection connection, File file) throws IOException {
        System.out.println("Response code: " + connection.getResponseCode());
        checkResponseStatus(connection);

        try (InputStream in = connection.getInputStream(); OutputStream outputStream = new FileOutputStream(file);) {
            byte[] buffer = new byte[1024];
            int length;
            while ((length = in.read(buffer)) > 0) {
                outputStream.write(buffer, 0, length);
            }
        } catch (Exception e) {
            _log.error(e);
            throw e;
        }
    }

    /**
     * Check Response status for client and server errors
     *
     * @param connection - associated with the REST request to be made
     * @throws java.io.IOException
     */
    public static void checkResponseStatus(HttpURLConnection connection) throws IOException {
        try {
            final int resCode = connection.getResponseCode();
            if ((resCode >= 400) && (resCode < 500)) {
                throw new IOException("Client error : Error code - " + resCode + ", Error response message: " + connection.getResponseMessage());
            } else if ((connection.getResponseCode() >= 500) && (connection.getResponseCode() < 600)) {
                throw new IOException("Server error : Error code - " + resCode + ", Error response message: " + connection.getResponseMessage());
            }
        } catch (IOException e) {
            _log.error(e, e);
            throw e;
        }
    }

}
