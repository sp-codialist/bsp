#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>

/** Callback function type for software timer expiration. */
typedef void (*SWTimerCallbackFunction)(void);

/**
 * Timer module structure that holds configuration and state for each timer.
 */
typedef struct swtimer_module
{
    uint32_t                expiration;        /**< Tick value when the timer expires */
    uint32_t                interval;          /**< Timer interval in ticks */
    SWTimerCallbackFunction pCallbackFunction; /**< Function to call when timer expires */
    bool                    active;            /**< Flag indicating if timer is currently running */
    bool                    periodic;          /**< Flag indicating if timer should restart after expiration */
} SWTimerModule;

/**
 * @brief Initialize software timer module. Initializes all timers as not active.
 *
 * @param pSwTimerModule Pointer to the software timer module to initialize.
 * @return true on success
 */
bool SWTimerInit(SWTimerModule* const pSwTimerModule);

/**
 * @brief Start a software timer.
 *        Timer must be pre-configured with interval, callback, and periodic flag.
 *
 * @param pSwTimerModule Pointer to the software timer module.
 * @return true on success, false if parameter is invalid
 */
bool SWTimerStart(SWTimerModule* const pSwTimerModule);

/**
 * @brief Stop a running software timer.
 *
 * @param pSwTimerModule Pointer to the software timer module to stop.
 */
void SWTimerStop(SWTimerModule* const pSwTimerModule);

/**
 * @brief Process software timer - must be called periodically from main loop or tick interrupt.
 *        Checks if timer has expired and calls callback if necessary.
 *
 * @param pSwTimerModule Pointer to the software timer module to process.
 */
void SWTimerProcess(SWTimerModule* const pSwTimerModule);

/**
 * @brief Check if a software timer is currently active/running.
 *
 * @param pSwTimerModule Pointer to the software timer module to check.
 * @return true if timer is active, false otherwise
 */
bool SWTimerIsActive(const SWTimerModule* const pSwTimerModule);

/**
 * @brief Get remaining time in milliseconds until timer expiration.
 *
 * @param pSwTimerModule Pointer to the software timer module.
 * @return Remaining time in milliseconds, or 0 if timer is not active
 */
uint32_t SWTimerGetRemaining(const SWTimerModule* const pSwTimerModule);

#ifdef __cplusplus
}
#endif
