#include <stddef.h>

#include "bsp_compiler_attributes.h"

#include "bsp_swtimer.h"
#include "stm32f4xx_hal.h"

/** Maximum number of software timers that can be registered */
#define MAX_SW_TIMERS 16

/** Static array to hold pointers to all registered timers */
FORCE_STATIC SWTimerModule* registeredTimers[MAX_SW_TIMERS] = {NULL};

/** Count of currently registered timers */
FORCE_STATIC uint8_t registeredTimerCount = 0;

/**
 * @brief Initialize software timer module by registering it for automatic processing.
 *        Does not clear timer contents - user should pre-initialize the structure.
 *
 * @param pSwTimerModule Pointer to the software timer module to initialize.
 * @return true on success, false if pointer is NULL or registry is full
 */
bool SWTimerInit(SWTimerModule* const pSwTimerModule)
{
    bool result = false;

    do
    {
        if (pSwTimerModule == NULL)
        {
            break;
        }

        // Check if timer is already registered
        for (uint8_t i = 0; i < registeredTimerCount; i++)
        {
            if (registeredTimers[i] == pSwTimerModule)
            {
                result = true; // Already registered
                break;
            }
        }

        if (result)
        {
            break;
        }

        // Register timer if space available
        if (registeredTimerCount < MAX_SW_TIMERS)
        {
            registeredTimers[registeredTimerCount++] = pSwTimerModule;
            result                                   = true;
        }
    } while (false);

    return result;
}

/**
 * @brief Start a software timer.
 *        Timer must be pre-configured with interval, callback, and periodic flag.
 *
 * @param pSwTimerModule Pointer to the software timer module.
 * @return true on success, false if parameter is invalid
 */
bool SWTimerStart(SWTimerModule* const pSwTimerModule)
{
    bool result = false;

    do
    {
        if (pSwTimerModule == NULL)
        {
            break;
        }

        uint32_t currentTick       = HAL_GetTick();
        pSwTimerModule->expiration = currentTick + pSwTimerModule->interval;
        pSwTimerModule->active     = true;

        result = true;
    } while (false);

    return result;
}

/**
 * @brief Stop a running software timer.
 *
 * @param pSwTimerModule Pointer to the software timer module to stop.
 */
void SWTimerStop(SWTimerModule* const pSwTimerModule)
{
    do
    {
        if (pSwTimerModule == NULL)
        {
            break;
        }

        pSwTimerModule->active = false;
    } while (false);
}

/**
 * @brief Process software timer - must be called periodically.
 *        Checks if timer has expired and calls callback if necessary.
 *
 * @param pSwTimerModule Pointer to the software timer module to process.
 */
void SWTimerProcess(SWTimerModule* const pSwTimerModule)
{
    do
    {
        if (pSwTimerModule == NULL || !pSwTimerModule->active)
        {
            break;
        }

        uint32_t currentTick = HAL_GetTick();

        // Check for timer expiration (handles tick counter rollover)
        if ((currentTick - pSwTimerModule->expiration) < 0x80000000UL)
        {
            // Timer has expired
            if (pSwTimerModule->pCallbackFunction != NULL)
            {
                pSwTimerModule->pCallbackFunction();
            }

            if (pSwTimerModule->periodic)
            {
                // Restart periodic timer
                pSwTimerModule->expiration = currentTick + pSwTimerModule->interval;
            }
            else
            {
                // One-shot timer - deactivate
                pSwTimerModule->active = false;
            }
        }
    } while (false);
}

/**
 * @brief Check if a software timer is currently active/running.
 *
 * @param pSwTimerModule Pointer to the software timer module to check.
 * @return true if timer is active, false otherwise
 */
bool SWTimerIsActive(const SWTimerModule* const pSwTimerModule)
{
    bool result = false;

    do
    {
        if (pSwTimerModule == NULL)
        {
            break;
        }

        result = pSwTimerModule->active;
    } while (false);

    return result;
}

/**
 * @brief Get remaining time in milliseconds until timer expiration.
 *
 * @param pSwTimerModule Pointer to the software timer module.
 * @return Remaining time in milliseconds, or 0 if timer is not active
 */
uint32_t SWTimerGetRemaining(const SWTimerModule* const pSwTimerModule)
{
    uint32_t remaining = 0;

    do
    {
        if (pSwTimerModule == NULL || !pSwTimerModule->active)
        {
            break;
        }

        uint32_t currentTick = HAL_GetTick();

        // Check if timer has already expired
        if ((currentTick - pSwTimerModule->expiration) < 0x80000000UL)
        {
            break;
        }

        // Calculate remaining time
        remaining = (pSwTimerModule->expiration - currentTick);
    } while (false);

    return remaining;
}

/**
 * @brief SysTick interrupt callback to process all registered software timers.
 * This function is called every 1 ms by the SysTick interrupt.
 */
void HAL_SYSTICK_Callback(void)
{
    // Process all registered timers
    for (uint8_t i = 0; i < registeredTimerCount; i++)
    {
        if (registeredTimers[i] != NULL)
        {
            SWTimerProcess(registeredTimers[i]);
        }
    }
}
