#include <Adafruit_NeoPixel.h>

typedef unsigned long       millis_t;
typedef int8_t              direction_t;
typedef uint32_t            color_t;

class NeoPatterns; // forward decl

class NeoPattern
{
public:
    static const direction_t FORWARD = 1;
    static const direction_t REVERSE = -1;

    NeoPattern(NeoPatterns& pixels, millis_t interval, color_t color1, color_t color2, uint16_t totalSteps, direction_t dir = FORWARD) :
      Pixels(pixels),
      Interval(interval),
      Color1(color1),
      Color2(color2),
      TotalSteps(totalSteps),
      Direction(dir)
    {
        Reset();
    }

    void Reset()
    {
        Index = LastUpdate = 0;
    }

    bool ShouldUpdate() const
    {
        return millis() - LastUpdate > Interval;
    }

    void AfterUpdate()
    {
        LastUpdate = millis();
        Increment();
    }

    // Completion callback
    virtual void OnComplete()
    {
    }
    
    // Update the pattern
    virtual void Update() = 0;

    // Increment the Index and reset at the end
    void Increment()
    {
        Index += Direction;
        if (Index >= TotalSteps)
        {
            Index = 0;
            OnComplete();
        }
        else if (Index <= 0)
        {
            Index = TotalSteps - 1;
            OnComplete();
        }
    }
    
    // Reverse pattern direction
    void Reverse()
    {
        Direction *= -1;
        Index = (Direction == FORWARD) ? 0 : TotalSteps - 1;
    }

    NeoPatterns& Pixels;        // Which pixels to manipulate
    millis_t Interval;          // Milliseconds between updates
    millis_t LastUpdate;        // last update of position
    color_t Color1, Color2;     // What color(s) to use
    uint16_t TotalSteps;        // Total number of steps in the pattern
    uint16_t Index;             // Current step within the pattern
    direction_t Direction;      // Direction to run the pattern
};

// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
public:
    NeoPattern* ActivePattern;  // which pattern is running
    
    // Constructor - calls base-class constructor to initialize strip
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type = NEO_GRB + NEO_KHZ800) :
      Adafruit_NeoPixel(pixels, pin, type),
      ActivePattern(NULL)
    {
    }

    bool IsActive() const
    {
        return ActivePattern != NULL;
    }

    bool IsActive(const NeoPattern* pattern) const
    {
        return ActivePattern == pattern;
    }

    void Start(NeoPattern* pattern)
    {
        ActivePattern = pattern;
        ActivePattern->Reset();
    }

    void Stop()
    {
        ActivePattern = NULL;
        clear();
        show();
    }

    // Update the pattern
    void Update()
    {
        if (IsActive())
        {
            if (ActivePattern->ShouldUpdate())
            {
                ActivePattern->Update();
                show();
                ActivePattern->AfterUpdate();
            }
        }
    }
  
    // Calculate 50% dimmed version of a color (used by Scanner::Update)
    static color_t DimColor(color_t color)
    {
        // Shift R, G and B components one bit to the right
        return Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
    }

    // Set all pixels to a color (synchronously)
    void ColorSet(color_t color)
    {
        for (int i = 0; i < numPixels(); ++i)
        {
            setPixelColor(i, color);
        }
        show();
    }

    // Returns the Red component of a 32-bit color
    static uint8_t Red(color_t color)
    {
        return (color >> 16) & 0xFF;
    }

    // Returns the Green component of a 32-bit color
    static uint8_t Green(color_t color)
    {
        return (color >> 8) & 0xFF;
    }

    // Returns the Blue component of a 32-bit color
    static uint8_t Blue(color_t color)
    {
        return color & 0xFF;
    }
    
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    static color_t Wheel(byte WheelPos)
    {
        WheelPos = 255 - WheelPos;
        if (WheelPos < 85)
        {
            return Color(255 - WheelPos * 3, 0, WheelPos * 3);
        }
        else if (WheelPos < 170)
        {
            WheelPos -= 85;
            return Color(0, WheelPos * 3, 255 - WheelPos * 3);
        }
        else
        {
            WheelPos -= 170;
            return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
    }
};

class RainbowCycle : public NeoPattern
{
public:
    RainbowCycle(NeoPatterns& pixels, millis_t interval, direction_t dir = FORWARD) :
      NeoPattern(pixels, interval, 0, 0, 255, dir)
    {
    }
    
    virtual void Update()
    {
        for (int i = 0; i < Pixels.numPixels(); ++i)
        {
            Pixels.setPixelColor(i, Pixels.Wheel(((i * 256 / Pixels.numPixels()) + Index) & 255));
        }
    }
};

class TheaterChase : public NeoPattern
{
public:
    TheaterChase(NeoPatterns& pixels, millis_t interval, color_t color1, color_t color2, direction_t dir = FORWARD) :
      NeoPattern(pixels, interval, color1, color2, pixels.numPixels(), dir)
    {
    }
    
    virtual void Update()
    {
        for (int i = 0; i < Pixels.numPixels(); ++i)
        {
            Pixels.setPixelColor(i, ((i + Index) % 3 == 0) ? Color1 : Color2);
        }
    }
};

class ColorWipe : public NeoPattern
{
public:
    ColorWipe(NeoPatterns& pixels, millis_t interval, color_t color, direction_t dir = FORWARD) :
      NeoPattern(pixels, interval, color, 0, pixels.numPixels(), dir)
    {
    }

    virtual void Update()
    {
        Pixels.setPixelColor(Index, Color1);
    }
};

class Scanner : public NeoPattern
{
public:
    Scanner(NeoPatterns& pixels, millis_t interval, color_t color, bool split = false) :
      NeoPattern(pixels, interval, color, 0, pixels.numPixels() * (split ? 1 : 2))
    {
    }

    virtual void Update()
    { 
        for (int i = 0; i < Pixels.numPixels(); ++i)
        {
            Pixels.setPixelColor(i, (i == Index || i == TotalSteps - Index) ? Color1 : Pixels.DimColor(Pixels.getPixelColor(i)));
        }
    }
};

class Pulsar : public NeoPattern
{
public:
    Pulsar(NeoPatterns& pixels, millis_t interval, color_t color1, color_t color2) :
      NeoPattern(pixels, interval, color1, color2, 2)
    {
    }

    virtual void Update()
    { 
        Pixels.ColorSet(Index == 0 ? Color1 : Color2);
    }
};

class Fade : public NeoPattern
{
public:
    Fade(NeoPatterns& pixels, millis_t interval, color_t color1, color_t color2, uint16_t steps, direction_t dir = FORWARD) :
      NeoPattern(pixels, interval, color1, color2, steps, dir)
    {
    }
    
    virtual void Update()
    {
        // Calculate linear interpolation between Color1 and Color2
        // Optimise order of operations to minimize truncation error
        uint8_t red = ((Pixels.Red(Color1) * (TotalSteps - Index)) + (Pixels.Red(Color2) * Index)) / TotalSteps;
        uint8_t green = ((Pixels.Green(Color1) * (TotalSteps - Index)) + (Pixels.Green(Color2) * Index)) / TotalSteps;
        uint8_t blue = ((Pixels.Blue(Color1) * (TotalSteps - Index)) + (Pixels.Blue(Color2) * Index)) / TotalSteps;
        
        Pixels.ColorSet(Pixels.Color(red, green, blue));
    }
};
