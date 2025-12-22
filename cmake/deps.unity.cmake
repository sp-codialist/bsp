# Unity Test Framework
CPMAddPackage(
    NAME unity
    GITHUB_REPOSITORY ThrowTheSwitch/Unity
    VERSION ${UNITY_VERSION}
    OPTIONS
        "UNITY_EXTENSION_FIXTURE ON"
        "UNITY_EXTENSION_MEMORY ON"
)

# CMock Mocking Framework
CPMAddPackage(
    NAME cmock
    GITHUB_REPOSITORY ThrowTheSwitch/CMock
    VERSION ${CMOCK_VERSION}
    OPTIONS
        "CMOCK_MOCK_PREFIX Mock"
)

if(unity_ADDED)
    # Configure Unity to work with embedded targets
    target_compile_definitions(unity PUBLIC
        UNITY_INCLUDE_CONFIG_H
        UNITY_FIXTURE_NO_EXTRAS
    )

    # Make Unity available
    set(UNITY_INCLUDE_DIRS ${unity_SOURCE_DIR}/src ${unity_SOURCE_DIR}/extras/fixture/src ${unity_SOURCE_DIR}/extras/memory/src)
    set(UNITY_LIBRARIES unity)
endif()

if(cmock_ADDED)
    # Make CMock available
    set(CMOCK_INCLUDE_DIRS ${cmock_SOURCE_DIR}/src)
    set(CMOCK_LIBRARIES cmock)
endif()
