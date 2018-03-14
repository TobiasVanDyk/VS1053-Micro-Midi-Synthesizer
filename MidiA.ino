/****************************************************************************************** 
 * This is an example for the Adafruit VS1053 Codec Breakout
 * 
 * Designed specifically to work with the Adafruit VS1053 Codec Breakout 
 * ----> https://www.adafruit.com/products/1381
 * 
 * Adafruit invests time and resources providing this open source code, 
 * please support Adafruit and open-source hardware by purchasing 
 * products from Adafruit!
 * 
 * Written by Limor Fried/Ladyada for Adafruit Industries.  
 * BSD license, all text above must be included in any redistribution
 * Code adapted By Tobias van Dyk for use with an Adafruit VS1053 module and an Arduino Uno
 ********************************************************************************************/
 // VS1103b MIDI Controls
 // The implemented midi messages here. Some like tempo are used in file mode only. 
 // meta: 0x51 : set tempo
 // other meta: MidiMeta() called (user code can send out lyrics and other meta information)
 // SysEx device control: 0x01 : master volume (only in file mode!)
 // channel message: 0x80 note off, 0x90 note on, 0xc0 program, 0xe0 pitch wheel
 // channel message 0xb0: parameter ? 0x00: bank select (0 is default, 0x78 and 0x7f is drums, 0x79 melodic)
 //         0x06: RPN MSB: 0 = bend range, 2 = coarse tune
 //         0x07: channel volume
 //         0x0a: pan control
 //         0x0b: expression (changes volume)
 //         0x0c: effect control 1 (sets global reverb decay)
 //         0x26: RPN LSB: 0 = bend range
 //         0x40: hold1
 //         0x42: sustenuto
 //         0x5b effects level (channel reverb level)
 //         0x62,0x63,0x64,0x65: NRPN and RPN selects
 //         0x78: all sound off
 //         0x79: reset all controllers
 //         0x7b, 0x7c, 0x7d: all notes off
 // 
 // The channel-specific level and overall 'duration' of reverb can be adjusted, "expression" adjusts the volume of audible notes on that 
 // channel, and pitch-wheel adjusts the note frequencies.
 //
 // All 16 channels and multiple notes per channel are available. The total simultaneous audible notes depends on the clock frequency and 
 // what instruments are used (different instruments take varying amounts of processing power).
 //
 // Reverb: There are 15 different default Reverb settings in the low 4 bits of the parametric_x.config1 register (X:0x1e03). Values 2 to 15
 // select a default room size (internal 350 to 4900). 0 will enable the reverb depending on the value of CLOCKF (3.0x or higher clock enables
 // reverb). 1 means reverb is disabled. When you control Reverb through MIDI controls (Parameter / effect control 1), all control values from
 // 0 to 127 are valid (internal 0..4064). In vs1103b Reverb is mutually exclusive with EarSpeaker.
 //

byte Instrument = 0x00;
byte SavedInstrument = 0x00;
byte Instrument0 = 0x00;        // Channel 0
byte SavedInstrument0 = 0x00;
byte Instrument1 = 0x00;        // Channel 1 Channel 10 is percussion
byte SavedInstrument1 = 0x00;
byte Bank = 0x00; // Valid Banks for VS1053 0x00 0x78 0x79
byte Reverb = 0; // off
byte EPrm = 0;   // Eeprom saved

int FunctionNum = 1;                                // 1 is Change Instrument Program P  Can take value 1 to 8
const char* const Function PROGMEM = " PBVCRSXbt "; // Function Indicator P = Change Intrumnet (Midi Program Change) 
//booleans to activate when btnpressed
boolean AnyBtnPress = false;
byte Volume = 1;
byte Channel = 0; // VS1053 receives on all Midi Channels 1 to 16

// include SPI, SD, Wire Adafruit_VS1053, GFX and SSD libraries
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>
#include <avr/pgmspace.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>

// Pin Numbers
#define btnPrev 5  // D5 Down Button
#define btnNext 6  // D6 Up Button
#define btnFunc 7  // D7 Select Function
#define MidiLED 2  // D2 Show Midi Activity
// define the other pins used
#define VS1053_RESET 9         // This is the pin that connects to the RESET pin on VS1053
//#define CLK 13               // SPI Clock, shared with SD card
//#define MISO 12              // Input data, from VS1053/SD card
//#define MOSI 11              // Output data, to VS1053/SD card
// Connect CLK, MISO and MOSI to hardware SPI pins. 
// These are the pins used for the breakout example
#define BREAKOUT_RESET  9      // VS1053 reset pin (output) rst 
#define BREAKOUT_CS     10     // VS1053 chip select pin (output) cs 
#define BREAKOUT_DCS    8      // VS1053 Data/command select pin (output) xdcs 
// These are the pins used for the music maker shield
#define SHIELD_CS     7        // VS1053 chip select pin (output)
#define SHIELD_DCS    6        // VS1053 Data/command select pin (output)
// These are common pins between breakout and shield
#define CARDCS 4               // Card chip select pin sdcs 
// DREQ should be an Int pin i.e. Arduino Uno Pin 2 or 3
#define DREQ 3                 // VS1053 Data request, ideally an Interrupt pin 
#define RESET 9                // VS1053 reset pin (RST) (output)
#define CS 10                  // VS1053 chip select pin (CS) (output)
#define DCS 8                  // VS1053 Data/command select pin (XDCS) (output)

#define OLED_RESET 0     // GPIO0
Adafruit_SSD1306 display(OLED_RESET);
#if (SSD1306_LCDHEIGHT != 48)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// General Midi Instruments
#define InstrNumber 128
// Piano Family:
const char Instr000[] PROGMEM = "Acoustic Grand Piano";
const char Instr001[] PROGMEM = "Bright Acoustic Piano";
const char Instr002[] PROGMEM = "Electric Grand Piano";
const char Instr003[] PROGMEM = "Honky-tonk Piano";
const char Instr004[] PROGMEM = "Electric Piano 1";
const char Instr005[] PROGMEM = "Electric Piano 2";
const char Instr006[] PROGMEM = "Harpsichord";
const char Instr007[] PROGMEM = "Clavinet";
// Chromatic Percussion Family:
const char Instr008[] PROGMEM = "Celesta";
const char Instr009[] PROGMEM = "Glockenspiel";
const char Instr010[] PROGMEM = "Music Box";
const char Instr011[] PROGMEM = "Vibraphone";
const char Instr012[] PROGMEM = "Marimba";
const char Instr013[] PROGMEM = "Xylophone";
const char Instr014[] PROGMEM = "Tubular Bells";
const char Instr015[] PROGMEM = "Dulcimer";
// Organ Family:
const char Instr016[] PROGMEM = "Drawbar Organ";
const char Instr017[] PROGMEM = "Percussive Organ";
const char Instr018[] PROGMEM = "Rock Organ";
const char Instr019[] PROGMEM = "Church Organ";
const char Instr020[] PROGMEM = "Reed Organ";
const char Instr021[] PROGMEM = "Accordion";
const char Instr022[] PROGMEM = "Harmonica";
const char Instr023[] PROGMEM = "Tango Accordion";
// Guitar Family:
const char Instr024[] PROGMEM = "Acoustic Guitar (nylon)";
const char Instr025[] PROGMEM = "Acoustic Guitar (steel)";
const char Instr026[] PROGMEM = "Electric Guitar (jazz)";
const char Instr027[] PROGMEM = "Electric Guitar (clean)";
const char Instr028[] PROGMEM = "Electric Guitar (muted)";
const char Instr029[] PROGMEM = "Overdriven Guitar";
const char Instr030[] PROGMEM = "Distortion Guitar";
const char Instr031[] PROGMEM = "Guitar harmonics";
// Bass Family:
const char Instr032[] PROGMEM = "Acoustic Bass";
const char Instr033[] PROGMEM = "Electric Bass (finger)";
const char Instr034[] PROGMEM = "Electric Bass (pick)";
const char Instr035[] PROGMEM = "Fretless Bass";
const char Instr036[] PROGMEM = "Slap Bass 1";
const char Instr037[] PROGMEM = "Slap Bass 2";
const char Instr038[] PROGMEM = "Synth Bass 1";
const char Instr039[] PROGMEM = "Synth Bass 2";
// Strings Family:
const char Instr040[] PROGMEM = "Violin";
const char Instr041[] PROGMEM = "Viola";
const char Instr042[] PROGMEM = "Cello";
const char Instr043[] PROGMEM = "Contrabass";
const char Instr044[] PROGMEM = "Tremolo Strings";
const char Instr045[] PROGMEM = "Pizzicato Strings";
const char Instr046[] PROGMEM = "Orchestral Harp";
const char Instr047[] PROGMEM = "Timpani";
// Ensemble Family:
const char Instr048[] PROGMEM = "String Ensemble 1";
const char Instr049[] PROGMEM = "String Ensemble 2";
const char Instr050[] PROGMEM = "Synth Strings 1";
const char Instr051[] PROGMEM = "Synth Strings 2";
const char Instr052[] PROGMEM = "Choir Aahs";
const char Instr053[] PROGMEM = "Voice Oohs";
const char Instr054[] PROGMEM = "Synth Voice";
const char Instr055[] PROGMEM = "Orchestra Hit";
// Brass Family:
const char Instr056[] PROGMEM = "Trumpet";
const char Instr057[] PROGMEM = "Trombone";
const char Instr058[] PROGMEM = "Tuba";
const char Instr059[] PROGMEM = "Muted Trumpet";
const char Instr060[] PROGMEM = "French Horn";
const char Instr061[] PROGMEM = "Brass Section";
const char Instr062[] PROGMEM = "Synth Brass 1";
const char Instr063[] PROGMEM = "Synth Brass 2";
// Reed Family:
const char Instr064[] PROGMEM = "Soprano Sax";
const char Instr065[] PROGMEM = "Alto Sax";
const char Instr066[] PROGMEM = "Tenor Sax";
const char Instr067[] PROGMEM = "Baritone Sax";
const char Instr068[] PROGMEM = "Oboe";
const char Instr069[] PROGMEM = "English Horn";
const char Instr070[] PROGMEM = "Bassoon";
const char Instr071[] PROGMEM = "Clarinet";
// Pipe Family:
const char Instr072[] PROGMEM = "Piccolo";
const char Instr073[] PROGMEM = "Flute";
const char Instr074[] PROGMEM = "Recorder";
const char Instr075[] PROGMEM = "Pan Flute";
const char Instr076[] PROGMEM = "Blown Bottle";
const char Instr077[] PROGMEM = "Shakuhachi";
const char Instr078[] PROGMEM = "Whistle";
const char Instr079[] PROGMEM = "Ocarina";
// Synth Lead Family:
const char Instr080[] PROGMEM = "Lead 1 (square)";
const char Instr081[] PROGMEM = "Lead 2 (sawtooth)";
const char Instr082[] PROGMEM = "Lead 3 (calliope)";
const char Instr083[] PROGMEM = "Lead 4 (chiff)";
const char Instr084[] PROGMEM = "Lead 5 (charang)";
const char Instr085[] PROGMEM = "Lead 6 (voice)";
const char Instr086[] PROGMEM = "Lead 7 (fifths)";
const char Instr087[] PROGMEM = "Lead 8 (bass)";
// Synth Pad Family:
const char Instr088[] PROGMEM = "Pad 1 (new age)";
const char Instr089[] PROGMEM = "Pad 2 (warm)";
const char Instr090[] PROGMEM = "Pad 3 (polysynth)";
const char Instr091[] PROGMEM = "Pad 4 (choir)";
const char Instr092[] PROGMEM = "Pad 5 (bowed)";
const char Instr093[] PROGMEM = "Pad 6 (metallic)";
const char Instr094[] PROGMEM = "Pad 7 (halo)";
const char Instr095[] PROGMEM = "Pad 8 (sweep)";
// Synth Effects Family:
const char Instr096[] PROGMEM = "FX 1 (rain)";
const char Instr097[] PROGMEM = "FX 2 (soundtrack)";
const char Instr098[] PROGMEM = "FX 3 (crystal)";
const char Instr099[] PROGMEM = "FX 4 (atmosphere)";
const char Instr100[] PROGMEM = "FX 5 (brightness)";
const char Instr101[] PROGMEM = "FX 6 (goblins)";
const char Instr102[] PROGMEM = "FX 7 (echoes)";
const char Instr103[] PROGMEM = "FX 8 (scifi)";
// Ethnic Family:
const char Instr104[] PROGMEM = "Sitar";
const char Instr105[] PROGMEM = "Banjo";
const char Instr106[] PROGMEM = "Shamisen";
const char Instr107[] PROGMEM = "Koto";
const char Instr108[] PROGMEM = "Kalimba";
const char Instr109[] PROGMEM = "Bag pipe";
const char Instr110[] PROGMEM = "Fiddle";
const char Instr111[] PROGMEM = "Shanai";
// Percussive Family:
const char Instr112[] PROGMEM = "Tinkle Bell";
const char Instr113[] PROGMEM = "Agogo";
const char Instr114[] PROGMEM = "Steel Drums";
const char Instr115[] PROGMEM = "Woodblock";
const char Instr116[] PROGMEM = "Taiko Drum";
const char Instr117[] PROGMEM = "Melodic Tom";
const char Instr118[] PROGMEM = "Synth Drum";
const char Instr119[] PROGMEM = "Reverse Cymbal";
// Sound Effects Family:
const char Instr120[] PROGMEM = "Guitar Fret Noise";
const char Instr121[] PROGMEM = "Breath Noise";
const char Instr122[] PROGMEM = "Seashore";
const char Instr123[] PROGMEM = "Bird Tweet";
const char Instr124[] PROGMEM = "Telephone Ring";
const char Instr125[] PROGMEM = "Helicopter";
const char Instr126[] PROGMEM = "Applause";
const char Instr127[] PROGMEM = "Gunshot";

const char Instr128[] PROGMEM = "PercussionInstrument";

const char* const InstrBank1[] PROGMEM = 
{ Instr000, Instr001, Instr002, Instr003, Instr004, Instr005, Instr006, Instr007, Instr008, Instr009, Instr010, Instr011, Instr012, Instr013, Instr014, Instr015, 
  Instr016, Instr017, Instr018, Instr019, Instr020, Instr021, Instr022, Instr023, Instr024, Instr025, Instr026, Instr027, Instr028, Instr029, Instr030, Instr031, 
  Instr032, Instr033, Instr034, Instr035, Instr036, Instr037, Instr038, Instr039, Instr040, Instr041, Instr042, Instr043, Instr044, Instr045, Instr046, Instr047, 
  Instr048, Instr049, Instr050, Instr051, Instr052, Instr053, Instr054, Instr055, Instr056, Instr057, Instr058, Instr059, Instr060, Instr061, Instr062, Instr063, 
  Instr064, Instr065, Instr066, Instr067, Instr068, Instr069, Instr070, Instr071, Instr072, Instr073, Instr074, Instr075, Instr076, Instr077, Instr078, Instr079, 
  Instr080, Instr081, Instr082, Instr083, Instr084, Instr085, Instr086, Instr087, Instr088, Instr089, Instr090, Instr091, Instr092, Instr093, Instr094, Instr095, 
  Instr096, Instr097, Instr098, Instr099, Instr100, Instr101, Instr102, Instr103, Instr104, Instr105, Instr106, Instr107, Instr108, Instr109, Instr110, Instr111, 
  Instr112, Instr113, Instr114, Instr115, Instr116, Instr117, Instr118, Instr119, Instr120, Instr121, Instr122, Instr123, Instr124, Instr125, Instr126, Instr127, Instr128
};

const static word plugin[1039] PROGMEM = { /* Compressed plugin */
	0x0007,0x0001, /*copy 1*/
	0x8050,
	0x0006,0x03f0, /*copy 1008*/
	0x2800,0x8080,0x0006,0x2016,0xf400,0x4095,0x0006,0x0017,
	0x3009,0x1c40,0x3009,0x1fc2,0x6020,0x0024,0x0000,0x1fc2,
	0x2000,0x0000,0xb020,0x4542,0x3613,0x0024,0x0006,0x0057,
	0x3e15,0x1c15,0x0020,0x1fd4,0x3580,0x3802,0xf204,0x3804,
	0x0fff,0xfe44,0xa244,0x1804,0xf400,0x4094,0x2800,0x1985,
	0x3009,0x1bc2,0xf400,0x4500,0x2000,0x0000,0x36f5,0x3c15,
	0x3009,0x3857,0x2800,0x1b40,0x0030,0x0457,0x3009,0x3857,
	0x0030,0x0a57,0x3e14,0xf806,0x3701,0x8024,0x0006,0x0017,
	0x3e04,0x9c13,0x0020,0x1fd2,0x3b81,0x8024,0x36f4,0xbc13,
	0x36f4,0xd806,0x0030,0x0717,0x2100,0x0000,0x3f05,0xdbd7,
	0x0030,0xf80f,0x0000,0x1f0e,0x2800,0x7680,0x0000,0x004d,
	0xf400,0x4595,0x3e00,0x17cc,0x3505,0xf802,0x3773,0x0024,
	0x3763,0x0024,0x3700,0x0024,0x0000,0x09c2,0x6024,0x0024,
	0x3600,0x1802,0x2830,0xf855,0x0000,0x004d,0x2800,0x2240,
	0x36f3,0x0024,0x3613,0x0024,0x3e12,0xb817,0x3e12,0x3815,
	0x3e05,0xb814,0x3625,0x0024,0x0000,0x800a,0x3e10,0x3801,
	0x3e10,0xb803,0x3e11,0x3810,0x3e04,0x7812,0x34c3,0x0024,
	0x3440,0x0024,0x4080,0x0024,0x001b,0x3301,0x2800,0x2c85,
	0x0000,0x0180,0x0000,0x0551,0x0000,0xaf02,0x293c,0x1f40,
	0x0007,0xffc1,0xb010,0x134c,0x0018,0x0001,0x4010,0x10d0,
	0x0007,0xffc1,0xfe20,0x020c,0x0000,0x0591,0x48b6,0x0024,
	0x4dd6,0x0024,0x0001,0x2202,0x293c,0x1f40,0x4380,0x2003,
	0xb010,0x134c,0x0018,0x0001,0x4010,0x1010,0xfe20,0x020c,
	0x48b6,0x844c,0x4dd6,0x0024,0xb880,0x2003,0x3434,0x0024,
	0x2800,0x5280,0x3083,0x0024,0x001c,0xccc2,0x0000,0x05d1,
	0x34d3,0x0024,0x3404,0x0024,0x3404,0x420c,0x3001,0x05cc,
	0xa408,0x044c,0x3100,0x0024,0x6010,0x0024,0xfe20,0x0024,
	0x48b6,0x0024,0x4dd6,0x0024,0x4310,0x0024,0x4488,0x2400,
	0x0000,0x0551,0x2800,0x3295,0x3404,0x0024,0xf290,0x00cc,
	0x3800,0x0024,0x3434,0x0024,0x3073,0x0024,0x3013,0x0024,
	0x2800,0x4340,0x3800,0x0024,0x3083,0x0024,0x3000,0x0024,
	0x6402,0x0024,0x0000,0x1001,0x2800,0x3618,0x0018,0x0002,
	0x3434,0x4024,0x3133,0x0024,0x3100,0x0024,0xfe20,0x0024,
	0x48b6,0x0024,0x4dd6,0x0024,0x2800,0x4340,0x3900,0xc024,
	0x4010,0x1011,0x6402,0x0024,0x0000,0x0590,0x2800,0x3918,
	0x0000,0x0024,0xf290,0x04cc,0x3900,0x0024,0x3434,0x0024,
	0x3073,0x0024,0x3013,0x0024,0x2800,0x4340,0x3800,0x0024,
	0x3183,0x0024,0x3100,0x0024,0x6402,0x0024,0x0000,0x1001,
	0x2800,0x3c98,0x0019,0x9982,0x3434,0x0024,0x3033,0x0024,
	0x3000,0x0024,0xfe20,0x0024,0x48b6,0x0024,0x4dd6,0x0024,
	0x2800,0x4340,0x3800,0xc024,0x4010,0x0024,0x6402,0x0024,
	0x001d,0x7082,0x2800,0x4198,0x0000,0x0024,0xf290,0x1010,
	0x3033,0x0024,0x3800,0x0024,0x3404,0x0024,0x3073,0x0024,
	0x3013,0x0024,0x3800,0x0024,0x0004,0x4d50,0x3010,0x0024,
	0x30f0,0x4024,0x3434,0x4024,0x3143,0x0024,0x3910,0x0024,
	0x2800,0x4340,0x39f0,0x4024,0x3434,0x0024,0x3033,0x0024,
	0x3000,0x0024,0xfe20,0x0024,0x48b6,0x0024,0x4dd6,0x0024,
	0x3800,0xc024,0x001e,0x9982,0x0001,0x1012,0x0000,0x0381,
	0x34d3,0x184c,0x3444,0x0024,0x3073,0x0024,0x3013,0x0024,
	0x3000,0x0024,0xfe20,0x0024,0x48b6,0x0024,0x4dd6,0x0024,
	0x4380,0x3003,0x3400,0x0024,0x293d,0x2900,0x3e00,0x0024,
	0x3009,0x33c0,0x293b,0xc540,0x0010,0x0004,0x34d3,0x184c,
	0x3444,0x0024,0x3073,0x13c0,0x3073,0x0024,0x293b,0xf880,
	0x0001,0x1011,0x0001,0x0010,0x0001,0x1011,0x34d3,0x184c,
	0x3430,0x0024,0x4010,0x0024,0x0000,0x05c1,0x3e10,0x0024,
	0x293b,0xac80,0x0006,0x0092,0x0000,0x05d1,0x36f3,0x134c,
	0x3404,0x0024,0x3083,0x0024,0x3000,0x0024,0x6012,0x0024,
	0x0013,0x3304,0x2800,0x5198,0x0001,0xc682,0x0000,0x0500,
	0x0001,0x0012,0x3404,0x584c,0x3133,0x0024,0x3100,0x4024,
	0x0000,0x05d1,0xfe22,0x0024,0x48b6,0x0024,0x4dd6,0x0024,
	0x3e10,0xc024,0x3430,0x8024,0x4204,0x0024,0x293b,0xb580,
	0x3e00,0x8024,0x36e3,0x134c,0x3434,0x0024,0x3083,0x0024,
	0x3000,0x0024,0x6090,0x0024,0x3800,0x1812,0x36f4,0x4024,
	0x36f1,0x1810,0x36f0,0x9803,0x36f0,0x1801,0x3405,0x9014,
	0x36f3,0x0024,0x36f2,0x1815,0x2000,0x0000,0x36f2,0x9817,
	0x3613,0x0024,0x3e12,0xb817,0x3e12,0x3815,0x3e05,0xb814,
	0x3615,0x0024,0x0000,0x800a,0x3e10,0x3801,0x3e10,0xb804,
	0x3e01,0x7810,0x0008,0x04d0,0x2900,0x1480,0x3001,0x0024,
	0x4080,0x03cc,0x3000,0x0024,0x2800,0x7485,0x4090,0x0024,
	0x0000,0x0024,0x2800,0x6245,0x0000,0x0024,0x0000,0x0081,
	0x3000,0x0024,0x6012,0x0024,0x0000,0x0401,0x2800,0x70c5,
	0x0000,0x0024,0x6012,0x0024,0x0000,0x0024,0x2800,0x6645,
	0x0000,0x0024,0x2900,0x1680,0x0000,0x0024,0x4088,0x008c,
	0x0000,0x2000,0x6400,0x0024,0x0000,0x3c00,0x2800,0x5ed8,
	0x0000,0x0024,0x2800,0x6300,0x3801,0x0024,0x6400,0x038c,
	0x0000,0x0024,0x2800,0x6318,0x0000,0x0024,0x3013,0x0024,
	0x2900,0x1480,0x3801,0x0024,0x4080,0x0024,0x0000,0x0024,
	0x2800,0x6255,0x0000,0x0024,0x6890,0x03cc,0x2800,0x7480,
	0x3800,0x0024,0x2900,0x1680,0x0008,0x0510,0x3800,0x0024,
	0x0000,0x3c00,0x6400,0x0024,0x003f,0xff00,0x2800,0x6b08,
	0x0000,0x0024,0x0000,0x3fc0,0x6400,0x0024,0x0000,0x3c00,
	0x2800,0x73c5,0x6400,0x0024,0x0000,0x0024,0x2800,0x73d5,
	0x0000,0x0024,0xb880,0x184c,0x2900,0x1480,0x3009,0x3800,
	0x4082,0x9bc0,0x6014,0x0024,0x0000,0x3c04,0x2800,0x6941,
	0x0000,0x3dc1,0x2900,0x1680,0x0000,0x0024,0xf400,0x4004,
	0x0000,0x3dc1,0x6412,0x0024,0x0008,0x0490,0x2800,0x6a85,
	0x0000,0x0000,0x0000,0x0400,0x2800,0x7480,0x3800,0x0024,
	0x0008,0x04d0,0x3001,0x4024,0xa50a,0x0024,0x0000,0x03c0,
	0xb50a,0x0024,0x0000,0x0300,0x6500,0x0024,0x0000,0x0024,
	0x2900,0x1488,0x0000,0x6f48,0x0000,0x0380,0x6500,0x0024,
	0x0000,0x0024,0x2800,0x7195,0x0000,0x0024,0x2900,0x1480,
	0x0000,0x0024,0x4080,0x03cc,0x0000,0x0080,0x2800,0x70d5,
	0x0000,0x0024,0x2800,0x7480,0x3800,0x0024,0x2900,0x1680,
	0x0000,0x0024,0x408a,0x0024,0x0008,0x0510,0x3613,0x0024,
	0x3e11,0x4024,0x30f0,0x0024,0x3e10,0x0024,0x3000,0x4024,
	0x2931,0xe080,0x3e00,0x4024,0x36d3,0x0024,0x0000,0x0000,
	0x0008,0x0490,0x3800,0x0024,0x36f1,0x5810,0x36f0,0x9804,
	0x36f0,0x1801,0x3405,0x9014,0x36f3,0x0024,0x36f2,0x1815,
	0x2000,0x0000,0x36f2,0x9817,0x0005,0xbe51,0x0001,0x0010,
	0x3613,0x0024,0x3e05,0xb814,0x3635,0x0024,0x0000,0x800a,
	0xb880,0x104c,0xb882,0x33c0,0x2914,0xbec0,0x0004,0xc580,
	0x0019,0x98c0,0x0004,0x4e90,0x3800,0x0024,0x001f,0xff00,
	0x2931,0x6c40,0x3900,0x0024,0x2931,0x6640,0x0000,0x0024,
	0x2900,0x5500,0x0000,0x8001,0x2912,0x0d00,0x3613,0x0024,
	0x6012,0x0024,0x0000,0x8005,0x2800,0x7b18,0x0004,0x4d50,
	0x2912,0x0d00,0x3613,0x108c,0x2934,0x4180,0x3ce0,0x0024,
	0x0000,0x1000,0x3423,0x0024,0x2900,0x0a80,0x34e1,0x0024,
	0xb882,0x0042,0x30f0,0xc024,0x4dc2,0x0024,0x3810,0x0024,
	0x2800,0x7b00,0x38f0,0x4024,0x3e12,0xb817,0x3e12,0x3815,
	0x3e05,0xb814,0x3615,0x0024,0x0000,0x800a,0x3e10,0x3801,
	0x0000,0x0081,0xb880,0xb811,0x0030,0x0291,0x3e14,0x0024,
	0x0030,0x0690,0x3e14,0xb813,0x0030,0x00d3,0x0007,0x9252,
	0x3800,0x0024,0x3910,0x0024,0x3a00,0x0024,0x0000,0xc0c0,
	0x3900,0x0024,0x0030,0x0000,0x0006,0x0051,0x2908,0x6400,
	0x3b00,0x0024,0xb880,0x008c,0x3800,0x0024,0x3800,0x0024,
	0x0003,0x0d40,0x0006,0xc490,0x2908,0x7f80,0x3009,0x2000,
	0x0030,0x0ad0,0x3800,0x184c,0x002b,0x1100,0x3e10,0x0024,
	0x2909,0xa9c0,0x3e10,0x4024,0x000a,0x8001,0x2908,0x7f80,
	0x36e3,0x0024,0xb880,0x2000,0x0006,0x0010,0x3009,0x2410,
	0x0006,0x0011,0x3009,0x2410,0x0008,0x0490,0x3810,0x0024,
	0x3800,0x0024,0x0000,0x0890,0x290f,0xfcc0,0x0006,0x8380,
	0x000a,0x8001,0x0000,0x0950,0x290f,0xfcc0,0x0006,0xb380,
	0x0000,0x09c0,0x0030,0x0690,0x6890,0x2000,0x0030,0x1310,
	0x6890,0x2000,0x0030,0x0490,0x2900,0x1e00,0x3800,0x0024,
	0x36f4,0x9813,0x36f4,0x1811,0x36f0,0x1801,0x3405,0x9014,
	0x36f3,0x0024,0x36f2,0x1815,0x2000,0x0000,0x36f2,0x9817,
	0x0007,0x0001, /*copy 1*/
	0x5800,
	0x0006,0x0004, /*copy 4*/
	0x1800,0x1800,0x98cc,0x7395,
	0x0007,0x0001, /*copy 1*/
	0x8025,
	0x0006,0x0002, /*copy 2*/
	0x2a00,0x1ace,
	0x0007,0x0001, /*copy 1*/
	0x8022,
	0x0006,0x0002, /*copy 2*/
	0x2a00,0x1a0e,0x000a,0x0001,0x0050,};

#define PLUGIN_SIZE 1039

// CWPlayer.begin();
// CWPlayer.setVolume(1,1);
// CWPlayer.sciWrite (addr , val);
Adafruit_VS1053 CWPlayer = Adafruit_VS1053_FilePlayer (RESET, CS, DCS, DREQ, CARDCS);

//int onceonly = 0;

byte ReadByte = 0;
byte MidiByte[5] = {0,0,0};
byte DoInstr = 0;
byte ChannelTemp = 0;
byte ChannelSelect = 0;
byte n = 0;

// Can connect Rx Midi in via 220 ohm resistor to Uno pin 1 TX
// Can also connect Rx Midi in to USbToMidi opto-isolator output

void setup() {

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
  display.clearDisplay();
  display.display();
  
  Serial.begin(31250); // MIDI uses a 'strange baud rate'

  CWPlayer.begin(); // This should also reset board
  
  LoadUserCode(); // This will start MIDI mode
  
  CWPlayer.setVolume(Volume,Volume); // Turn up volume max 0x00 min 0xFE
  
 //set btn modes, telling them you want to RECEIVE information, not give current
  pinMode (btnNext, INPUT);
  pinMode (btnPrev, INPUT);
  pinMode (btnFunc, INPUT);
  pinMode (MidiLED, OUTPUT);
  digitalWrite (MidiLED, LOW);
  
  DoDisplay2();
  
  ReadEeprom(0);
  ChannelTemp = Channel; 
}
/************************************/
void loop() {  
  //if ( onceonly == 0 ) 
  //{    
  //     onceonly ++;
  //}
  
  //check if the buttons are being pressed
  chkBtn();
  if (AnyBtnPress) readBtn();
  
  digitalWrite (MidiLED, LOW); // Midi activity indicator
  // Echo Midi from Arduino Rx to Tx
  // Arduino Rx to output op opto-isolator
  // Arduino Tx connected to VS1053 Rx via 220ohm resistor
  // Can then filter and manipulate Midi
  // NB: Must disconnect Arduino Rx temporarily whilest uploading sketch
  // Check if data has been sent from the computer using a three byte buffer 
  
     if (Serial.available()) 
     { // Read the most recent byte
       ReadByte = Serial.read();
       digitalWrite (MidiLED, HIGH);
       if (ReadByte==0xC0) DoInstr = 1; 
       // If it is NoteOn 0x90 or NoteOff 0x80 get Channel
       if ((ReadByte>0x7F)&&(ReadByte<0xA0)) ChannelTemp = ReadByte & 0x0F;
       // delay(1);
       // Echo the value that was read back to the serial port
       // Only resppond to selected Channel 0, 1 and/or 9 - Midispeak 1, 2, and/or 10 
       if (ChannelTemp==Channel) Serial.write(ReadByte);
       if ((ChannelSelect>2)&&(ChannelTemp==9)) Serial.write(ReadByte);
       if (DoInstr==1) { MidiByte[n] = ReadByte; n++; }
     }
       
     if (n==2) {   // Instrument Change MidLED will shine brightly here        
          DoInstr = 0; 
          n = 0;
          ChannelTemp = Channel; 
          Channel = MidiByte[0] & 0x0F;
          if (Channel==0) Instrument0 = MidiByte[1]; 
          if (Channel==1) Instrument1 = MidiByte[1];
          if (Channel<2) { ChangeInstr(2); DoDisplay2();} 
          Channel = ChannelTemp;
        }
                  
}


/*******************************************************************************************************************************/
// P Option 1: Change Program: Select Instrument from 0-127 with Up/Dwn Buttons. Can select different instruments for each 
//             channel 0 or 1 but channel 9 different keys are different percussion instruments
// B Option 2: Change Bank: Select from 0 or 1 i.e. Melodic or Percussion
// V Option 3: Volume up or down from 0 to 10
// C Option 4: Select channel 0, 1, 9, 0+9, 1+9 Will only receive noteon/off on these Midi channels 1, 2, 9 or 1 and 9 or 2 and 9
// R Option 5: Select Reverb on or off (0 or 1)
// S Option 6: Save or Read Saved Parameters Up is Write (Save) Parameters, Down is Read (Load) saved parameters back.
// X Option 7: Dwn is Reset, Up is All Notes of Midi 
// M Option 8: Select Midi Path - direct to VS1053 Rx, or via Arduino Rx echo to Arduino Tx and then to VS1053 Rx
//             Can do Midi filtering if not direct to VS1053 Rx
//             Also needs a physical two pole two way switch to reroute Midi signals to the two different Rx's
/********************************************************************************************************************************/

void DoDisplay2 ()
{
  char buffer1[24]; 
  
  //byte ChannelTwo = 0;
  //if (ChannelSelect>2) ChannelTwo = ChannelSelect - 3;
  
  byte RealVol = (0xFE - Volume)/5 - 40; // Max Volume is 0 now it is 10
  if (RealVol>100) RealVol = 0;
  
  if (FunctionNum<8)
    { display.setFont(&FreeSans9pt7b);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,12);
      display.clearDisplay();
      display.print(Function[FunctionNum]); 
      display.print(" ");
      
      if (FunctionNum==1) display.print(Instrument);
      if (FunctionNum==2) { if (Bank==0) display.print(Bank); else display.print("1"); }
      if (FunctionNum==3) display.print(RealVol);
      //if (FunctionNum==4) { display.print(Channel); display.print(" "); display.print(ChannelSelect);}
      if (FunctionNum==4) { display.print(Channel); if (ChannelSelect>2) display.print(" 9");}
      if (FunctionNum==5) display.print(Reverb);
      if (FunctionNum==6) { if (EPrm==1) display.print("r"); if (EPrm==2) display.print("w");}
      if (FunctionNum==7) display.print("r n");
      display.setFont();
      display.println();
      display.print("----------");
      if (Bank==0) {strcpy_P(buffer1, (char*)pgm_read_word(&(InstrBank1[Instrument]))); display.print(buffer1);}
      if (Bank!=0) {strcpy_P(buffer1, (char*)pgm_read_word(&(InstrBank1[128])));        display.print(buffer1);}
      
      // Always visble info at bottom
      display.setCursor(24,40);
      display.print("B");
      if (Bank==0) display.print(Bank); else display.print("1");
      display.setCursor(40,40);
      display.print("V");
      display.print(RealVol);
      
      display.display();
    } else delay(200);
}
/************************************/
// Bank 0: Instrument 0 to 127
// Bank 0x78: Instrument 0x1E
/************************************/
void ChangeInstr(byte UpDwn) // UpDwn 0 = Down Key 1 = Up Key 2 = Instrument Change via Midi Control
  {  if (Bank == 0) // Melodic Instrument
        { if (Channel==0) Instrument = Instrument0; else Instrument = Instrument1;
    
          if (UpDwn==1) {if (Instrument==127) Instrument = 0; else Instrument = Instrument + 1;}
          if (UpDwn==0) {if (Instrument==0) Instrument = 127; else Instrument = Instrument - 1;}
  
          if (Channel==0) Instrument0 = Instrument; else Instrument1 = Instrument;
        }

     if (Bank != 0) Instrument = 0x1E; // Percussion Bank the different notes are the different instruments
     
     byte ProgChange = 0xC0 | Channel;
 
     CWPlayer.sciWrite (VS1053_REG_MODE, 0x0c00);
     delay(100);
     SPI.transfer(ProgChange);
     SPI.transfer(0x00);
     SPI.transfer(Instrument);
     SPI.transfer(0x00);
     delay(100);
     
   }
/**************************************************************************************************************************************/
// write(0xB0 | channel, 0, 0x78);
// write(0xC0 | channel , Intrument);
// Bank Select Number: 0x00: Some MIDI devices have more than 128 Programs (ie, Patches, Instruments, Preset, etc). MIDI Program Change 
// messages only support switching between 128 programs. So, Bank Select Controller (sometimes called Bank Switch) is sometimes used to 
// allow switching between groups of 128 programs. For example, let's say that a device has 512 Programs. It may divide these into 4 
// banks of 128 programs apiece. So, if you want program #129, that would actually be the first program within the second bank. You 
// would send a Bank Select Controller to switch to the second bank (ie, the first bank is #0), and then follow with a Program Change 
// to select the first Program in this bank.
/**************************************************************************************************************************************/
void ChangeBank(byte UpDwn) {
 
  if (UpDwn != 3)
     { delay (100);
       if (Bank == 0x00) { Bank = 0x78;
                           SavedInstrument = Instrument;
                           Instrument = 0x1E;
                         }
       else { Bank = 0x00;
              Instrument = SavedInstrument;
            }
     }
    
  CWPlayer.sciWrite (VS1053_REG_MODE, 0x0c00);
  delay(100);
  SPI.transfer(0xB0);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer(Bank);
  SPI.transfer(0x00);
  if (UpDwn != 3) ChangeInstr(2);
  delay(300);
}

/************************************/
void LoadUserCode(void) {
  // User application code loading tables for VS10xx
  // See http://www.vlsi.fi/en/support/software/vs10xxpatches.html

  int i = 0;
  while (i< PLUGIN_SIZE) {
    word addr, n, val;
    addr = pgm_read_word (&(plugin[i++]));
    n = pgm_read_word (&(plugin[i++]));
    if (n & 0x8000U) { /* RLE run, replicate n samples */
      n &= 0x7FFF;
      val = pgm_read_word (&(plugin[i++]));
      while (n--) {
        CWPlayer.sciWrite (addr , val);       
      }
    } 
    else {           /* Copy run, copy n samples */
      while (n--) {
        val = pgm_read_word (&(plugin[i++]));
        CWPlayer.sciWrite (addr , val);        
      }
    }
  }
  return;
}

/****************************************/
// Send left and right as bytes
// Volume word v left is MSB right is LSB
// 0 Loud FE Quiet FF  is reset analog
/****************************************/
void ChangeVol(byte UpDwn) 
{
     if (UpDwn==0) { if (Volume<0x2F) Volume = Volume + 1; if (Volume==0x2F) Volume = 0xFE; }      // Softer no wrap-around
     if (UpDwn==1) { if (Volume==0xFE) Volume = 0x2F; else if (Volume>0x00) Volume = Volume - 1; } // Louder
     CWPlayer.setVolume(Volume,Volume); 
     
}
/*************************************************************************/
// Channels 1 to 8 and 10 to 16 is melodic instruments
// Channel 10 is percussion (0x00-0x08 and 0x0A-0x0F) (0x09 is Percussion)
// Choices: 0, 1, 9, 0+9, 1+9 = ChannelSelect = 0, 1, 2, 3, 4
// ChannelSelect 0 Responds to Channel 0 only
// ChannelSelect 1 Responds to Channel 1 only
// ChannelSelect 2 Responds to Channel 9 only
// ChannelSelect 3 or 4 Responds to Channel 0 and 9, or 1 and 9 only
/*************************************************************************/
void ChangeChannel(byte UpDwn) 
{
     if (UpDwn==1) { if (ChannelSelect==0) {Channel = 1; ChannelSelect++; return; } 
                     if (ChannelSelect==1) {Channel = 9; ChannelSelect++; return; }
                     if (ChannelSelect==2) {Channel = 0; ChannelSelect++; return; }
                     if (ChannelSelect==3) {Channel = 1; ChannelSelect++; return; }
                     } // No wrap around
     if (UpDwn==0) { if (ChannelSelect==4) {Channel = 0; ChannelSelect--; return; } 
                     if (ChannelSelect==3) {Channel = 9; ChannelSelect--; return; }
                     if (ChannelSelect==2) {Channel = 1; ChannelSelect--; return; }
                     if (ChannelSelect==1) {Channel = 0; ChannelSelect--; return; }
                   }
}
/******************************************************************************************************************************************/
// 0x5b effects level (channel reverb level)
// channel message 0xb0: parameter 
// 0x0c: effect control 1 (sets global reverb decay)
// Reverb: There are 15 different default Reverb settings in the low 4 bits of the parametric_x.config1 register (X:0x1e03). Values 2 to 15
// select a default room size (internal 350 to 4900). 0 will enable the reverb depending on the value of CLOCKF (3.0x or higher clock 
// enables reverb). 1 means reverb is disabled. When you control Reverb through MIDI controls (Parameter / effect control 1), all control 
// values from 0 to 127 are valid (internal 0..4064). In vs1103b Reverb is mutually exclusive with EarSpeaker.
// Will only use Enable or Disable Revern here
/******************************************************************************************************************************************/
void ChangeReverb(byte UpDwn) 
{
  if (UpDwn==1) { if (Reverb==0) Reverb = 1; else Reverb = 0; } // Wrap around
  if (UpDwn==0) { if (Reverb==1) Reverb = 0; else Reverb = 1; }

  byte RevLevel = Reverb*0x7F;
  
  CWPlayer.sciWrite (VS1053_REG_MODE, 0x0c00);
  delay(100);
  SPI.transfer(0xB0);
  SPI.transfer(0x00);
  SPI.transfer(0x5B);
  SPI.transfer(0x00);
  SPI.transfer(0x3F);
  SPI.transfer(0x00);
  delay(100);
  SPI.transfer(0xB0);
  SPI.transfer(0x00);
  SPI.transfer(0x0C);
  SPI.transfer(0x00);
  SPI.transfer(RevLevel);
  SPI.transfer(0x00);
  delay(300);
  ChangeInstr(2);
}
/***************************************/
// Must restore values in right sequence
/***************************************/
void ReadEeprom(byte DoIt) // DoIt 0 = Initialise DoIt = 1 Implement Changes
{ Instrument0 = EEPROM.read(0);
  Instrument1 = EEPROM.read(1);
  Instrument = EEPROM.read(2) ;
  Volume = EEPROM.read(3);
  Channel = EEPROM.read(4);
  byte EBank = EEPROM.read(5);
  Reverb = EEPROM.read(6);
  ChannelSelect = EEPROM.read(7);
  //ChannelSelect = 0;
  EPrm = DoIt;
  if (EBank != Bank) ChangeBank(2); // Also set Instrument but no increase in Bank or Instrument
  ChangeVol(2);
  ChangeReverb(2); 
  DoDisplay2();
  
}
/***********************************************************/
// Could use EEPROM.update(address, value) to save #writes
// Leaves with EPrm = 2 Changes Done
/***********************************************************/
void WriteEeprom() 
{ EEPROM.write(0, Instrument0);
  EEPROM.write(1, Instrument1);
  EEPROM.write(2, Instrument);
  EEPROM.write(3, Volume);
  EEPROM.write(4, Channel);
  EEPROM.write(5, Bank);
  EEPROM.write(6, Reverb);
  EEPROM.write(7, ChannelSelect);
  EPrm = 2;
}
/************************************/
//void ChangeBass() {
//;
//}
/************************************/
//void ChangeTreble() {
//;
//}
/**************************************/
// Reset Eeprom Values or All Notes Off
/**************************************/
void DoResets(byte UpDwn) // UpDwn 0 = Hard Reset UpDwn 1 = All Notes Off 2 Soft Reset
{ if (UpDwn==0) 
     { EEPROM.write(0, 0);
       EEPROM.write(1, 0);
       EEPROM.write(2, 0);
       EEPROM.write(3, 0); // Volume = 0 is full volume
       EEPROM.write(4, 0);
       EEPROM.write(5, 0);
       EEPROM.write(6, 0);
       EEPROM.write(7, 0);
     }
     
  if (UpDwn==1) 
     { CWPlayer.sciWrite (VS1053_REG_MODE, 0x0c00);
       delay(100);
       SPI.transfer(0xB0);
       SPI.transfer(0x00);
       SPI.transfer(0x7B); // All Notes Off can also loop and send 128 0x80 n 0x00 noteoffs
       SPI.transfer(0x00);
       SPI.transfer(0x00);
       SPI.transfer(0x00);
       return;
      }
}
/******************************************************/
void PlayNote(byte Note1, byte Vol1, byte Delay1)
{
CWPlayer.sciWrite (VS1053_REG_MODE, 0x0c00);
  delay(Delay1);
  SPI.transfer(0x90);
  //SPI.transfer(0x91);
  SPI.transfer(0x00);
  SPI.transfer(Note1);
  SPI.transfer(0x00);
  SPI.transfer(Vol1);
  SPI.transfer(0x00);
  delay(Delay1);
  SPI.transfer(0x80);
  //SPI.transfer(0x81);
  SPI.transfer(0x00);
  SPI.transfer(Note1);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
  SPI.transfer(0x00);
}

/************************************/
void chkBtn()
{
  int btnNextState = digitalRead(btnNext);
  int btnPrevState = digitalRead(btnPrev);
  int btnFunction  = digitalRead(btnFunc);
  
  AnyBtnPress = false;
  if (btnNextState) AnyBtnPress = true;
  if (btnPrevState) AnyBtnPress = true;
  if (btnFunction)  AnyBtnPress = true;

  if (AnyBtnPress) {
  ;
  }
}

/************************************/
void readBtn()
{
  //Check if your buttons are pressed
  int btnNextState = digitalRead(btnNext);
  int btnPrevState = digitalRead(btnPrev);
  int btnFunction  = digitalRead(btnFunc);
  
  if (btnNextState) 
     { if (FunctionNum==1) ChangeInstr(1);
       if (FunctionNum==2) ChangeBank(1);
       if (FunctionNum==3) ChangeVol(1);
       if (FunctionNum==4) ChangeChannel(1);
       if (FunctionNum==5) ChangeReverb(1);
       if (FunctionNum==6) WriteEeprom();
       if (FunctionNum==7) DoResets(1);
     }
  else
  if (btnPrevState)  
     { if (FunctionNum==1) ChangeInstr(0);
       if (FunctionNum==2) ChangeBank(0);
       if (FunctionNum==3) ChangeVol(0);
       if (FunctionNum==4) ChangeChannel(0);
       if (FunctionNum==5) ChangeReverb(0);
       if (FunctionNum==6) ReadEeprom(1);
       if (FunctionNum==7) DoResets(0);
     }

  if (btnFunction) { delay(200) ; 
                     if (FunctionNum<7)  FunctionNum = FunctionNum + 1; 
                         else FunctionNum = 1;  
                   }

  delay(200);
  DoDisplay2();
  EPrm = 0;
}

