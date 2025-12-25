# BSP Common Utilities

Common utilities and compiler attributes shared across all BSP modules.

## Overview

The `bsp_common` module provides shared macros and compiler attributes that enable better testability while maintaining encapsulation in production code.

## FORCE_STATIC Pattern

The `FORCE_STATIC` macro enables testing of internal functions without breaking encapsulation:

- **In production builds**: `FORCE_STATIC` = `static` (symbols are encapsulated)
- **In unit tests**: `FORCE_STATIC` = empty (symbols are global for testing)
- **Automatic**: Configured via `UNITY_UNIT_TESTS` preprocessor define

## Usage Example

```c
// In source file
#include "bsp_compiler_attributes.h"

FORCE_STATIC uint8_t moduleState = 0;
FORCE_STATIC void processInternal(void) { /* ... */ }

// In unit test
extern uint8_t moduleState;
extern void processInternal(void);

void tearDown(void) { moduleState = 0; }
void test_ProcessInternal(void) { processInternal(); }
```

## Benefits

- Direct testing of internal functions
- Maintains encapsulation in production
- Zero runtime overhead

## See Also

- [BSP GPIO](bsp_gpio.md)
- [Testing Guide](testing.md)
