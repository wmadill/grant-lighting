/**
 * benches.h -- Collection of bench control definitions and functions.
 *
 * TODO convert to a library
 *
 * @author Bill <bill@jamimi.com>
 */

#ifndef BENCHES_H
#define BENCHES_H

#define hm2min(h, m) (((h) * 60) + (m))

// TODO extend the structure to include years and months. It only
// works now for days within one month.
// A Wakeperiod is a day (of the month), a wake time and a sleep time;
// all are ints. The wake and sleep times are in minutes since
// midnight. The "nm2min()" macro converts an hour and minute
// to make it easier to read.
typedef struct {
    int day;
    int wake;
    int sleep;
} Wakeperiod;

const byte ASLEEP = 0;
const byte AWAKE = 1;

void ledOn (const Port ledPort, const byte mask) {
    if (mask & 1) {
        ledPort.digiWrite(1);
    }
    if (mask & 2) {
        ledPort.digiWrite2(1);
    }
}

void ledOff (const Port ledPort, const byte mask) {
    if (mask & 1) {
        ledPort.digiWrite(0);
    }
    if (mask & 2) {
        ledPort.digiWrite2(0);
    }
}

void doFlash(const Port ledPort, const byte mask, const byte cnt) {
    byte cntr = cnt;
    while (cntr > 0) {
        cntr--;
        ledOn(ledPort, mask);
        Sleepy::loseSomeTime(50);
        ledOff(ledPort, mask);
        if (cntr < 1) break;
        Sleepy::loseSomeTime(250);
    }
}

int toMin(const unsigned char hh, const unsigned char mm) {
    return (hh * 60) + mm;
}


byte checkAwake(const DateTime dt, Wakeperiod* ps, const byte num_periods) {
    // Hold the current period index so scanning the Wakeperiod array
    // starts where it left off rather than always at the beginning
    static byte i = 0;

    for (; i < num_periods; i++) {
        Wakeperiod wp = ps[i];
        if (dt.day() > wp.day) continue;
        if (dt.day() < wp.day) return ASLEEP;
        int dt_min = toMin(dt.hour(), dt.minute());
        if (dt_min < wp.wake) return ASLEEP;
        if (dt_min < wp.sleep) return AWAKE;
    }
    return ASLEEP;
}

void maxmOff(DeviceI2C maxm) {
    maxm.send();
    maxm.write('o');
    maxm.stop();
    maxm.send();
    maxm.write('n');
    maxm.write(0);
    maxm.write(0);
    maxm.write(0);
    maxm.stop();
}

void maxmRun(DeviceI2C maxm) {
    maxm.send();
    maxm.write('p');
    maxm.write(0);
    maxm.write(1);
    maxm.write(0);
    maxm.stop();
}
#endif
