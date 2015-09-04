/**
 * "Aim lights" light script
 *
 * Turn on all the lights for 30 seconds so they can be aimed
 */

// Copied from todbot 
// format of light script lines: duration, command, arg1,arg2,arg3
typedef struct _blinkm_script_line {
  uint8_t dur;
  uint8_t cmd[4];    // cmd,arg1,arg2,arg3
} blinkm_script_line;

// start dark, lighten to mid-day, then slowly fade away
blinkm_script_line script_lines[] = {
 {  1,  {'f', 1,00,00}},        // fade speed (slow)
 {  1,  {'t', 1,0,0}},          // time adjust (slower)
 { 300, {'c', 0xff,0xff,0xff}},  // stay bright for mid-day
 { 100, {'c', 0x00,0x00,0x00}},  // fade to black
};

