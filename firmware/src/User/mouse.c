#include "mouse.h"
#include "gpio.h"
#include <stdio.h>
#include <stdlib.h>

volatile int8_t mouseDirectionX = 0;		// X direction (0 = decrement, 1 = increment)
volatile int8_t mouseEncoderPhaseX = 0;		// X Quadrature phase (0-3)
volatile int8_t mouseDirectionY = 0;		// Y direction (0 = decrement, 1 = increment)
volatile int8_t mouseEncoderPhaseY = 0;		// Y Quadrature phase (0-3)
volatile int16_t mouseDistanceX = 0;		// Distance left for mouse to move
volatile int16_t mouseDistanceY = 0;		// Distance left for mouse to move
volatile uint8_t xTimerTop = 1;				// X axis timer TOP value
volatile uint8_t yTimerTop = 1;				// Y axis timer TOP value
FIFO_Utils_TypeDef ScrollBuffer;
uint8_t code = 0;
volatile uint8_t AmigaACK = 0;
volatile uint8_t previousMMB = 0;



void InitMouse()
{
    //Init circular buffer
    FifoInit(&ScrollBuffer);

}

uint8_t processMouseMovement(int8_t movementUnits, uint8_t axis, int limitRate,
		int dpiDivide) {




	uint16_t timerTopValue = 0;

	// Set the mouse movement direction and record the movement units
	if (movementUnits > 0) {
		// Moving in the positive direction

		// Apply DPI limiting if required
		if (dpiDivide) {
			movementUnits /= DPI_DIVIDER;
			if (movementUnits < 1)
				movementUnits = 1;
		}

		// Add the movement units to the quadrature output buffer
		if (axis == MOUSEX)
			mouseDistanceX += movementUnits;
		else
			mouseDistanceY += movementUnits;
	} else if (movementUnits < 0) {
		// Moving in the negative direction

		// Apply DPI limiting if required
		if (dpiDivide) {
			movementUnits /= DPI_DIVIDER;
			if (movementUnits > -1)
				movementUnits = -1;
		}

		// Add the movement units to the quadrature output buffer
		if (axis == MOUSEX)
			mouseDistanceX += -movementUnits;
		else
			mouseDistanceY += -movementUnits;
	} else {
		if (axis == MOUSEX)
			mouseDistanceX = 0;
		else
			mouseDistanceY = 0;
	}

	// Apply the quadrature output buffer limit
	if (axis == MOUSEX) {
		if (mouseDistanceX > Q_BUFFERLIMIT)
			mouseDistanceX = Q_BUFFERLIMIT;
	} else {
		if (mouseDistanceY > Q_BUFFERLIMIT)
			mouseDistanceY = Q_BUFFERLIMIT;
	}

	// Get the current value of the quadrature output buffer
	if (axis == MOUSEX)
		timerTopValue = mouseDistanceX;
	else
		timerTopValue = mouseDistanceY;

	// Range check the quadrature output buffer
	if (timerTopValue > 127)
		timerTopValue = 127;

	// Since the USB reports arrive at 100-125 Hz (even if there is only
	// a small amount of movement, we have to output the quadrature
	// at minimum rate to keep up with the reports (otherwise it creates
	// a slow lag).  If we assume 100 Hz of reports then the following
	// is true:
	//
	// 127 movements = 12,700 interrupts/sec
	// 100 movements = 10,000 interrupts/sec
	//  50 movements =  5,000 interrupts/sec
	//  10 movements =  1,000 interrupts/sec
	//   1 movement  =    100 interrupts/sec
	//
	// Timer speed is 15,625 ticks per second = 64 uS per tick
	//
	// Required timer TOP values (0 is fastest so all results are x-1):
	// 1,000,000 / 12,700 = 78.74 / 64 uS = 1.2 - 1
	// 1,000,000 / 10,000 = 100 / 64 uS = 1.56 - 1
	// 1,000,000 / 5,000 = 200 / 64 uS = 3.125 - 1
	// 1,000,000 / 1,000 = 1000 uS / 64 uS = 15.63 - 1
	// 1,000,000 / 100 = 10000 uS / 64 uS = 156.25 - 1
	//
	// So:
	//   timerTopValue = 10000 / timerTopValue; // i.e. 1,000,000 / (timerTopValue * 100)
	//   timerTopValue = timerTopValue / 64;
	//   timerTopValue = timerTopValue - 1;
	if (timerTopValue != 0) {
		timerTopValue = ((10000 / timerTopValue) / 64) - 1;
	} else {
		timerTopValue = 255;
	}
	// If the 'Slow' configuration jumper is shorted; apply the quadrature rate limit
	if (limitRate) {
		// Rate limit is on

		// Rate limit is provided in hertz
		// Each timer tick is 64 uS
		//
		// Convert hertz into period in uS
		// 1500 Hz = 1,000,000 / 1500 = 666.67 uS
		//
		// Convert period into timer ticks (* 4 due to quadrature)
		// 666.67 us / (64 * 4) = 2.6 ticks
		//
		// Timer TOP is 0-255, so subtract 1
		// 10.42 ticks - 1 = 9.42 ticks

		uint32_t rateLimit = ((1000000 / Q_RATELIMIT) / 256) - 1;

		// If the timerTopValue is less than the rate limit, we output
		// at the maximum allowed rate.  This will cause addition lag that
		// is handled by the quadrature output buffer limit above.
		if (timerTopValue < (uint16_t) rateLimit)
			timerTopValue = (uint16_t) rateLimit;
	}

	// Return the timer TOP value
	return (uint8_t) timerTopValue;
}

void ProcessMouse(HID_MOUSE_Data *mousemap) {

	if (mousemap == NULL) return;


}
