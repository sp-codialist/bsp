# BSP RTC Module

Hardware-abstracted Real-Time Clock (RTC) driver for STM32F4 with UTC-based time management and Unix timestamp support.

## Features

- Simple date/time management (no local time or DST complexity)
- Unix timestamp conversion (seconds since Jan 1, 1970)
- UTC-based operations for universal time handling
- Year range: 2000-2099
- Comprehensive date validation (leap years, month lengths)
- 100% test coverage (38 tests)

## API Reference

### Initialization

- `BspRtcInit()` - Initialize RTC module (call once at startup)

### Date/Time Operations

- `BspRtcSetDateTime(pDateTime)` - Set current date and time
- `BspRtcGetDateTime(pDateTime)` - Get current date and time

### Unix Timestamp Operations

- `BspRtcSetUnixTime(uUnixTime)` - Set date/time from Unix timestamp
- `BspRtcGetUnixTime(pUnixTime)` - Get current time as Unix timestamp

## Data Structures

### BspRtcDateTime_t

```c
typedef struct {
    uint16_t wYear;    // Full year (2000-2099)
    uint8_t  byMonth;  // Month (1-12)
    uint8_t  byDay;    // Day of month (1-31)
    uint8_t  byHour;   // Hour (0-23)
    uint8_t  byMinute; // Minute (0-59)
    uint8_t  bySecond; // Second (0-59)
} BspRtcDateTime_t;
```

**All times are in UTC** - no local time zones or daylight saving time.

## Error Codes

- `eBSP_RTC_ERR_NONE` - No error
- `eBSP_RTC_ERR_INVALID_PARAM` - Invalid parameter (NULL pointer, invalid date/time)
- `eBSP_RTC_ERR_HAL_ERROR` - Hardware abstraction layer error
- `eBSP_RTC_ERR_NOT_INIT` - RTC not initialized (call BspRtcInit first)

## Usage Examples

### Basic Date/Time Setting

```c
#include "bsp_rtc.h"

void setSystemTime(void) {
    // Initialize RTC module
    BspRtcError_e err = BspRtcInit();
    if (err != eBSP_RTC_ERR_NONE) {
        // Handle initialization error
        return;
    }

    // Set date to December 26, 2025 14:30:00 UTC
    BspRtcDateTime_t dateTime = {
        .wYear    = 2025,
        .byMonth  = 12,
        .byDay    = 26,
        .byHour   = 14,
        .byMinute = 30,
        .bySecond = 0
    };

    err = BspRtcSetDateTime(&dateTime);
    if (err == eBSP_RTC_ERR_NONE) {
        // RTC is now set to the specified time
    }
}
```

### Reading Current Time

```c
#include "bsp_rtc.h"
#include <stdio.h>

void displayCurrentTime(void) {
    BspRtcDateTime_t now;

    BspRtcError_e err = BspRtcGetDateTime(&now);
    if (err == eBSP_RTC_ERR_NONE) {
        printf("Current time: %04u-%02u-%02u %02u:%02u:%02u UTC\n",
               now.wYear, now.byMonth, now.byDay,
               now.byHour, now.byMinute, now.bySecond);
    }
}
```

### Unix Timestamp Operations

```c
#include "bsp_rtc.h"

void syncTimeFromServer(void) {
    // Example: Time received from NTP server or GPS
    uint32_t ntpTimestamp = 1735224600;  // Dec 26, 2025 14:30:00 UTC

    BspRtcInit();

    // Set RTC from Unix timestamp
    BspRtcError_e err = BspRtcSetUnixTime(ntpTimestamp);
    if (err == eBSP_RTC_ERR_NONE) {
        // RTC now synchronized with server time
    }
}

void logEventTimestamp(void) {
    uint32_t eventTime;

    // Get current time as Unix timestamp
    BspRtcError_e err = BspRtcGetUnixTime(&eventTime);
    if (err == eBSP_RTC_ERR_NONE) {
        // Store timestamp in event log
        printf("Event occurred at timestamp: %lu\n", eventTime);
    }
}
```

### Data Logger with Timestamps

```c
#include "bsp_rtc.h"
#include <stdio.h>

typedef struct {
    uint32_t timestamp;
    float    temperature;
    float    humidity;
} SensorReading_t;

static SensorReading_t readings[100];
static uint16_t readingCount = 0;

void logSensorData(float temp, float humidity) {
    if (readingCount < 100) {
        uint32_t currentTime;

        if (BspRtcGetUnixTime(&currentTime) == eBSP_RTC_ERR_NONE) {
            readings[readingCount].timestamp   = currentTime;
            readings[readingCount].temperature = temp;
            readings[readingCount].humidity    = humidity;
            readingCount++;
        }
    }
}

void printReadings(void) {
    BspRtcDateTime_t dt;

    for (uint16_t i = 0; i < readingCount; i++) {
        // Convert Unix timestamp to readable format
        BspRtcSetUnixTime(readings[i].timestamp);
        BspRtcGetDateTime(&dt);

        printf("%04u-%02u-%02u %02u:%02u:%02u - Temp: %.1f°C, Humidity: %.1f%%\n",
               dt.wYear, dt.byMonth, dt.byDay,
               dt.byHour, dt.byMinute, dt.bySecond,
               readings[i].temperature,
               readings[i].humidity);
    }
}
```

### Time-based Event Scheduling

```c
#include "bsp_rtc.h"
#include <stdbool.h>

typedef struct {
    BspRtcDateTime_t scheduledTime;
    void (*callback)(void);
    bool active;
} ScheduledEvent_t;

static ScheduledEvent_t events[10];

void scheduleEvent(uint16_t year, uint8_t month, uint8_t day,
                   uint8_t hour, uint8_t minute, void (*callback)(void)) {
    for (int i = 0; i < 10; i++) {
        if (!events[i].active) {
            events[i].scheduledTime.wYear    = year;
            events[i].scheduledTime.byMonth  = month;
            events[i].scheduledTime.byDay    = day;
            events[i].scheduledTime.byHour   = hour;
            events[i].scheduledTime.byMinute = minute;
            events[i].scheduledTime.bySecond = 0;
            events[i].callback = callback;
            events[i].active = true;
            break;
        }
    }
}

void checkScheduledEvents(void) {
    BspRtcDateTime_t now;

    if (BspRtcGetDateTime(&now) != eBSP_RTC_ERR_NONE) {
        return;
    }

    for (int i = 0; i < 10; i++) {
        if (events[i].active) {
            // Simple time comparison (second precision)
            if (now.wYear == events[i].scheduledTime.wYear &&
                now.byMonth == events[i].scheduledTime.byMonth &&
                now.byDay == events[i].scheduledTime.byDay &&
                now.byHour == events[i].scheduledTime.byHour &&
                now.byMinute == events[i].scheduledTime.byMinute) {

                // Execute scheduled callback
                events[i].callback();
                events[i].active = false;
            }
        }
    }
}
```

### Time Duration Calculation

```c
#include "bsp_rtc.h"

void measureElapsedTime(void) {
    uint32_t startTime, endTime;

    // Record start time
    BspRtcGetUnixTime(&startTime);

    // Do some work...
    performLongOperation();

    // Record end time
    BspRtcGetUnixTime(&endTime);

    // Calculate duration in seconds
    uint32_t durationSeconds = endTime - startTime;

    // Convert to human-readable format
    uint32_t hours   = durationSeconds / 3600;
    uint32_t minutes = (durationSeconds % 3600) / 60;
    uint32_t seconds = durationSeconds % 60;

    printf("Operation took: %lu hours, %lu minutes, %lu seconds\n",
           hours, minutes, seconds);
}
```

### Alarm/Wake-up Implementation

```c
#include "bsp_rtc.h"

void setDailyAlarm(uint8_t hour, uint8_t minute) {
    BspRtcDateTime_t now;
    BspRtcGetDateTime(&now);

    // Set alarm for today at specified time
    BspRtcDateTime_t alarmTime = now;
    alarmTime.byHour   = hour;
    alarmTime.byMinute = minute;
    alarmTime.bySecond = 0;

    // If alarm time has passed today, set for tomorrow
    uint32_t nowUnix, alarmUnix;
    BspRtcGetUnixTime(&nowUnix);

    BspRtcSetDateTime(&alarmTime);
    BspRtcGetUnixTime(&alarmUnix);

    if (alarmUnix < nowUnix) {
        // Add 24 hours (86400 seconds)
        alarmUnix += 86400;
        BspRtcSetUnixTime(alarmUnix);
    }

    // In main loop, check if alarm time reached
    // (alternatively, use STM32 RTC alarm interrupts via HAL)
}

bool checkAlarmTriggered(uint32_t alarmUnix) {
    uint32_t currentUnix;
    BspRtcGetUnixTime(&currentUnix);
    return (currentUnix >= alarmUnix);
}
```

### Leap Year Handling

```c
#include "bsp_rtc.h"

void demonstrateLeapYearSupport(void) {
    BspRtcInit();

    // Leap year (2024) - February 29 is valid
    BspRtcDateTime_t leapDay = {
        .wYear = 2024, .byMonth = 2, .byDay = 29,
        .byHour = 12, .byMinute = 0, .bySecond = 0
    };

    BspRtcError_e err = BspRtcSetDateTime(&leapDay);
    if (err == eBSP_RTC_ERR_NONE) {
        // February 29, 2024 is valid (leap year)
    }

    // Non-leap year (2023) - February 29 is invalid
    BspRtcDateTime_t invalidDate = {
        .wYear = 2023, .byMonth = 2, .byDay = 29,
        .byHour = 12, .byMinute = 0, .bySecond = 0
    };

    err = BspRtcSetDateTime(&invalidDate);
    if (err == eBSP_RTC_ERR_INVALID_PARAM) {
        // February 29, 2023 is invalid (not a leap year)
    }
}
```

### Error Handling Best Practices

```c
#include "bsp_rtc.h"

bool initializeRtcWithDefaults(void) {
    BspRtcError_e err = BspRtcInit();

    if (err != eBSP_RTC_ERR_NONE) {
        // Initialization failed - check HAL configuration
        return false;
    }

    // Try to read current time
    BspRtcDateTime_t currentTime;
    err = BspRtcGetDateTime(&currentTime);

    if (err != eBSP_RTC_ERR_NONE) {
        // Failed to read - set default time
        BspRtcDateTime_t defaultTime = {
            .wYear = 2000, .byMonth = 1, .byDay = 1,
            .byHour = 0, .byMinute = 0, .bySecond = 0
        };

        err = BspRtcSetDateTime(&defaultTime);
        if (err != eBSP_RTC_ERR_NONE) {
            return false;
        }
    }

    return true;
}

bool validateAndSetTime(const BspRtcDateTime_t* pDateTime) {
    // Pre-validation checks
    if (pDateTime == NULL) {
        return false;
    }

    if (pDateTime->wYear < 2000 || pDateTime->wYear > 2099) {
        return false;
    }

    if (pDateTime->byMonth < 1 || pDateTime->byMonth > 12) {
        return false;
    }

    if (pDateTime->byDay < 1 || pDateTime->byDay > 31) {
        return false;
    }

    // Attempt to set time
    BspRtcError_e err = BspRtcSetDateTime(pDateTime);
    return (err == eBSP_RTC_ERR_NONE);
}
```

## Important Notes

### Time Zone Handling

This module operates exclusively in **UTC**. If you need local time support:

1. Store UTC in the RTC
2. Apply time zone offset in your application
3. Handle daylight saving time in application code

```c
// Example: Convert UTC to local time (EST = UTC-5)
void getLocalTime(BspRtcDateTime_t* pLocalTime) {
    BspRtcDateTime_t utcTime;
    BspRtcGetDateTime(&utcTime);

    // Convert to Unix timestamp for easy calculation
    uint32_t utcUnix;
    BspRtcSetDateTime(&utcTime);
    BspRtcGetUnixTime(&utcUnix);

    // Apply -5 hour offset (EST)
    utcUnix -= (5 * 3600);

    // Convert back to date/time
    BspRtcSetUnixTime(utcUnix);
    BspRtcGetDateTime(pLocalTime);
}
```

### Year Range Limitations

- **Supported**: 2000-2099 (years stored as offset from 2000 in hardware)
- **Unix epoch** (1970) cannot be directly represented
- Setting Unix timestamps before year 2000 returns `eBSP_RTC_ERR_INVALID_PARAM`

### Date Validation

The module performs comprehensive validation:
- Leap years (divisible by 4, except century years not divisible by 400)
- Month lengths (28/29 for February, 30/31 for others)
- Hour (0-23), Minute (0-59), Second (0-59)

Invalid dates return `eBSP_RTC_ERR_INVALID_PARAM`.

### HAL Prerequisites

Before calling `BspRtcInit()`, ensure STM32 HAL RTC is configured:
- Clock source selected (LSE, LSI, or HSE/32)
- Prescalers configured for 1 Hz output
- RTC peripheral enabled in CubeMX or manually

### Battery Backup

If using VBAT (battery backup):
- RTC continues running during power loss
- Date/time preserved across resets
- Check for valid time after power-up (year != 2000 or compare against known minimum)

### Thread Safety

This module is **not thread-safe**. In RTOS environments, protect RTC calls with mutexes.

### Performance

- `BspRtcInit()`: One-time initialization (~1 µs)
- `BspRtcSetDateTime()`: HAL I2C write (~100 µs)
- `BspRtcGetDateTime()`: HAL I2C read (~100 µs)
- `Unix conversions`: Pure computation (~5 µs)

## Validation Rules

### Leap Year Algorithm

```
Leap year if:
  (Year divisible by 4 AND NOT divisible by 100)
  OR
  (Year divisible by 400)

Examples:
  2024 - Leap year ✓
  2000 - Leap year ✓ (divisible by 400)
  1900 - Not leap year ✗ (divisible by 100 but not 400)
  2023 - Not leap year ✗
```

### Month Lengths

```
January   - 31 days
February  - 28 days (29 in leap years)
March     - 31 days
April     - 30 days
May       - 31 days
June      - 30 days
July      - 31 days
August    - 31 days
September - 30 days
October   - 31 days
November  - 30 days
December  - 31 days
```

## Unix Timestamp Reference

```
Unix Epoch:      0           = Jan  1, 1970 00:00:00 UTC
Y2K:             946684800   = Jan  1, 2000 00:00:00 UTC
Current example: 1735224600  = Dec 26, 2025 14:30:00 UTC
32-bit limit:    2147483647  = Jan 19, 2038 03:14:07 UTC
```

**Note**: This module's year range (2000-2099) avoids the Year 2038 problem for most embedded use cases.

## Testing Coverage

- **100% line coverage** (112/112 lines)
- **38 test cases** covering:
  - Initialization and re-initialization
  - Valid and invalid date/time setting
  - Date/time retrieval with HAL error scenarios
  - Unix timestamp conversion (round-trip verification)
  - Leap year detection (2024, 2000, 1900, 2025)
  - Date validation (month 0, day 0, Feb 29 leap/non-leap)
  - Edge cases (min/max valid dates, year boundaries)
  - Error conditions (NULL pointers, not initialized, HAL failures)

## See Also

- STM32F4 Reference Manual - RTC chapter
- [bsp_common.md](bsp_common.md) - FORCE_STATIC pattern
- [Testing Guide](testing.md) - Unit testing approach
