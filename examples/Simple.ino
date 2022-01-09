#include <Arduino.h>
#include <Esp8266PwmShiftRegister.hpp>

#define SHIFT_COUNT 1

// create a global shift register object
// parameters: data pin, clock pin, latch pin, number of shift registers, resolution
Esp8266PwmShiftRegister shiftRegister(13, 14, 15, SHIFT_COUNT, 255);

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(9600);

    // Start the register and its timer
    if (shiftRegister.Start())
    {
        Serial.print(F("Starting shiftRegister on millis() = "));
        Serial.println(millis());

        // Setting the first pin to half brightness
        shiftRegister.set(0, 125);

        // Setting the second pin to full brightness
        shiftRegister.set(1, 255);
    }
    else
    {
        Serial.print(F("Something went wrong!"));
    }
}

void loop()
{
    // put your main code here, to run repeatedly:

    // Run the timer and let it update the register and its chain
    shiftRegister.Run();
}