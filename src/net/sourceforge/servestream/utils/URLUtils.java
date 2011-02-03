/*
 * ServeStream: A HTTP stream browser/player for Android
 * Copyright 2010 William Seemann
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package net.sourceforge.servestream.utils;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;

import javax.net.ssl.HttpsURLConnection;

public class URLUtils {
	
	public final static int DIRECTORY = 100;
	public final static int MEDIA_FILE = 200;
	public final static int NOT_FOUND = -1;
	
	private final static String MIME_HTML = "text/html";
	
	/*
	 * Default constructor
	 */
    public URLUtils() {
    	
    }
	
	public static int getContentTypeCode(URL url) {
		
		boolean contentFound = false;
		int contentTypeCode = NOT_FOUND;
	    HttpURLConnection conn = null;
		
		try {
			
			if (url == null)
				return contentTypeCode;

		    String contentType = null;
			
        	if (url.getProtocol().equals("http")) {
        		conn = (HttpURLConnection) url.openConnection();
        	} else if (url.getProtocol().equals("https")) {
        		conn = (HttpsURLConnection) url.openConnection();        		
        	}
			
        	if (conn == null)
        		return NOT_FOUND;
        	
    		conn.setConnectTimeout(6000);
    		conn.setReadTimeout(6000);
	        conn.setRequestMethod("GET");
	    
	        if ((contentType = conn.getContentType()) != null)
	        	contentFound = true;
            
            if (contentFound) {
            	//TODO fix this
            	if (contentType.contains(MIME_HTML)) {
        			contentTypeCode = DIRECTORY;
                } else {
        			contentTypeCode = MEDIA_FILE;
                }
            }
            
		} catch (IOException ex) {
			ex.printStackTrace();
		} catch (IllegalArgumentException ex) {
		    ex.printStackTrace();
		} finally {
			closeHttpConnection(conn);
		}
		
		return contentTypeCode;
	}
	
	public static String getContentType(URL url) {
		
		String contentType = null;
	    HttpURLConnection conn = null;
		
		try {
			
			if (url == null)
				return contentType;
			
        	if (url.getProtocol().equals("http")) {
        		conn = (HttpURLConnection) url.openConnection();
        	} else if (url.getProtocol().equals("https")) {
        		conn = (HttpsURLConnection) url.openConnection();        		
        	}
			
        	if (conn == null)
        		return contentType;
        	
    		conn.setConnectTimeout(6000);
    		conn.setReadTimeout(6000);
	        conn.setRequestMethod("GET");
	    
	        contentType = conn.getContentType();
            
		} catch (IOException ex) {
			ex.printStackTrace();
		} catch (IllegalArgumentException ex) {
		    ex.printStackTrace();
		} finally {
			closeHttpConnection(conn);
		}
		
		return contentType;
	}
	
	/**
	 * Closes a HttpURLConnection
	 * 
	 * @param conn The connection to close
	 */
    private static void closeHttpConnection(HttpURLConnection conn) {
    	
    	if (conn == null)
    		return;
    	
    	conn.disconnect();
    }
}
