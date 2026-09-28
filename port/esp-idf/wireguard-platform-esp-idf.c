/* The platform functions declared at the bottom of src/wireguard-platform.h,
 * implemented for ESP-IDF.
 *
 * The library leaves these to its integrator and ships a reference version in
 * example/wireguard-platform.c. Providing an ESP-IDF one here rather than in the
 * consuming project also avoids a circular link: this library's archive needs
 * these symbols, so a project that defines them in its own component has to
 * fight the linker's archive ordering to be found.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "esp_random.h"
#include "esp_timer.h"

#include "wireguard-platform.h"


uint32_t wireguard_sys_now(void)
{
    /* Milliseconds since boot. esp_timer_get_time is monotonic across light
     * sleep and does not wrap for 292,000 years; truncating to 32 bits wraps
     * every 49.7 days, which is what the library expects and handles. */
    return (uint32_t)(esp_timer_get_time() / 1000);
}

void wireguard_random_bytes(void *bytes, size_t size)
{
    esp_fill_random(bytes, size);
}

void wireguard_tai64n_now(uint8_t *output)
{
    /* WireGuard's handshake timestamp is TAI64N: 64 bits of seconds with 2^62 as
     * the epoch offset (plus 10 for TAI minus UTC), then 32 bits of nanoseconds.
     *
     * A responder remembers the greatest timestamp it has accepted from each peer
     * and rejects anything not greater, so this has to increase monotonically per
     * peer -- across reboots, not just within one.
     *
     * Use the real clock when it looks set, which also makes our timestamps
     * comparable with every other WireGuard implementation. Fall back to uptime
     * when it is not, so that a device with no clock can still hand shake: note
     * that the fallback restarts from near zero after a reboot, so a peer that
     * remembers a previous session will reject the first handshakes until uptime
     * passes that value. Synchronising the clock (SNTP) before the first
     * handshake avoids that entirely.
     */
    uint64_t sec;
    uint32_t nano;
    struct timeval tv;

    /* Anything past 2020 means something has set the clock; the epoch default is
     * 1970. */
    if (gettimeofday(&tv, NULL) == 0 && tv.tv_sec > 1577836800) {
        sec = 0x400000000000000aULL + (uint64_t)tv.tv_sec;
        nano = (uint32_t)tv.tv_usec * 1000u;
    } else {
        uint64_t usec = (uint64_t)esp_timer_get_time();
        sec = 0x400000000000000aULL + usec / 1000000ULL;
        nano = (uint32_t)((usec % 1000000ULL) * 1000ULL);
    }

    for (int i = 0; i < 8; i++) output[i] = (uint8_t)(sec >> (56 - 8 * i));
    for (int i = 0; i < 4; i++) output[8 + i] = (uint8_t)(nano >> (24 - 8 * i));
}

bool wireguard_is_under_load(void)
{
    /* Returning true makes the responder demand a cookie reply before spending
     * CPU on a handshake, which is the DoS defence in section 5.3. A device that
     * expects a handful of peers is never "under load" in that sense, and
     * tailcat servers on a microcontroller are not internet-scanned: the node
     * key is the unguessable part of the address. Revisit if we ever accept
     * unsolicited handshakes at volume.
     */
    return false;
}
