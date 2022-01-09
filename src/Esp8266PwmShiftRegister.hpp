#ifndef ESP_PWM_SHIFT_REGISTER
#define ESP_PWM_SHIFT_REGISTER

#if !defined(ESP8266)
#error This code is designed to run on ESP8266 and ESP8266-based boards! Please check your Tools->Board setting.
#endif

#include <Arduino.h>
#include <ESP8266_ISR_Timer.h>
#include <FastEsp8266ShiftRegister.hpp>

void IRAM_ATTR timerUpdate(void);

class Esp8266PwmShiftRegister
{
public:
    /**
    * @brief Construct a new Esp 8266 Pwm Shift Register object
    * 
    * @param dataPin The data pin number to use as output
    * @param clockPin The clock pin number to use as output
    * @param latchPin The latch pin number to use as output
    * @param shiftRegisterCount The count of shift registers to use
    * @param resolution The time resolution to use a finer data array
    */
    Esp8266PwmShiftRegister(const uint8_t dataPin, const uint8_t clockPin, const uint8_t latchPin, const uint8_t shiftRegisterCount = 1, const uint8_t resolution = 255)
        : Esp8266PwmShiftRegister(new FastEsp8266ShiftRegister(dataPin, clockPin, latchPin, shiftRegisterCount), resolution)
    {
    }

    /**
     * @brief Construct a new Esp 8266 Pwm Shift Register object
     * 
     * @param shiftRegister A fast Esp 8266 Shift Register object pointer
     * @param resolution The time resolution to use a finer data array
     */
    Esp8266PwmShiftRegister(FastEsp8266ShiftRegister *shiftRegister, const uint8_t resolution = 255)
    {
        this->_shiftRegister = shiftRegister;
        this->_resolution = resolution;
        this->_shiftRegisterCount = this->_shiftRegister->GetRegisterCount();

        // init data
        // internally a two-dimensional array: first dimension time, second dimension shift register bytes
        // data[t + sr * resolution]
        this->data = (uint8_t *)malloc(this->_resolution * this->_shiftRegisterCount * sizeof(uint8_t));
        for (int t = 0; t < this->_resolution; ++t)
        {
            for (int i = 0; i < this->_shiftRegisterCount; ++i)
            {
                this->data[t + i * this->_resolution] = 0;
            }
        }

        Esp8266PwmShiftRegister::singleton = this; // make this object accessible for timer interrupts

        // the boolean will be used to increase the performance in other functions
        this->singleShiftRegister = (this->_shiftRegisterCount == 1);

        this->ISR_Timer = new ISRTimer();
    }

    /**
    * Set a pin of the shift register to a given PWM value.
    * @param pin The index of the pin (starting at 0). If multiple shift registers are chained, the first pin of the second shift register would be addressed with pin = 8.
    * @param value The PWM value (between 0 and 255 as it will be scaled to the resolution that was passed to the constructor). 
    */
    void IRAM_ATTR set(uint8_t pin, uint8_t value)
    {
        value = (uint8_t)(value / 255.0 * this->_resolution + .5); // round
        uint8_t shiftRegister = pin / 8;
        for (int t = 0; t < this->_resolution; ++t)
        {
            // set (pin % 8)th bit to (value > t)
            this->data[t + shiftRegister * this->_resolution] ^= (-(value > t) ^ this->data[t + shiftRegister * this->_resolution]) & (1 << (pin % 8));
        }
    };

    /**
     * @brief Swaps the current data array to another array of the same size
     * 
     * @param newDataArray The pointer to a new data array, size needs to be resolution * shiftRegisterCount * sizeof(uint8_t) (see constructor arguments)
     * @return uint8_t* The pointer to the current data array (can be swaped back)
     */
    uint8_t IRAM_ATTR *SwapDataArray(uint8_t *newDataArray)
    {
        noInterrupts();
        uint8_t *oldDataArray = this->data;
        this->data = newDataArray;
        time = 0;
        interrupts();
        return oldDataArray;
    }

    /**
     * @brief updates the shift register chain to the next values in time, executed by the Timer
     */
    void IRAM_ATTR update()
    {
        // higher performance for single shift register mode
        if (singleShiftRegister)
        {
            this->_shiftRegister->shiftOut(this->data[time]);
        }
        else
        {
            for (int shiftRegisterIndex = this->_shiftRegisterCount - 1; shiftRegisterIndex >= 0; shiftRegisterIndex--)
            {
                this->_shiftRegister->shiftOut(this->data[time + shiftRegisterIndex * this->_resolution]);
            }
        }
        this->_shiftRegister->update();

        if (++time == this->_resolution)
        {
            time = 0;
        }
    }

    /**
     * @brief Starts the ISR_Timer of this PwmShiftRegister
     * 
     * @param updateIntervalMicrosecond The intervall time in microseconds to update the values of the shift register chain
     * @return true 
     * @return false 
     */
    bool IRAM_ATTR Start(const long updateIntervalMicrosecond = 10)
    {
        return this->ISR_Timer->setInterval(clockCyclesPerMicrosecond() * updateIntervalMicrosecond, timerUpdate) != -1;
    }

    /**
     * @brief Runs the Timer of this PwmShiftRegister
     */
    void IRAM_ATTR Run()
    {
        this->ISR_Timer->run();
    }

    static Esp8266PwmShiftRegister IRAM_ATTR *singleton; // used inside the ISR
    ISRTimer *ISR_Timer;

private:
    FastEsp8266ShiftRegister *_shiftRegister;
    uint8_t _resolution;
    uint8_t _shiftRegisterCount;
    uint8_t *data;             // data matrix [t + sr * resolution]
    volatile uint8_t time = 0; // time resolution of resolution steps

    bool singleShiftRegister; // true if (shiftRegisterCount == 1)
};

void IRAM_ATTR timerUpdate(void)
{
    Esp8266PwmShiftRegister::singleton->update();
}

// One static reference to the Esp8266PwmShiftRegister that was lastly created. Used for access through timer interrupts.
Esp8266PwmShiftRegister IRAM_ATTR *Esp8266PwmShiftRegister::singleton = NULL;

#endif
