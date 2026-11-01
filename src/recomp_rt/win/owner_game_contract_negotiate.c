/* Owner Game contract-negotiation boolean gate experiment.
 *
 * Build 18 keeps the proven Figo -> Aston Villa interest-score override and Build 16's
 * reason-mask experiment unchanged, so this is a direct A/B comparison with Build 16.
 *
 * Generated-source tracing of contract_screens_build_basic_terms_tab (0x004e06e0)
 * established a separate native gate:
 *
 *     call CONTRACT_MANAGER::will_negotiate_offer (0x004d4660)
 *     test eax, eax
 *     je refusal_text_path
 *
 * The exact Basic-tab caller returns to 0x004e0ac2. For one deliberately exact test only
 * -- Luis Figo (staff 1730) considering Aston Villa (club 374) at that caller -- this wrapper
 * records CM's native boolean and changes only a native 0 to an effective 1.
 *
 * No contract terms, wages, bonuses, clauses or later accept/reject decision are changed.
 */
#include "win.h"
#include "host.h"

#include <stdint.h>
#include <stdio.h>

#define OG_TEST_STAFF_ID          1730
#define OG_TEST_CLUB_ID           374
#define OG_BASIC_TAB_RETURN       0x004e0ac2u
#define OG_CONTRACT_STAFF_ID      0x00u
#define OG_CONTRACT_CLUB_ID       0x04u

static int og_negotiate_memory_ok(uint32_t p, uint32_t bytes)
{
    if (p < 0x10000u || p >= 0xff000000u) return 0;
    return bytes <= 0xff000000u - p;
}

static void og_negotiate_write_event(int32_t staff_id, int32_t club_id,
                                     uint32_t return_address,
                                     int native_result, int effective_result)
{
    if (!win_user_dir[0]) return;

    char path[1200], tmp[1200];
    int year = 0, month = 0, day = 0;
    win_game_date(&year, &month, &day);

    snprintf(path, sizeof path, "%s/%s", win_user_dir,
             "owner-game-contract-negotiate.json");
    snprintf(tmp, sizeof tmp, "%s.tmp", path);

    FILE *f = host_fopen(tmp, "wb");
    if (!f) return;

    fprintf(f,
            "{\n"
            "  \"bridge_version\": \"0.9-poc\",\n"
            "  \"event\": \"contract_will_negotiate_experiment\",\n"
            "  \"date\": \"%04d-%02d-%02d\",\n"
            "  \"staff_id\": %d,\n"
            "  \"staff_name\": \"Figo\",\n"
            "  \"human_club_id\": %d,\n"
            "  \"human_club_name\": \"Aston Villa\",\n"
            "  \"return_address\": \"0x%08x\",\n"
            "  \"native_will_negotiate\": %d,\n"
            "  \"effective_will_negotiate\": %d,\n"
            "  \"boolean_override_applied\": true,\n"
            "  \"experiment\": \"basic_tab_force_will_negotiate_true_only\"\n"
            "}\n",
            year, month, day, staff_id, club_id, return_address,
            native_result, effective_result);

    int ok = fflush(f) == 0;
    if (ok) host_sync(f);
    fclose(f);

    if (!ok || host_replace(tmp, path))
        host_unlink(tmp);
}

/* CONTRACT_MANAGER::will_negotiate_offer(this, STAFF_CONTRACT *offer), returns 1 or 0 in EAX. */
void o_004d4660(void);
RT_WRAP(004d4660, owner_game_contract_will_negotiate)
{
    uint32_t entry_esp = cpu.esp;
    uint32_t return_address = og_negotiate_memory_ok(entry_esp, 4)
                                  ? rd32(g_mem, entry_esp) : 0u;
    uint32_t contract = rt_arg(0);

    int32_t staff_id = -1;
    int32_t club_id = -1;
    if (og_negotiate_memory_ok(contract, OG_CONTRACT_CLUB_ID + 4)) {
        staff_id = (int32_t)rd32(g_mem, contract + OG_CONTRACT_STAFF_ID);
        club_id = (int32_t)rd32(g_mem, contract + OG_CONTRACT_CLUB_ID);
    }

    int exact_test = return_address == OG_BASIC_TAB_RETURN &&
                     staff_id == OG_TEST_STAFF_ID &&
                     club_id == OG_TEST_CLUB_ID;

    o_004d4660();
    if (rt_unwinding || !exact_test) return;

    int native_result = cpu.eax ? 1 : 0;
    cpu.eax = 1u;

    og_negotiate_write_event(staff_id, club_id, return_address,
                             native_result, 1);

    fprintf(stderr,
            "owner-game: contract negotiate test: staff %d -> club %d caller=%#x native=%d effective=1\n",
            staff_id, club_id, return_address, native_result);
}
