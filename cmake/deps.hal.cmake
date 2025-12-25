include(CMakePackageConfigHelpers)
# dependency on the toolchain is brought by CPM as a package
CPMAddPackage (
  NAME cpu_precompiled_hal
  URL "https://github.com/sp-codialist/stm32cubef4/releases/download/${STM32CubeF4_VERSION}/cpb-hal-1.28.3-gnuarm14.3.tar.gz"
  URL_HASH SHA256=${STM32CubeF4_CHECKSUM}
  OPTIONS
    DOWNLOAD_ONLY ${ENABLE_TESTING}
)
