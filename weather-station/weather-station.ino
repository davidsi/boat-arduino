#include <Wire.h>
#include <math.h> 
#include "TimerOne.h" // Timer Interrupt set to 2 second for read sensors 

#define SLAVE_ADDR  4
#define UNO_DEBUG

#ifdef UNO_DEBUG
//#define WindSensorPin (2)       // The pin location of the anemometer sensor 
//#define WindVanePin (A1)        // The pin the wind vane sensor is connected to 
//#else
#define WindSensorPin (3)       // The pin location of the anemometer sensor 
#define WindVanePin (A3)        // The pin the wind vane sensor is connected to 
#endif

unsigned int           windSpeed[10];               // speed knots
unsigned int           windSpeedIdx;

unsigned int           windDirectionIdx;            // translated 0 - 360 direction 
unsigned int           windDirection[10];           // array of the last 10 values - we sum them to get an average

volatile bool          isSpeedSampleRequired;       // this is set true every 2.5s. Get wind speed 
volatile unsigned int  timerCount;                  // used to determine 2.5sec timer count 
volatile unsigned long vaneRotations;               // cup rotation counter used in interrupt routine 
volatile unsigned long contactBounceTime;           // Timer to avoid contact bounce in isr 

/**
 * get the average wind direction. We store the last ten values and average over them
 *
 * for direction, as we 
 */
int getWindDirectionAverage() {

    signed long result = 0;
    for( int idx = 0; idx < 10; idx ++ ) {
        signed long dir = (signed long)windDirection[idx];
        result += (dir - 180);
    }
    return (int)(signed long)((result / 10) + 180);
}

/**
 *
 */
int getWindSpeedAverage() {

    long result = 0;
    for( int idx = 0; idx < 10; idx ++ ) {
        result += windSpeed[idx];
    }
    return (int)(result / 10);
}

/**
 * get the i2c string
 */
void prepareI2C( char *i2cBuffer ) {

    int theSpeed     = getWindSpeedAverage();
    int theDirection = getWindDirectionAverage();

    i2cBuffer[0] = (theSpeed / 10) + '0';
    i2cBuffer[1] = (theSpeed % 10) + '0';
    i2cBuffer[2] = '-';
    i2cBuffer[3] = (theDirection / 100) + '0';
    i2cBuffer[4] = ((theDirection / 10) % 10) + '0';
    i2cBuffer[5] = (theDirection % 10) + '0';
    i2cBuffer[6] = '!';
    i2cBuffer[7] = '\0';
}

/**
 * note that the slave can not control the size of the buffer.  Therefore we use a termainating character
 * for now, we use "!"
 */
void i2cDataRequested() {

#ifdef UNO_DEBUG
    Serial.println( "i2c data requested" );
#endif
    char i2cBuffer[8];
    
    prepareI2C( i2cBuffer );
    Wire.write( i2cBuffer );
#ifdef UNO_DEBUG
    Serial.println( "... i2c data sent" );
#endif
}

/**
 * function that executes whenever data is received from i2c master
 * this function is registered as an event, see setup()
 */
void i2cDataRecieved( int howMany ) {

#ifdef UNO_DEBUG
    Serial.println( "i2c data recieved" );
#endif

    char str[2];
    str[1] = 0;
    
    while( 1 < Wire.available() ) {     // loop through all but the last
#ifdef UNO_DEBUG
        str[0] = Wire.read();           // receive byte as a character
        Serial.print(str);              // print the character
#endif        
    }

    str[0] = Wire.read();               // receive byte as a character
#ifdef UNO_DEBUG
    Serial.println(str);                // print the character
#endif    
}

/**
 * arduino setup routine
 */
void setup() { 
    
    isSpeedSampleRequired = false; 
    timerCount            = 0; 
    vaneRotations         = 0;  // Set vaneRotations to 0 ready for calculations 
    windDirectionIdx      = 0;
    windSpeedIdx          = 0;

    for( int i=0; i < 10; i++ ) {
        windDirection[i] = 0;
        windSpeed[i]     = 0;
    }

#ifdef UNO_DEBUG
    Serial.begin(19200); 
#endif
    pinMode( WindSensorPin, INPUT ); 
    attachInterrupt( digitalPinToInterrupt( WindSensorPin ), isrRotation, FALLING ); 

#ifdef UNO_DEBUG
    Serial.println( "Davis Anemometer Test" ); 
    Serial.println( "Speed (Kn)\tDirection\tStrength\ti2c msg" ); 
#endif

    // Setup the timer interupt 
    // Timer interrupt every 2.5 seconds 
    //
    Timer1.initialize( 500000 );
    Timer1.attachInterrupt( isrTimer ); 

    Wire.begin( SLAVE_ADDR );               // join i2c bus with address specified
    Wire.onReceive( i2cDataRecieved );      // register event
    Wire.onRequest( i2cDataRequested );     // register event
} 

/**
 * arduino main loop routine
 */
void loop() { 

    getWindDirection(); 

    if( isSpeedSampleRequired ) { 

        // convert to mp/h using the formula V=P(2.25/T) 
        // V = P(2.25/2.5) = P * 0.9 
        //
        float rawSpeed = vaneRotations * 0.9; 
        float rawKnots = rawSpeed * 0.868976;
        int   theSpeed = (int)round( rawKnots );;

        if( theSpeed > 99 ) {
            theSpeed = 99;
        }
        
        windSpeed[windSpeedIdx++] = theSpeed;
        isSpeedSampleRequired     = false; 
        vaneRotations             = 0; // Reset count for next sample 

        if( windSpeedIdx == 10 ) {
            windSpeedIdx = 0;
        }

#ifdef UNO_DEBUG
        unsigned speed = getWindSpeedAverage();
        unsigned dir   = getWindDirectionAverage();
        
        Serial.print(  speed );     Serial.print("\t\t"); 
                                    Serial.print( dir ); 
        getHeading( dir );          Serial.print("\t\t"); 
        getWindStrength( speed);    Serial.print("\t\t"); 

        char i2cBuffer[8];
        prepareI2C( i2cBuffer );    Serial.println( (char *)i2cBuffer );
#endif        
    } 
} 

/**
 * isr handler for timer interrupt 
 */
void isrTimer() { 

    timerCount++; 

    if( timerCount == 6 )  { 
        isSpeedSampleRequired = true; 
        timerCount            = 0; 
    } 
} 

/**
 * This is the function that the interrupt calls to increment the rotation count 
 */
void isrRotation() { 

    if( (millis() - contactBounceTime) > 8 ) {  

        // debounce the switch contact. 
        //
        vaneRotations++; 
        contactBounceTime = millis(); 
    } 
} 

// Get Wind Direction 
//
void getWindDirection() { 

    unsigned int vaneValue = analogRead( WindVanePin ); 
    int      theDirection  = map(vaneValue, 0, 1023, 0, 360); 

    if( theDirection > 360 ) {
        theDirection = theDirection - 360; 
    }

    if( theDirection < 0 )  {
        theDirection = theDirection + 360; 
    }

    windDirection[windDirectionIdx++] = theDirection;
    if( windDirectionIdx == 10 ) {
        windDirectionIdx = 0;
    }
} 

#ifdef UNO_DEBUG

// Converts compass direction to heading 
//
void getHeading( int direction ) { 

    if(direction < 22) 
        Serial.print( " N" ); 

    else if (direction < 67) 
        Serial.print( " NE" ); 

    else if (direction < 112) 
        Serial.print( " E" ); 

    else if (direction < 157) 
        Serial.print( " SE" ); 

    else if (direction < 212) 
        Serial.print( " S" ); 

    else if (direction < 247) 
        Serial.print( " SW" ); 

    else if (direction < 292) 
        Serial.print( " W" ); 

    else if (direction < 337) 
        Serial.print( " NW" ); 

    else 
        Serial.print( " N" ); 
} 

// converts wind speed to wind strength 
//
void getWindStrength( float speed ) { 

    if(speed < 2) 
        Serial.print( "Calm" ); 

    else if(speed >= 2 && speed < 4) 
        Serial.print( "Light Air" ); 

    else if(speed >= 4 && speed < 8) 
        Serial.print( "Light Breeze" ); 

    else if(speed >= 8 && speed < 13) 
        Serial.print( "Gentle Breeze" ); 

    else if(speed >= 13 && speed < 18) 
        Serial.print( "Moderate Breeze" ); 

    else if(speed >= 18 && speed < 25) 
        Serial.print( "Fresh Breeze" ); 

    else if(speed >= 25 && speed < 31) 
        Serial.print( "Strong Breeze" ); 

    else if(speed >= 31 && speed < 39) 
        Serial.print( "Near Gale" ); 

    else 
        Serial.print("RUN"); 
}

#endif
