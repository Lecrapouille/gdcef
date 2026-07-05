# Shared compiler warning flags for gdcef browser and subprocess SConstruct files.

UNIX_CXX_WARNING_ENABLE = [
    '-Wall', '-Wextra', '-Wuninitialized', '-Wundef',
    '-Wunused', '-Wunused-result', '-Wunused-parameter',
    '-Wtype-limits', '-Wcast-align', '-Wcast-qual',
    '-Wconversion', '-Wfloat-equal', '-Wpointer-arith',
    '-Wswitch-enum', '-Wpacked', '-Wold-style-cast',
    '-Wdeprecated', '-Wvariadic-macros', '-Wvla',
    '-Wsign-conversion',
]

# Our code and CEF third-party headers.
UNIX_CXX_WARNING_DISABLE = [
    '-Wno-unused-parameter',
    '-Wno-conversion',
    '-Wno-cast-qual',
    '-Wno-float-equal',
    '-Wno-unused-but-set-parameter',
    '-Wno-old-style-cast',
    '-Wno-sign-conversion',
    '-Wno-undef',
    '-Wno-missing-field-initializers',
    '-Wno-deprecated-copy-with-user-provided-dtor',
    '-Wno-cast-align',
    '-Wno-unused-variable',
    '-Wno-switch-enum',
]

# GCC-only warning suppressions (unknown to Clang).
GCC_ONLY_CXX_WARNING_DISABLE = [
    '-Wno-stringop-overflow',
]

MSVC_CXX_WARNING_DISABLE = [
    '/wd4100',  # unreferenced formal parameter
    '/wd4101',  # unreferenced local variable
    '/wd4065',  # switch statement contains default but no case labels
    '/wd5026',  # move constructor was implicitly defined as deleted
    '/wd5027',  # move assignment operator was implicitly defined as deleted
]

CLANG_EXTRA_WARNING_ENABLE = [
    '-Wused-but-marked-unused', '-Wzero-length-array',
    '-Wunused-member-function', '-Wvector-conversion',
    '-Wunused-getter-return-value', '-Wthread-safety',
    '-Wunneeded-member-function', '-Wshadow-all',
    '-Wunused-exception-parameter',
    '-Wunneeded-internal-declaration',
    '-Wunreachable-code-aggressive',
    '-Wsuper-class-method-mismatch',
    '-Werror=implicit-function-declaration',
    '-Wtautological-compare',
]
