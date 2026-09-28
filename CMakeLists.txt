# ESP-IDF component packaging for wireguard-lwip.
#
# The library is platform-agnostic and expects the integrator to supply the
# functions declared at the bottom of src/wireguard-platform.h:
#
#   uint32_t wireguard_sys_now(void);
#   void     wireguard_random_bytes(void *bytes, size_t size);
#   void     wireguard_tai64n_now(uint8_t *output);
#   bool     wireguard_is_under_load(void);
#
# See example/wireguard-platform.c for a reference implementation. Those symbols
# are resolved at link time from the component that depends on this one, so this
# component intentionally does not provide them.
#
# The static limits in src/wireguard-platform.h can be set from the build, e.g.
#   target_compile_definitions(... PUBLIC WIREGUARD_MAX_PEERS=16)
idf_component_register(
    SRCS
        "src/wireguard.c"
        "src/wireguardif.c"
        "src/crypto.c"
        "src/crypto/refc/blake2s.c"
        "src/crypto/refc/chacha20.c"
        "src/crypto/refc/chacha20poly1305.c"
        "src/crypto/refc/poly1305-donna.c"
        "src/crypto/refc/x25519.c"
    INCLUDE_DIRS
        "src"
        "src/crypto/refc"
    REQUIRES
        lwip
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
