/**
 * Test arrays for holding times on and off
 */
#include <stdio.h>
void main() {

  typedef struct {
    int day;
    int hh_on;
    int mm_on;
    int hh_off;
    int mm_off;
  } period;

  period periods[] = {
    {5729, 11, 00, 14, 59},
    {5729, 15, 15, 16, 59},
  };


  printf("Hello, world\n");
  
  int count = sizeof(periods) / sizeof(periods[0]);
  int i = 0;
  for (i = 0; i < count; ++i) {
    period p = periods[i];
    int day = p.day;
    int hh_on = p.hh_on;
    int mm_on = p.mm_on;
    int hh_off = p.hh_off;
    int mm_off = p.mm_off;
    printf("Day: %i hh:mm on %02i:%02i, hh:mm off %02i:%02i\n", day, hh_on, mm_on, hh_off, mm_off);
  }
}
