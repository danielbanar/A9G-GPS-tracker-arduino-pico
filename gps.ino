#include <Arduino.h>

struct GPSData {
    String formattedDate;
    String formattedTime;
    String latitude;
    String longitude;
    String nsIndicator;
    String ewIndicator;
    String altitude;
    String speedKmh;
    String fixQuality;
    String numSatellites;
};

GPSData gpsData; // Global variable to store GPS data

// Function declarations
void runCommand(const char* command);
void parseGNGGA(const String& nmeaSentence);
void parseGNRMC(const String& nmeaSentence);
void parseGNVTG(const String& nmeaSentence);
String convertLatitude(const String& nmeaLatitude);
String convertLongitude(const String& nmeaLongitude);
String formatGPSData(const GPSData& data);

// Setup function
void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
    delay(5000);
    runCommand("AT");  // Check communication
    runCommand("AT+CSCS=\"GSM\"");  // Set character set to GSM
    runCommand("AT+CMGF=1");  // Set SMS mode to text
    runCommand("AT+CSMP=17,167,0,0");  // Set SMS text mode parameters
    runCommand("AT+GPS=1");    // Turn on GPS
    runCommand("AT+GPSRD=1");  // Request GPS data
}

// Run AT command
void runCommand(const char* command) {
    Serial.print("Sending command: ");
    Serial.println(command);

    Serial1.println(command);   // Send AT command
    delay(1000);                // Wait 1 second (1000 milliseconds)
    while (Serial1.available()) {
        Serial.print((char)Serial1.read());
    }
}

// Loop function
void loop() {
    while (Serial1.available()) {
        String sentence = Serial1.readStringUntil('\n');
        if (sentence.startsWith("+GPSRD:")) {
            sentence = sentence.substring(7);  // Remove the "+GPSRD:" prefix
        }
        
        // Parse the NMEA sentence
        if (sentence.startsWith("$GNGGA")) {
            parseGNGGA(sentence);
        } else if (sentence.startsWith("$GNRMC")) {
            parseGNRMC(sentence);
        } else if (sentence.startsWith("$GNVTG")) {
            parseGNVTG(sentence);
        } else if (sentence.startsWith("+CIEV: \"MESSAGE\"")) {
            String smsInfo = Serial1.readStringUntil('\n');
            int start = smsInfo.indexOf("+CMT: \"") + 7;
            int end = smsInfo.indexOf("\"", start);
            // Extract the phone number substring
            String phoneNumber = smsInfo.substring(start, end);
            String message = Serial1.readStringUntil('\n');
            Serial.print("SMS from ");
            Serial.print(phoneNumber);
            Serial.print(": ");
            Serial.println(message);
            Serial1.println("AT+CMGS=\"" + phoneNumber + "\"");
            delay(1000);
            Serial1.print(formatGPSData(gpsData));
            Serial1.write(0x1A);
        } else {
            Serial.println(sentence);
        }
    }
}

// Function to convert NMEA format latitude (ddmm.mmmm) to degrees decimal format
String convertLatitude(const String& nmeaLatitude) {
    double latitude = nmeaLatitude.substring(0, 2).toDouble() + nmeaLatitude.substring(2).toDouble() / 60.0;
    return String(latitude, 6); // Convert to string with 6 decimal places
}

// Function to convert NMEA format longitude (dddmm.mmmm) to degrees decimal format
String convertLongitude(const String& nmeaLongitude) {
    double longitude = nmeaLongitude.substring(0, 3).toDouble() + nmeaLongitude.substring(3).toDouble() / 60.0;
    return String(longitude, 6); // Convert to string with 6 decimal places
}

// Parse $GNGGA sentence
void parseGNGGA(const String& nmeaSentence) {
    int index = 0;
    String token;
    
    token = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += token.length() + 1; // Skip the sentence type ($GNGGA)
    
    String time = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += time.length() + 1; // Time
    
    String latitudeNMEA = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    gpsData.latitude = convertLatitude(latitudeNMEA);
    index += latitudeNMEA.length() + 1; // Latitude
    
    gpsData.nsIndicator = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += gpsData.nsIndicator.length() + 1; // N/S indicator
    
    String longitudeNMEA = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    gpsData.longitude = convertLongitude(longitudeNMEA);
    index += longitudeNMEA.length() + 1; // Longitude
    
    gpsData.ewIndicator = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += gpsData.ewIndicator.length() + 1; // E/W indicator
    
    gpsData.fixQuality = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += gpsData.fixQuality.length() + 1; // Fix quality
    
    gpsData.numSatellites = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += gpsData.numSatellites.length() + 1; // Number of satellites
    
    String hdop = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += hdop.length() + 1; // HDOP
    
    gpsData.altitude = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += gpsData.altitude.length() + 1; // Altitude
    
    String heightGeoid = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += heightGeoid.length() + 1; // Height of geoid
    
    // Parse and format the time (hhmmss.sss)
    int hours = time.substring(0, 2).toInt();
    int minutes = time.substring(2, 4).toInt();
    int seconds = time.substring(4, 6).toInt();
    
    // Adjust for timezone (2 hours ahead)
    hours += 2;
    if (hours >= 24) {
        hours -= 24;
    }
    
    // Format time components with leading zeros if necessary
    gpsData.formattedTime = (hours < 10 ? "0" : "") + String(hours) + ":" +
                            (minutes < 10 ? "0" : "") + String(minutes) + ":" +
                            (seconds < 10 ? "0" : "") + String(seconds);
    
    Serial.println("--- $GNGGA ---");
    Serial.print("Time: "); Serial.println(gpsData.formattedTime);
    Serial.print("Latitude: "); Serial.print(gpsData.latitude); Serial.print(" "); Serial.println(gpsData.nsIndicator);
    Serial.print("Longitude: "); Serial.print(gpsData.longitude); Serial.print(" "); Serial.println(gpsData.ewIndicator);
    Serial.print("Fix quality: "); Serial.println(gpsData.fixQuality);
    Serial.print("Number of satellites: "); Serial.println(gpsData.numSatellites);
    Serial.print("HDOP: "); Serial.println(hdop);
    Serial.print("Altitude: "); Serial.print(gpsData.altitude); Serial.println(" M");
    Serial.print("Height of geoid: "); Serial.print(heightGeoid); Serial.println(" M");
    Serial.print("Google Maps link: https://www.google.com/maps/search/"); Serial.print(gpsData.latitude); Serial.print(","); Serial.println(gpsData.longitude);
    Serial.println();
}

// Parse $GNRMC sentence
void parseGNRMC(const String& nmeaSentence) {
    int index = 0;
    String token;
    
    token = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += token.length() + 1; // Skip the sentence type ($GNRMC)
    
    String time = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += time.length() + 1; // Time
    
    String status = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += status.length() + 1; // Status
    
    String latitudeNMEA = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    gpsData.latitude = convertLatitude(latitudeNMEA);
    index += latitudeNMEA.length() + 1; // Latitude
    
    gpsData.nsIndicator = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += gpsData.nsIndicator.length() + 1; // N/S indicator
    
    String longitudeNMEA = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    gpsData.longitude = convertLongitude(longitudeNMEA);
    index += longitudeNMEA.length() + 1; // Longitude
    
    gpsData.ewIndicator = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += gpsData.ewIndicator.length() + 1; // E/W indicator
    
    String speed = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += speed.length() + 1; // Speed over ground in knots
    
    String course = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += course.length() + 1; // Course over ground
    
    String date = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += date.length() + 1; // Date (DDMMYY)
    
    // Parse and format the date (DDMMYY)
    int day = date.substring(0, 2).toInt();
    int month = date.substring(2, 4).toInt();
    int year = date.substring(4, 6).toInt() + 2000; // Assuming 21st century
    
    gpsData.formattedDate = String(day) + "." + (month) + "." + (year);
    
    // Parse and format the time (hhmmss.sss)
    int hours = time.substring(0, 2).toInt();
    int minutes = time.substring(2, 4).toInt();
    int seconds = time.substring(4, 6).toInt();
    
    // Adjust for timezone (2 hours ahead)
    hours += 2;
    if (hours >= 24) {
        hours -= 24;
    }
    
    // Format time components with leading zeros if necessary
    gpsData.formattedTime = (hours < 10 ? "0" : "") + String(hours) + ":" +
                            (minutes < 10 ? "0" : "") + String(minutes) + ":" +
                            (seconds < 10 ? "0" : "") + String(seconds);
    
    Serial.println("--- $GNRMC ---");
    Serial.print("Date: "); Serial.println(gpsData.formattedDate);
    Serial.print("Time: "); Serial.println(gpsData.formattedTime);
    Serial.print("Latitude: "); Serial.print(gpsData.latitude); Serial.print(" "); Serial.println(gpsData.nsIndicator);
    Serial.print("Longitude: "); Serial.print(gpsData.longitude); Serial.print(" "); Serial.println(gpsData.ewIndicator);
    Serial.print("Speed: "); Serial.print(speed); Serial.println(" knots");
    Serial.print("Course: "); Serial.println(course);
    Serial.print("Google Maps link: https://www.google.com/maps/search/"); Serial.print(gpsData.latitude); Serial.print(","); Serial.println(gpsData.longitude);
    Serial.println();
}

// Parse $GNVTG sentence
void parseGNVTG(const String& nmeaSentence) {
    int index = 0;
    String token;
    
    token = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += token.length() + 1; // Skip the sentence type ($GNVTG)
    
    String trackTrue = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += trackTrue.length() + 1; // Track angle in degrees True
    
    index += nmeaSentence.substring(index, nmeaSentence.indexOf(',', index)).length() + 1; // Skip next field
    index += nmeaSentence.substring(index, nmeaSentence.indexOf(',', index)).length() + 1; // Skip next field
    index += nmeaSentence.substring(index, nmeaSentence.indexOf(',', index)).length() + 1; // Skip next field
    
    String speedKnots = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += speedKnots.length() + 1; // Speed over ground in knots
    
    index += nmeaSentence.substring(index, nmeaSentence.indexOf(',', index)).length() + 1; // Skip next field
    
    gpsData.speedKmh = nmeaSentence.substring(index, nmeaSentence.indexOf(',', index));
    index += gpsData.speedKmh.length() + 1; // Speed over ground in km/h

    Serial.println("--- $GNVTG ---");
    Serial.print("Track angle (True): "); Serial.print(trackTrue); Serial.println(" degrees");
    Serial.print("Speed: "); Serial.print(speedKnots); Serial.println(" knots");
    Serial.print("Speed: "); Serial.print(gpsData.speedKmh); Serial.println(" km/h");
    Serial.println();
}

// Function to format GPS data into a string for SMS
String formatGPSData(const GPSData& data) {
    String message = data.formattedDate + " " + data.formattedTime + "\n";
    message += data.latitude + " " + data.nsIndicator + " " + data.longitude + " " + data.ewIndicator + "\n";
    message += data.altitude + " mnm\n";
    message += "Fix: " + data.fixQuality + "\n";
    message += "Satellites: " + data.numSatellites + "\n";
    message += "Speed: " + data.speedKmh + " km/h\n";
    message += "https://www.google.com/maps/search/" + data.latitude + "," + data.longitude;
    return message;
}
