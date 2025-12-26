# Mock library for STM32 HAL GPIO functions

set(libName mock_stm32_hal)

# Only create mock library if it doesn't already exist
if(NOT TARGET ${libName})

# HAL headers to mock
set(${libName}_HEADERS
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_gpio.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_cortex.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_adc.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_spi.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_i2c.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_can.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_tim.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_rcc.h
)

# Additional HAL headers that need to be copied (dependencies)
set(${libName}_DEPENDENCY_HEADERS
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_def.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_gpio_ex.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_adc_ex.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_dma.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_i2c_ex.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Legacy/stm32f4xx_hal_can_legacy.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_tim_ex.h
    ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/stm32f4xx_hal_rcc_ex.h
)

# Create directory for mocks
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/tests/${libName})
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests/${libName})

# Set environment for mock generation
set(ENV{MOCK_OUT} ${CMAKE_BINARY_DIR}/tests/${libName}/mocks)
set(ENV{MOCK_PREFIX} Mock)

# Copy dependency headers first
foreach(dep_header IN LISTS ${libName}_DEPENDENCY_HEADERS)
    if(EXISTS ${dep_header})
        configure_file(${dep_header} ${CMAKE_BINARY_DIR}/tests/${libName} COPYONLY)
    else()
        message(WARNING "Dependency header not found: ${dep_header}")
    endif()
endforeach()

# Copy host-compatible hal_def.h to override the STM32 version
configure_file(${CMAKE_SOURCE_DIR}/tests/cmake/host.hal_def.h
               ${CMAKE_BINARY_DIR}/tests/${libName}/stm32f4xx_hal_def.h
               COPYONLY)

# Copy stub stm32f4xx_hal.h for production code includes
configure_file(${CMAKE_SOURCE_DIR}/tests/cmake/stm32f4xx_hal.h
               ${CMAKE_BINARY_DIR}/tests/${libName}/stm32f4xx_hal.h
               COPYONLY)

# Copy stub stm32f4xx_ll_gpio.h for production code includes
configure_file(${CMAKE_SOURCE_DIR}/tests/cmake/stm32f4xx_ll_gpio.h
               ${CMAKE_BINARY_DIR}/tests/${libName}/stm32f4xx_ll_gpio.h
               COPYONLY)

# Copy stub stm32f4xx_ll_adc.h for ADC includes
configure_file(${CMAKE_SOURCE_DIR}/tests/cmake/stm32f4xx_ll_adc.h
               ${CMAKE_BINARY_DIR}/tests/${libName}/stm32f4xx_ll_adc.h
               COPYONLY)

# Copy stub stm32f4xx_ll_spi.h for SPI includes
configure_file(${CMAKE_SOURCE_DIR}/tests/cmake/stm32f4xx_ll_spi.h
               ${CMAKE_BINARY_DIR}/tests/${libName}/stm32f4xx_ll_spi.h
               COPYONLY)

# Copy stub stm32f4xx_ll_i2c.h for I2C includes
configure_file(${CMAKE_SOURCE_DIR}/tests/cmake/stm32f4xx_ll_i2c.h
               ${CMAKE_BINARY_DIR}/tests/${libName}/stm32f4xx_ll_i2c.h
               COPYONLY)

# Copy stub stm32f4xx_ll_tim.h for TIM includes
configure_file(${CMAKE_SOURCE_DIR}/tests/cmake/stm32f4xx_ll_tim.h
               ${CMAKE_BINARY_DIR}/tests/${libName}/stm32f4xx_ll_tim.h
               COPYONLY)

# Copy stub stm32f4xx_hal_can.h for CAN includes (instead of the real one)
configure_file(${CMAKE_SOURCE_DIR}/tests/cmake/stm32f4xx_hal_can.h
               ${CMAKE_BINARY_DIR}/tests/${libName}/stm32f4xx_hal_can.h
               COPYONLY)

# Copy main.h from cpu_precompiled_hal for production code includes (gpio_struct.c needs this)
configure_file(${cpu_precompiled_hal_SOURCE_DIR}/include/cpb/main.h
               ${CMAKE_BINARY_DIR}/tests/${libName}/main.h
               COPYONLY)

# Generate mocks for each header
foreach(element IN LISTS ${libName}_HEADERS)
    # Extract filename first
    get_filename_component(fileName ${element} NAME)

    # Skip stm32f4xx_hal.h since we use a custom stub
    # For stm32f4xx_hal_can.h, the custom stub is already copied above, so skip the copy here
    if(fileName STREQUAL "stm32f4xx_hal.h")
        continue()
    elseif(NOT fileName STREQUAL "stm32f4xx_hal_can.h")
        # Copy header to build directory (unless it's hal_can which is already copied as stub)
        configure_file(${element} ${CMAKE_CURRENT_BINARY_DIR}/${libName} COPYONLY)
    endif()

    # Patch stm32f4xx_hal_gpio.h to remove duplicate GPIO_PinState definition
    if(fileName STREQUAL "stm32f4xx_hal_gpio.h")
        file(READ ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} FILE_CONTENTS)
        # Remove the GPIO_PinState typedef that conflicts with hal_def.h
        string(REGEX REPLACE "typedef enum[\r\n\t ]*\\{[\r\n\t ]*GPIO_PIN_RESET[\r\n\t ]*=[\r\n\t ]*0[\r\n\t ]*,[\r\n\t ]*GPIO_PIN_SET[\r\n\t ]*\\}[\r\n\t ]*GPIO_PinState[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Also remove the comment block for GPIO Bit SET and Bit RESET enumeration if it exists
        string(REGEX REPLACE "/\\*\\*[\r\n\t ]*\\*[\r\n\t ]*@brief[ \t]+GPIO Bit SET and Bit RESET enumeration[^\n]*\n[^\n]*\\*/" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_GPIO_EXTI_Callback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_GPIO_EXTI_Callback[\r\n\t ]*\\([\r\n\t ]*uint16_t[\r\n\t ]+GPIO_Pin[\r\n\t ]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} "${FILE_CONTENTS}")
    endif()

    # Patch stm32f4xx_hal_cortex.h to remove user callbacks
    if(fileName STREQUAL "stm32f4xx_hal_cortex.h")
        file(READ ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} FILE_CONTENTS)
        # Remove HAL_SYSTICK_Callback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_SYSTICK_Callback[\r\n\t ]*\\([\r\n\t ]*void[\r\n\t ]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} "${FILE_CONTENTS}")
    endif()

    # Patch stm32f4xx_hal.h to replace hal_conf.h include and remove user callbacks
    if(fileName STREQUAL "stm32f4xx_hal.h")
        file(READ ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} FILE_CONTENTS)
        # Replace hal_conf.h include with hal_def.h
        string(REGEX REPLACE "#include[ \t]+\"stm32f4xx_hal_conf\\.h\"" "#include \"stm32f4xx_hal_def.h\"" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_MspInit declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_MspInit[\r\n\t ]*\\([\r\n\t ]*void[\r\n\t ]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_MspDeInit declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_MspDeInit[\r\n\t ]*\\([\r\n\t ]*void[\r\n\t ]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove duplicate HAL_TickFreqTypeDef definition (already in hal_def.h)
        string(REGEX REPLACE "typedef enum[\r\n\t ]*\\{[^}]*HAL_TICK_FREQ_[^}]*\\}[\r\n\t ]*HAL_TickFreqTypeDef[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove __IO uwTick variable declaration (not available in host environment)
        string(REGEX REPLACE "extern[ \t]+__IO[ \t]+uint32_t[ \t]+uwTick[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove uwTickPrio variable declaration
        string(REGEX REPLACE "extern[ \t]+HAL_TickFreqTypeDef[ \t]+uwTickPrio[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove uwTickFreq variable declaration
        string(REGEX REPLACE "extern[ \t]+HAL_TickFreqTypeDef[ \t]+uwTickFreq[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} "${FILE_CONTENTS}")
    endif()

    # Patch stm32f4xx_hal_adc.h to remove user callback and register callback functions
    if(fileName STREQUAL "stm32f4xx_hal_adc.h")
        file(READ ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} FILE_CONTENTS)
        # Remove HAL_ADC_ConvCpltCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_ADC_ConvCpltCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_ADC_RegisterCallback and HAL_ADC_UnRegisterCallback (not needed for tests)
        string(REGEX REPLACE "HAL_StatusTypeDef[\r\n\t ]+HAL_ADC_RegisterCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "HAL_StatusTypeDef[\r\n\t ]+HAL_ADC_UnRegisterCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} "${FILE_CONTENTS}")
    endif()

    # Patch stm32f4xx_hal_spi.h to remove user callbacks and register callback functions
    if(fileName STREQUAL "stm32f4xx_hal_spi.h")
        file(READ ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} FILE_CONTENTS)
        # Remove HAL_SPI_TxCpltCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_SPI_TxCpltCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_SPI_RxCpltCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_SPI_RxCpltCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_SPI_TxRxCpltCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_SPI_TxRxCpltCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_SPI_ErrorCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_SPI_ErrorCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_SPI_RegisterCallback and HAL_SPI_UnRegisterCallback (not needed for tests)
        string(REGEX REPLACE "HAL_StatusTypeDef[\r\n\t ]+HAL_SPI_RegisterCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "HAL_StatusTypeDef[\r\n\t ]+HAL_SPI_UnRegisterCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} "${FILE_CONTENTS}")
    endif()

    # Patch stm32f4xx_hal_i2c.h to remove user callbacks and register callback functions
    if(fileName STREQUAL "stm32f4xx_hal_i2c.h")
        file(READ ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} FILE_CONTENTS)
        # Remove HAL_I2C_MasterTxCpltCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_I2C_MasterTxCpltCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_I2C_MasterRxCpltCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_I2C_MasterRxCpltCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_I2C_MemTxCpltCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_I2C_MemTxCpltCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_I2C_MemRxCpltCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_I2C_MemRxCpltCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_I2C_ErrorCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_I2C_ErrorCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_I2C_RegisterCallback and HAL_I2C_UnRegisterCallback (not needed for tests)
        string(REGEX REPLACE "HAL_StatusTypeDef[\r\n\t ]+HAL_I2C_RegisterCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "HAL_StatusTypeDef[\r\n\t ]+HAL_I2C_UnRegisterCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_I2C_RegisterAddrCallback and HAL_I2C_UnRegisterAddrCallback (not needed for tests)
        string(REGEX REPLACE "HAL_StatusTypeDef[\r\n\t ]+HAL_I2C_RegisterAddrCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "HAL_StatusTypeDef[\r\n\t ]+HAL_I2C_UnRegisterAddrCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove pI2C_AddrCallbackTypeDef typedef
        string(REGEX REPLACE "typedef[\r\n\t ]+void[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*pI2C_AddrCallbackTypeDef[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} "${FILE_CONTENTS}")
    endif()

    # Patch stm32f4xx_hal_tim.h to remove user callbacks and register callback functions
    if(fileName STREQUAL "stm32f4xx_hal_tim.h")
        file(READ ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} FILE_CONTENTS)
        # Remove HAL_TIM_RegisterCallback and HAL_TIM_UnRegisterCallback (not needed for tests)
        string(REGEX REPLACE "HAL_StatusTypeDef[\r\n\t ]+HAL_TIM_RegisterCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "HAL_StatusTypeDef[\r\n\t ]+HAL_TIM_UnRegisterCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")

        # Add include guard before TIM_HandleTypeDef (looking for typedef struct __TIM_HandleTypeDef)
        string(REGEX REPLACE "(typedef struct __TIM_HandleTypeDef[^}]*\\}[\r\n\t ]*TIM_HandleTypeDef[\r\n\t ]*;)" "#ifndef TIM_HANDLETYPEDEF\n#define TIM_HANDLETYPEDEF\n\\1\n#endif /* TIM_HANDLETYPEDEF */" FILE_CONTENTS "${FILE_CONTENTS}")

        # Add include guards for TIM channel definitions
        string(REGEX REPLACE "(#define TIM_CHANNEL_1[\r\n\t ]+0x00000000U)" "#ifndef TIM_CHANNEL_1\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define TIM_CHANNEL_2[\r\n\t ]+0x00000004U)" "#ifndef TIM_CHANNEL_2\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define TIM_CHANNEL_3[\r\n\t ]+0x00000008U)" "#ifndef TIM_CHANNEL_3\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define TIM_CHANNEL_4[\r\n\t ]+0x0000000CU)" "#ifndef TIM_CHANNEL_4\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define TIM_CHANNEL_ALL[\r\n\t ]+0x0000003CU)" "#ifndef TIM_CHANNEL_ALL\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")

        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} "${FILE_CONTENTS}")
    endif()

    # Patch stm32f4xx_hal_rcc.h to add include guards for types already in hal_def.h
    if(fileName STREQUAL "stm32f4xx_hal_rcc.h")
        file(READ ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} FILE_CONTENTS)

        # Add include guard before RCC_ClkInitTypeDef
        string(REGEX REPLACE "(typedef struct[\r\n\t ]*\\{[^}]*APB2CLKDivider[^}]*\\}[\r\n\t ]*RCC_ClkInitTypeDef[\r\n\t ]*;)" "#ifndef RCC_CLKINITTYPEDEF\n#define RCC_CLKINITTYPEDEF\n\\1\n#endif /* RCC_CLKINITTYPEDEF */" FILE_CONTENTS "${FILE_CONTENTS}")

        # Add include guards for RCC clock divider definitions (match the actual format with RCC_CFGR_PPRE1_DIVx)
        string(REGEX REPLACE "(#define RCC_HCLK_DIV1[\r\n\t ]+RCC_CFGR_[A-Z0-9_]+)" "#ifndef RCC_HCLK_DIV1\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define RCC_HCLK_DIV2[\r\n\t ]+RCC_CFGR_[A-Z0-9_]+)" "#ifndef RCC_HCLK_DIV2\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define RCC_HCLK_DIV4[\r\n\t ]+RCC_CFGR_[A-Z0-9_]+)" "#ifndef RCC_HCLK_DIV4\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define RCC_HCLK_DIV8[\r\n\t ]+RCC_CFGR_[A-Z0-9_]+)" "#ifndef RCC_HCLK_DIV8\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define RCC_HCLK_DIV16[\r\n\t ]+RCC_CFGR_[A-Z0-9_]+)" "#ifndef RCC_HCLK_DIV16\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")

        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} "${FILE_CONTENTS}")
    endif()

    # Patch stm32f4xx_hal_can.h to remove user callbacks and add include guards for types already in hal_def.h
    if(fileName STREQUAL "stm32f4xx_hal_can.h")
        file(READ ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} FILE_CONTENTS)

        # Add include guard before HAL_CAN_StateTypeDef enum
        string(REGEX REPLACE "(typedef enum[\r\n\t ]*\\{[^}]*HAL_CAN_STATE_[^}]*\\}[\r\n\t ]*HAL_CAN_StateTypeDef[\r\n\t ]*;)" "#ifndef HAL_CAN_STATE_TYPEDEF\n#define HAL_CAN_STATE_TYPEDEF\n\\1\n#endif /* HAL_CAN_STATE_TYPEDEF */" FILE_CONTENTS "${FILE_CONTENTS}")

        # Add include guard before CAN_HandleTypeDef
        string(REGEX REPLACE "(#if USE_HAL_CAN_REGISTER_CALLBACKS == 1[\r\n\t ]+typedef struct __CAN_HandleTypeDef[\r\n\t ]+#else[\r\n\t ]+typedef struct[\r\n\t ]+#endif[^}]*\\}[\r\n\t ]*CAN_HandleTypeDef[\r\n\t ]*;)" "#ifndef CAN_HANDLETYPEDEF\n#define CAN_HANDLETYPEDEF\n\\1\n#endif /* CAN_HANDLETYPEDEF */" FILE_CONTENTS "${FILE_CONTENTS}")

        # Add include guard before CAN_RxHeaderTypeDef
        string(REGEX REPLACE "(typedef struct[\r\n\t ]*\\{[^}]*FilterMatchIndex[^}]*\\}[\r\n\t ]*CAN_RxHeaderTypeDef[\r\n\t ]*;)" "#ifndef CAN_RXHEADERTYPEDEF\n#define CAN_RXHEADERTYPEDEF\n\\1\n#endif /* CAN_RXHEADERTYPEDEF */" FILE_CONTENTS "${FILE_CONTENTS}")

        # Add include guard before CAN_TxHeaderTypeDef
        string(REGEX REPLACE "(typedef struct[\r\n\t ]*\\{[^}]*TransmitGlobalTime[^}]*\\}[\r\n\t ]*CAN_TxHeaderTypeDef[\r\n\t ]*;)" "#ifndef CAN_TXHEADERTYPEDEF\n#define CAN_TXHEADERTYPEDEF\n\\1\n#endif /* CAN_TXHEADERTYPEDEF */" FILE_CONTENTS "${FILE_CONTENTS}")

        # Add include guard before CAN_FilterTypeDef
        string(REGEX REPLACE "(typedef struct[\r\n\t ]*\\{[^}]*SlaveStartFilterBank[^}]*\\}[\r\n\t ]*CAN_FilterTypeDef[\r\n\t ]*;)" "#ifndef CAN_FILTERTYPEDEF\n#define CAN_FILTERTYPEDEF\n\\1\n#endif /* CAN_FILTERTYPEDEF */" FILE_CONTENTS "${FILE_CONTENTS}")

        # Add include guards for error code macros
        string(REGEX REPLACE "(#define HAL_CAN_ERROR_NONE)" "#ifndef HAL_CAN_ERROR_NONE\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define HAL_CAN_ERROR_EWG)" "#ifndef HAL_CAN_ERROR_EWG\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define HAL_CAN_ERROR_EPV)" "#ifndef HAL_CAN_ERROR_EPV\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define HAL_CAN_ERROR_BOF)" "#ifndef HAL_CAN_ERROR_BOF\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define HAL_CAN_ERROR_STF)" "#ifndef HAL_CAN_ERROR_STF\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define HAL_CAN_ERROR_FOR)" "#ifndef HAL_CAN_ERROR_FOR\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define HAL_CAN_ERROR_ACK)" "#ifndef HAL_CAN_ERROR_ACK\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define HAL_CAN_ERROR_BR)" "#ifndef HAL_CAN_ERROR_BR\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define HAL_CAN_ERROR_BD)" "#ifndef HAL_CAN_ERROR_BD\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "(#define HAL_CAN_ERROR_CRC)" "#ifndef HAL_CAN_ERROR_CRC\n\\1\n#endif" FILE_CONTENTS "${FILE_CONTENTS}")

        # Remove HAL_CAN_RxFifo0MsgPendingCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_CAN_RxFifo0MsgPendingCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_CAN_RxFifo1MsgPendingCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_CAN_RxFifo1MsgPendingCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_CAN_TxMailbox0CompleteCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_CAN_TxMailbox0CompleteCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_CAN_TxMailbox1CompleteCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_CAN_TxMailbox1CompleteCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_CAN_TxMailbox2CompleteCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_CAN_TxMailbox2CompleteCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_CAN_ErrorCallback declaration (implemented by user code, not mocked)
        string(REGEX REPLACE "void[\r\n\t ]+HAL_CAN_ErrorCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        # Remove HAL_CAN_RegisterCallback and HAL_CAN_UnRegisterCallback (not needed for tests)
        string(REGEX REPLACE "HAL_StatusTypeDef[\r\n\t ]+HAL_CAN_RegisterCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        string(REGEX REPLACE "HAL_StatusTypeDef[\r\n\t ]+HAL_CAN_UnRegisterCallback[\r\n\t ]*\\([^)]*\\)[\r\n\t ]*;" "" FILE_CONTENTS "${FILE_CONTENTS}")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName} "${FILE_CONTENTS}")
    endif()

    # Generate mock
    message(STATUS "Creating mock for ${fileName}...")
    execute_process(
        COMMAND ruby ${CMAKE_SOURCE_DIR}/tests/cmake/create_mocks.rb
                ${CMAKE_CURRENT_BINARY_DIR}/${libName}/${fileName}
        RESULT_VARIABLE mock_result
    )

    if(NOT mock_result EQUAL 0)
        message(WARNING "Failed to generate mock for ${fileName}")
    endif()
endforeach()

# Create mock library
add_library(${libName} STATIC)

# Add generated mock sources
file(GLOB_RECURSE ${libName}_MOCK_SOURCES $ENV{MOCK_OUT}/*.c)
file(GLOB_RECURSE ${libName}_MOCK_HEADERS $ENV{MOCK_OUT}/*.h)

target_sources(${libName}
    PRIVATE
        ${${libName}_MOCK_SOURCES}
)

target_include_directories(${libName}
        PUBLIC
            $ENV{MOCK_OUT}
            ${CMAKE_BINARY_DIR}/tests/${libName}
            ${cpu_precompiled_hal_SOURCE_DIR}/include/cpb
            ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Drivers/STM32F4xx_HAL_Driver/Inc
            ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Drivers/CMSIS/Device/ST/STM32F4xx/Include
            ${cpu_precompiled_hal_SOURCE_DIR}/include/stm32cubef4/Drivers/CMSIS/Core/Include
    )

target_link_libraries(${libName}
    PUBLIC
        cmock::framework
)

target_compile_definitions(${libName}
    PUBLIC
        STM32F429xx
)

endif() # NOT TARGET ${libName}

unset(libName)
