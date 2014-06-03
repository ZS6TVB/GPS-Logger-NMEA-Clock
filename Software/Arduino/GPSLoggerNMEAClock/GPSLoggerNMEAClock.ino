//
#include <SPI.h> // Include the Arduino SPI library
#include <Wire.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>

// Define the pin used by the Sparkfun 7 Segment Shield.  We are making use
// of SPI in order to communiccate with the shield with the ss signal connected
// to pin #10 of the Arduino board.  The pins used are ad follows:
//   Arduino -------------- Sparkfun 7-Segment Shield
//      8   --------------------  SS
//     11   --------------------  SDI
//     13   --------------------  SCK
const int ssPin = 10;

// Set your Time Offset here.  A value of 2 for GMT+2 is used.  For GMT 
// time, Set the value to 0 or use negative values for negative offset.
int OFFSET = 2;

int fourdigitTime; // Variable used to calculate time and pad wit zero's
int HR; // Used to calculate hours
int MN;  // Used to calculate minutes
int nowDecimal = (0b110000); // Used to flash the seconds indicator dot

String hrString; // Used for an hour string value
String mnString; // Used for a minute string value
String fourDigitString; // To send 4 x digits to the display in one string

SoftwareSerial mySerial(8, 7); // Define the pins used by the Adafruit GPS Shield
Adafruit_GPS GPS(&mySerial); // Initialize the GPS on the above defined pins


// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you would like to send the the raw GPS sentences to the serial port.
#define GPSECHO true

// this keeps track of whether we're using the interrupt off by default!
boolean usingInterrupt = false;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

void setup()  
{
  SPI.begin();  // Begin SPI hardware
  SPI.setClockDivider(SPI_CLOCK_DIV64);  // Slow down SPI clock
  clearDisplaySPI();  // Clears display, resets cursor
  setDecimalsSPI(0b110000);  // Disable all decimals
  s7sSendStringSPI("helo");
  delay(2000);
  s7sSendStringSPI("pada");
  delay(1500);
  setBrightnessSPI(30);  // Set your display brightness.  Values of 0 - 127 accepted.
  //delay(10000);
  
  // connect at 4800 in order to stick with the NMEA standard baud rate.
  Serial.begin(4800);

  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  
  //
  // Set your NMEA requirement below if using the GPS as an NMEA GPS on the USB(Serial) port
  // 
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
  useInterrupt(true);
  delay(1000);
  // Ask for firmware version
  mySerial.println(PMTK_Q_RELEASE);
}


// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  if (GPSECHO)
    if (c) UDR0 = c;  
    // writing direct to UDR0 is much much faster than Serial.print 
    // but only one character can be written at a time. 
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

void clearDisplaySPI()
{
  digitalWrite(ssPin, LOW);
  SPI.transfer(0x76);  // Clear display command
  digitalWrite(ssPin, HIGH);
}

void setDecimalsSPI(byte decimals)
{
  digitalWrite(ssPin, LOW);
  SPI.transfer(0x77);
  SPI.transfer(decimals);
  digitalWrite(ssPin, HIGH);
}

void s7sSendStringSPI(String toSend)
{
  digitalWrite(ssPin, LOW);
  for (int i=0; i<4; i++)
  {
    SPI.transfer(toSend[i]);
  }
  digitalWrite(ssPin, HIGH);
}

void setBrightnessSPI(byte value)
{
  digitalWrite(ssPin, LOW);
  SPI.transfer(0x7A);  // Set brightness command byte
  SPI.transfer(value);  // brightness data byte
  digitalWrite(ssPin, HIGH);
}

uint32_t timer = millis();
void loop()                     // run over and over again
{
  // in case you are not using the interrupt above, you'll
  // need to 'hand query' the GPS, not suggested :(
  if (! usingInterrupt) {
    // read data from the GPS in the 'main loop'
    char c = GPS.read();
    // if you want to debug, this is a good time to do it!
    if (GPSECHO)
      if (c) UDR0 = c;
      // writing direct to UDR0 is much much faster than Serial.print 
      // but only one character can be written at a time. 
  }
  
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences! 
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false
  
    GPS.parse(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false

    HR = (GPS.hour + OFFSET);
    MN = (GPS.minute);
    if ( HR >= 24 ) {
    HR -= 24;
    }
    if ( HR < 0 ) {
    HR += 24;
    }
    if (HR < 10) {
      String tmpString = String(HR);
      hrString = "0" +tmpString;
    } else {
      String tmpString = String(HR);
      hrString = tmpString;
      //Serial.println(hrString);
    }
    if ( MN < 10) {
      String tmpString = String(MN);
      mnString = "0" + tmpString;
    } else {      
    String tmpString = String(MN);
    mnString = tmpString;
    }
    fourDigitString = hrString + mnString;

    if (nowDecimal == 0) {
      nowDecimal = 1;
      setDecimalsSPI(0b110010);
    }
    else {
      nowDecimal = 0;
      setDecimalsSPI(0b110000);
    }
    s7sSendStringSPI(fourDigitString);
    delay(1000);
  }

  // if millis() or timer wraps around, we'll just reset it
  if (timer > millis())  timer = millis();

  // Print Stats Manually
  if (millis() - timer > 1000) { 
    timer = millis(); // reset the timer
    //Serial.print("\nTime: ");
    //Serial.print(GPS.hour, DEC); Serial.print(':');
    //Serial.print(GPS.minute, DEC); Serial.print(':');
    //Serial.print(GPS.seconds, DEC); Serial.print('.');
    //Serial.println(GPS.milliseconds);
  }
}
