# Bring unity test framework to the test
set (libName unity)

CPMAddPackage (
    NAME ${libName}
    VERSION ${GITHUB_BRANCH_${libName}}
    URL https://github.com/ThrowTheSwitch/Unity/archive/refs/tags/v${GITHUB_BRANCH_${libName}}.tar.gz
    URL_HASH SHA256=${GITHUB_BRANCH_${libName}_sha}
)

if (${${libName}_ADDED})
    set(ENV{UNITY_DIR} ${${libName}_SOURCE_DIR})
endif ()

unset (libName)
