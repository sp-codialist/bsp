# Bring cmock to the test
set (libName cmock)

CPMAddPackage (
    NAME ${libName}
    VERSION ${GITHUB_BRANCH_cmock}
    URL https://github.com/ThrowTheSwitch/CMock/archive/refs/tags/v${GITHUB_BRANCH_cmock}.tar.gz
    URL_HASH SHA256=${GITHUB_BRANCH_cmock_sha}
    OPTIONS
    DOWNLOAD_ONLY
)

if (${${libName}_ADDED})
    set (ENV{CMOCK_DIR} ${${libName}_SOURCE_DIR})

    add_library (${libName} STATIC)
    add_library (${libName}::framework ALIAS ${libName})

    # Makes the mock
    target_sources (${libName}
    PRIVATE
        ${cmock_SOURCE_DIR}/src/cmock.c
    )

    target_include_directories (${libName}
    PUBLIC
    ${cmock_SOURCE_DIR}/src
    )

    set(${libName}_PUBLIC_HEADERS
    src/cmock.h
    src/cmock_internals.h
    )

    set_target_properties (${libName}
    PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
        PUBLIC_HEADER "${cmock_PUBLIC_HEADERS}"
        EXPORT_NAME framework
    )

    target_link_libraries (${libName}
    unity
    )


    target_compile_options (${libName}
    PRIVATE
        $<$<C_COMPILER_ID:Clang>:-Wcast-align
                                    -Wcast-qual
                                    -Wconversion
                                    -Wexit-time-destructors
                                    -Wglobal-constructors
                                    -Wmissing-noreturn
                                    -Wmissing-prototypes
                                    -Wno-missing-braces
                                    -Wold-style-cast
                                    -Wshadow
                                    -Wweak-vtables
                                    -Werror
                                    -Wall>
        $<$<C_COMPILER_ID:GNU>:-Waddress
                                -Waggregate-return
                                -Wformat-nonliteral
                                -Wformat-security
                                -Wformat
                                -Winit-self
                                -Wmissing-declarations
                                -Wmissing-include-dirs
                                -Wno-multichar
                                -Wno-parentheses
                                -Wno-type-limits
                                -Wno-unused-parameter
                                -Wunreachable-code
                                -Wwrite-strings
                                -Wpointer-arith
                                -Werror
                                -Wall>
        $<$<C_COMPILER_ID:MSVC>:/Wall>
    )
endif ()

unset (libName)
