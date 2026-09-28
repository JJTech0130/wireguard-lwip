# ESP-IDF component packaging for wireguard-lwip.
#
# The library itself is platform-agnostic and expects the integrator to supply
# the functions declared at the bottom of src/wireguard-platform.h. An ESP-IDF
# implementation ships in port/esp-idf and is built by default, because defining
# those symbols in the consuming component instead means fighting the linker's
# archive ordering: this library needs them, and its archive is searched after
# the component that requires it.
#
# Set WIREGUARD_LWIP_OWN_PLATFORM to provide your own instead.
#
# The static limits in src/wireguard-platform.h can be set from the build, e.g.
#   target_compile_definitions(... PUBLIC WIREGUARD_MAX_PEERS=16)
set(srcs
    "src/wireguard.c"
    "src/wireguardif.c"
    "src/crypto.c"
    "src/crypto/refc/blake2s.c"
    "src/crypto/refc/chacha20.c"
    "src/crypto/refc/chacha20poly1305.c"
    "src/crypto/refc/poly1305-donna.c"
    "src/crypto/refc/x25519.c"
)

if(NOT WIREGUARD_LWIP_OWN_PLATFORM)
    list(APPEND srcs "port/esp-idf/wireguard-platform-esp-idf.c")
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS
        "src"
        "src/crypto/refc"
    REQUIRES
        lwip
        esp_timer
        esp_hw_support
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    WIREGUARD_CRYPTO_REFC=1
)

# GCC's array-bounds analysis misreads the limb arithmetic in the reference
# X25519 as an over-read of a small object. The code is correct.
set_source_files_properties(
    "src/crypto/refc/x25519.c"
    PROPERTIES COMPILE_OPTIONS "-Wno-stringop-overread"
)
