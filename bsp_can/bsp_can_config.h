/**
 * @file bsp_can_config.h
 * @brief CAN BSP module compile-time configuration options
 *
 * This file provides configuration constants for the CAN BSP module.
 * Users can override these defaults by defining values before including
 * this header or by modifying this file directly for project-specific tuning.
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/* --- Memory Configuration --- */

/**
 * @brief Maximum number of CAN instances supported.
 * Typically 2 for STM32 devices with CAN1 and CAN2.
 */
#ifndef BSP_CAN_MAX_INSTANCES
    #define BSP_CAN_MAX_INSTANCES (2u)
#endif

/**
 * @brief TX queue depth (total entries across all priorities).
 * Each entry is ~16 bytes. Recommended: 16-64.
 * Memory impact: BSP_CAN_TX_QUEUE_DEPTH × 16 bytes per instance.
 */
#ifndef BSP_CAN_TX_QUEUE_DEPTH
    #define BSP_CAN_TX_QUEUE_DEPTH (32u)
#endif

/**
 * @brief RX circular buffer depth.
 * Must be power of 2 for efficient modulo operations.
 * Recommended: 8-32 depending on reception rate.
 * Memory impact: BSP_CAN_RX_BUFFER_DEPTH × 16 bytes per instance.
 */
#ifndef BSP_CAN_RX_BUFFER_DEPTH
    #define BSP_CAN_RX_BUFFER_DEPTH (16u)
#endif

/**
 * @brief Number of priority levels for TX queue.
 * Valid values: 2, 4, 8. Priority 0 = highest.
 * TX queue capacity is distributed equally across priority levels.
 */
#ifndef BSP_CAN_PRIORITY_LEVELS
    #define BSP_CAN_PRIORITY_LEVELS (8u)
#endif

/**
 * @brief Maximum number of hardware filters per CAN instance.
 * STM32 supports up to 14 filter banks per CAN peripheral.
 */
#ifndef BSP_CAN_MAX_FILTERS
    #define BSP_CAN_MAX_FILTERS (14u)
#endif

/* --- Feature Configuration --- */

/**
 * @brief Enable statistics counters (TX/RX/error counts).
 * Set to 1 to enable, 0 to disable.
 * When enabled, adds ~16 bytes per instance and BspCanGetStatistics() API.
 * Recommended: 1 (enabled) for debugging and monitoring.
 */
#ifndef BSP_CAN_ENABLE_STATISTICS
    #define BSP_CAN_ENABLE_STATISTICS (1u)
#endif

/* --- Validation --- */

#if (BSP_CAN_PRIORITY_LEVELS != 2) && (BSP_CAN_PRIORITY_LEVELS != 4) && (BSP_CAN_PRIORITY_LEVELS != 8)
    #error "BSP_CAN_PRIORITY_LEVELS must be 2, 4, or 8"
#endif

#if (BSP_CAN_TX_QUEUE_DEPTH < BSP_CAN_PRIORITY_LEVELS)
    #error "BSP_CAN_TX_QUEUE_DEPTH must be >= BSP_CAN_PRIORITY_LEVELS"
#endif

#if (BSP_CAN_RX_BUFFER_DEPTH < 4) || (BSP_CAN_RX_BUFFER_DEPTH > 128)
    #error "BSP_CAN_RX_BUFFER_DEPTH must be between 4 and 128"
#endif

#ifdef __cplusplus
}
#endif
