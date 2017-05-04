/*
 *  Simple HTTP get webclient test
 */

#include <ESP8266WiFi.h>

const char *ssid     = "Silver-Lining";
const char *password = "Iw2hl&loaswJ!";
const char *descJson = "{ \"nodeType\" : [\"lights\"], \"name\" : \"salon overheads\", \"hardware\" : \"feather huzzah\", \"battery\" : false, \"id\" : 100, \"lights\" : { \"colourType\" : \"rgb\", \"queryAvailable\" : true } }";

#define NUM_PORTS 1

word red[NUM_PORTS];
word green[NUM_PORTS];
word blue[NUM_PORTS];

char  urlBuff[100];
char *params[5][2];

// we use port 8882 by default in node - so lets be consistent
//
WiFiServer server( 8882 );

void setup() {
    
    Serial.begin(115200);
    Serial.println();
    Serial.printf("Connecting to %s\n", ssid);

    //     char  buff[100] ;
    //     char *params[5][2];
    //     char  cmd       = 0;
    //     int   numParams = 0;
    //     char *str       = "red=255&blue=0&green=0&port=2";
    //     char *dst = buff;
    //     char *src = str;

    //     while( *src ) {
    //         *dst++ = *src++;
    //     }
    //     *dst = '\0';
        
    //     Serial.printf( "parsing params from %s\n", str );
        
    // int count = parseUrlParams( buff, params, 5, 0) ;

    //     Serial.println( "parsed params" );
        
    // for( int i = 0; i < count; i++ ) {
    //     Serial.print( params[i][0] );
    //     Serial.print( " = " );
    //     Serial.println( params[i][1] );
    // }
    
    // Serial.println( "done printing params" );

    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" connected");
    
    server.begin();
    Serial.printf("Web server started, open %s in a web browser\n", WiFi.localIP().toString().c_str());

    for( int i = 0; i < NUM_PORTS; i++ ) {
        red[i]   = 64;
        green[i] = 128;
        blue[i]  = 249;
    }
}

/**
 * queryString: the string with is to be parsed.
 * WARNING! This function overwrites the content of this string. Pass this function a copy
 * if you need the value preserved.
 * results: place to put the pairs of param name/value.
 * resultsMaxCt: maximum number of results, = sizeof(results)/sizeof(*results)
 * decodeUrl: if this is true, then url escapes will be decoded as per RFC 2616
 */
int parseUrlParams( char *queryString, char *results[][2], int resultsMaxCt, boolean decodeUrl ) {

    int ct = 0;

    while( queryString && *queryString && ct < resultsMaxCt ) {

        results[ct][0] = queryString;
        while( *queryString ) {
            if( *queryString == '=' ) {
                break;
            }
            queryString++;
        }

        if( *queryString == '\0' ) {
            return ct;
        }

        *queryString++ = '\0';
        
        results[ct][1] = queryString;

        while( *queryString ) {
            if( *queryString == '&' ) {
                break;
            }
            queryString ++;
        }

        ct++;
        if( *queryString == '\0' ) {
            return ct;
        }

        *queryString++ = '\0';
    }
    return ct;
}

///**
// * compare two strings, ranged
// */
//char compareStrings( char *str1, char offs1, char *str2, char offs2, char len ) {
//
//    for( char idx = 0; idx < len; idx ++ ) {
//        if( str1[offs1] == '\0' ) {
//            return 0;
//        }
//        
//        if( str2[offs2] == '\0' ) {
//            return 0;
//        }
//        
//        if( str1[offs1] != str2[offs2] ) {
//            return 0;
//        }
//        offs1 ++ ;
//        offs2 ++ ;
//    }
//
//    return 1;
//}

/**
 * prepare a web page to be send to a client (web browser)
 */
String prepareHtmlPage( WiFiClient client, char *type, char *content ) {

    // add this to the header to make the client refresh
    //
    //            "Refresh: 5\r\n" +  // refresh the page automatically every 5 sec

//    String htmlPage = 
//        String("HTTP/1.1 200 OK\r\n") +
//        "Content-Type: text/html\r\n" +
//        "Connection: close\r\n" +  // the connection will be closed after completion of the response
//        "\r\n" +
//        "<!DOCTYPE HTML>" +
//        "<html>" +
//        "Analog input:  " + String(analogRead(A0)) +
//        "</html>" +
//        "\r\n";
//    return htmlPage;

    char *part1 = "HTTP/1.1 200 OK\r\nContent-Type: ";
    char *part2 = "\r\nConnection: close\r\n\r\n";
    char *part3 = "\r\n";
    char buffer[100];

    char *dst = &buffer[0];
    char *src = &part1[0];

    while( *src ) {
        *dst++ = *src++;
    }

    src = type;
    while( *src ) {
        *dst++ = *src++;
    }
    
    src = &part2[0];
    while( *src ) {
        *dst++ = *src++;
    }

    src = content;
    while( *src ) {
        *dst++ = *src++;
    }
    
    src = &part3[0];
    while( *src ) {
        *dst++ = *src++;
    }

    *dst = '\0';
    Serial.println( buffer );
    client.println( buffer);
//
//    
//    String htmlPage = 
//        String("HTTP/1.1 200 OK\r\n") +
//        "Content-Type: " + 
//        type +
//        "\r\n" +
//        "Connection: close\r\n" +  // the connection will be closed after completion of the response
//        "\r\n" +
//        content +
//        "\r\n";
//
//    client.println( htmlPage );
}

/**
 * A utility function to reverse a string  
 */
void reverse( char str[], int start, int end ) {

    end--;

    while( start < end ) {
        char tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;
        start++;
        end--;
    }
}

/**
 * char* itoa(int num, char* str, int base)
 */
char* itoaInPlace( int num, char* buff, int base ){

    bool isNegative = false;
    char *dst       = buff;

    // Handle 0 explicitely, otherwise empty string is printed for 0 
    //
    if( num == 0 ) {
        *dst++ = '0';
        *dst++ = '\0';
        return dst;
    }
 
    // In standard itoa(), negative numbers are handled only with 
    // base 10. Otherwise numbers are considered unsigned.
    //
    if( num < 0 && base == 10 ) {

        isNegative = true;
        num        = -num;
    }
 
    // Process individual digits
    //
    while( num != 0 ) {

        int rem = num % base;
        *dst++ = (rem > 9)? (rem-10) + 'a' : rem + '0';
        num = num/base;
    }
 
    // If number is negative, append '-'
    //
    if( isNegative ) {
        *dst++ = '-';
    }
 
    *dst++ = '\0';  // Append string terminator
 
    // Reverse the string
    //
    reverse( buff, 0, dst-buff );
 
    return dst;
}

/**
 *
 */
void reportLights( WiFiClient client ) {

    char buffer[100];
    char *dst = buffer;
    char *src;

    *dst++ = '[';

    for( int i = 0;  i < NUM_PORTS; i++ ) {

        src = "{ 'port' : ";
        while( *src ) {
            *dst++ = *src++;
        }
        dst = itoaInPlace( i, dst, 10 ) ;

        src = "{ , 'red ' : ";
        while( *src ) {
            *dst++ = *src++;
        }
        dst = itoaInPlace( red[i], dst, 10 );


        src = "{ , 'green ' : ";
        while( *src ) {
            *dst++ = *src++;
        }
        dst = itoa( green[i], dst, 10 );

        src = "{ , 'blue ' : ";
        while( *src ) {
            *dst++ = *src++;
        }
        dst = itoaInPlace( blue[i], dst, 10 );

        *dst++ = '}';

        if( i < NUM_PORTS-1 ) {
            *dst++ = ',';
        }
    }
    *dst = ']';
    *dst = '\0';

    prepareHtmlPage( client, "application/json", buffer );
}

///**
// * Perform URL percent decoding.
// * Decoding is done in-place and will modify the parameter.
// */
//void percentDecode( char *src ) {
//
//    char *dst = src;
//
//    while (*src) {
//        if (*src == '+') {
//            src++;
//            *dst++ = ' ';
//        }
//        else if (*src == '%') {
//            // handle percent escape
//            //
//            *dst = '\0';
//            src++;
//
//            if (*src >= '0' && *src <= '9') {
//                *dst = *src++ - '0';
//            }
//            else if (*src >= 'A' && *src <= 'F') {
//                *dst = 10 + *src++ - 'A';
//            }
//            else if (*src >= 'a' && *src <= 'f') {
//                *dst = 10 + *src++ - 'a';
//            }
//
//            // this will cause %4 to be decoded to ascii @, but %4 is invalid
//            // and we can't be expected to decode it properly anyway
//            //
//            *dst <<= 4;
//
//            if (*src >= '0' && *src <= '9') {
//                *dst |= *src++ - '0';
//            }
//            else if (*src >= 'A' && *src <= 'F') {
//                *dst |= 10 + *src++ - 'A';
//            }
//            else if (*src >= 'a' && *src <= 'f') {
//                *dst |= 10 + *src++ - 'a';
//            }
//
//            dst++;
//        }
//        else {
//            *dst++ = *src++;
//        }
//    }
//    *dst = '\0';
//}

boolean getLine( WiFiClient client, char *buff, int len ) {
    
    String line = client.readStringUntil('\r');
    line.toCharArray( buff, len );

    return line.startsWith( "GET /", 0  );
}

/**
 * change the lights on a port
 */
void changeLights() {
    
}

/**
 * 
 */
void loop() {
    WiFiClient client = server.available(); 
    
    // wait for a client (web browser) to connect
    //
    if (client) {
        
        Serial.println("\n[Client connected]");
        
        char  cmd       = 0;
        int   numParams = 0;
        
        while( client.connected() ) {
            
            // read line by line what the client (web browser) is requesting
            //
            if( client.available() ) {
                boolean lineReqd = getLine( client, urlBuff, 100 );
                
                Serial.print( urlBuff );

                if( lineReqd ) {

                    cmd = urlBuff[5];
                    if( urlBuff[6] = '?' ) {
                        numParams = parseUrlParams( &urlBuff[7], params, 5, true );
                    }
                    
                    Serial.printf( " ... Command found: [%c]\n... params", cmd );

                    if( numParams == 0 ) {
                        Serial.println( "... ... (none)" );
                    }
                    else {
                        for( int i = 0; i < numParams; i++ ) {
                            Serial.printf( "... ... %s = %s", params[i][0], params[i][1] );
                        }
                    }
                }

                // wait for end of client's request, that is marked with an empty line
                //
                if( urlBuff[1] == '\0' && urlBuff[0]  == '\n') {

                    Serial.printf( " processing command: [%c]", cmd );
                    switch( cmd ) {
                        case 'q' : 
                            prepareHtmlPage( client, "application/json", (char *)descJson );
                            break;

                        case 'l' :
                            changeLights();
                            break;

                        case 'r' :
                            reportLights( client );
                            break;

                        default:
                            prepareHtmlPage( client, "text/html",  "<h1>Errror</h1><p>Unknown command" );
                            break;
                    }
                    break;
                }
            }
    }
    // give the web browser time to receive the data
    //
    delay(1); 
    
    // close the connection:
    //
    client.stop();
    Serial.println("[Client disonnected]");
    }
}

