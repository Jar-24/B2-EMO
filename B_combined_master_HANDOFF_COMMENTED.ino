/*
  B2EMO — DRIVE + SERVOS + SOUND + LEDs (MERGED MASTER)
  =================================================================================================
  OWNER / HANDOFF NOTES
  =================================================================================================
  This is the combined operating sketch for B.  Current B hardware uses an Arduino Mega ADK
  (ATmega2560 + onboard USB host) with the USB Host Shield 2.0 library/PS3BT interface and TWO
  PS3 Navigation controllers.  The library retains "USB Host Shield" naming even though the Mega ADK
  has the host hardware onboard.

  IMPORTANT: THIS FILE HAS BEEN COMMENTED FOR HANDOFF, BUT THE OPERATING LOGIC HAS NOT BEEN CHANGED.

  CONTROLLERS
  -----------
  Nav1 = PRIMARY / DRIVE controller — drive/body/wheel controls
  Input		Droid action
  Stick up/down		Drive forward/back
  Stick left/right		Rotate/turn left/right
  Hold L3 + stick left/right	Strafe left/right instead of rotate
  Hold L2 while driving	Turbo drive speed
  D-pad Up		Raise/extend both body servos
  D-pad Down		Lower/retract both body servos
  D-pad Left		Equalize all four wheel-height servos once, then move all wheels one direction
  D-pad Right		Equalize all four wheel-height servos once, then move all wheels opposite direction
  L1 + D-pad Left		Turtle / park pose; saves current pose first
  L1 + D-pad Right		Restore saved pose
  L1 + D-pad Up		Wheelbase extend
  L1 + D-pad Down		Wheelbase retract
  L1 + Cross		Body up + rear wheels up + front wheels down
  L1 + Circle		Body up + rear wheels down + front wheels up
  L2 + Cross		Body up + right wheels up + left wheels down
  L2 + Circle		Body up + right wheels down + left wheels up
  L2 + D-pad Up		Equalize left/right once, then rear up/front down
  L2 + D-pad Down		Equalize left/right once, then rear down/front up
  L2 + D-pad Left		Equalize front/rear once, then right up/left down
  L2 + D-pad Right		Equalize front/rear once, then right down/left up

  Nav2 = SECONDARY / HEAD + SOUND controller — head/eye/sound controls
  Input		Droid action
  Stick up/down		Head pitch/look up-down, yaw-aware
  Stick left/right		Head yaw/rotation left-right
  Hold L3 + stick left/right	Head roll / “dog tilt” instead of yaw
  Cross		Raise head height
  Circle		Lower head height
  L1 + Cross		Toggle eye ring LEDs on/off
  L1 + Circle		Toggle eye LEDs dim/normal
  D-pad Up		Vocal slot 0
  D-pad Right		Vocal slot 1
  D-pad Down		Vocal slot 2
  D-pad Left		Vocal slot 3
  L2 + D-pad Up/Right/Down/Left	Vocal slots 4/5/6/7
  L1 + D-pad Up/Right/Down/Left	Vocal slots 8/9/10/11
  L1 + L2 + D-pad Up/Right/Down/Left	Vocal slots 12/13/14/15
  Hold PS		Stutter effect active
  PS + D-pad Up		Pulse external amp volume up pin
  PS + D-pad Down		Pulse external amp volume down pin

  Automatic actions
  Trigger		Droid action
  Nav2 disconnects after being connected	Auto-turtle / park pose
  Body not fully collapsed	Body rings randomly auto-spin
  Nav1 D-pad Up/Down body movement held		Ring override spin while body is moving
  Any servo or ring moving	Random servo FX may play
  Drive movement		Drive bed/tank/squeak sounds and random drive FX

  CONNECTION BEHAVIOR
  -------------------
  - Nav1 alone CAN drive the robot.
  - Manual body/head servo controls require Nav2 to be connected as well.
  - If Nav2 disconnects after being connected, the sketch commands the TURTLE/PARK pose.
  - At startup, the servo targets are immediately commanded to the TURTLE/PARK pose.
  - RESTORE is meaningful only after a TURTLE command has first saved the current pose; do not treat
    L1+D-pad RIGHT as a generic "go to startup pose" command.
  - If Nav1 disconnects, drive motors and body-ring motors are stopped.

  MOST COMMON THINGS A NEW OWNER MAY NEED TO ADJUST
  -------------------------------------------------
  1) DRIVE SPEED / FEEL
       MAX_PWM, MAX_PWM_TURBO, TURN_GAIN, RAMP_RATE, DEADZONE, EXPO
       Search for: "DRIVE TUNING"

  2) DRIVE MOTOR WIRING / PINS
       M_FL, M_FR, M_RL, M_RR
       Each motor has a PWM pin, DIR pin, and an invert flag.
       Search for: "DRIVE (MDD10A)"

  3) SERVO CALIBRATION / SPEEDS
       servoDef[] contains PCA board number, channel, min/max/neutral pulse, direction, and speed.
       CAUTION: min_us/max_us are mechanical travel limits.  Change them in small increments and
       verify clearance before commanding full travel.
       Search for: "SERVO DEFINITIONS"

  4) BODY/WHEEL MOTION SPEED
       COMMAND_STEP_US affects body extension and wheelbase target nudging.
       WHEEL_PCT_PER_SEC separately controls manual wheel-height movement.
       Individual servoDef[].speed_us_per_s values are final slew-rate limits.

  5) HEAD SPEED / FEEL / SAFETY
       HEAD_DEAD, PITCH_RATE_N_PER_S, ROLL_RATE_N_PER_S, YAW_RATE_US_PER_S,
       HEIGHT_RATE_US_PER_S, PITCH/ROLL gains, pitch-at-height limits, and TILT_DEV_US_*.
       Head rotation also has a separate HEAD_ROT_MAX_US_PER_S safety/smoothness cap.

  6) LED WIRING / PIXEL COUNTS
       LED_PIN, LED_MAX_MA, BODY_*_PIXELS, EYE_CENTER_PIXELS, RING_*_PIXELS, brightness constants.
       The pixel counts include physical "gap" pixels that must still be counted in the WS2812 data chain.

       *** CURRENT IMPORTANT LED STATE ***
       renderStatusLeds() below is currently an ALL-PIXELS SOLID-RED DIAGNOSTIC TEST.
       The normal body/eye renderer is immediately below it but commented out.  Therefore the normal
       flicker, eye-ring toggle, and eye-dim calculations still run, but their visual pattern is overwritten
       by the solid-red test renderer.  This was intentionally NOT changed in this handoff-comment pass.

  7) BODY RING MOTORS
       RING_*_PWM_PIN / DIR_PIN, RING_PWM_MIN/MAX, and random run/idle time constants.

  8) SOUND / CHIRP / VT-4
       CHIRP is Serial2 (Mega TX2=16, RX2=17), VT-4 MIDI is Serial3 (TX3=14, RX3=15).
       AMP_VOL_UP_PIN and AMP_VOL_DN_PIN pulse the external amp-volume interface.
       VOCALS[] maps the 16 Nav2 D-pad sound slots.
       DRIVE_LOOP_SETS and the *_VOLUME / threshold constants tune movement audio.

  9) PCA9685 SERVO BOARDS / I2C
       PCA0 = address 0x40; PCA1 = address 0x41; both run at 50 Hz.
       This sketch DOES NOT define or drive a PCA9685 OE (Output Enable) pin.
       Do not assume an old "OE = Arduino pin 22" note applies to this file.  If OE wiring is changed,
       verify the hardware behavior and add explicit OE control code if needed.

  PIN QUICK REFERENCE (Arduino Mega numbering)
  ---------------------------------------------
    Drive FL: PWM 2,  DIR 3
    Drive RL: PWM 44, DIR 5
    Drive FR: PWM 6,  DIR 7
    Drive RR: PWM 8,  DIR 9
    Upper body-ring motor: PWM 10, DIR 11
    Lower body-ring motor: PWM 12, DIR 13
    CHIRP Serial2: TX2 16 / RX2 17
    I2C/PCA9685: SDA 20 / SCL 21 (also duplicated on the Mega R3 SDA/SCL header)
    VT-4 MIDI Serial3: TX3 14 / RX3 15
    Amp volume UP/DOWN: 30 / 31
    WS2812 LED data: 46

  LIBRARIES USED
  --------------
    SPI, Wire, USB Host Shield 2.0 (PS3BT/usbhub), Adafruit_PWMServoDriver, FastLED

  TUNING RULE OF THUMB
  --------------------
  Change ONE group of values at a time, upload, and test with the robot supported so a bad servo limit
  or direction cannot bind the mechanism.  Keep a known-good copy of this file before recalibrating.
*/


// ============================ SERVO IDS (must be before Arduino auto-prototypes) ============================
// NOTE: intentionally not using a fixed underlying type here so it can live above the includes safely.
enum ServoId {
  S_FL_WHEEL_H = 0,
  S_FR_WHEEL_H,
  S_RL_WHEEL_H,
  S_RR_WHEEL_H,
  S_WHEELBASE,
  S_LOWER_BODY_EXT,
  S_UPPER_BODY_EXT,
  S_HEAD_HEIGHT,
  S_HEAD_TILT_L,
  S_HEAD_TILT_R,
  S_HEAD_ROT,
  SERVO_COUNT
};
// ==========================================================================================================

#include <SPI.h>
#include <Wire.h>
#include <PS3BT.h>
#include <usbhub.h>

#ifdef PENDING
  #undef PENDING
#endif

#include <Adafruit_PWMServoDriver.h>
#include <FastLED.h>
#include <math.h>

// ============================ SERVO DEFINITIONS (moved up to satisfy Arduino prototypes) ============================
struct ServoDef {
  uint8_t pcaIndex;   // 0 or 1
  uint8_t channel;    // 0..15
  uint16_t min_us;
  uint16_t max_us;
  uint16_t neutral_us;
  int8_t   direction; // +1 normal, -1 inverted
  uint16_t speed_us_per_s;
};

// === SERVO CALIBRATION TABLE ===
// This is the FIRST place to look if a servo is moving too far, the wrong direction, or too quickly.
//
// Columns:
//   pca      = 0 -> PCA0 / address 0x40, 1 -> PCA1 / address 0x41
//   ch       = PCA9685 output channel (0..15)
//   min/max  = absolute allowed pulse range in microseconds; treat these as MECHANICAL LIMITS
//   neutral  = startup/reference pulse for that mechanism
//   dir      = +1 normal command sense, -1 reversed command sense
//   speed    = maximum servo slew speed in microseconds/second
//
// CAUTION: min_us/max_us are not merely "software sensitivity" values.  Moving them outward can drive
// a linkage into a hard stop.  Adjust in small steps and test with the droid supported/unloaded.
//
// NOTE ON SPEEDS:
// - speed_us_per_s limits how fast current_us can chase target_us.
// - Wheel-height manual motion is ALSO limited by WHEEL_PCT_PER_SEC.
// - Body extension/wheelbase targets are generated using COMMAND_STEP_US.
// - Head rotation has an additional HEAD_ROT_MAX_US_PER_S cap later in the sketch.
ServoDef servoDef[SERVO_COUNT] = {
  // pca, ch,   min,  max,  neutral, dir, speed_us_per_s
  // ----- Base PCA (0x40) -----
  {0, 0,  650, 2320, 650,  +1, 5000}, // FL wheel height
  {0, 1,  650, 2270, 2270, -1, 5000}, // FR wheel height
  {0, 2,  700, 2270, 2270, -1, 5000}, // RL wheel height
  {0, 3,  1140,2120, 1140, +1, 5000}, // RR wheel height
  {0, 4,  550, 1900, 1225, -1, 700},  // Wheelbase

  // ----- Upper PCA (0x41) -----
  {1, 0,  650, 2550, 2550, -1, 900},  // Lower body extend
  {1, 1,  850, 2600, 850,  +1, 900},  // Upper body extend
  {1, 2,  1330,2220, 1330, +1, 850},  // Head height
  {1, 3,  1070,2120, 1565, +1, 800},  // Head tilt left
  {1, 4,  880, 1930, 1465, -1, 800},  // Head tilt right
  {1, 5,  730, 2300, 1515, +1, 800},  // Head rotation
};

// Pose state (targets + current)
uint16_t target_us[SERVO_COUNT];
uint16_t current_us[SERVO_COUNT];

// ==============================================================================================================


// ============================ DEBUG SWITCHES ============================
// Legacy note: DEBUG_DRIVE, DEBUG_SERVOS, and DEBUG_INTERVAL_MS are currently NOT referenced elsewhere
// in this merged sketch, so toggling them alone will not enable logging.  SOUND_DEBUG below IS active
// and controls the current CHIRP/sound serial diagnostics.
#define DEBUG_DRIVE 0
#define DEBUG_SERVOS 0
const uint16_t DEBUG_INTERVAL_MS = 100;   // legacy/unused in current merged sketch
// =======================================================================

// ============================ USB / BLUETOOTH ===========================
USB Usb;
BTD Btd(&Usb);

// Two PS3 Nav connections
PS3BT *Nav1 = new PS3BT(&Btd);   // DRIVE
PS3BT *Nav2 = new PS3BT(&Btd);   // SERVOS

static bool nav1Inited = false;
static bool nav2Inited = false;

// Guard against ghost Nav2 D-pad clicks after connect/reconnect.
static bool nav2DpadArmed = false;
static bool prevNav2SoundConn = false;
static uint32_t nav2DpadArmAtMs = 0;
static const uint16_t NAV2_DPAD_ARM_DELAY_MS = 120;

void onInitNav1() { nav1Inited = true; Nav1->setLedOn(LED1); }
void onInitNav2() { nav2Inited = true; Nav2->setLedOn(LED2); }

static inline bool bothNavsConnected() {
  return Nav1->PS3NavigationConnected && Nav2->PS3NavigationConnected;
}

// ============================ DRIVE (MDD10A) =============================
// Each motor uses: PWM (speed) + DIR (direction)
struct MotorPins {
  uint8_t pwm;
  uint8_t dir;
  bool invert; // true if motor polarity is flipped relative to "forward"
};

// DRIVE MOTOR PIN MAP.
// Format = { PWM pin, DIR pin, invert }.
// If one wheel runs backward when the others run forward, change ONLY that motor's invert flag
// before swapping code elsewhere.  If hardware is rewired, update these pin numbers to match.
MotorPins M_FL = { 2,  3, false };    // Front Left  : PWM 2,  DIR 3
MotorPins M_RL = { 44, 5, false };    // Rear  Left   : PWM 44, DIR 5
MotorPins M_FR = { 6,  7, false };    // Front Right : PWM 6,  DIR 7
MotorPins M_RR = { 8,  9, false };    // Rear  Right  : PWM 8,  DIR 9

// ---------------------------- DRIVE TUNING ----------------------------
// These are the main "how fast / how touchy does B feel?" settings.
//
// STICK_CENTER: nominal PS3 analog center. Normally leave at 127.
// DEADZONE: larger ignores more small stick drift; too large makes the center feel unresponsive.
// EXPO: 0 = linear. Higher values soften response near center while retaining full stick output.
// MAX_PWM: normal drive ceiling, 0..255. 180 is ~71% of the available PWM command.
// MAX_PWM_TURBO: ceiling while Nav1 L2 is held. 255 = full software PWM command.
// TURN_GAIN: scales rotation only; lower values tame spinning without reducing straight-line speed.
// RAMP_RATE: acceleration/deceleration slew in normalized command units/second.
//            Higher = gets to requested speed sooner; lower = gentler starts/stops.
//
// NOTE: Nav1 L2 is both the TURBO modifier for driving and a modifier used by some body/wheel commands.
const int   STICK_CENTER = 127;
float   DEADZONE = 0.08f;
float   EXPO     = 0.35f;
uint8_t MAX_PWM        = 180;
uint8_t MAX_PWM_TURBO  = 255;
float   TURN_GAIN = 0.75f;
float   RAMP_RATE = 4.0f;

// Drive helpers
static inline float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// Generic shaping helpers (used by drive + head)
static inline float applyDeadzone(float v, float dz) {
  float a = (v >= 0) ? v : -v;
  if (a < dz) return 0.0f;
  float sign = (v >= 0) ? 1.0f : -1.0f;
  float t = (a - dz) / (1.0f - dz);
  return sign * clampf(t, 0.0f, 1.0f);
}
static inline float applyExpo(float v, float expo) {
  // linear*(1-expo) + cubic*expo
  return (v * (1.0f - expo)) + (v * v * v * expo);
}



static inline float stickToNorm(uint8_t raw) {
  int v = (int)raw - STICK_CENTER;     // ~ -127..+128
  float n = (float)v / 127.0f;
  return clampf(n, -1.0f, 1.0f);
}
static void motorWriteRaw(const MotorPins &m, float cmd /* -1..+1 */, uint8_t pwmCap) {
  cmd = clampf(cmd, -1.0f, 1.0f);
  if (m.invert) cmd = -cmd;

  bool forward = (cmd >= 0.0f);
  float mag = (cmd >= 0.0f) ? cmd : -cmd;

  uint8_t pwm = (uint8_t)clampf(mag * (float)pwmCap, 0.0f, (float)pwmCap);

  digitalWrite(m.dir, forward ? HIGH : LOW);
  analogWrite(m.pwm, pwm);
}
static inline float slewLimit(float target, float current, float ratePerSec, float dt) {
  float delta = target - current;
  float maxStep = ratePerSec * dt;
  if (delta >  maxStep) delta =  maxStep;
  if (delta < -maxStep) delta = -maxStep;
  return current + delta;
}
static inline void mixMecanum(float fwd, float strafe, float rot,
                              float &oFL, float &oFR, float &oRL, float &oRR) {
  // Standard mecanum mix
  oFL = fwd + strafe + rot;
  oFR = fwd - strafe - rot;
  oRL = fwd - strafe + rot;
  oRR = fwd + strafe - rot;

  // Normalize if any exceeds magnitude 1
  float m = fabs(oFL);
  if (fabs(oFR) > m) m = fabs(oFR);
  if (fabs(oRL) > m) m = fabs(oRL);
  if (fabs(oRR) > m) m = fabs(oRR);
  if (m > 1.0f) {
    oFL /= m; oFR /= m; oRL /= m; oRR /= m;
  }
}
static void allStop() {
  motorWriteRaw(M_FL, 0, 0);
  motorWriteRaw(M_FR, 0, 0);
  motorWriteRaw(M_RL, 0, 0);
  motorWriteRaw(M_RR, 0, 0);
}


// ============================ SOUND / CHIRP / VT-4 ============================
extern float cmdFL, cmdFR, cmdRL, cmdRR;
static uint8_t currentDrivePwmCap = MAX_PWM;
static const uint32_t CHIRP_BAUD = 115200;   // Match CHIRP.INI BAUD_RATE
static const uint32_t MIDI_BAUD  = 31250;    // MIDI standard baud

// Serial wiring:
//   CHIRP = Serial2 -> Mega TX2 pin 16 / RX2 pin 17
//   MIDI  = Serial3 -> Mega TX3 pin 14 / RX3 pin 15
// CHIRP_RX_ENABLED should be false if only Arduino->CHIRP TX control is wired and CHIRP TX is not
// returned to Mega RX2. CHIRP_FORCE_SIMPLE_MODE bypasses stream tracking if RX feedback is unreliable.
// SOUND_DEBUG prints CHIRP/sound diagnostic messages to the USB serial monitor.
static HardwareSerial &CHIRP = Serial2;
static HardwareSerial &MIDI  = Serial3;
static const bool CHIRP_RX_ENABLED = true;
static const bool CHIRP_FORCE_SIMPLE_MODE = false;
static const bool SOUND_DEBUG = true;

// VT-4 scene recall
// Roland VT-4 MIDI setup.
// Change these only if the VT-4 MIDI channel or stored scene numbers are changed.
// Scene recall is sent as MIDI Program Change; PS-hold toggles between NORMAL and STUTTER.
static const uint8_t VT4_MIDI_CH = 1;
static const uint8_t VT4_SCENE_NORMAL  = 1;
static const uint8_t VT4_SCENE_STUTTER = 2;

// Amp volume trigger outputs.
// These pins emulate momentary UP/DOWN button presses on the external amp-volume interface.
// If that board is replaced or rewired, these are the Arduino pins to change.
// AMP_BUTTON_PRESS_MS controls how long each virtual button press is held.
static const uint8_t AMP_VOL_UP_PIN = 30;
static const uint8_t AMP_VOL_DN_PIN = 31;
static const uint16_t AMP_BUTTON_PRESS_MS = 150;

static void pulseAmpButton(uint8_t pin) {
  digitalWrite(pin, HIGH);
  delay(AMP_BUTTON_PRESS_MS);
  digitalWrite(pin, LOW);
}

static void chirpSendLine(const char *line) {
  CHIRP.print(line);
  CHIRP.print("\r\n");
  CHIRP.flush();
  if (SOUND_DEBUG) {
    Serial.print(F("CHIRP> "));
    Serial.println(line);
  }
}

static void chirpStopAll() {
  chirpSendLine("STOP");
}

static void chirpStopStream(int stream) {
  char buf[24];
  snprintf(buf, sizeof(buf), "STOP:%d", stream);
  chirpSendLine(buf);
}

static void chirpPlay(uint16_t index, uint8_t bank, char page, int16_t volume) {
  char buf[64];
  if (volume >= 0) {
    snprintf(buf, sizeof(buf), "PLAY:%u,%u,%c,%d", index, bank, page, (int)volume);
  } else {
    snprintf(buf, sizeof(buf), "PLAY:%u,%u,%c", index, bank, page);
  }
  chirpSendLine(buf);
}

static void chirpSetGlobalVolume(uint8_t vol0to99) {
  char buf[16];
  snprintf(buf, sizeof(buf), "VOL:%u", (unsigned)vol0to99);
  chirpSendLine(buf);
}

static void chirpSetStreamVolume(int stream, uint8_t vol0to99) {
  if (stream < 0) return;
  char buf[24];
  snprintf(buf, sizeof(buf), "VOL:%d,%u", stream, (unsigned)vol0to99);
  chirpSendLine(buf);
  if (SOUND_DEBUG) {
    Serial.print(F("Stream vol s="));
    Serial.print(stream);
    Serial.print(F(" v="));
    Serial.println(vol0to99);
  }
}

static void midiProgramChange(uint8_t channel1to16, uint8_t program0to127) {
  uint8_t status = 0xC0 | ((channel1to16 - 1) & 0x0F);
  MIDI.write(status);
  MIDI.write(program0to127 & 0x7F);
}

static void midiControlChange(uint8_t channel1to16, uint8_t controller, uint8_t value) {
  uint8_t status = 0xB0 | ((channel1to16 - 1) & 0x0F);
  MIDI.write(status);
  MIDI.write(controller & 0x7F);
  MIDI.write(value & 0x7F);
}

static void vt4RecallNormalSceneAtStartup() {
  // If startup feels "hung" for a few seconds, this intentional delay gives the VT-4 time to boot
  // before the sketch sends bypass/scene MIDI commands.
  delay(4000);                        // allow VT-4 to boot
  midiControlChange(VT4_MIDI_CH, 51, 0); // BYPASS OFF
  delay(50);
  midiProgramChange(VT4_MIDI_CH, VT4_SCENE_NORMAL);
}

static void setStutterActive(bool active) {
  static bool wasActive = false;
  if (active == wasActive) return;
  midiProgramChange(VT4_MIDI_CH, active ? VT4_SCENE_STUTTER : VT4_SCENE_NORMAL);
  wasActive = active;
}

static const uint8_t TARGET_NONE          = 0;
static const uint8_t TARGET_VOCAL         = 1;
static const uint8_t TARGET_DRIVE_BED     = 2;
static const uint8_t TARGET_TANK_TRACKS   = 3;
static const uint8_t TARGET_SQUEAKY_DRIVE = 4;
static const uint8_t TARGET_SERVO_FX      = 5;

struct DriveLoopSet {
  uint8_t bedIndex;
  uint8_t tankIndex;
  uint8_t squeakIndex;
};

struct VocalTrigger {
  const char *name;
  uint8_t bank;
  char page;
  uint16_t index;
  int16_t volume;
};

// CHIRP location containing the looping drive sounds.
// If the SD-card bank/page layout changes, update this and DRIVE_LOOP_SETS below.
static const uint8_t DRIVE_BANK = 2;
static const char    DRIVE_PAGE = 'A';

// Random one-shot mechanical FX layered on top of the drive bed.
// Assumption: your clank_01..09 and scrape_01..06 files live in Bank 3 / Page A
// as indexes 1..15 in this order:
//   1-9  = clank_01..09
//   10-15 = scrape_01..06
static const uint8_t DRIVE_FX_BANK = 3;
static const char    DRIVE_FX_PAGE = 'A';
static const uint16_t DRIVE_FX_CLANK_FIRST_INDEX  = 1;
static const uint8_t  DRIVE_FX_CLANK_COUNT        = 9;
static const uint16_t DRIVE_FX_SCRAPE_FIRST_INDEX = 10;
static const uint8_t  DRIVE_FX_SCRAPE_COUNT       = 6;
static const int16_t  DRIVE_FX_CLANK_VOLUME       = 46;
static const int16_t  DRIVE_FX_SCRAPE_VOLUME      = 44;
static const uint8_t  DRIVE_FX_MIN_DRIVE_PERCENT  = 12;
static const uint32_t DRIVE_FX_MIN_GAP_MS         = 2200;
static const uint32_t DRIVE_FX_MAX_GAP_MS         = 6800;
static const uint32_t DRIVE_FX_RESTART_DELAY_MS   = 900;

// Random one-shot servo/mechanical FX layered on top of servo and body-ring movement.
// Assumption from SD card folder name/picture: 3B_ServoFX = Bank 3 / Page B,
// with Servo_01.mp3 .. Servo_14.mp3 mapped as indexes 1..14.
static const uint8_t  SERVO_FX_BANK             = 3;
static const char     SERVO_FX_PAGE             = 'B';
static const uint16_t SERVO_FX_FIRST_INDEX      = 1;
static const uint8_t  SERVO_FX_COUNT            = 14;
static const int16_t  SERVO_FX_VOLUME           = 34;    // intentionally lower than drive FX
static const uint32_t SERVO_FX_MIN_GAP_MS       = 900;
static const uint32_t SERVO_FX_MAX_GAP_MS       = 2600;
static const uint32_t SERVO_FX_RESTART_DELAY_MS = 180;

// Servo FX behavior:
// - A servo/ring movement only OPENS a random chance window. A sound is not mandatory.
// - One may start right when motion begins, or later while motion is still active.
// - If SERVO_FX_STOP_ON_MOTION_END_CHANCE_PERCENT hits for that specific sound, that sound is stopped
//   when all servo/ring motion stops; otherwise it is allowed to finish naturally.
static const uint8_t  SERVO_FX_START_CHANCE_PERCENT              = 45;
static const uint8_t  SERVO_FX_RUNNING_CHANCE_PERCENT            = 35;
static const uint8_t  SERVO_FX_STOP_ON_MOTION_END_CHANCE_PERCENT = 55;

// Indexes: 1-3 Drive_Bed slow/med/fast, 4-6 Tank_Tracks, 7-9 Squeaky_Drive
static const DriveLoopSet DRIVE_LOOP_SETS[3] = {
  {1, 4, 7},
  {2, 5, 8},
  {3, 6, 9},
};

// DRIVE-SOUND MIX LEVELS, ordered slow / medium / fast.
// These are CHIRP per-stream volume commands.  Tune here if the motion bed, tank tracks, or squeaks
// are too loud/quiet relative to B's voice.  Keep changes modest and test all three speed bands.
static const int16_t DRIVE_BED_VOLUMES[3]  = {28, 34, 38};
static const int16_t TANK_TRACK_VOLUMES[3] = {24, 26, 28};
static const int16_t SQUEAKY_VOLUMES[3]    = {32, 36, 40};

static const uint32_t DRIVE_TANK_START_DELAY_MS   = 800;
static const uint32_t DRIVE_SQUEAK_START_DELAY_MS = 1400;

// Drive-band thresholds use the ACTUAL mixed/ramped drive command, with hysteresis
// so tiny stick motion stays in the slow band longer and the loops do not chatter.
static const uint8_t DRIVE_SOUND_START_PERCENT      = 10;   // must exceed this to begin from stopped
static const uint8_t DRIVE_SOUND_FADE_START_PERCENT = 10;   // below this, fade current drive-loop volumes down
static const uint8_t DRIVE_SOUND_STOP_PERCENT       = 3;    // fall to/below this to fully stop
static const uint8_t DRIVE_SOUND_MED_ENTER_PERCENT  = 42;   // slow -> medium
static const uint8_t DRIVE_SOUND_MED_EXIT_PERCENT   = 28;   // medium -> slow
static const uint8_t DRIVE_SOUND_FAST_ENTER_PERCENT = 78;   // medium -> fast
static const uint8_t DRIVE_SOUND_FAST_EXIT_PERCENT  = 62;   // fast -> medium

// 16 Nav2 D-pad vocal slots.
// Slot order is:
//   0-3   = D-pad U/R/D/L with no modifier
//   4-7   = L2 + U/R/D/L
//   8-11  = L1 + U/R/D/L
//   12-15 = L1+L2 + U/R/D/L
// Each entry = {"friendly name", CHIRP bank, page, index, volume}.
// If sound files are rearranged on the CHIRP card, this table is the mapping to update.
static const VocalTrigger VOCALS[] = {
  {"Pausing_for_data_lag",            1, 'A',  1, 99},
  {"I_can_lie",                       1, 'A',  2, 99},
  {"Thats_two_lies",                  1, 'A',  3, 99},
  {"Hes_back",                        1, 'A',  4, 99},
  {"I_was_coming_to_find_you",        1, 'A',  5, 99},
  {"She_says_we_cant_go",             1, 'A',  6, 99},
  {"M-m-maarva",                      1, 'A',  7, 99},
  {"Cassian",                         1, 'A',  8, 99},
  {"I_brought_what_you_told_me",      1, 'A',  9, 99},
  {"Brasso_was_looking_for_you",      1, 'A', 10, 99},
  {"I_dont_want_to_be_alone",         1, 'A', 11, 99},
  {"Health_and_reputation",           1, 'A', 12, 99},
  {"Can_I_speak_now",                 1, 'A', 13, 99},
  {"Couldnt_talk_your_way_out_of",    1, 'A', 14, 99},
  {"Jezzi_and_Penny_brought",         1, 'A', 15, 99},
  {"Maarva_told_him",                 1, 'A', 16, 99},
};

static int driveBedStream = -1;
static int tankTracksStream = -1;
static int squeakyDriveStream = -1;
static int lastVocalStream = -1;
static int8_t currentDriveBand = -1;
static int8_t desiredDriveBand = -1;
static bool restartDriveBed = false;
static bool restartTankTracks = false;
static bool restartSqueakyDrive = false;
static bool tankStartPending = false;
static bool squeakyStartPending = false;
static uint32_t tankStartAtMs = 0;
static uint32_t squeakyStartAtMs = 0;
static int16_t lastAppliedDriveBedVolume = -1;
static int16_t lastAppliedTankTrackVolume = -1;
static int16_t lastAppliedSqueakyVolume = -1;
// CHIRP master volume sent at startup (0..99).  This scales the whole CHIRP source.
static const uint8_t CHIRP_GLOBAL_BASE_VOLUME = 85;
static int16_t lastAppliedGlobalFadeVolume = -1;
static bool driveStopFadeActive = false;
static uint32_t driveStopFadeStartMs = 0;
static uint32_t lastDriveFadeDebugMs = 0;
static const uint32_t DRIVE_STOP_FADE_DURATION_MS = 2000;
static uint8_t pendingPlayQueue[8];
static uint8_t pendingPlayHead = 0;
static uint8_t pendingPlayTail = 0;
static bool randomDriveFxArmed = false;
static uint32_t nextRandomDriveFxAtMs = 0;
static bool randomServoFxArmed = false;
static uint32_t nextRandomServoFxAtMs = 0;
static int servoFxStream = -1;
static bool servoFxStopWhenMotionEnds = false;

static bool enqueuePendingTarget(uint8_t target) {
  uint8_t nextTail = (uint8_t)((pendingPlayTail + 1) % (sizeof(pendingPlayQueue) / sizeof(pendingPlayQueue[0])));
  if (nextTail == pendingPlayHead) return false;
  pendingPlayQueue[pendingPlayTail] = target;
  pendingPlayTail = nextTail;
  return true;
}

static uint8_t dequeuePendingTarget() {
  if (pendingPlayHead == pendingPlayTail) return TARGET_NONE;
  uint8_t target = pendingPlayQueue[pendingPlayHead];
  pendingPlayHead = (uint8_t)((pendingPlayHead + 1) % (sizeof(pendingPlayQueue) / sizeof(pendingPlayQueue[0])));
  return target;
}

static bool pendingQueueHasTarget(uint8_t wantedTarget) {
  uint8_t i = pendingPlayHead;
  while (i != pendingPlayTail) {
    if (pendingPlayQueue[i] == wantedTarget) return true;
    i = (uint8_t)((i + 1) % (sizeof(pendingPlayQueue) / sizeof(pendingPlayQueue[0])));
  }
  return false;
}

static void chirpPlayTracked(uint8_t target, uint16_t index, uint8_t bank, char page, int16_t volume) {
  enqueuePendingTarget(target);
  chirpPlay(index, bank, page, volume);
}

static void clearTrackedStreamById(int streamId) {
  if (streamId < 0) return;
  if (driveBedStream == streamId) {
    driveBedStream = -1;
    if (desiredDriveBand >= 0) restartDriveBed = true;
  }
  if (tankTracksStream == streamId) {
    tankTracksStream = -1;
    if (desiredDriveBand >= 0) restartTankTracks = true;
  }
  if (squeakyDriveStream == streamId) {
    squeakyDriveStream = -1;
    if (desiredDriveBand >= 0) restartSqueakyDrive = true;
  }
  if (servoFxStream == streamId) {
    servoFxStream = -1;
    servoFxStopWhenMotionEnds = false;
  }
}

static void stopTrackedStream(int &streamRef) {
  if (streamRef >= 0) {
    chirpStopStream(streamRef);
    streamRef = -1;
  }
}

static int8_t drivePercentToBand(uint8_t pct, int8_t currentBand) {
  if (currentBand < 0) {
    if (pct <= DRIVE_SOUND_STOP_PERCENT) return -1;
    if (pct < DRIVE_SOUND_START_PERCENT) return -1;
    if (pct >= DRIVE_SOUND_FAST_ENTER_PERCENT) return 2;
    if (pct >= DRIVE_SOUND_MED_ENTER_PERCENT)  return 1;
    return 0;
  }

  switch (currentBand) {
    case 0:
      if (pct <= DRIVE_SOUND_STOP_PERCENT) return -1;
      if (pct >= DRIVE_SOUND_FAST_ENTER_PERCENT) return 2;
      if (pct >= DRIVE_SOUND_MED_ENTER_PERCENT)  return 1;
      return 0;

    case 1:
      if (pct <= DRIVE_SOUND_STOP_PERCENT) return -1;
      if (pct >= DRIVE_SOUND_FAST_ENTER_PERCENT) return 2;
      if (pct <= DRIVE_SOUND_MED_EXIT_PERCENT)   return 0;
      return 1;

    case 2:
      if (pct <= DRIVE_SOUND_STOP_PERCENT) return -1;
      if (pct <= DRIVE_SOUND_FAST_EXIT_PERCENT)  return 1;
      return 2;

    default:
      if (pct <= DRIVE_SOUND_STOP_PERCENT) return -1;
      if (pct >= DRIVE_SOUND_FAST_ENTER_PERCENT) return 2;
      if (pct >= DRIVE_SOUND_MED_ENTER_PERCENT)  return 1;
      return 0;
  }
}

static uint8_t currentDriveCommandPercent() {
  float m = fabs(cmdFL);
  if (fabs(cmdFR) > m) m = fabs(cmdFR);
  if (fabs(cmdRL) > m) m = fabs(cmdRL);
  if (fabs(cmdRR) > m) m = fabs(cmdRR);
  if (m < 0.0f) m = 0.0f;
  if (m > 1.0f) m = 1.0f;

  float pwmScale = (float)currentDrivePwmCap / (float)MAX_PWM_TURBO;
  float effective = m * pwmScale;
  if (effective > 1.0f) effective = 1.0f;

  return (uint8_t)(effective * 100.0f + 0.5f);
}

static void resetDriveFadeState() {
  lastAppliedDriveBedVolume = -1;
  lastAppliedTankTrackVolume = -1;
  lastAppliedSqueakyVolume = -1;
  lastAppliedGlobalFadeVolume = -1;
  driveStopFadeActive = false;
  driveStopFadeStartMs = 0;
  lastDriveFadeDebugMs = 0;
}

static int16_t scaleDriveLoopVolume(int16_t baseVol, uint8_t drivePct) {
  if (driveStopFadeActive) {
    uint32_t elapsed = millis() - driveStopFadeStartMs;
    if (elapsed >= DRIVE_STOP_FADE_DURATION_MS) return 0;
    uint32_t remaining = DRIVE_STOP_FADE_DURATION_MS - elapsed;
    int16_t scaled = (int16_t)((long)baseVol * (long)remaining / (long)DRIVE_STOP_FADE_DURATION_MS);
    if (scaled < 1 && baseVol > 0) scaled = 1;
    return scaled;
  }

  if (drivePct >= DRIVE_SOUND_FADE_START_PERCENT) return baseVol;
  if (drivePct <= DRIVE_SOUND_STOP_PERCENT) return 0;

  const int span = (int)DRIVE_SOUND_FADE_START_PERCENT - (int)DRIVE_SOUND_STOP_PERCENT;
  if (span <= 0) return baseVol;
  int numer = (int)drivePct - (int)DRIVE_SOUND_STOP_PERCENT;
  if (numer < 0) numer = 0;
  if (numer > span) numer = span;
  int16_t scaled = (int16_t)((long)baseVol * numer / span);
  if (scaled < 1 && baseVol > 0) scaled = 1;
  return scaled;
}

static void applyDriveFadeVolumes(uint8_t drivePct) {
  if (!driveStopFadeActive) {
    if (lastAppliedGlobalFadeVolume != CHIRP_GLOBAL_BASE_VOLUME) {
      chirpSetGlobalVolume(CHIRP_GLOBAL_BASE_VOLUME);
      lastAppliedGlobalFadeVolume = CHIRP_GLOBAL_BASE_VOLUME;
    }
    lastDriveFadeDebugMs = 0;
    return;
  }

  uint32_t elapsed = millis() - driveStopFadeStartMs;
  if (elapsed > DRIVE_STOP_FADE_DURATION_MS) elapsed = DRIVE_STOP_FADE_DURATION_MS;
  uint8_t fadeVol = (uint8_t)(((uint32_t)CHIRP_GLOBAL_BASE_VOLUME * (DRIVE_STOP_FADE_DURATION_MS - elapsed)) / DRIVE_STOP_FADE_DURATION_MS);

  if (SOUND_DEBUG) {
    uint32_t nowMs = millis();
    if (lastDriveFadeDebugMs == 0 || (nowMs - lastDriveFadeDebugMs) >= 200) {
      lastDriveFadeDebugMs = nowMs;
      Serial.print(F("Global fade vol="));
      Serial.println(fadeVol);
    }
  }

  if (fadeVol != lastAppliedGlobalFadeVolume) {
    chirpSetGlobalVolume(fadeVol);
    lastAppliedGlobalFadeVolume = fadeVol;
  }
}

static void startDriveBand(int8_t band, bool immediateFullLayerStart = false) {
  if (band < 0 || band > 2) return;
  const DriveLoopSet &set = DRIVE_LOOP_SETS[band];

  desiredDriveBand = band;
  currentDriveBand = band;
  restartDriveBed = false;
  restartTankTracks = false;
  restartSqueakyDrive = false;
  pendingPlayHead = 0;
  pendingPlayTail = 0;
  resetDriveFadeState();
  resetRandomDriveFxScheduler(millis(), false);
  nextRandomDriveFxAtMs = 0;
  chirpSetGlobalVolume(CHIRP_GLOBAL_BASE_VOLUME);
  resetRandomDriveFxScheduler(millis(), true);

  stopTrackedStream(driveBedStream);
  stopTrackedStream(tankTracksStream);
  stopTrackedStream(squeakyDriveStream);

  chirpPlayTracked(TARGET_DRIVE_BED, set.bedIndex, DRIVE_BANK, DRIVE_PAGE, DRIVE_BED_VOLUMES[band]);

  if (immediateFullLayerStart) {
    tankStartPending = false;
    squeakyStartPending = false;
    chirpPlayTracked(TARGET_TANK_TRACKS, set.tankIndex, DRIVE_BANK, DRIVE_PAGE, TANK_TRACK_VOLUMES[band]);
    chirpPlayTracked(TARGET_SQUEAKY_DRIVE, set.squeakIndex, DRIVE_BANK, DRIVE_PAGE, SQUEAKY_VOLUMES[band]);
  } else {
    tankStartPending = true;
    squeakyStartPending = true;
    tankStartAtMs = millis() + DRIVE_TANK_START_DELAY_MS;
    squeakyStartAtMs = millis() + DRIVE_SQUEAK_START_DELAY_MS;
  }
}

static void stopDriveSounds() {
  bool hadDriveActivity =
      (desiredDriveBand >= 0) || (currentDriveBand >= 0) ||
      (driveBedStream >= 0) || (tankTracksStream >= 0) || (squeakyDriveStream >= 0) ||
      tankStartPending || squeakyStartPending ||
      restartDriveBed || restartTankTracks || restartSqueakyDrive ||
      (pendingPlayHead != pendingPlayTail);

  bool vocalActiveOrPending = (lastVocalStream >= 0) || pendingQueueHasTarget(TARGET_VOCAL);

  desiredDriveBand = -1;
  currentDriveBand = -1;
  driveStopFadeActive = false;
  driveStopFadeStartMs = 0;
  restartDriveBed = false;
  restartTankTracks = false;
  restartSqueakyDrive = false;
  tankStartPending = false;
  squeakyStartPending = false;
  pendingPlayHead = 0;
  pendingPlayTail = 0;
  resetDriveFadeState();
  chirpSetGlobalVolume(CHIRP_GLOBAL_BASE_VOLUME);

  stopTrackedStream(driveBedStream);
  stopTrackedStream(tankTracksStream);
  stopTrackedStream(squeakyDriveStream);

  if (hadDriveActivity && !vocalActiveOrPending) {
    chirpStopAll();
  }
}

static void updateDriveSoundsFromMotion() {
  uint8_t drivePct = currentDriveCommandPercent();
  int8_t newBand = drivePercentToBand(drivePct, desiredDriveBand);

  bool driveActive = (desiredDriveBand >= 0) || (currentDriveBand >= 0) ||
                     (driveBedStream >= 0) || (tankTracksStream >= 0) || (squeakyDriveStream >= 0) ||
                     tankStartPending || squeakyStartPending ||
                     restartDriveBed || restartTankTracks || restartSqueakyDrive ||
                     (pendingPlayHead != pendingPlayTail);

  if (driveActive) {
    if (drivePct <= DRIVE_SOUND_FADE_START_PERCENT) {
      if (!driveStopFadeActive) {
        driveStopFadeActive = true;
        driveStopFadeStartMs = millis();
      }
    } else if (driveStopFadeActive) {
      driveStopFadeActive = false;
      driveStopFadeStartMs = 0;
      resetDriveFadeState();
    }
  } else {
    driveStopFadeActive = false;
    driveStopFadeStartMs = 0;
  }

  if (driveStopFadeActive) {
    uint32_t fadeElapsed = millis() - driveStopFadeStartMs;
    if (fadeElapsed >= DRIVE_STOP_FADE_DURATION_MS) {
      newBand = -1;
    } else {
      // Hold the current band during the timed fade so we do not hard-cut
      // the drive bed as soon as the commanded drive percent falls to the
      // normal stop threshold.
      if (desiredDriveBand >= 0) {
        newBand = desiredDriveBand;
      } else if (currentDriveBand >= 0) {
        newBand = currentDriveBand;
      }
    }
  }

  static int8_t lastReportedBand = -99;
  static uint8_t lastReportedPct = 255;
  static int lastFadeActive = -1;
  if (SOUND_DEBUG && (newBand != lastReportedBand || drivePct != lastReportedPct || (int)driveStopFadeActive != lastFadeActive)) {
    Serial.print(F("Drive sound pct="));
    Serial.print(drivePct);
    Serial.print(F(" band="));
    Serial.print(newBand);
    Serial.print(F(" fade="));
    Serial.println(driveStopFadeActive ? F("Y") : F("N"));
    lastReportedBand = newBand;
    lastReportedPct = drivePct;
    lastFadeActive = (int)driveStopFadeActive;
  }

  if (desiredDriveBand >= 0) {
    applyDriveFadeVolumes(drivePct);
  } else {
    resetDriveFadeState();
  }

  if (newBand < 0) {
    if (driveActive) {
      stopDriveSounds();
    }
    return;
  }

  if (newBand == desiredDriveBand) return;

  bool bandWasAlreadyActive = (desiredDriveBand >= 0) || (currentDriveBand >= 0) ||
                              (driveBedStream >= 0) || (tankTracksStream >= 0) || (squeakyDriveStream >= 0);
  driveStopFadeActive = false;
  driveStopFadeStartMs = 0;
  startDriveBand(newBand, bandWasAlreadyActive);
}



static void serviceDriveLoopRestarts() {
  if (desiredDriveBand < 0 || desiredDriveBand > 2) return;
  const DriveLoopSet &set = DRIVE_LOOP_SETS[desiredDriveBand];
  uint32_t nowMs = millis();

  if (tankStartPending && tankTracksStream < 0 && nowMs >= tankStartAtMs) {
    tankStartPending = false;
    chirpPlayTracked(TARGET_TANK_TRACKS, set.tankIndex, DRIVE_BANK, DRIVE_PAGE, TANK_TRACK_VOLUMES[desiredDriveBand]);
  }
  if (squeakyStartPending && squeakyDriveStream < 0 && nowMs >= squeakyStartAtMs) {
    squeakyStartPending = false;
    chirpPlayTracked(TARGET_SQUEAKY_DRIVE, set.squeakIndex, DRIVE_BANK, DRIVE_PAGE, SQUEAKY_VOLUMES[desiredDriveBand]);
  }
  if (restartDriveBed && driveBedStream < 0) {
    restartDriveBed = false;
    chirpPlayTracked(TARGET_DRIVE_BED, set.bedIndex, DRIVE_BANK, DRIVE_PAGE, DRIVE_BED_VOLUMES[desiredDriveBand]);
  }
  if (restartTankTracks && tankTracksStream < 0 && !tankStartPending) {
    restartTankTracks = false;
    chirpPlayTracked(TARGET_TANK_TRACKS, set.tankIndex, DRIVE_BANK, DRIVE_PAGE, TANK_TRACK_VOLUMES[desiredDriveBand]);
  }
  if (restartSqueakyDrive && squeakyDriveStream < 0 && !squeakyStartPending) {
    restartSqueakyDrive = false;
    chirpPlayTracked(TARGET_SQUEAKY_DRIVE, set.squeakIndex, DRIVE_BANK, DRIVE_PAGE, SQUEAKY_VOLUMES[desiredDriveBand]);
  }
}

static void handleChirpLine(const char *line) {
  if (strncmp(line, "S:", 2) != 0) return;

  int stream = atoi(line + 2);
  if (strstr(line, ",ply") != nullptr) {
    uint8_t target = dequeuePendingTarget();
    switch (target) {
      case TARGET_VOCAL:         lastVocalStream = stream;   break;
      case TARGET_DRIVE_BED:     driveBedStream = stream;    break;
      case TARGET_TANK_TRACKS:   tankTracksStream = stream;  break;
      case TARGET_SQUEAKY_DRIVE: squeakyDriveStream = stream; break;
      case TARGET_SERVO_FX:      servoFxStream = stream;     break;
      default: break;
    }
  }

  if (strstr(line, ",stp") != nullptr || strstr(line, ",sto") != nullptr) {
    if (lastVocalStream == stream) lastVocalStream = -1;
    clearTrackedStreamById(stream);
  }
}

static void pollChirpRx() {
  if (!CHIRP_RX_ENABLED) return;
  static char line[128];
  static uint8_t pos = 0;

  while (CHIRP.available()) {
    char c = (char)CHIRP.read();
    if (c == '\n' || c == '\r') {
      if (pos > 0) {
        line[pos] = '\0';
        handleChirpLine(line);
        pos = 0;
      }
    } else if (pos < sizeof(line) - 1) {
      line[pos++] = c;
    }
  }
}

static void playVocalBySlot(uint8_t slot0based) {
  if (slot0based >= (sizeof(VOCALS) / sizeof(VOCALS[0]))) return;

  // Never let a pending stop-fade drag vocals down.
  if (driveStopFadeActive) {
    driveStopFadeActive = false;
    driveStopFadeStartMs = 0;
    chirpSetGlobalVolume(CHIRP_GLOBAL_BASE_VOLUME);
    resetDriveFadeState();
  }

  // For one-shot vocals, the most reliable behavior is to hard-stop any prior vocal
  // before launching the next one. This does not touch the drive-bed loops.
  if (lastVocalStream >= 0 && !CHIRP_FORCE_SIMPLE_MODE) {
    chirpStopStream(lastVocalStream);
    lastVocalStream = -1;
  }

  const VocalTrigger &v = VOCALS[slot0based];
  chirpPlayTracked(TARGET_VOCAL, v.index, v.bank, v.page, v.volume);

  if (SOUND_DEBUG) {
    Serial.print(F("Vocal slot -> "));
    Serial.print(slot0based);
    Serial.print(F("  name="));
    Serial.println(v.name);
  }
}



static uint32_t chooseNextRandomDriveFxDelayMs(uint8_t drivePct) {
  uint32_t minGap = DRIVE_FX_MIN_GAP_MS;
  uint32_t maxGap = DRIVE_FX_MAX_GAP_MS;

  if (drivePct >= 70) {
    minGap = 1600;
    maxGap = 4200;
  } else if (drivePct >= 40) {
    minGap = 1900;
    maxGap = 5200;
  }

  return minGap + (uint32_t)random((long)(maxGap - minGap + 1UL));
}

static void resetRandomDriveFxScheduler(uint32_t nowMs, bool keepArmed) {
  randomDriveFxArmed = keepArmed;
  nextRandomDriveFxAtMs = nowMs + DRIVE_FX_RESTART_DELAY_MS;
}

static void triggerRandomDriveFx(uint8_t drivePct) {
  bool chooseScrape = (random(100) < ((drivePct >= 45) ? 45 : 28));

  if (chooseScrape) {
    uint16_t idx = DRIVE_FX_SCRAPE_FIRST_INDEX + (uint16_t)random(DRIVE_FX_SCRAPE_COUNT);
    chirpPlay(idx, DRIVE_FX_BANK, DRIVE_FX_PAGE, DRIVE_FX_SCRAPE_VOLUME);
    if (SOUND_DEBUG) {
      Serial.print(F("Drive FX scrape idx="));
      Serial.println(idx);
    }
  } else {
    uint16_t idx = DRIVE_FX_CLANK_FIRST_INDEX + (uint16_t)random(DRIVE_FX_CLANK_COUNT);
    chirpPlay(idx, DRIVE_FX_BANK, DRIVE_FX_PAGE, DRIVE_FX_CLANK_VOLUME);
    if (SOUND_DEBUG) {
      Serial.print(F("Drive FX clank idx="));
      Serial.println(idx);
    }
  }
}

static void serviceRandomDriveFx(uint32_t nowMs) {
  uint8_t drivePct = currentDriveCommandPercent();
  bool driveRunning = (desiredDriveBand >= 0) || (currentDriveBand >= 0) ||
                      (driveBedStream >= 0) || (tankTracksStream >= 0) || (squeakyDriveStream >= 0);

  if (!driveRunning || drivePct < DRIVE_FX_MIN_DRIVE_PERCENT || driveStopFadeActive) {
    randomDriveFxArmed = false;
    nextRandomDriveFxAtMs = 0;
    return;
  }

  if (!randomDriveFxArmed) {
    resetRandomDriveFxScheduler(nowMs, true);
    nextRandomDriveFxAtMs = nowMs + chooseNextRandomDriveFxDelayMs(drivePct);
    return;
  }

  if ((int32_t)(nowMs - nextRandomDriveFxAtMs) < 0) return;

  triggerRandomDriveFx(drivePct);
  nextRandomDriveFxAtMs = nowMs + chooseNextRandomDriveFxDelayMs(drivePct);
}


static void updateNav2DpadArmState(uint32_t now) {
  bool nav2Conn = Nav2->PS3NavigationConnected;

  if (!nav2Conn) {
    nav2DpadArmed = false;
    nav2DpadArmAtMs = 0;
    prevNav2SoundConn = false;
    return;
  }

  if (!prevNav2SoundConn) {
    nav2DpadArmed = false;
    nav2DpadArmAtMs = 0;
    // Flush any stale click edges right at connect/reconnect.
    Nav2->getButtonClick(UP);
    Nav2->getButtonClick(RIGHT);
    Nav2->getButtonClick(DOWN);
    Nav2->getButtonClick(LEFT);
    prevNav2SoundConn = true;
  }

  bool dpadHeld = Nav2->getButtonPress(UP) || Nav2->getButtonPress(RIGHT) ||
                  Nav2->getButtonPress(DOWN) || Nav2->getButtonPress(LEFT);

  if (!nav2DpadArmed) {
    if (!dpadHeld) {
      if (nav2DpadArmAtMs == 0) nav2DpadArmAtMs = now;
      if ((uint32_t)(now - nav2DpadArmAtMs) >= NAV2_DPAD_ARM_DELAY_MS) {
        nav2DpadArmed = true;
        // Flush once more at arm time so only fresh presses count.
        Nav2->getButtonClick(UP);
        Nav2->getButtonClick(RIGHT);
        Nav2->getButtonClick(DOWN);
        Nav2->getButtonClick(LEFT);
        if (SOUND_DEBUG) Serial.println(F("Nav2 D-pad armed"));
      }
    } else {
      nav2DpadArmAtMs = 0;
      // Ignore/flush any click edges while waiting for a clean release.
      Nav2->getButtonClick(UP);
      Nav2->getButtonClick(RIGHT);
      Nav2->getButtonClick(DOWN);
      Nav2->getButtonClick(LEFT);
    }
  }
}

static void handleSoundControls() {
  if (!Nav2->PS3NavigationConnected) {
    nav2DpadArmed = false;
    nav2DpadArmAtMs = 0;
    prevNav2SoundConn = false;
    setStutterActive(false);
    return;
  }

  updateNav2DpadArmState(millis());

  const bool psHeld = Nav2->getButtonPress(PS);
  const bool l1Held = Nav2->getButtonPress(L1);
  const bool l2Held = Nav2->getButtonPress(L2);

  const bool upClick    = nav2DpadArmed ? Nav2->getButtonClick(UP)    : false;
  const bool rightClick = nav2DpadArmed ? Nav2->getButtonClick(RIGHT) : false;
  const bool downClick  = nav2DpadArmed ? Nav2->getButtonClick(DOWN)  : false;
  const bool leftClick  = nav2DpadArmed ? Nav2->getButtonClick(LEFT)  : false;

  if (SOUND_DEBUG && (upClick || rightClick || downClick || leftClick)) {
    Serial.print(F("Nav2 D-pad click U/R/D/L = "));
    Serial.print(upClick); Serial.print('/');
    Serial.print(rightClick); Serial.print('/');
    Serial.print(downClick); Serial.print('/');
    Serial.println(leftClick);
  }

  if (psHeld) {
    if (upClick)   pulseAmpButton(AMP_VOL_UP_PIN);
    if (downClick) pulseAmpButton(AMP_VOL_DN_PIN);
  }

  const bool stutterHeld = psHeld && !upClick && !downClick;
  setStutterActive(stutterHeld);

  if (psHeld) return;

  if (!l2Held && !l1Held) {
    if (upClick)    playVocalBySlot(0);
    if (rightClick) playVocalBySlot(1);
    if (downClick)  playVocalBySlot(2);
    if (leftClick)  playVocalBySlot(3);
  } else if (l2Held && !l1Held) {
    if (upClick)    playVocalBySlot(4);
    if (rightClick) playVocalBySlot(5);
    if (downClick)  playVocalBySlot(6);
    if (leftClick)  playVocalBySlot(7);
  } else if (l1Held && !l2Held) {
    if (upClick)    playVocalBySlot(8);
    if (rightClick) playVocalBySlot(9);
    if (downClick)  playVocalBySlot(10);
    if (leftClick)  playVocalBySlot(11);
  } else {
    if (upClick)    playVocalBySlot(12);
    if (rightClick) playVocalBySlot(13);
    if (downClick)  playVocalBySlot(14);
    if (leftClick)  playVocalBySlot(15);
  }
}


// ============================ STATUS / EYE LEDs ============================
// LED electrical/configuration settings.
// LED_PIN is the WS2812 data output.  LED_MAX_MA is FastLED's SOFTWARE power limiter, not a fuse;
// the LED supply and wiring must still be sized/fused appropriately.
// If colors are wrong after replacing strip type, COLOR_ORDER is the first setting to check.
#define LED_PIN        46
#define CHIPSET        WS2812B
#define COLOR_ORDER    GRB
#define LED_TYPE       CHIPSET
#define LED_VOLTS      5
#define LED_MAX_MA     2000

// LED strip layout, in physical wire order.
// Change these block sizes here if you rewire/re-space the strips later.
// Body: 4 lit, 4 dark, 4 lit, 60 dark, 4 lit, 4 dark, 4 lit.
// = 84 physical pixels / 16 lit pixels.

constexpr uint16_t BODY_LIT_A_PIXELS   = 4;
constexpr uint16_t BODY_GAP_A_PIXELS   = 4;
constexpr uint16_t BODY_LIT_B_PIXELS   = 4;
constexpr uint16_t BODY_GAP_MID_PIXELS = 60;
constexpr uint16_t BODY_LIT_C_PIXELS    = 4;
constexpr uint16_t BODY_GAP_B_PIXELS   = 4;
constexpr uint16_t BODY_LIT_D_PIXELS    = 4;
constexpr uint16_t BODY_TOTAL_PIXELS = BODY_LIT_A_PIXELS + BODY_GAP_A_PIXELS + BODY_LIT_B_PIXELS + BODY_GAP_MID_PIXELS + BODY_LIT_C_PIXELS + BODY_GAP_B_PIXELS + BODY_LIT_D_PIXELS;

// Eye center: 7 lit pixels.
constexpr uint16_t EYE_CENTER_PIXELS   = 7;

// Eye ring: 20 lit, 4 dark, 20 lit, 10 dark, 4 lit, 4 dark, 4 lit, 10 dark, 20 lit, 4 dark, 20 lit
// = 120 physical pixels / 88 lit pixels.
constexpr uint16_t RING_LIT_A_PIXELS   = 20;
constexpr uint16_t RING_GAP_A_PIXELS   = 4;
constexpr uint16_t RING_LIT_B_PIXELS   = 20;
constexpr uint16_t RING_GAP_B_PIXELS   = 10;
constexpr uint16_t RING_LIT_C_PIXELS   = 4;
constexpr uint16_t RING_GAP_C_PIXELS   = 4;
constexpr uint16_t RING_LIT_D_PIXELS   = 4;
constexpr uint16_t RING_GAP_D_PIXELS   = 10;
constexpr uint16_t RING_LIT_E_PIXELS   = 20;
constexpr uint16_t RING_GAP_E_PIXELS   = 4;
constexpr uint16_t RING_LIT_F_PIXELS   = 20;
constexpr uint16_t EYE_RING_TOTAL_PIXELS = RING_LIT_A_PIXELS + RING_GAP_A_PIXELS + RING_LIT_B_PIXELS + RING_GAP_B_PIXELS +
                                          RING_LIT_C_PIXELS + RING_GAP_C_PIXELS + RING_LIT_D_PIXELS + RING_GAP_D_PIXELS +
                                          RING_LIT_E_PIXELS + RING_GAP_E_PIXELS + RING_LIT_F_PIXELS;

constexpr uint16_t NUM_LEDS = BODY_TOTAL_PIXELS + EYE_CENTER_PIXELS + EYE_RING_TOTAL_PIXELS;  // currently 211

CRGB leds[NUM_LEDS];

// LED brightness tuning.
// EYE_MIN/MAX define the animated center-eye breathing range.
// EYE_RING_BRIGHTNESS is the normal ring level when enabled.
// LED_GLOBAL_BRIGHTNESS is a final FastLED-wide brightness multiplier (0..255).
const uint8_t EYE_MIN_BRIGHTNESS  = 35;
const uint8_t EYE_MAX_BRIGHTNESS  = 160;
const uint8_t EYE_RING_BRIGHTNESS = 70;
const uint8_t LED_GLOBAL_BRIGHTNESS = 180;

struct BodyFlickerGroup {
  uint8_t currentBrightness;
  uint8_t targetBrightness;
  uint32_t nextChangeMs;
  uint8_t pulseBurstsRemaining;
};

// Explicit prototypes to stop Arduino auto-prototype generation from creating
// broken declarations before BodyFlickerGroup is defined.
static inline uint32_t randomRangeUL(uint32_t minVal, uint32_t maxVal);
static void fillRangeRed(uint16_t startIdx, uint16_t count, uint8_t brightness);
static void fillRangeOff(uint16_t startIdx, uint16_t count);
static uint16_t renderBodyLayout(uint16_t startIdx);
static uint16_t renderEyeCenter(uint16_t startIdx);
static uint16_t renderEyeRingLayout(uint16_t startIdx);
static uint8_t chooseBodyFlickerTarget();
static void updateBodyFlicker(BodyFlickerGroup &group, uint32_t nowMs);
static void updateEyeAnimation(uint32_t nowMs);
static inline bool secondary_L1_CROSS_pressed();
static inline bool secondary_L1_CIRCLE_pressed();
static void updateEyeLedCombos();
static void updateEyeDimFade(uint32_t nowMs);
static void renderStatusLeds();
static void updateStatusLeds(uint32_t nowMs);

BodyFlickerGroup bodyLed1 {
  0,   // currentBrightness
  0,   // targetBrightness
  0,   // nextChangeMs
  0    // pulseBurstsRemaining
};

BodyFlickerGroup bodyLed2 {
  0,
  0,
  0,
  0
};

float eyeBreathPhase = 0.0f;
float eyeWobblePhase = 0.0f;
uint32_t eyeNextUpdateMs = 0;
uint32_t eyeFadeIntervalMs = 20;
int eyeCurrentBrightness = 90;

bool eyeRingEnabled = false;
bool eyeLedDimmed = false;

// Eye DIM mode, toggled with Nav2 L1+CIRCLE.
// Brightness values are the dimmed targets; EYE_DIM_FADE_MS controls transition time.
// NOTE: while the all-red diagnostic renderer is enabled, these calculations are not visible.
const uint8_t EYE_DIM_BRIGHTNESS      = 28;
const uint8_t EYE_RING_DIM_BRIGHTNESS = 18;
const uint32_t EYE_DIM_FADE_MS        = 2000UL;

uint8_t eyeDimBlend = 0;  // 0 = normal brightness, 255 = fully dimmed
uint32_t eyeDimFadeLastMs = 0;

static inline uint32_t randomRangeUL(uint32_t minVal, uint32_t maxVal) {
  return minVal + (uint32_t)random((long)(maxVal - minVal + 1UL));
}

static void fillRangeRed(uint16_t startIdx, uint16_t count, uint8_t brightness) {
  for (uint16_t i = startIdx; i < (startIdx + count) && i < NUM_LEDS; i++) leds[i] = CRGB(brightness, 0, 0);
}

static void fillRangeOff(uint16_t startIdx, uint16_t count) {
  for (uint16_t i = startIdx; i < (startIdx + count) && i < NUM_LEDS; i++) leds[i] = CRGB::Black;
}

static uint16_t renderBodyLayout(uint16_t startIdx) {
  uint16_t idx = startIdx;

  // Outer 4-pixel body blocks share bodyLed1. Inner 4-pixel body blocks share bodyLed2.
  fillRangeRed(idx, BODY_LIT_A_PIXELS, bodyLed1.currentBrightness); idx += BODY_LIT_A_PIXELS;
  fillRangeOff(idx, BODY_GAP_A_PIXELS);                                idx += BODY_GAP_A_PIXELS;
  fillRangeRed(idx, BODY_LIT_B_PIXELS, bodyLed2.currentBrightness); idx += BODY_LIT_B_PIXELS;
  fillRangeOff(idx, BODY_GAP_MID_PIXELS);                              idx += BODY_GAP_MID_PIXELS;
  fillRangeRed(idx, BODY_LIT_C_PIXELS, bodyLed2.currentBrightness); idx += BODY_LIT_C_PIXELS;
  fillRangeOff(idx, BODY_GAP_B_PIXELS);                                idx += BODY_GAP_B_PIXELS;
  fillRangeRed(idx, BODY_LIT_D_PIXELS, bodyLed1.currentBrightness); idx += BODY_LIT_D_PIXELS;

  return idx;
}

static uint16_t renderEyeCenter(uint16_t startIdx) {
  uint8_t eyeBrightnessToShow = (uint8_t)(
    ((uint16_t)(255 - eyeDimBlend) * (uint16_t)eyeCurrentBrightness +
     (uint16_t)eyeDimBlend * (uint16_t)EYE_DIM_BRIGHTNESS) / 255U);

  fillRangeRed(startIdx, EYE_CENTER_PIXELS, eyeBrightnessToShow);
  return startIdx + EYE_CENTER_PIXELS;
}

static uint16_t renderEyeRingLayout(uint16_t startIdx) {
  uint16_t idx = startIdx;
  uint8_t ringBrightnessToShow = (uint8_t)(
    ((uint16_t)(255 - eyeDimBlend) * (uint16_t)EYE_RING_BRIGHTNESS +
     (uint16_t)eyeDimBlend * (uint16_t)EYE_RING_DIM_BRIGHTNESS) / 255U);

  if (!eyeRingEnabled) ringBrightnessToShow = 0;

  fillRangeRed(idx, RING_LIT_A_PIXELS, ringBrightnessToShow); idx += RING_LIT_A_PIXELS;
  fillRangeOff(idx, RING_GAP_A_PIXELS);                       idx += RING_GAP_A_PIXELS;
  fillRangeRed(idx, RING_LIT_B_PIXELS, ringBrightnessToShow); idx += RING_LIT_B_PIXELS;
  fillRangeOff(idx, RING_GAP_B_PIXELS);                       idx += RING_GAP_B_PIXELS;
  fillRangeRed(idx, RING_LIT_C_PIXELS, ringBrightnessToShow); idx += RING_LIT_C_PIXELS;
  fillRangeOff(idx, RING_GAP_C_PIXELS);                       idx += RING_GAP_C_PIXELS;
  fillRangeRed(idx, RING_LIT_D_PIXELS, ringBrightnessToShow); idx += RING_LIT_D_PIXELS;
  fillRangeOff(idx, RING_GAP_D_PIXELS);                       idx += RING_GAP_D_PIXELS;
  fillRangeRed(idx, RING_LIT_E_PIXELS, ringBrightnessToShow); idx += RING_LIT_E_PIXELS;
  fillRangeOff(idx, RING_GAP_E_PIXELS);                       idx += RING_GAP_E_PIXELS;
  fillRangeRed(idx, RING_LIT_F_PIXELS, ringBrightnessToShow); idx += RING_LIT_F_PIXELS;

  return idx;
}

static uint8_t chooseBodyFlickerTarget() {
  int r = random(100);

  if (r < 50) return 0;    // off a lot of the time
  if (r < 80) return 22;   // faint glow
  if (r < 94) return 48;   // medium glow
  return 78;               // occasional brighter pulse
}

static void updateBodyFlicker(BodyFlickerGroup &group, uint32_t nowMs) {
  if ((int32_t)(nowMs - group.nextChangeMs) >= 0) {
    // Occasional quick double-pulse or triple-pulse like tired electronics.
    if (group.pulseBurstsRemaining > 0) {
      group.targetBrightness = (group.targetBrightness == 0) ? (uint8_t)random(58, 86) : 0;
      group.pulseBurstsRemaining--;
      group.nextChangeMs = nowMs + randomRangeUL(55, 140);
    } else {
      if (random(100) < 12) {
        group.targetBrightness = (uint8_t)random(58, 86);
        group.pulseBurstsRemaining = (uint8_t)random(2, 5);  // high-low-high or a short burst
        group.nextChangeMs = nowMs + randomRangeUL(70, 160);
      } else {
        group.targetBrightness = chooseBodyFlickerTarget();
        if (group.targetBrightness == 0) {
          group.nextChangeMs = nowMs + randomRangeUL(600, 2400);
        } else if (group.targetBrightness < 30) {
          group.nextChangeMs = nowMs + randomRangeUL(250, 1400);
        } else {
          group.nextChangeMs = nowMs + randomRangeUL(140, 700);
        }
      }
    }
  }

  // Make the panels pop on a little quicker and decay more slowly.
  if (group.currentBrightness < group.targetBrightness) {
    uint8_t stepUp = (uint8_t)random(2, 5);
    uint16_t nextVal = group.currentBrightness + stepUp;
    group.currentBrightness = (nextVal > group.targetBrightness) ? group.targetBrightness : (uint8_t)nextVal;
  } else if (group.currentBrightness > group.targetBrightness) {
    uint8_t stepDown = (uint8_t)random(1, 3);
    group.currentBrightness = (group.currentBrightness > stepDown) ? (group.currentBrightness - stepDown) : 0;
    if (group.currentBrightness < group.targetBrightness) group.currentBrightness = group.targetBrightness;
  }
}

static void updateEyeAnimation(uint32_t nowMs) {
  if ((int32_t)(nowMs - eyeNextUpdateMs) >= 0) {
    eyeNextUpdateMs = nowMs + eyeFadeIntervalMs;

    // Slow breathing curve for the center eye in normal mode.
    eyeBreathPhase += 0.015f;
    if (eyeBreathPhase > TWO_PI) eyeBreathPhase -= TWO_PI;

    // Very slow secondary wobble so the eye feels less perfectly periodic.
    eyeWobblePhase += 0.0045f;
    if (eyeWobblePhase > TWO_PI) eyeWobblePhase -= TWO_PI;

    float breath = (sin(eyeBreathPhase) + 1.0f) * 0.5f;
    float wobble = sin(eyeWobblePhase) * 0.10f;
    float level = breath * (1.0f + wobble);

    int brightness = EYE_MIN_BRIGHTNESS + (int)(level * (float)(EYE_MAX_BRIGHTNESS - EYE_MIN_BRIGHTNESS));
    brightness += random(-3, 4);  // slight organic variation

    if (brightness < EYE_MIN_BRIGHTNESS) brightness = EYE_MIN_BRIGHTNESS;
    if (brightness > EYE_MAX_BRIGHTNESS) brightness = EYE_MAX_BRIGHTNESS;

    eyeCurrentBrightness = brightness;
  }
}

static inline bool secondary_L1_CROSS_pressed() {
  return Nav2->PS3NavigationConnected &&
         Nav2->getButtonPress(L1) &&
         Nav2->getButtonClick(CROSS);
}

static inline bool secondary_L1_CIRCLE_pressed() {
  return Nav2->PS3NavigationConnected &&
         Nav2->getButtonPress(L1) &&
         Nav2->getButtonClick(CIRCLE);
}

static void updateEyeLedCombos() {
  if (secondary_L1_CROSS_pressed()) {
    eyeRingEnabled = !eyeRingEnabled;
  }

  if (secondary_L1_CIRCLE_pressed()) {
    eyeLedDimmed = !eyeLedDimmed;
  }
}

static void updateEyeDimFade(uint32_t nowMs) {
  uint32_t elapsed = nowMs - eyeDimFadeLastMs;
  eyeDimFadeLastMs = nowMs;

  if (elapsed == 0) return;

  uint32_t step = (elapsed * 255UL) / EYE_DIM_FADE_MS;
  if (step < 1UL) step = 1UL;

  if (eyeLedDimmed) {
    uint32_t nextBlend = (uint32_t)eyeDimBlend + step;
    eyeDimBlend = (nextBlend > 255UL) ? 255 : (uint8_t)nextBlend;
  } else {
    eyeDimBlend = (step >= eyeDimBlend) ? 0 : (uint8_t)(eyeDimBlend - step);
  }
}




// !!! LED DIAGNOSTIC MODE CURRENTLY ENABLED !!!
// This deliberately ignores the normal body/eye layout and fills EVERY counted pixel solid red.
// CRGB(100,0,0) controls the diagnostic red intensity before global brightness/power limiting.
// To restore normal body/eye behavior, this diagnostic function must be disabled/replaced with the
// normal renderStatusLeds() implementation that is preserved in the commented block immediately below.
// This handoff pass intentionally leaves the operating behavior unchanged.
static void renderStatusLeds() {
  fill_solid(leds, NUM_LEDS, CRGB(100, 0, 0));
}




/* NORMAL LED RENDERER (currently disabled while the diagnostic function above is active):
static void renderStatusLeds() {
  // Always clear first so any unlit spacer pixels stay dark.
  fill_solid(leds, NUM_LEDS, CRGB::Black);

  uint16_t idx = 0;
  idx = renderBodyLayout(idx);
  idx = renderEyeCenter(idx);
  idx = renderEyeRingLayout(idx);
} */

static void updateStatusLeds(uint32_t nowMs) {
  // IMPORTANT:
  // FastLED.show() disables interrupts while it clocks out WS2812 data.
  // With the expanded 211-pixel layout, doing that every loop can starve
  // the USB Host/Bluetooth stack badly enough that the second PS3 Nav may
  // fail to connect or reconnect.  Throttle LED refreshes, and throttle
  // harder while the second controller is still coming online.
  static uint32_t lastLedShowMs = 0;
  const uint16_t minLedShowIntervalMs = bothNavsConnected() ? 33 : 150; // ~30 FPS normal, ~6 FPS during pairing

  if (lastLedShowMs != 0 && (uint32_t)(nowMs - lastLedShowMs) < minLedShowIntervalMs) {
    return;
  }
  lastLedShowMs = nowMs;

  updateEyeLedCombos();
  updateBodyFlicker(bodyLed1, nowMs);
  updateBodyFlicker(bodyLed2, nowMs);
  updateEyeAnimation(nowMs);
  updateEyeDimFade(nowMs);
  renderStatusLeds();
  FastLED.show();
}

// ============================ BODY RINGS (AUTO) ============================
// Third Cytron MDD10A.
// Pin pairs below are PWM(speed) + DIR(direction), just like the drive motor channels.
// The rings run automatically at random speeds/directions when the body is not fully collapsed.
// When body movement is commanded, both rings can be forced into a movement override.
//
// TUNING:
//   RING_PWM_MIN/MAX     = random speed range (0..255)
//   RING_IDLE_*_MS       = random stopped-time range
//   RING_RUN_*_MS        = random spinning-time range
//   BODY_COLLAPSED_DEADBAND_US = how close body servos must be to "fully in" before rings are stopped
const uint8_t RING_UPPER_PWM_PIN = 10;
const uint8_t RING_UPPER_DIR_PIN = 11;
const uint8_t RING_LOWER_PWM_PIN = 12;
const uint8_t RING_LOWER_DIR_PIN = 13;

const uint8_t  RING_PWM_MIN      = 90;          // slowest motor speed when spinning
const uint8_t  RING_PWM_MAX      = 180;         // fastest motor speed when spinning
const uint32_t RING_IDLE_MIN_MS  = 1500UL;      // minimum time rings stay stopped
const uint32_t RING_IDLE_MAX_MS  = 6000UL;      // maximum time rings stay stopped
const uint32_t RING_RUN_MIN_MS   = 1200UL;      // minimum time rings spin
const uint32_t RING_RUN_MAX_MS   = 4500UL;      // maximum time rings spin
const uint16_t BODY_COLLAPSED_DEADBAND_US = 25; // prevents ring spin when the body is collapsed

static bool     ringRunning[2] = { false, false };
static bool     ringIdle[2]    = { true,  true  };
static bool     ringForward[2] = { true,  true  };
static uint8_t  ringPwm[2]     = { 0,     0     };
static uint32_t ringNextMs[2]  = { 0,     0     };
static bool     ringOverride   = false;

static inline uint8_t ringPwmPin(uint8_t idx) {
  return (idx == 0) ? RING_UPPER_PWM_PIN : RING_LOWER_PWM_PIN;
}

static inline uint8_t ringDirPin(uint8_t idx) {
  return (idx == 0) ? RING_UPPER_DIR_PIN : RING_LOWER_DIR_PIN;
}

static void ringWriteIndex(uint8_t idx, bool forward, uint8_t pwm) {
  digitalWrite(ringDirPin(idx), forward ? HIGH : LOW);
  analogWrite(ringPwmPin(idx), pwm);
}

static void stopRingIndex(uint8_t idx) {
  ringRunning[idx] = false;
  ringWriteIndex(idx, ringForward[idx], 0);
}

static void stopAllRings() {
  stopRingIndex(0);
  stopRingIndex(1);
  ringOverride = false;
}

static bool bodyFullyCollapsed() {
  return (abs((int)current_us[S_UPPER_BODY_EXT] - (int)servoDef[S_UPPER_BODY_EXT].min_us) <= BODY_COLLAPSED_DEADBAND_US) &&
         (abs((int)current_us[S_LOWER_BODY_EXT] - (int)servoDef[S_LOWER_BODY_EXT].max_us) <= BODY_COLLAPSED_DEADBAND_US);
}

static void scheduleRingIdleIndex(uint8_t idx, uint32_t nowMs) {
  ringIdle[idx] = true;
  stopRingIndex(idx);
  ringNextMs[idx] = nowMs + (uint32_t)random((long)RING_IDLE_MIN_MS, (long)RING_IDLE_MAX_MS + 1L);
}

static void scheduleRingRunIndex(uint8_t idx, uint32_t nowMs) {
  ringIdle[idx] = false;
  ringForward[idx] = (random(0L, 2L) == 0L);
  ringPwm[idx] = (uint8_t)random((long)RING_PWM_MIN, (long)RING_PWM_MAX + 1L);
  ringRunning[idx] = true;
  ringWriteIndex(idx, ringForward[idx], ringPwm[idx]);
  ringNextMs[idx] = nowMs + (uint32_t)random((long)RING_RUN_MIN_MS, (long)RING_RUN_MAX_MS + 1L);
}

static void updateAutoRingIndex(uint8_t idx, uint32_t nowMs) {
  if ((int32_t)(nowMs - ringNextMs[idx]) < 0) return;
  if (ringIdle[idx]) scheduleRingRunIndex(idx, nowMs);
  else               scheduleRingIdleIndex(idx, nowMs);
}

static void startRingOverrideIfNeeded() {
  if (ringOverride) return;
  ringOverride = true;
  for (uint8_t i = 0; i < 2; i++) {
    ringIdle[i] = false;
    ringRunning[i] = true;
    ringForward[i] = (random(0L, 2L) == 0L);
    ringPwm[i] = (uint8_t)random((long)RING_PWM_MIN, (long)RING_PWM_MAX + 1L);
    ringWriteIndex(i, ringForward[i], ringPwm[i]);
  }
}

static void updateBodyRings(uint32_t nowMs, bool bodyMoveHeld) {
  if (bodyFullyCollapsed()) {
    stopAllRings();
    return;
  }

  if (bodyMoveHeld) {
    startRingOverrideIfNeeded();
    return;
  }

  if (ringOverride) {
    ringOverride = false;
    scheduleRingIdleIndex(0, nowMs);
    scheduleRingIdleIndex(1, nowMs);
  }

  updateAutoRingIndex(0, nowMs);
  updateAutoRingIndex(1, nowMs);
}

static bool anyServoOrRingMoving() {
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    if (current_us[i] != target_us[i]) return true;
  }
  if (ringRunning[0] || ringRunning[1] || ringOverride) return true;
  return false;
}

static uint32_t chooseNextRandomServoFxDelayMs() {
  return SERVO_FX_MIN_GAP_MS + (uint32_t)random((long)(SERVO_FX_MAX_GAP_MS - SERVO_FX_MIN_GAP_MS + 1UL));
}

static void resetRandomServoFxScheduler(uint32_t nowMs, bool keepArmed) {
  randomServoFxArmed = keepArmed;
  nextRandomServoFxAtMs = nowMs + SERVO_FX_RESTART_DELAY_MS;
}

static void triggerRandomServoFx() {
  // Do not stack servo FX on top of another servo FX. Let the current one finish
  // unless this particular sound was randomly tagged to stop when motion ends.
  if (servoFxStream >= 0 || pendingQueueHasTarget(TARGET_SERVO_FX)) return;

  uint16_t idx = SERVO_FX_FIRST_INDEX + (uint16_t)random(SERVO_FX_COUNT);
  servoFxStopWhenMotionEnds = (random(100) < SERVO_FX_STOP_ON_MOTION_END_CHANCE_PERCENT);
  chirpPlayTracked(TARGET_SERVO_FX, idx, SERVO_FX_BANK, SERVO_FX_PAGE, SERVO_FX_VOLUME);

  if (SOUND_DEBUG) {
    Serial.print(F("Servo FX idx="));
    Serial.print(idx);
    Serial.print(F(" stopWithMotion="));
    Serial.println(servoFxStopWhenMotionEnds ? F("yes") : F("no"));
  }
}

static void serviceRandomServoFx(uint32_t nowMs) {
  static bool wasMotionActive = false;
  bool motionActive = anyServoOrRingMoving();

  if (!motionActive) {
    randomServoFxArmed = false;
    nextRandomServoFxAtMs = 0;
    wasMotionActive = false;

    if (servoFxStream >= 0 && servoFxStopWhenMotionEnds) {
      chirpStopStream(servoFxStream);
      servoFxStream = -1;
      servoFxStopWhenMotionEnds = false;
    }
    return;
  }

  // New motion event: maybe start a sound immediately, maybe just schedule a later chance.
  if (!wasMotionActive) {
    wasMotionActive = true;
    randomServoFxArmed = true;

    if (random(100) < SERVO_FX_START_CHANCE_PERCENT) {
      triggerRandomServoFx();
      nextRandomServoFxAtMs = nowMs + chooseNextRandomServoFxDelayMs();
    } else {
      nextRandomServoFxAtMs = nowMs + chooseNextRandomServoFxDelayMs();
    }
    return;
  }

  if (!randomServoFxArmed) {
    resetRandomServoFxScheduler(nowMs, true);
    nextRandomServoFxAtMs = nowMs + chooseNextRandomServoFxDelayMs();
    return;
  }

  if ((int32_t)(nowMs - nextRandomServoFxAtMs) < 0) return;

  // While motion continues, each timer hit is only a chance to play, not a requirement.
  if (random(100) < SERVO_FX_RUNNING_CHANCE_PERCENT) {
    triggerRandomServoFx();
  }
  nextRandomServoFxAtMs = nowMs + chooseNextRandomServoFxDelayMs();
}

// Read Nav stick (compatible with LeftHat/RightHat variance)
static inline void readNavStick(PS3BT *nav,
                                float &x, float &y,
                                uint8_t &rawX, uint8_t &rawY,
                                bool &usingLeftHat) {
  // Some builds report Nav analog stick as LeftHat, others RightHat.
  rawX = nav->getAnalogHat(LeftHatX);
  rawY = nav->getAnalogHat(LeftHatY);
  usingLeftHat = true;

  // Heuristic: if both are 0 (rare) or wildly off, try RightHat.
  if (rawX == 0 && rawY == 0) {
    rawX = nav->getAnalogHat(RightHatX);
    rawY = nav->getAnalogHat(RightHatY);
    usingLeftHat = false;
  }
  x = stickToNorm(rawX);
  y = stickToNorm(rawY);
}

// High-frequency PWM tweak (from drive sketch).
// This directly changes ATmega2560 timer prescalers for motor-PWM pins.  Do not casually remove or
// rewrite this if motor noise/response is acceptable.  Note that Timer5 covers pins 44/45/46:
// pin 44 is a drive PWM output while pin 46 is used as FastLED DIGITAL data (not analogWrite PWM).
void setupHighFreqPWM() {
  // Timer3 (pin 5)
  TCCR3B = (TCCR3B & 0b11111000) | 0x01; // prescaler = 1
  // Timer4 (pin 6)
  TCCR4B = (TCCR4B & 0b11111000) | 0x01; // prescaler = 1
  // Timer2 (pins 9, 10)
  TCCR2B = (TCCR2B & 0b11111000) | 0x01; // prescaler = 1
  // Timer5 (pins 44, 45, 46)
  TCCR5B = (TCCR5B & 0b11111000) | 0x01; // prescaler = 1
}

// ============================ SERVOS (PCA9685) ============================
// PCA9685 I2C settings.
// PCA0 (base mechanisms) = 0x40; PCA1 (upper/body/head) = 0x41.
// Both share the same SDA/SCL bus.  Standard hobby-servo output frequency is 50 Hz here.
// If a board is replaced, make sure its address jumpers match these values.
//
// IMPORTANT OE NOTE:
// There is NO Arduino OE pin assignment in this sketch.  The code never toggles PCA Output Enable.
// In particular, pin 22 is NOT used here for OE control.  Do not reconnect OE to an Arduino pin merely
// because an older wiring note mentions it unless the sketch is also deliberately updated/tested.
const uint8_t  PCA0_ADDR = 0x40;
const uint8_t  PCA1_ADDR = 0x41;
const uint16_t PCA_FREQ_HZ = 50;

Adafruit_PWMServoDriver pca0 = Adafruit_PWMServoDriver(PCA0_ADDR);
Adafruit_PWMServoDriver pca1 = Adafruit_PWMServoDriver(PCA1_ADDR);

static inline Adafruit_PWMServoDriver& pcaByIndex(uint8_t idx) {
  return (idx == 0) ? pca0 : pca1;
}

// Utility: microseconds -> PCA ticks
static inline uint16_t usToTicks(uint16_t us, uint16_t freqHz) {
  // 1 second = 1,000,000 us, one cycle = 1/freq
  // ticks = us / (period_us/4096)
  uint32_t period_us = 1000000UL / (uint32_t)freqHz;
  uint32_t ticks = ((uint32_t)us * 4096UL) / period_us;
  if (ticks > 4095) ticks = 4095;
  return (uint16_t)ticks;
}


// ===================== HEAD "KEEP LOOKING UP" (yaw-aware tilt mixer) =====================
// This integrates the B_head_tilt.ino behavior into the merged master.
//
// Controller intent (Nav2):
//  - Hat Y   : WORLD pitch command (persistent); stick UP commands positive/"look up" pitch
//  - Hat X   : yaw (rotate) when NOT holding L3
//  - L3+Hat X: manual roll ("dog tilt") when holding L3
//  - CROSS/CIRCLE: head height up/down
//
// The key feature:
//   world pitch is transformed into local pitch+roll as yaw changes,
//   so the face keeps the same world "look up/down" while rotating.

static inline float applyDeadF(float v, float dead) {
  if (v > -dead && v < dead) return 0.0f;
  // rescale outside deadzone to keep full range feel
  if (v > 0.0f) return (v - dead) / (1.0f - dead);
  return (v + dead) / (1.0f - dead);
}

static inline uint16_t axisToUs(const ServoDef &d, float axis) {
  axis = clampf(axis, -1.0f, 1.0f);

  int32_t posSpan = (int32_t)d.max_us - (int32_t)d.neutral_us;
  int32_t negSpan = (int32_t)d.neutral_us - (int32_t)d.min_us;

  int32_t delta;
  if (axis >= 0.0f) delta = (int32_t)(axis * (float)posSpan);
  else              delta = (int32_t)(axis * (float)negSpan);

  delta *= (int32_t)d.direction;

  int32_t us = (int32_t)d.neutral_us + delta;
  if (us < (int32_t)d.min_us) us = (int32_t)d.min_us;
  if (us > (int32_t)d.max_us) us = (int32_t)d.max_us;
  return (uint16_t)us;

}

static inline float usToAxis(const ServoDef &d, uint16_t us) {
  int32_t delta = (int32_t)us - (int32_t)d.neutral_us;
  // invert axisToUs direction multiply
  delta *= (int32_t)d.direction;
  int32_t posSpan = (int32_t)d.max_us - (int32_t)d.neutral_us;
  int32_t negSpan = (int32_t)d.neutral_us - (int32_t)d.min_us;
  if (delta >= 0) {
    if (posSpan <= 0) return 0.0f;
    return clampf((float)delta / (float)posSpan, -1.0f, 1.0f);
  } else {
    if (negSpan <= 0) return 0.0f;
    return clampf((float)delta / (float)negSpan, -1.0f, 1.0f);
  }
}

// ---------- Mixer state ----------
static float g_pitchCmd = 0.0f; // WORLD pitch command, -1..+1 (down..up)
static float g_rollCmd  = 0.0f; // manual roll command,  -1..+1 (left..right)

// float accumulators so dt math stays smooth
static float g_yawTargetF        = 0.0f;
static float g_headHeightTargetF = 0.0f;

// ---------- HEAD TUNING KNOBS ----------
// HEAD_DEAD: stick deadzone for head controls.
// PITCH/ROLL_RATE_N_PER_S: how quickly the persistent normalized pitch/roll command changes.
// YAW_RATE_US_PER_S: how quickly the yaw target moves across its servo pulse range.
// HEIGHT_RATE_US_PER_S: how quickly the head-height target moves.
// These set TARGET-command rates; servoDef[] slew limits can still make the physical servo slower.
static const float HEAD_DEAD              = 0.07f;

static const float PITCH_RATE_N_PER_S     = 1.2f;
static const float ROLL_RATE_N_PER_S      = 1.2f;

static const float YAW_RATE_US_PER_S      = 1500.0f;
static const float HEIGHT_RATE_US_PER_S   = 900.0f;

// Lift-based pitch limits
static const float PITCH_UP_AT_HEIGHT_HIGH   = 1.00f; // head raised
static const float PITCH_UP_AT_HEIGHT_LOW    = 0.25f; // head lowered
static const float PITCH_DOWN_AT_HEIGHT_HIGH = -1.00f;
static const float PITCH_DOWN_AT_HEIGHT_LOW  = -1.00f;

// Mixer gains
static const float PITCH_GAIN = 1.0f;
static const float ROLL_GAIN  = 0.75f;   // affects BOTH manual roll and yaw-generated roll

// Interprets your yaw servo range as +/- YAW_MAX_DEG about neutral.
static const float YAW_MAX_DEG = 80.0f;

// returns 0 raised, 1 lowered
static inline float headLowered01() {
  const ServoDef &h = servoDef[S_HEAD_HEIGHT];
  float span = (float)h.max_us - (float)h.min_us;
  if (span < 1.0f) span = 1.0f;

  float t = ((float)current_us[S_HEAD_HEIGHT] - (float)h.min_us) / span;

  // if the servo direction is reversed, flip the meaning
  if (servoDef[S_HEAD_HEIGHT].direction < 0) {
    t = 1.0f - t;
  }

  t = clampf(t, 0.0f, 1.0f);
  return 1.0f - t;
}

static inline float limitPitchByHeight(float p) {
  float lowered = headLowered01();

  float up =
    PITCH_UP_AT_HEIGHT_HIGH +
    (PITCH_UP_AT_HEIGHT_LOW - PITCH_UP_AT_HEIGHT_HIGH) * lowered;

  float down =
    PITCH_DOWN_AT_HEIGHT_HIGH +
    (PITCH_DOWN_AT_HEIGHT_LOW - PITCH_DOWN_AT_HEIGHT_HIGH) * lowered;

  return clampf(p, down, up);
}

static inline float yawDegFromUs(uint16_t yawUs) {
  const ServoDef &y = servoDef[S_HEAD_ROT];

  if (yawUs >= y.neutral_us) {
    float span = (float)(y.max_us - y.neutral_us);
    if (span < 1.0f) return 0.0f;
    float n = ((float)yawUs - (float)y.neutral_us) / span; // 0..1
    n = clampf(n, 0.0f, 1.0f);
    return n * YAW_MAX_DEG;
  } else {
    float span = (float)(y.neutral_us - y.min_us);
    if (span < 1.0f) return 0.0f;
    float n = ((float)y.neutral_us - (float)yawUs) / span; // 0..1
    n = clampf(n, 0.0f, 1.0f);
    return -n * YAW_MAX_DEG;
  }
}

//static inline void updateTiltTargetsYaw
static inline void syncHeadMixerFromServos() {
  // Set mixer state to match current physical head pose (prevents snap on connect)
  g_yawTargetF        = (float)current_us[S_HEAD_ROT];
  g_headHeightTargetF = (float)current_us[S_HEAD_HEIGHT];

  float leftAxis  = usToAxis(servoDef[S_HEAD_TILT_L], current_us[S_HEAD_TILT_L]);
  float rightAxis = usToAxis(servoDef[S_HEAD_TILT_R], current_us[S_HEAD_TILT_R]);

  float p = 0.5f * (leftAxis + rightAxis);
  float r = 0.5f * (leftAxis - rightAxis);

  // Approximate inverse of gains (ignores yaw fade/trim, but keeps things stable)
  g_pitchCmd = clampf(p / PITCH_GAIN, -1.0f, 1.0f);
  g_rollCmd  = clampf(r / ROLL_GAIN,  -1.0f, 1.0f);
}

void updateTiltTargetsYawAware() {
  // 1) Get WORLD pitch and apply head-height limiter
  float pitchWorld = limitPitchByHeight(g_pitchCmd);

  // Small mechanical trim so "neutral" visually looks level (tune later if needed)
  const float PITCH_TRIM = -0.07f;
  pitchWorld = clampf(pitchWorld + PITCH_TRIM, -1.0f, 1.0f);

  // 2) Determine yaw angle from servo position
  float yawDegTarget  = yawDegFromUs(target_us[S_HEAD_ROT]);
  float yawDegCurrent = yawDegFromUs(current_us[S_HEAD_ROT]);

  const float YAW_BLEND = 0.0f; // 0=current (smooth), 1=target (snappy)
  float yawDeg = yawDegCurrent + (yawDegTarget - yawDegCurrent) * YAW_BLEND;
  float yawRad = yawDeg * 0.01745329252f;

  // 3) Convert WORLD pitch into local pitch + roll
  float pitchLocal = pitchWorld * cosf(yawRad);
  const float ROLL_SIGN = -1.0f;   // flip if behavior is reversed
  float rollFromPitch = ROLL_SIGN * pitchWorld * sinf(yawRad);

  // 4) Reduce leftover pitch as yaw increases (cos^2 feel)
  float pitchFade = cosf(yawRad);
  pitchLocal *= pitchFade;

  // 5) Add manual roll
  float rollTotal = g_rollCmd + rollFromPitch;

  // 6) Apply gains
  float p = pitchLocal * PITCH_GAIN;
  float r = rollTotal  * ROLL_GAIN;

  // 7) Convert pitch/roll to left/right axes
  float leftAxis  = clampf(p + r, -1.0f, 1.0f);
  float rightAxis = clampf(p - r, -1.0f, 1.0f);

  // 8) Convert axes to servo pulses using your servo table
  target_us[S_HEAD_TILT_L] = axisToUs(servoDef[S_HEAD_TILT_L], leftAxis);
  target_us[S_HEAD_TILT_R] = axisToUs(servoDef[S_HEAD_TILT_R], rightAxis);
}


// Turtle restore
bool turtleActive = false;
uint16_t savedPose_us[SERVO_COUNT];

// Servo command/motion tick.
// SERVO_UPDATE_MS = how often manual targets and servo motion are updated; 20 ms = 50 Hz.
// COMMAND_STEP_US = target nudge per tick for BODY EXTENSION and WHEELBASE manual commands.
// Larger COMMAND_STEP_US makes those controls request motion faster, but servoDef[].speed_us_per_s
// remains the final physical slew-rate limiter.  Wheel-height controls use WHEEL_PCT_PER_SEC instead.
const uint16_t SERVO_UPDATE_MS = 20;
const uint16_t COMMAND_STEP_US = 25;

// ============================ HEAD SAFETY VS HEIGHT ============================
// As the head retracts/lowers, allowed head-tilt range shrinks around neutral.
// Tune these once you see real clearances.
const uint16_t TILT_DEV_US_AT_LOW  = 20;  // allowed deviation from neutral when head is LOW
const uint16_t TILT_DEV_US_AT_HIGH = 350;  // allowed deviation from neutral when head is HIGH

// -------- PS3 helpers for servo side --------
static inline bool btn(PS3BT *nav, uint8_t b) { return nav->getButtonPress((ButtonEnum)b); }
static inline uint8_t hatX(PS3BT *nav) { return nav->getAnalogHat(LeftHatX); }
static inline uint8_t hatY(PS3BT *nav) { return nav->getAnalogHat(LeftHatY); }

static inline int16_t signedHatX(uint8_t raw) { return (int16_t)raw - 127; }
static inline int16_t signedHatY(uint8_t raw) { return (int16_t)raw - 127; }

static inline bool dpadUp(PS3BT *nav)    { return btn(nav, UP); }
static inline bool dpadDown(PS3BT *nav)  { return btn(nav, DOWN); }
static inline bool dpadLeft(PS3BT *nav)  { return btn(nav, LEFT); }
static inline bool dpadRight(PS3BT *nav) { return btn(nav, RIGHT); }

static inline int16_t applyDirDelta(ServoId id, int16_t delta) {
  return (int16_t)((int32_t)delta * (int32_t)servoDef[id].direction);
}
static inline void clampTarget(ServoId id) {
  if (target_us[id] < servoDef[id].min_us) target_us[id] = servoDef[id].min_us;
  if (target_us[id] > servoDef[id].max_us) target_us[id] = servoDef[id].max_us;
}
static inline void nudgeTarget(ServoId id, int16_t deltaDirApplied) {
  int32_t v = (int32_t)target_us[id] + (int32_t)deltaDirApplied;
  if (v < 0) v = 0;
  target_us[id] = (uint16_t)v;
  clampTarget(id);
}


static inline uint16_t clampU16(uint16_t v, uint16_t lo, uint16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

// ============================ HEAD TILT SAFETY ============================
static uint16_t safeTiltClamp(ServoId tiltServo, uint16_t desiredUs) {
  // Head height normalized 0..1 where:
  //   t = 0.0  -> physically LOW / retracted (most restrictive)
  //   t = 1.0  -> physically HIGH / extended (least restrictive)
  //
  // IMPORTANT: head-height may be mechanically inverted (direction = -1), so we
  // flip t in that case. We also use *current_us* so the clamp tightens/loosens
  // as the head is actually moving.
  const ServoDef &hh = servoDef[S_HEAD_HEIGHT];
  float span = (float)hh.max_us - (float)hh.min_us;
  if (span < 1.0f) span = 1.0f;

  float t = ((float)current_us[S_HEAD_HEIGHT] - (float)hh.min_us) / span; // 0 at min, 1 at max
  if (hh.direction < 0) {
    t = 1.0f - t;
  }
  t = clampf(t, 0.0f, 1.0f);

  // Allowed deviation from neutral grows with height.
  //float dev = (float)TILT_DEV_US_AT_LOW + t * (float)(TILT_DEV_US_AT_HIGH - TILT_DEV_US_AT_LOW);

  // how aggressively tilt is reduced as the head retracts
  const float TILT_TAPER_EXP = 2.5f;   // 1.0 = linear, >1.0 = clamps faster down low

  float tt = powf(t, TILT_TAPER_EXP);
  float dev = TILT_DEV_US_AT_LOW + tt * (TILT_DEV_US_AT_HIGH - TILT_DEV_US_AT_LOW);



  const ServoDef &sd = servoDef[tiltServo];
  int32_t n  = (int32_t)sd.neutral_us;
  int32_t lo = n - (int32_t)dev;
  int32_t hi = n + (int32_t)dev;

  // Also respect absolute min/max.
  if (lo < (int32_t)sd.min_us) lo = sd.min_us;
  if (hi > (int32_t)sd.max_us) hi = sd.max_us;

  return clampU16(desiredUs, (uint16_t)lo, (uint16_t)hi);
}

static void enforceHeadSafety() {
  // If head height is low, tilt targets might now be illegal. Pull them in.
  target_us[S_HEAD_TILT_L] = safeTiltClamp(S_HEAD_TILT_L, target_us[S_HEAD_TILT_L]);
  target_us[S_HEAD_TILT_R] = safeTiltClamp(S_HEAD_TILT_R, target_us[S_HEAD_TILT_R]);
}
static void writeServoUs(ServoId id, uint16_t us) {
  ServoDef &sd = servoDef[id];
  // Absolute clamp first.
  us = clampU16(us, sd.min_us, sd.max_us);

  // Special clamp for head tilts based on head height (prevents collisions when retracted).
  if (id == S_HEAD_TILT_L || id == S_HEAD_TILT_R) {
    us = safeTiltClamp(id, us);
  }

  Adafruit_PWMServoDriver &pca = pcaByIndex(sd.pcaIndex);
  uint16_t ticks = usToTicks(us, PCA_FREQ_HZ);
  pca.setPWM(sd.channel, 0, ticks);
}

static void applyPoseNowFromTargets() {
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    current_us[i] = target_us[i];
    writeServoUs((ServoId)i, current_us[i]);
  }
}

static void initPoseToNeutral() {
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    target_us[i]  = servoDef[i].neutral_us;
    current_us[i] = servoDef[i].neutral_us;
  }
  applyPoseNowFromTargets();
}

static void equalizeAllWheelsOnce() {
  // Set all wheel heights to the average of current targets to keep them aligned.
  uint32_t sum = 0;
  sum += target_us[S_FL_WHEEL_H];
  sum += target_us[S_FR_WHEEL_H];
  sum += target_us[S_RL_WHEEL_H];
  sum += target_us[S_RR_WHEEL_H];
  uint16_t avg = (uint16_t)(sum / 4);
  target_us[S_FL_WHEEL_H] = avg;
  target_us[S_FR_WHEEL_H] = avg;
  target_us[S_RL_WHEEL_H] = avg;
  target_us[S_RR_WHEEL_H] = avg;
}


static void equalizeLeftRightOnce() {
  // Make FL=FR and RL=RR (equalize each axle left/right)
  uint16_t frontAvg = (uint16_t)(((uint32_t)target_us[S_FL_WHEEL_H] + (uint32_t)target_us[S_FR_WHEEL_H]) / 2);
  uint16_t rearAvg  = (uint16_t)(((uint32_t)target_us[S_RL_WHEEL_H] + (uint32_t)target_us[S_RR_WHEEL_H]) / 2);
  target_us[S_FL_WHEEL_H] = frontAvg;
  target_us[S_FR_WHEEL_H] = frontAvg;
  target_us[S_RL_WHEEL_H] = rearAvg;
  target_us[S_RR_WHEEL_H] = rearAvg;
}

static void equalizeFrontRearOnce() {
  // Make FL=RL and FR=RR (equalize each side front/rear)
  uint16_t leftAvg  = (uint16_t)(((uint32_t)target_us[S_FL_WHEEL_H] + (uint32_t)target_us[S_RL_WHEEL_H]) / 2);
  uint16_t rightAvg = (uint16_t)(((uint32_t)target_us[S_FR_WHEEL_H] + (uint32_t)target_us[S_RR_WHEEL_H]) / 2);
  target_us[S_FL_WHEEL_H] = leftAvg;
  target_us[S_RL_WHEEL_H] = leftAvg;
  target_us[S_FR_WHEEL_H] = rightAvg;
  target_us[S_RR_WHEEL_H] = rightAvg;
}


static inline uint16_t wheelStepUs(ServoId /*id*/, uint16_t dtMs) {
  // Wheel-height tuning:
  // Use ONE common step size for all wheel-height servos so they stay synchronized,
  // even if individual servo min/max ranges differ slightly.
  //
  // Bigger = faster. This is "percent of the AVERAGE wheel range per second".
  // 0.45 = request roughly 45% of the average calibrated wheel travel per second.
  // This is the main manual WHEEL-HEIGHT speed knob; individual servoDef speeds can still cap it.
  const float WHEEL_PCT_PER_SEC = 0.45f;

  static uint16_t avgRange = 0;
  if (avgRange == 0) {
    uint32_t sum =
      (uint32_t)(servoDef[S_FL_WHEEL_H].max_us - servoDef[S_FL_WHEEL_H].min_us) +
      (uint32_t)(servoDef[S_FR_WHEEL_H].max_us - servoDef[S_FR_WHEEL_H].min_us) +
      (uint32_t)(servoDef[S_RL_WHEEL_H].max_us - servoDef[S_RL_WHEEL_H].min_us) +
      (uint32_t)(servoDef[S_RR_WHEEL_H].max_us - servoDef[S_RR_WHEEL_H].min_us);
    avgRange = (uint16_t)(sum / 4);
    if (avgRange < 200) avgRange = 200; // sanity guard
  }

  float usPerSecF = (float)avgRange * WHEEL_PCT_PER_SEC;
  uint32_t usPerSec = (uint32_t)(usPerSecF + 0.5f);
  if (usPerSec < 200) usPerSec = 200;

  uint32_t step = (usPerSec * (uint32_t)dtMs) / 1000UL;
  if (step < 1) step = 1;
  if (step > 60) step = 60; // cap to keep it gentle per loop
  return (int16_t)step;
}



static void applyTurtlePose() {
  // "Park" pose: wheels down (accounting for reversed wheel-height servos),
  // body mostly in, wheelbase retracted, head DOWN + neutral/flat.

  // Wheel heights:
  // Match the original servo sketch behavior:
  //   FL and RR go to MIN. FR and RL are reversed -> MAX.
  target_us[S_FL_WHEEL_H] = servoDef[S_FL_WHEEL_H].min_us;
  target_us[S_RR_WHEEL_H] = servoDef[S_RR_WHEEL_H].min_us;
  target_us[S_FR_WHEEL_H] = servoDef[S_FR_WHEEL_H].max_us;  // reversed
  target_us[S_RL_WHEEL_H] = servoDef[S_RL_WHEEL_H].max_us;  // reversed

  // Body extension:
  target_us[S_UPPER_BODY_EXT] = servoDef[S_UPPER_BODY_EXT].min_us;
  target_us[S_LOWER_BODY_EXT] = servoDef[S_LOWER_BODY_EXT].max_us; // reversed

  // Wheelbase retract: reversed -> MAX
  target_us[S_WHEELBASE] = servoDef[S_WHEELBASE].max_us;

  // Head: retract (down) + neutral orientation
  target_us[S_HEAD_HEIGHT] = servoDef[S_HEAD_HEIGHT].min_us;   // DOWN
  target_us[S_HEAD_TILT_L] = servoDef[S_HEAD_TILT_L].neutral_us;
  target_us[S_HEAD_TILT_R] = servoDef[S_HEAD_TILT_R].neutral_us;
  target_us[S_HEAD_ROT]    = servoDef[S_HEAD_ROT].neutral_us;

  // Keep mixer state consistent with TURTLE targets (so Nav2 logic can't "snap back")
  g_headHeightTargetF = (float)target_us[S_HEAD_HEIGHT];
  g_yawTargetF        = (float)target_us[S_HEAD_ROT];

  // Reset yaw-aware "world" commands so they don't re-assert tilt after turtle.
  g_pitchCmd = 0.0f;
  g_rollCmd  = 0.0f;
}

// ---------- Connection-edge tracking for servo-nav stick centers ----------
static uint32_t g_nav2ConnectedAtMs = 0;
static uint32_t g_nav2FreezeUntilMs = 0;
// Nav2 connect/center-capture tuning.
// Freeze and settle delays prevent the head from twitching while a controller first connects.
// g_hatCaptureSamples controls how many stick-center samples are averaged.
// HAT_DEADZONE is a legacy constant in this version and is NOT used; HEAD_DEAD is the active head deadzone.
static const uint16_t NAV2_CONNECT_FREEZE_MS = 1000;
static bool     g_hatCentersCaptured = false;
static int16_t  g_hatCenterX = 0;
static int16_t  g_hatCenterY = 0;
static const uint16_t g_hatSettleMs = 600;
static const uint8_t  g_hatCaptureSamples = 12;
static const int16_t  HAT_DEADZONE = 18;  // legacy/unused; do not tune this expecting head-stick changes

// ---------- Servo motion update ----------
static void updateServoMotion(uint16_t dtMs) {
  // Keep head tilts within safe limits based on current head height target.
  enforceHeadSafety();
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    ServoId id = (ServoId)i;
    ServoDef &sd = servoDef[id];

    int32_t cur = current_us[i];
    int32_t tgt = target_us[i];

    if (cur == tgt) continue;

    // max delta this tick based on speed (us/s)
    uint32_t maxDelta = ((uint32_t)sd.speed_us_per_s * (uint32_t)dtMs) / 1000UL;
    if (maxDelta < 1) maxDelta = 1;

    // Additional cap for head rotation so it never gets "snappy/jerky" even if you edit servo speeds.
    if (id == S_HEAD_ROT) {
      // Separate hard cap for physical head-rotation slew.  Lower = smoother/finer; higher = snappier.
      // This can override a larger speed_us_per_s value in servoDef[S_HEAD_ROT].
      const uint32_t HEAD_ROT_MAX_US_PER_S = 1000;
      uint32_t cap = (HEAD_ROT_MAX_US_PER_S * (uint32_t)dtMs) / 1000UL;
      if (cap < 1) cap = 1;
      if (maxDelta > cap) maxDelta = cap;
    }

    int32_t delta = tgt - cur;
    if (delta > 0) {
      if ((uint32_t)delta > maxDelta) delta = (int32_t)maxDelta;
    } else {
      if ((uint32_t)(-delta) > maxDelta) delta = -(int32_t)maxDelta;
    }

    cur += delta;
    current_us[i] = (uint16_t)cur;
    writeServoUs(id, current_us[i]);
  }
}

// ============================ SERVO / BODY INPUTS ============================
// Despite this section's historical name, the controls are split:
//   Nav1 = body, wheel heights, wheelbase, turtle/restore
//   Nav2 = head yaw/pitch/roll/height
struct Edge {
  bool prev = false;
  bool rising(bool now) { bool r = (now && !prev); prev = now; return r; }
};
Edge e_turtle;
Edge e_restore;

// Primary (D-pad) controls on Nav2
static void processPrimaryControls() {
  // PRIMARY controller = Nav1 (Drive + Body/Wheels/Wheelbase/Turtle)
  PS3BT *nav = Nav1;
  if (!Nav1->PS3NavigationConnected) return;

  bool L1p = btn(nav, L1);
  bool L2p = btn(nav, L2);

  bool up = dpadUp(nav);
  bool down = dpadDown(nav);
  bool left = dpadLeft(nav);
  bool right = dpadRight(nav);

  bool cross  = btn(nav, CROSS);
  bool circle = btn(nav, CIRCLE);

  // Turtle / Restore (edge triggered):
  //   L1 + D-pad LEFT  = turtle
  //   L1 + D-pad RIGHT = restore previous pose
  bool turtleCombo  = L1p && left;
  bool restoreCombo = L1p && right;

  if (e_turtle.rising(turtleCombo)) {
    for (uint8_t i = 0; i < SERVO_COUNT; i++) savedPose_us[i] = target_us[i];
    turtleActive = true;
    applyTurtlePose();
  }
  if (e_restore.rising(restoreCombo)) {
    for (uint8_t i = 0; i < SERVO_COUNT; i++) target_us[i] = savedPose_us[i];
    turtleActive = false;
  }

  // If turtle is active, still allow restore, but otherwise ignore other commands.
  if (turtleActive) return;

  // Convenience steps (wheel motion uses per-servo speed scaling)
  int16_t stepFL = (int16_t)wheelStepUs(S_FL_WHEEL_H, SERVO_UPDATE_MS);
  int16_t stepFR = (int16_t)wheelStepUs(S_FR_WHEEL_H, SERVO_UPDATE_MS);
  int16_t stepRL = (int16_t)wheelStepUs(S_RL_WHEEL_H, SERVO_UPDATE_MS);
  int16_t stepRR = (int16_t)wheelStepUs(S_RR_WHEEL_H, SERVO_UPDATE_MS);

  // ===================== NAV1 BODY / WHEEL CONTROL MAP =====================
  //
  // L1 + D-pad LEFT  = TURTLE/PARK pose (edge-triggered)
  // L1 + D-pad RIGHT = restore the pose saved immediately before turtle
  //
  // Plain D-pad UP/DOWN    = body extend up/down
  // Plain D-pad LEFT/RIGHT = equalize all wheel heights once, then move all wheels down/up
  //
  // L1 + D-pad UP/DOWN = wheelbase extend/retract
  // L2 + D-pad UP/DOWN = equalize left/right once, then rear/front differential motion
  // L2 + D-pad LEFT/RIGHT = equalize front/rear once, then right/left differential motion
  //
  // L1 + CROSS  = body up, rear up, front down (while pressed)
  // L1 + CIRCLE = body up, rear down, front up (while pressed)
  // L2 + CROSS  = body up, right up, left down (while pressed)
  // L2 + CIRCLE = body up, right down, left up (while pressed)

  bool L1_cross  = L1p && cross;
  bool L1_circle = L1p && circle;
  bool L2_cross  = L2p && cross;
  bool L2_circle = L2p && circle;

  if (L1_cross || L1_circle || L2_cross || L2_circle) {
    // body extend up (both) while any of these combos held
    nudgeTarget(S_LOWER_BODY_EXT, applyDirDelta(S_LOWER_BODY_EXT, +COMMAND_STEP_US));
    nudgeTarget(S_UPPER_BODY_EXT, applyDirDelta(S_UPPER_BODY_EXT, +COMMAND_STEP_US));

    if (L1_cross) {
      // rear up, front down
      nudgeTarget(S_RL_WHEEL_H, applyDirDelta(S_RL_WHEEL_H, +stepRL));
      nudgeTarget(S_RR_WHEEL_H, applyDirDelta(S_RR_WHEEL_H, +stepRR));
      nudgeTarget(S_FL_WHEEL_H, applyDirDelta(S_FL_WHEEL_H, -stepFL));
      nudgeTarget(S_FR_WHEEL_H, applyDirDelta(S_FR_WHEEL_H, -stepFR));
    } else if (L1_circle) {
      // rear down, front up
      nudgeTarget(S_RL_WHEEL_H, applyDirDelta(S_RL_WHEEL_H, -stepRL));
      nudgeTarget(S_RR_WHEEL_H, applyDirDelta(S_RR_WHEEL_H, -stepRR));
      nudgeTarget(S_FL_WHEEL_H, applyDirDelta(S_FL_WHEEL_H, +stepFL));
      nudgeTarget(S_FR_WHEEL_H, applyDirDelta(S_FR_WHEEL_H, +stepFR));
    } else if (L2_cross) {
      // right up, left down
      nudgeTarget(S_FR_WHEEL_H, applyDirDelta(S_FR_WHEEL_H, +stepFR));
      nudgeTarget(S_RR_WHEEL_H, applyDirDelta(S_RR_WHEEL_H, +stepRR));
      nudgeTarget(S_FL_WHEEL_H, applyDirDelta(S_FL_WHEEL_H, -stepFL));
      nudgeTarget(S_RL_WHEEL_H, applyDirDelta(S_RL_WHEEL_H, -stepRL));
    } else if (L2_circle) {
      // right down, left up
      nudgeTarget(S_FR_WHEEL_H, applyDirDelta(S_FR_WHEEL_H, -stepFR));
      nudgeTarget(S_RR_WHEEL_H, applyDirDelta(S_RR_WHEEL_H, -stepRR));
      nudgeTarget(S_FL_WHEEL_H, applyDirDelta(S_FL_WHEEL_H, +stepFL));
      nudgeTarget(S_RL_WHEEL_H, applyDirDelta(S_RL_WHEEL_H, +stepRL));
    }

    // Important: when these combos are active, ignore other wheel/extend commands
    // to prevent conflicting nudges in a single tick.
    return;
  }

  // D-pad UP/DOWN = BOTH body extend up/down (while pressed)
  if (up && !L1p && !L2p) {
    nudgeTarget(S_LOWER_BODY_EXT, applyDirDelta(S_LOWER_BODY_EXT, +COMMAND_STEP_US));
    nudgeTarget(S_UPPER_BODY_EXT, applyDirDelta(S_UPPER_BODY_EXT, +COMMAND_STEP_US));
  }
  if (down && !L1p && !L2p) {
    nudgeTarget(S_LOWER_BODY_EXT, applyDirDelta(S_LOWER_BODY_EXT, -COMMAND_STEP_US));
    nudgeTarget(S_UPPER_BODY_EXT, applyDirDelta(S_UPPER_BODY_EXT, -COMMAND_STEP_US));
  }

  // D-pad LEFT/RIGHT = ALL FOUR WHEEL HEIGHTS TO EQUAL (once), then move all down/up (while pressed)
  if (!L1p && !L2p) {
    static bool prevLeft = false;
    static bool prevRight = false;

    if (left && !prevLeft)  equalizeAllWheelsOnce();
    if (right && !prevRight) equalizeAllWheelsOnce();

    prevLeft = left;
    prevRight = right;

    if (left && !right) {
      nudgeTarget(S_FL_WHEEL_H, applyDirDelta(S_FL_WHEEL_H, -stepFL));
      nudgeTarget(S_FR_WHEEL_H, applyDirDelta(S_FR_WHEEL_H, -stepFR));
      nudgeTarget(S_RL_WHEEL_H, applyDirDelta(S_RL_WHEEL_H, -stepRL));
      nudgeTarget(S_RR_WHEEL_H, applyDirDelta(S_RR_WHEEL_H, -stepRR));
    } else if (right && !left) {
      nudgeTarget(S_FL_WHEEL_H, applyDirDelta(S_FL_WHEEL_H, +stepFL));
      nudgeTarget(S_FR_WHEEL_H, applyDirDelta(S_FR_WHEEL_H, +stepFR));
      nudgeTarget(S_RL_WHEEL_H, applyDirDelta(S_RL_WHEEL_H, +stepRL));
      nudgeTarget(S_RR_WHEEL_H, applyDirDelta(S_RR_WHEEL_H, +stepRR));
    }
  }

  // L2 + D-pad UP/DOWN = L/R HEIGHTS TO EQUAL (once), rear/front differential (while pressed)
  if (L2p && (up || down)) {
    static bool prevCombo = false;
    bool combo = (L2p && (up || down));
    if (combo && !prevCombo) equalizeLeftRightOnce();
    prevCombo = combo;

    if (up) {
      // rear up, front down
      nudgeTarget(S_RL_WHEEL_H, applyDirDelta(S_RL_WHEEL_H, +stepRL));
      nudgeTarget(S_RR_WHEEL_H, applyDirDelta(S_RR_WHEEL_H, +stepRR));
      nudgeTarget(S_FL_WHEEL_H, applyDirDelta(S_FL_WHEEL_H, -stepFL));
      nudgeTarget(S_FR_WHEEL_H, applyDirDelta(S_FR_WHEEL_H, -stepFR));
    } else if (down) {
      // rear down, front up
      nudgeTarget(S_RL_WHEEL_H, applyDirDelta(S_RL_WHEEL_H, -stepRL));
      nudgeTarget(S_RR_WHEEL_H, applyDirDelta(S_RR_WHEEL_H, -stepRR));
      nudgeTarget(S_FL_WHEEL_H, applyDirDelta(S_FL_WHEEL_H, +stepFL));
      nudgeTarget(S_FR_WHEEL_H, applyDirDelta(S_FR_WHEEL_H, +stepFR));
    }
  }

  // L2 + D-pad LEFT/RIGHT = FWD/REAR HEIGHTS TO EQUAL (once), right/left differential (while pressed)
  if (L2p && (left || right)) {
    static bool prevCombo2 = false;
    bool combo2 = (L2p && (left || right));
    if (combo2 && !prevCombo2) equalizeFrontRearOnce();
    prevCombo2 = combo2;

    if (left) {
      // right up, left down
      nudgeTarget(S_FR_WHEEL_H, applyDirDelta(S_FR_WHEEL_H, +stepFR));
      nudgeTarget(S_RR_WHEEL_H, applyDirDelta(S_RR_WHEEL_H, +stepRR));
      nudgeTarget(S_FL_WHEEL_H, applyDirDelta(S_FL_WHEEL_H, -stepFL));
      nudgeTarget(S_RL_WHEEL_H, applyDirDelta(S_RL_WHEEL_H, -stepRL));
    } else if (right) {
      // right down, left up
      nudgeTarget(S_FR_WHEEL_H, applyDirDelta(S_FR_WHEEL_H, -stepFR));
      nudgeTarget(S_RR_WHEEL_H, applyDirDelta(S_RR_WHEEL_H, -stepRR));
      nudgeTarget(S_FL_WHEEL_H, applyDirDelta(S_FL_WHEEL_H, +stepFL));
      nudgeTarget(S_RL_WHEEL_H, applyDirDelta(S_RL_WHEEL_H, +stepRL));
    }
  }

  // L1 + D-pad UP/DOWN = wheelbase extend/retract (while pressed)
  if (L1p && up) {
    nudgeTarget(S_WHEELBASE, applyDirDelta(S_WHEELBASE, +COMMAND_STEP_US));
  }
  if (L1p && down) {
    nudgeTarget(S_WHEELBASE, applyDirDelta(S_WHEELBASE, -COMMAND_STEP_US));
  }
}

// Nav2 HEAD controls.
//   Stick Y       = persistent world pitch
//   Stick X       = yaw
//   Hold L3+X     = manual roll/"dog tilt"
//   CROSS         = raise head
//   CIRCLE        = lower head
//   L1+CROSS      = RESERVED for eye-ring toggle (so head height does not change)
//   L1+CIRCLE     = RESERVED for eye dim toggle
// Uses yaw-aware tilt mixer so "look up" stays consistent while rotating.
static void processSecondaryControls() {
  PS3BT *nav = Nav2;
  if (!Nav2->PS3NavigationConnected) return;


  // If we're in TURTLE, lock out head controls so the park pose can complete.
  if (turtleActive) return;

  // Track connect edge for "no head drift" behavior
  static bool lastConn = false;
  bool conn = Nav2->PS3NavigationConnected;
  if (conn && !lastConn) {
    g_nav2ConnectedAtMs = millis();
    g_nav2FreezeUntilMs = g_nav2ConnectedAtMs + NAV2_CONNECT_FREEZE_MS;
    g_hatCentersCaptured = false;

    // Hold head exactly where it is and sync mixer state (prevents connect twitch)
    syncHeadMixerFromServos();
    target_us[S_HEAD_ROT]      = current_us[S_HEAD_ROT];
    target_us[S_HEAD_HEIGHT]   = current_us[S_HEAD_HEIGHT];
    target_us[S_HEAD_TILT_L]   = current_us[S_HEAD_TILT_L];
    target_us[S_HEAD_TILT_R]   = current_us[S_HEAD_TILT_R];
  }
  lastConn = conn;

  // Freeze head inputs briefly after Nav2 connects (kills connect twitch)
  if (conn && (int32_t)(millis() - g_nav2FreezeUntilMs) < 0) {
    return;
  }

  // timebase for smooth integration
  static uint32_t lastHeadMs = 0;
  uint32_t nowMs = millis();
  if (lastHeadMs == 0) lastHeadMs = nowMs;
  float dt = (nowMs - lastHeadMs) / 1000.0f;
  lastHeadMs = nowMs;
  if (dt < 0.001f) dt = 0.001f;
  if (dt > 0.050f) dt = 0.050f;

  bool L3p   = btn(nav, L3);
  bool cross = btn(nav, CROSS);
  bool circle= btn(nav, CIRCLE);

  // Capture stick centers after settle
  static uint32_t lastSampleMs = 0;
  static int32_t sumX = 0;
  static int32_t sumY = 0;
  static uint8_t sampleCount = 0;

  if (!g_hatCentersCaptured) {
    uint32_t age = millis() - g_nav2ConnectedAtMs;
    if (age < g_hatSettleMs) {
      sumX = 0; sumY = 0; sampleCount = 0;
    } else if (millis() - lastSampleMs >= 25) {
      lastSampleMs = millis();
      sumX += (int16_t)hatX(nav);
      sumY += (int16_t)hatY(nav);
      sampleCount++;
      if (sampleCount >= g_hatCaptureSamples) {
        g_hatCenterX = (int16_t)(sumX / (int32_t)sampleCount);
        g_hatCenterY = (int16_t)(sumY / (int32_t)sampleCount);
        g_hatCentersCaptured = true;

        // Initialize head mixer state to "neutral and stable"
        target_us[S_HEAD_ROT]    = servoDef[S_HEAD_ROT].neutral_us;
        g_yawTargetF             = (float)target_us[S_HEAD_ROT];

        // Keep current head height target
        g_headHeightTargetF      = (float)target_us[S_HEAD_HEIGHT];

        g_pitchCmd = 0.0f;
        g_rollCmd  = 0.0f;

        // Force a first update so the tilt targets match neutral yaw
        updateTiltTargetsYawAware();
      }
    }
  }

  int16_t sx = signedHatX(hatX(nav)) - (int16_t)(g_hatCentersCaptured ? g_hatCenterX - 127 : 0);
  int16_t sy = signedHatY(hatY(nav)) - (int16_t)(g_hatCentersCaptured ? g_hatCenterY - 127 : 0);

  if (!g_hatCentersCaptured || (millis() - g_nav2ConnectedAtMs) < g_hatSettleMs) {
    sx = 0; sy = 0;
  }

  // Convert to normalized floats (-1..+1), and match head sketch convention:
  // UP = positive hy.
  float hx = (float)sx / 127.0f;
  float hy = (float)sy / 127.0f;
  hx = applyDeadF(hx, HEAD_DEAD);
  hy = applyDeadF(hy, HEAD_DEAD);
  hy = -hy; // UP = positive

  // ---- Stick jitter filter ----
  // Require non-zero stick input to be held for a few consecutive cycles before we integrate it.
  // This prevents occasional single-sample twitches from analog noise.
  // Head-stick noise filter. Increase HAT_STABLE_CYCLES if the head twitches from one-off stick noise;
  // decrease it if the controls feel delayed. HAT_STABLE_MAX only caps the internal confidence counter.
  const uint8_t HAT_STABLE_CYCLES = 3;   // 2 = snappier, 3 = smoother, 4 = very stable
  const uint8_t HAT_STABLE_MAX    = 10;  // saturating counter cap
  static uint8_t xHold = 0;
  static uint8_t yHold = 0;

  bool wantX = (fabsf(hx) > 0.0f);
  bool wantY = (fabsf(hy) > 0.0f);

  xHold = wantX ? (xHold < HAT_STABLE_MAX ? (uint8_t)(xHold + 1) : HAT_STABLE_MAX) : 0;
  yHold = wantY ? (yHold < HAT_STABLE_MAX ? (uint8_t)(yHold + 1) : HAT_STABLE_MAX) : 0;

  if (xHold < HAT_STABLE_CYCLES) hx = 0.0f;
  if (yHold < HAT_STABLE_CYCLES) hy = 0.0f;


  // Exponential taper for pitch/roll as well (more precision near center).
  hy = applyExpo(hy, 0.55f);

  // WORLD pitch (Hat Y always)
  g_pitchCmd += hy * PITCH_RATE_N_PER_S * dt;
  g_pitchCmd = clampf(g_pitchCmd, -1.0f, 1.0f);

  // Hat X: roll (if L3) else yaw
  if (L3p) {
    hx = applyExpo(hx, 0.55f);
    g_rollCmd += hx * ROLL_RATE_N_PER_S * dt;
    g_rollCmd = clampf(g_rollCmd, -1.0f, 1.0f);
  } else {
    // Exponential taper for yaw so it's easy to make tiny adjustments near center.
    hx = applyExpo(hx, 0.85f);

    // invert so stick right -> head right
    g_yawTargetF -= hx * YAW_RATE_US_PER_S * dt;
    g_yawTargetF = clampf(g_yawTargetF, (float)servoDef[S_HEAD_ROT].min_us, (float)servoDef[S_HEAD_ROT].max_us);
    target_us[S_HEAD_ROT] = (uint16_t)(g_yawTargetF + 0.5f);
  }

  // Height (CROSS raise, CIRCLE lower)
  // Reserve L1 + CROSS on Nav2 for eye-ring toggle and L1 + CIRCLE for eye dim.
  if (!btn(nav, L1)) {
    if (cross && !circle) g_headHeightTargetF += HEIGHT_RATE_US_PER_S * dt;
    if (circle && !cross) g_headHeightTargetF -= HEIGHT_RATE_US_PER_S * dt;
  }

  g_headHeightTargetF = clampf(g_headHeightTargetF, (float)servoDef[S_HEAD_HEIGHT].min_us, (float)servoDef[S_HEAD_HEIGHT].max_us);
  target_us[S_HEAD_HEIGHT] = (uint16_t)(g_headHeightTargetF + 0.5f);

  // Update tilt targets (yaw-aware)
  updateTiltTargetsYawAware();
}

// ============================ SETUP / LOOP ============================

// Drive ramp state
float cmdFL = 0, cmdFR = 0, cmdRL = 0, cmdRR = 0;
uint32_t lastDriveMs = 0;

void setup() {
  Serial.begin(115200);

  pinMode(AMP_VOL_UP_PIN, OUTPUT);
  pinMode(AMP_VOL_DN_PIN, OUTPUT);
  digitalWrite(AMP_VOL_UP_PIN, LOW);
  digitalWrite(AMP_VOL_DN_PIN, LOW);

  CHIRP.begin(CHIRP_BAUD);
  MIDI.begin(MIDI_BAUD);

  delay(1200);
  chirpSendLine("PING");
  delay(50);
  randomSeed(analogRead(A0) + analogRead(A1) + micros());

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_GLOBAL_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, LED_MAX_MA);
  FastLED.clear(true);

  uint32_t ledNow = millis();
  bodyLed1.nextChangeMs = ledNow + randomRangeUL(100, 900);
  bodyLed2.nextChangeMs = ledNow + randomRangeUL(200, 1100);
  bodyLed1.currentBrightness = chooseBodyFlickerTarget();
  bodyLed1.targetBrightness  = bodyLed1.currentBrightness;
  bodyLed2.currentBrightness = chooseBodyFlickerTarget();
  bodyLed2.targetBrightness  = bodyLed2.currentBrightness;
  eyeCurrentBrightness = random(EYE_MIN_BRIGHTNESS, EYE_MAX_BRIGHTNESS + 1);
  eyeBreathPhase = 0.0f;
  eyeNextUpdateMs = ledNow + 20;

  // Motors
  setupHighFreqPWM();
  pinMode(M_FL.pwm, OUTPUT); pinMode(M_FL.dir, OUTPUT);
  pinMode(M_FR.pwm, OUTPUT); pinMode(M_FR.dir, OUTPUT);
  pinMode(M_RL.pwm, OUTPUT); pinMode(M_RL.dir, OUTPUT);
  pinMode(M_RR.pwm, OUTPUT); pinMode(M_RR.dir, OUTPUT);
  allStop();

  // I2C + PCA9685
  Wire.begin();
  pca0.begin();
  pca1.begin();
  pca0.setPWMFreq(PCA_FREQ_HZ);
  pca1.setPWMFreq(PCA_FREQ_HZ);

  initPoseToNeutral();
  applyTurtlePose();
  applyPoseNowFromTargets();

  // USB Host Shield init
  if (Usb.Init() == -1) {
    Serial.println(F("USB Host Shield init FAILED"));
    while (1) { delay(250); }
  }
  Serial.println(F("USB Host Shield init OK"));

  Nav1->attachOnInit(onInitNav1);
  Nav2->attachOnInit(onInitNav2);

  chirpSetGlobalVolume(CHIRP_GLOBAL_BASE_VOLUME);
  vt4RecallNormalSceneAtStartup();

  Serial.println(F("Waiting for TWO PS3 Nav connections... (Nav1=DRIVE, Nav2=SERVOS)"));

  pinMode(RING_UPPER_PWM_PIN, OUTPUT);
  pinMode(RING_UPPER_DIR_PIN, OUTPUT);
  pinMode(RING_LOWER_PWM_PIN, OUTPUT);
  pinMode(RING_LOWER_DIR_PIN, OUTPUT);
  analogWrite(RING_UPPER_PWM_PIN, 0);
  analogWrite(RING_LOWER_PWM_PIN, 0);
  scheduleRingIdleIndex(0, millis());
  scheduleRingIdleIndex(1, millis());

  lastDriveMs = millis();
}

void loop() {
  Usb.Task();
  pollChirpRx();
  serviceDriveLoopRestarts();

  // Drive is allowed with ONLY the primary controller (Nav1) connected.
  // All *manual* servo/head controls are locked out unless Nav2 is also connected.
  // If Nav2 disconnects after being connected, we automatically command TURTLE.
  if (!Nav1->PS3NavigationConnected) {
    allStop();
    stopAllRings();
    stopDriveSounds();
    setStutterActive(false);
    updateStatusLeds(millis());
    return;
  }

  // Auto-turtle on Nav2 disconnect edge
  static bool prevNav2Conn = false;
  bool nav2Conn = Nav2->PS3NavigationConnected;
  if (prevNav2Conn && !nav2Conn) {
    // Save current target pose once, then command turtle
    if (!turtleActive) {
      for (uint8_t i = 0; i < SERVO_COUNT; i++) savedPose_us[i] = target_us[i];
    }
    turtleActive = true;
    applyTurtlePose();
  }
  prevNav2Conn = nav2Conn;

  uint32_t now = millis();

  // ---------------- DRIVE update (Nav1) ----------------
  float dt = (now - lastDriveMs) / 1000.0f;
  if (dt < 0.001f) dt = 0.001f;
  if (dt > 0.050f) dt = 0.050f;
  lastDriveMs = now;

  float x = 0, y = 0;
  uint8_t rawX = 0, rawY = 0;
  bool usingLeftHat = true;
  readNavStick(Nav1, x, y, rawX, rawY, usingLeftHat);

  x = applyExpo(applyDeadzone(x, DEADZONE), EXPO);
  y = applyExpo(applyDeadzone(y, DEADZONE), EXPO);

  bool l3 = Nav1->getButtonPress(L3); // strafe
  bool l2 = Nav1->getButtonPress(L2); // turbo

  float fwd = -y;     // up on stick = forward
  float strafe = 0;
  float rot = 0;

  if (!l3) {
    rot = x * TURN_GAIN;
  } else {
    strafe = x;
  }

  float tFL, tFR, tRL, tRR;
  mixMecanum(fwd, strafe, rot, tFL, tFR, tRL, tRR);

  cmdFL = slewLimit(tFL, cmdFL, RAMP_RATE, dt);
  cmdFR = slewLimit(tFR, cmdFR, RAMP_RATE, dt);
  cmdRL = slewLimit(tRL, cmdRL, RAMP_RATE, dt);
  cmdRR = slewLimit(tRR, cmdRR, RAMP_RATE, dt);

  uint8_t cap = l2 ? MAX_PWM_TURBO : MAX_PWM;
  currentDrivePwmCap = cap;
  motorWriteRaw(M_FL, cmdFL, cap);
  motorWriteRaw(M_FR, cmdFR, cap);
  motorWriteRaw(M_RL, cmdRL, cap);
  motorWriteRaw(M_RR, cmdRR, cap);

  updateDriveSoundsFromMotion();
  serviceRandomDriveFx(now);

  bool bodyMoveHeld = false;
  if (bothNavsConnected()) {
    bodyMoveHeld = dpadUp(Nav1) || dpadDown(Nav1);
  }
  updateBodyRings(now, bodyMoveHeld);

  // ---------------- SERVO inputs + motion tick (Nav2) ----------------
  static uint32_t lastCmdMs = 0;
  static uint32_t lastServoMs = 0;

  if (now - lastCmdMs >= SERVO_UPDATE_MS) {
    lastCmdMs = now;

    // Manual servo/head controls only when BOTH Nav1+Nav2 are connected
    if (Nav2->PS3NavigationConnected) {
      processPrimaryControls();   // body/wheels/wheelbase/turtle/restore (Nav1 combos)
      processSecondaryControls(); // head controls (Nav2)
    }
  }

  if (now - lastServoMs >= SERVO_UPDATE_MS) {
    uint16_t dtServo = (uint16_t)(now - lastServoMs);
    lastServoMs = now;
    updateServoMotion(dtServo);
  }

  serviceRandomServoFx(now);

  handleSoundControls();

  // Keep LED animation and Nav2 combo handling running during normal operation.
  updateStatusLeds(now);
}
