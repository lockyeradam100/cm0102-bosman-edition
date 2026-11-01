/* Owner Game clean player-willingness seam.
 *
 * Build 23 keeps the one empirically proven intervention needed to get the Figo -> Aston Villa
 * proof deal into contract talks, but removes the broad Build 13 experiment. CM's native interest
 * scorer still runs first. Only the exact initial transfer-interest caller (return 0x008c0215),
 * Luis Figo (staff 1730) and Aston Villa (club 374) receives an effective interest score of 1.0.
 * Later contract-screen interest calls are left completely native.
 */
#include "win.h"
#include "host.h"

#include <stdint.h>
#include <stdio.h>

#define OG_PLAYER_MANAGER                    0x00b74c26u
#define OG_PLAYER_SLOT                       0x00b77c7au
#define OG_STAFF_CLUB                        0x39u
#define OG_CLUB_NAME                         0x04u
#define OG_CLUB_COUNT                        0x00ae235cu
#define OG_TEST_STAFF_ID                     1730
#define OG_TEST_CLUB_ID                      374
#define OG_INITIAL_TRANSFER_INTEREST_RETURN  0x008c0215u
#define OG_TEST_INTEREST_SCORE               1.0

static int og_willingness_memory_ok(uint32_t p, uint32_t bytes)
{
    if (p < 0x10000u || p >= 0xff000000u) return 0;
    return bytes <= 0xff000000u - p;
}

static uint32_t og_willingness_human_club(int32_t *club_id)
{
    if (club_id) *club_id = -1;
    if (!g_mem) return 0;

    int slot = (int16_t)rd16(g_mem, OG_PLAYER_SLOT);
    if (slot < 0 || slot >= 16) return 0;

    uint32_t manager_at = OG_PLAYER_MANAGER + (uint32_t)slot * 0x300u;
    if (!og_willingness_memory_ok(manager_at, 4)) return 0;
    uint32_t manager = rd32(g_mem, manager_at);
    if (!og_willingness_memory_ok(manager, OG_STAFF_CLUB + 4)) return 0;

    uint32_t club = rd32(g_mem, manager + OG_STAFF_CLUB);
    if (!og_willingness_memory_ok(club, OG_CLUB_NAME + 4)) return 0;

    int32_t id = (int32_t)rd32(g_mem, club);
    int32_t count = (int32_t)rd32(g_mem, OG_CLUB_COUNT);
    if (count <= 0 || id < 0 || id >= count) return 0;

    if (club_id) *club_id = id;
    return club;
}

static void og_willingness_write_event(uint32_t return_address,
                                       int32_t staff_id, int32_t club_id,
                                       double native_score, double effective_score,
                                       uint32_t reason_bits)
{
    if (!win_user_dir[0]) return;

    char path[1200], tmp[1200];
    int year = 0, month = 0, day = 0;
    win_game_date(&year, &month, &day);

    snprintf(path, sizeof path, "%s/%s", win_user_dir,
             "owner-game-willingness-score.json");
    snprintf(tmp, sizeof tmp, "%s.tmp", path);

    FILE *f = host_fopen(tmp, "wb");
    if (!f) return;

    fprintf(f,
            "{\n"
            "  \"bridge_version\": \"1.3-poc\",\n"
            "  \"event\": \"initial_transfer_interest_override\",\n"
            "  \"date\": \"%04d-%02d-%02d\",\n"
            "  \"staff_id\": %d,\n"
            "  \"staff_name\": \"Figo\",\n"
            "  \"human_club_id\": %d,\n"
            "  \"human_club_name\": \"Aston Villa\",\n"
            "  \"return_address\": \"0x%08x\",\n"
            "  \"native_interest_score\": %.6f,\n"
            "  \"effective_interest_score\": %.6f,\n"
            "  \"interest_reason_bits\": %u,\n"
            "  \"override_scope\": \"initial_transfer_interest_caller_only\"\n"
            "}\n",
            year, month, day, staff_id, club_id, return_address,
            native_score, effective_score, reason_bits);

    int ok = fflush(f) == 0;
    if (ok) host_sync(f);
    fclose(f);
    if (!ok || host_replace(tmp, path)) host_unlink(tmp);
}

/* CM's native player/staff interest scorer. */
void o_00846920(void);
RT_WRAP(00846920, owner_game_staff_interest)
{
    uint32_t entry_esp = cpu.esp;
    uint32_t return_address = og_willingness_memory_ok(entry_esp, 4)
                                ? rd32(g_mem, entry_esp) : 0u;
    uint32_t staff = rt_arg(0);
    uint32_t club = rt_arg(1);
    uint32_t out_reasons = rt_arg(5);

    int32_t human_club_id = -1;
    uint32_t human_club = og_willingness_human_club(&human_club_id);
    int32_t staff_id = (staff && og_willingness_memory_ok(staff, 4))
                       ? (int32_t)rd32(g_mem, staff) : -1;

    int exact_initial = human_club && club == human_club &&
                        staff_id == OG_TEST_STAFF_ID &&
                        human_club_id == OG_TEST_CLUB_ID &&
                        return_address == OG_INITIAL_TRANSFER_INTEREST_RETURN;

    o_00846920();
    if (rt_unwinding || !exact_initial) return;

    double native_score = fpu.st[fpu.top];
    uint32_t reason_bits = out_reasons && og_willingness_memory_ok(out_reasons, 4)
                           ? rd32(g_mem, out_reasons) : 0u;

    fpu.st[fpu.top] = OG_TEST_INTEREST_SCORE;
    og_willingness_write_event(return_address, staff_id, human_club_id,
                               native_score, fpu.st[fpu.top], reason_bits);

    fprintf(stderr,
            "owner-game: initial willingness seam: staff %d -> club %d caller=%#x native=%.6f effective=%.6f bits=%u\n",
            staff_id, human_club_id, return_address,
            native_score, fpu.st[fpu.top], reason_bits);
}
