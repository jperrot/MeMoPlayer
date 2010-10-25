/*
 * Copyright (C) 2009 The J2AB Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package javax.microedition.io;

import java.io.IOException;

public interface HttpConnection extends ContentConnection {
    public static final String GET = "GET";
    public static final String POST = "POST";
    public static final String HEAD = "HEAD";

    public static final int HTTP_OK = 200;
    public static final int HTTP_ACCEPTED = 202;
    public static final int HTTP_NOT_AUTHORITATIVE = 203;
    public static final int HTTP_NO_CONTENT = 204;
    public static final int HTTP_RESET = 205;

    public static final int HTTP_MOVED_PERM = 301;
    public static final int HTTP_SEE_OTHER = 303;
    public static final int HTTP_TEMP_REDIRECT = 307;

    public static final int HTTP_UNAUTHORIZED = 401;

    public static final int HTTP_BAD_GATEWAY = 502;
    public static final int HTTP_BAD_METHOD = 405;
    public static final int HTTP_BAD_REQUEST = 400;
    public static final int HTTP_CLIENT_TIMEOUT = 408;
    public static final int HTTP_CONFLICT = 409;
    public static final int HTTP_CREATED = 201;
    public static final int HTTP_ENTITY_TOO_LARGE = 413;
    public static final int HTTP_EXPECT_FAILED = 417;
    public static final int HTTP_FORBIDDEN = 403;
    public static final int HTTP_GATEWAY_TIMEOUT = 504;
    public static final int HTTP_GONE = 410;
    public static final int HTTP_INTERNAL_ERROR = 500;
    public static final int HTTP_LENGTH_REQUIRED = 411;
    public static final int HTTP_MOVED_TEMP = 302;
    public static final int HTTP_MULT_CHOICE = 300;
    public static final int HTTP_NOT_ACCEPTABLE = 406;
    public static final int HTTP_NOT_FOUND = 404;
    public static final int HTTP_NOT_IMPLEMENTED = 501;
    public static final int HTTP_NOT_MODIFIED = 304;
    public static final int HTTP_PARTIAL = 206;
    public static final int HTTP_PAYMENT_REQUIRED = 402;
    public static final int HTTP_PRECON_FAILED = 412;
    public static final int HTTP_PROXY_AUTH = 407;
    public static final int HTTP_REQ_TOO_LONG = 414;
    public static final int HTTP_UNAVAILABLE = 503;
    public static final int HTTP_UNSUPPORTED_RANGE = 316;
    public static final int HTTP_UNSUPPORTED_TYPE = 415;
    public static final int HTTP_USE_PROXY = 305;
    public static final int HTTP_VERSION = 505;

    int getResponseCode () throws IOException;

    String getResponseMessage () throws IOException;

    String getRequestProperty (String key);

    void setRequestProperty (String key, String value) throws IOException;

    void setRequestMethod (String method) throws IOException;

    String getHeaderField (String key);

    long getDate () throws IOException;

    long getExpiration () throws IOException;

    String getHeaderField (int n) throws IOException;

    long getHeaderFieldDate (String name, long def) throws IOException;

    int getHeaderFieldInt (String name, int def) throws IOException;

    String getHeaderFieldKey (int n) throws IOException;

    String getHost ();

    long getLastModified () throws IOException;

    int getPort ();

    String getProtocol ();

    String getQuery ();

    String getRef ();

    String getRequestMethod ();

    String getURL ();

}
