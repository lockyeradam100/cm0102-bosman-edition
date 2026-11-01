/* Owner Game contract-negotiation reason experiment.
 *
 * Build 16 leaves the proven Build 12/13 Figo -> Aston Villa interest-score override untouched.
 * This wrapper targets the next native seam one layer above it: f_0085fc50, the contract-detail
 * function used by the contract screen.
 *
 * Generated-source tracing established that the exact contract-context call returns to 0x004deccf,
 * takes the player/staff pointer as stack arg 0, and takes the native reason-output pointer as stack
 * arg 5. The caller subsequently stores that reason mask in negotiation field 0x19 and stores the
 * interest score separately in field 0x1a.
 *
 * For one deliberately exact A/B test only -- Luis Figo (staff 1730), Aston Villa (club 374), and
 * the exact contract-context caller -- Build 16 clears only native reason bit 0x00800000 after CM
 * has completed f_0085fc50. Every other reason bit and every other CM decision remains native.
 */
#include "win.h"
#include "host.h"

#include <stdint.h>
#include <stdio.h>

#define OG_PLAYER_MANAGER             0x00b74c26u
#define OG_PLAYER_SLOT                0x00b77c7au
#define OG_STAFF_CLUB                 0x39u
#define OG_CLUB_NAME                  0x04u
#define OG_CLUB_COUNT                 0x00ae235cu

#define OG_TEST_STAFF_ID              1730
#define OG_TEST_CLUB_ID               374
#define OG_CONTRACT_CONTEXT_RETURN    0x004deccfu
#define OG_REASON_NO_INTEREST_BIT     0x00800000u

static int og_contract_memory_ok(uint32_t p, uint32_t bytes)
{
    if (p < 0x10000u || p >= 0xff000000u) return 0;
    return bytes <= 0xff000000u - p;
}

static uint32_t og_contract_human_club(int32_t *club_id)
{
    if (club_id) *club_id = -1;
    if (!g_mem) return 0;

    int slot = (int16_t)rd16(g_mem, OG_PLAYER_SLOT);
    if (slot < 0 || slot >= 16) return 0;

    uint32_t manager_at = OG_PLAYER_MANAGER + (uint32_t)slot * 0x300u;
    if (!og_contract_memory_ok(manager_at, 4)) return 0;

    uint32_t manager = rd32(g_mem, manager_at);
    if (!og_contract_memory_ok(manager, OG_STAFF_CLUB + 4)) return 0;

    uint32_t club = rd32(g_mem, manager + OG_STAFF_CLUB);
    if (!og_contract_memory_ok(club, OG_CLUB_NAME + 4)) return 0;

    int32_t id = (int32_t)rd32(g_mem, club);
    int32_t count = (int32_t)rd32(g_mem, OG_CLUB_COUNT);
    if (count <= 0 || id < 0 || id >= count) return 0;

    if (club_id) *club_id = id;
    return club;
}

static void og_contract_write_event(int32_t staff_id, int32_t club_id,
                                    uint32_t native_bits, uint32_t effective_bits,
                                    uint32_t return_address)
{
    if (!win_user_dir[0]) return;

    char path[1200], tmp[1200];
    int year = 0, month = 0, day = 0;
    win_game_date(&year, &month, &day);

    snprintf(path, sizeof path, "%s/%s", win_user_dir, "owner-game-contract-reason.json");
    snprintf(tmp, sizeof tmp, "%s.tmp", path);

    FILE *f = host_fopen(tmp, "wb");
    if (!f) return;

    fprintf(f,
            "{\n"
            "  \"bridge_version\": \"0.8-poc\",\n"
            "  \"event\": \"contract_reason_bit_experiment\",\n"
            "  \"date\": \"%04d-%02d-%02d\",\n"
            "  \"staff_id\": %d,\n"
            "  \"staff_name\": \"Figo\",\n"
            "  \"human_club_id\": %d,\n"
            "  \"human_club_name\": \"Aston Villa\",\n"
            "  \"return_address\": \"0x%08x\",\n"
            "  \"native_reason_bits\": %u,\n"
            "  \"effective_reason_bits\": %u,\n"
            "  \"cleared_reason_bit\": %u,\n"
            "  \"reason_override_applied\": true,\n"
            "  \"experiment\": \"contract_context_clear_0x00800000_only\"\n"
            "}\n",
            year, month, day, staff_id, club_id, return_address,
            native_bits, effective_bits, OG_REASON_NO_INTEREST_BIT);

    int ok = fflush(f) == 0;
    if (ok) host_sync(f);
    fclose(f);

    if (!ok || host_replace(tmp, path))
        host_unlink(tmp);
}

/* Contract-detail layer. The exact contract screen caller returns to 0x004deccf.
 * Stack arg 0 is the staff pointer. Stack arg 5 is the native reason-output pointer. */
void o_0085fc50(void);
RT_WRAP(0085fc50, owner_game_contract_reason)
{
    uint32_t return_address = og_contract_memory_ok(cpu.esp, 4) ? rd32(g_mem, cpu.esp) : 0u;
    uint32_t staff = rt_arg(0);
    uint32_t out_reasons = rt_arg(5);

    int32_t human_club_id = -1;
    uint32_t human_club = og_contract_human_club(&human_club_id);
    (void)human_club;

    int32_t staff_id = og_contract_memory_ok(staff, 4) ? (int32_t)rd32(g_mem, staff) : -1;

    int exact_test = return_address == OG_CONTRACT_CONTEXT_RETURN &&
                     staff_id == OG_TEST_STAFF_ID &&
                     human_club_id == OG_TEST_CLUB_ID &&
                     out_reasons && og_contract_memory_ok(out_reasons, 4);

    o_0085fc50();
    if (rt_unwinding || !exact_test) return;

    uint32_t native_bits = rd32(g_mem, out_reasons);
    uint32_t effective_bits = native_bits & ~OG_REASON_NO_INTEREST_BIT;
    wr32(g_mem, out_reasons, effective_bits);

    og_contract_write_event(staff_id, human_club_id, native_bits, effective_bits, return_address);

    fprintf(stderr,
            "owner-game: contract reason test: staff %d -> club %d caller=%#x native_bits=%u effective_bits=%u cleared=%#x\n",
            staff_id, human_club_id, return_address, native_bits, effective_bits,
            OG_REASON_NO_INTEREST_BIT);
}
