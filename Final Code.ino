#include <Adafruit_NeoPixel.h>

#define sensorPin A3
#define LED_PIN 6
#define NUM_LEDS 500
#define NUM_TRAINS 6
#define WINDOW_SIZE 20
#define MAX_BRIGHTNESS 255    // Maximum brightness for LEDs
#define MIN_BRIGHTNESS 2      // Minimum brightness when LEDs start
#define START_POINTS 10       // Number of random starting points
#define BRIGHTNESS_INCREASE 3 // Amount to increase brightness each step

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

float distanceCM = 0.0;
unsigned long t1, t2 = 0;
bool shouldPlay = false;
short int animationToPlay = 0;
bool donePlaying = false;
int fadeValue = 255;
int currentPosition = 0;
int animationPosition = 0;
int universalCounter = 0;
bool lit[NUM_LEDS] = {false}; // Tracks which LEDs are currently lit
uint8_t brightness[NUM_LEDS];

void setup()
{
    Serial.begin(9600);
    strip.begin();
    strip.setBrightness(255);  // Ensure full brightness to start
    strip.show();              // Turn off all LEDs initially
    randomSeed(analogRead(0)); // Initialize random number generator
    t1 = millis();
    fadeValue = 0;
}

void read_sensor()
{
    int sensorValue = analogRead(sensorPin);
    float voltage = sensorValue * (3.3 / 1023.0);
    distanceCM = voltage * 120;
}

void print_data()
{
    Serial.print("Distance: ");
    Serial.print(distanceCM);
    Serial.println(" cm");
}

void donePlayingAnimation()
{
    donePlaying = true;
    strip.clear();
    strip.show();
}

void trains()
{
    strip.clear(); // More efficient than setting each LED to 0

    int segmentLength = NUM_LEDS / NUM_TRAINS;
    int brightRed = 255;   // Full brightness for red
    int brightGreen = 165; // Reduced green for orange color
    int brightBlue = 0;    // No blue component

    for (int train = 0; train < NUM_TRAINS; train++)
    {
        int trainStartPosition = (currentPosition + train * segmentLength) % NUM_LEDS;
        for (int i = trainStartPosition; i < trainStartPosition + WINDOW_SIZE; i++)
        {
            int ledPosition = i % NUM_LEDS;                                                    // Ensure we wrap around the strip correctly
            strip.setPixelColor(ledPosition, strip.Color(brightRed, brightGreen, brightBlue)); // Set to orange
        }
    }

    strip.show();

    // Move to the next position
    currentPosition = (currentPosition + 1) % NUM_LEDS;

    if (!shouldPlay && fadeValue > 0)
    {
        fadeValue -= 10; // This line might not be necessary for the `trains` function depending on your design
    }

    universalCounter += 1;

    if (universalCounter == 240)
    {
        universalCounter = 0;
        donePlayingAnimation();
    }
}

void split()
{
    int center = NUM_LEDS / 2; // Find the center of the strip
    if (universalCounter == 0)
    {
        strip.clear();
    }
    universalCounter += 1;

    if (animationPosition < center)
    {
        strip.setPixelColor(center + animationPosition, strip.Color(220, 160, 0)); // Set to yellow
        strip.setPixelColor(center - animationPosition, strip.Color(220, 160, 0)); // Set to yellow
        animationPosition++;
    }
    else
    {
        animationPosition = 0; // Ensure it doesn't go beyond the center
    }
    strip.show();

    if (universalCounter == 245)
    {
        universalCounter = 0;
        animationPosition = 0;
        donePlayingAnimation();
    }
}

void rnd()
{
    static bool wasInRange = false;
    static bool initialized = false; // Ensure we initialize the random starts only once when in range

    if (!wasInRange)
    {
        wasInRange = true;
        if (!initialized)
        {
            initializeRandomStarts();
            initialized = true;
        }
    }
    else
    {
        spreadLight(); // Continue to spread light from initial points
    }

    universalCounter += 1;
    if (universalCounter == 240)
    { // If it was in range before, turn off LEDs
        for (int i = 0; i < NUM_LEDS; i++)
        {
            strip.setPixelColor(i, 0); // Turn off LED
            lit[i] = false;            // Reset the lit array
            brightness[i] = 0;         // Reset brightness
        }
        strip.show();
        wasInRange = false;
        initialized = false; // Reset initialization flag
        universalCounter = 0;
        donePlayingAnimation();
    }
}

void initializeRandomStarts()
{
    for (int i = 0; i < START_POINTS; ++i)
    {
        int pos = random(NUM_LEDS);
        lit[pos] = true;
        brightness[pos] = MIN_BRIGHTNESS; // Start with MIN_BRIGHTNESS
        // Adjust initial color with MIN_BRIGHTNESS
        strip.setPixelColor(pos, strip.Color(brightness[pos], brightness[pos] * 160 / 255, 0));
    }
    strip.show();
}

void spreadLight()
{

    bool newLit[NUM_LEDS];
    memcpy(newLit, lit, sizeof(lit)); // Copy current state to a new buffer

    for (int i = 0; i < NUM_LEDS; ++i)
    {
        if (lit[i] && brightness[i] < MAX_BRIGHTNESS)
        {
            // Increase brightness if not already at max
            brightness[i] += BRIGHTNESS_INCREASE;
            if (brightness[i] > MAX_BRIGHTNESS)
                brightness[i] = MAX_BRIGHTNESS;
            // Update color with new brightness
            strip.setPixelColor(i, strip.Color(brightness[i], brightness[i] * 160 / 255, 0));
        }

        // Spread light to adjacent LEDs
        if (lit[i])
        {
            if (i > 0 && !lit[i - 1])
            {
                newLit[i - 1] = true;
                brightness[i - 1] = MIN_BRIGHTNESS; // Start with MIN_BRIGHTNESS for new LEDs
            }
            if (i < NUM_LEDS - 1 && !lit[i + 1])
            {
                newLit[i + 1] = true;
                brightness[i + 1] = MIN_BRIGHTNESS; // Start with MIN_BRIGHTNESS for new LEDs
            }
        }
    }

    memcpy(lit, newLit, sizeof(lit)); // Update the main array with new changes
    strip.show();
}

void smoothTransition()
{
    static int increment = 2;      // Reduced increment for slower transition
    static bool increasing = true; // Keep track of fade direction
    static int cycleCount = 0;     // Counts the number of full cycles
    static int cycleLimit = 3;     // Limit to 3 full cycles

    // Define the orange color components
    int orangeRed = 255;
    int orangeGreen = 165;
    int orangeBlue = 0;

    for (int i = 0; i < NUM_LEDS; i++)
    {
        // Calculate the current color based on fadeValue
        int currentRed = (orangeRed * fadeValue) / 255;
        int currentGreen = (orangeGreen * fadeValue) / 255;
        int currentBlue = (orangeBlue * fadeValue) / 255;

        strip.setPixelColor(i, strip.Color(currentRed, currentGreen, currentBlue));
    }
    strip.show();

    // Adjust fadeValue to create a fading effect
    if (increasing)
    {
        fadeValue += increment;
        if (fadeValue >= 255)
        {
            increasing = false; // Change direction
        }
    }
    else
    {
        fadeValue -= increment;
        if (fadeValue <= 0)
        {
            increasing = true; // Change direction
            cycleCount++;      // Increment cycle count after a complete fade in and out
        }
    }

    // Reset after 3 cycles to prevent flickering
    if (cycleCount >= cycleLimit)
    {
        // Reset all adjustments to prevent flicker effect and stop the cycle
        cycleCount = 0;    // Reset cycle count for future use
        fadeValue = 255;   // Reset fade value
        increasing = true; // Reset direction
        for (int i = 0; i < NUM_LEDS; i++)
        {
            strip.setPixelColor(i, 0); // Turn off LEDs to stop
        }
        strip.show();
        // Optionally, include a delay or another mechanism here to pause before restarting the cycle
    }
}

void loop()
{
    t2 = millis();
    if (t2 - t1 >= 100)
    { // Check every half second
        read_sensor();
        print_data();
        t1 = millis();

        if (distanceCM <= 75 && distanceCM >= 0)
        {
            shouldPlay = true;
            if (donePlaying)
            {
                // If an animation has just finished, reset donePlaying and select a new animation
                donePlaying = false;
                animationToPlay = random(0, 3); // Select a new animation
                Serial.print("Playing animation: ");
                Serial.println(animationToPlay);
            }
        }
        else
        {
            // No object is in range; we should be in the smoothTransition state
            shouldPlay = false;
        }
    }

    // Execute the smoothTransition or animations based on the shouldPlay flag
    if (!shouldPlay)
    {
        smoothTransition();
        donePlaying = false; // Reset donePlaying to ensure animations can restart when the condition changes
    }
    else if (!donePlaying)
    {
        // An object is in range, and we have an animation to play
        switch (animationToPlay)
        {
        case 0:
            trains();
            break;
        case 1:
            split();
            break;
        default:
            rnd();
            break;
        }
        // It's important that each animation function sets donePlaying to true when it completes.
        // This could be at the end of its cycle or based on some internal condition.
    }
}
