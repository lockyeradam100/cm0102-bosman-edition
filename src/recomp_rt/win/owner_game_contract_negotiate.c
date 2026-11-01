/* Owner Game contract affordability gate experiment.
 *
 * Build 19 proved the exact Figo -> Aston Villa Basic-tab call already returns native
 * will_negotiate = 1. Decoding the caller locals against the generated Basic-tab source then
 * identified the first later refusal branch:
 *
 *     contract_field_c = 154962
 *     local wage ceiling = 75080.0
 *     multiplier = 1.5
 *     threshold = 112620.0
 *
 * Because 154962 > 112620, CM jumps directly to the common refusal-text path.
 *
 * Build 20 changes one thing only for the exact Figo (1730) -> Aston Villa (374) Basic-tab
 * call: after CM's native will-negotiate decision has returned, replace that one caller-local
 * ceiling value with 120000.0. This raises the same native comparison threshold to 180000.0,
 * enough to let 154962 pass while leaving the contract object, player, club, bonuses, terms and
 * later accept/reject logic untouched.
 */
#include "win.h"
#include "host.h"

#include <stdint.h>
#include <stdio.h>

#define OG_TEST_STAFF_ID             1730
#define OG_TEST_CLUB_ID              374
#define OG_BASIC_TAB_RETURN          0x004e0ac2u
#define OG_CONTRACT_STAFF_ID         0x00u
#define OG_CONTRACT_CLUB_ID          0x04u
#define OG_TEST_LOCAL_WAGE_CEILING   120000.0
#define OG_AFFORDABILITY_MULTIPLIER  1.5

static int og_negotiate_memory_ok(uint32_t p, uint32_t bytes)
{
    if (p < 0x10000u || p >= 0xff000000u) return 0;
    return bytes <= 0xff000000u - p;
}

static uint32_t og_read_u32(uint32_t p)
{
    return og_negotiate_memory_ok(p, 4) ? rd32(g_mem, p) : 0u;
}

static double og_read_f64(uint32_t p)
{
    return og_negotiate_memory_ok(p, 8) ? rdf64(g_mem, p) : 0.0;
}

static void og_write_f64(uint32_t p, double value)
{
    if (og_negotiate_memory_ok(p, 8))
        wrf64(g_mem, p, value);
}

static void og_write_event(int32_t staff_id, int32_t club_id,
                           uint32_t return_address, int native_result,
                           uint32_t contract,
                           double native_ceiling, double effective_ceiling)
{
    if (!win_user_dir[0]) return;

    char path[1200], tmp[1200];
    int year = 0, month = 0, day = 0;
    win_game_date(&year, &month, &day);

    uint32_t contract_c = og_read_u32(contract + 0x0cu);
    double native_threshold = native_ceiling * OG_AFFORDABILITY_MULTIPLIER;
    double effective_threshold = effective_ceiling * OG_AFFORDABILITY_MULTIPLIER;

    snprintf(path, sizeof path, "%s/%s", win_user_dir,
             "owner-game-contract-affordability.json");
    snprintf(tmp, sizeof tmp, "%s.tmp", path);

    FILE *f = host_fopen(tmp, "wb");
    if (!f) return;

    fprintf(f,
            "{\n"
            "  \"bridge_version\": \"1.1-poc\",\n"
            "  \"event\": \"contract_affordability_gate_experiment\",\n"
            "  \"date\": \"%04d-%02d-%02d\",\n"
            "  \"staff_id\": %d,\n"
            "  \"staff_name\": \"Figo\",\n"
            "  \"human_club_id\": %d,\n"
            "  \"human_club_name\": \"Aston Villa\",\n"
            "  \"return_address\": \"0x%08x\",\n"
            "  \"native_will_negotiate\": %d,\n"
            "  \"contract_field_c\": %u,\n"
            "  \"native_local_wage_ceiling\": %.3f,\n"
            "  \"native_affordability_threshold\": %.3f,\n"
            "  \"effective_local_wage_ceiling\": %.3f,\n"
            "  \"effective_affordability_threshold\": %.3f,\n"
            "  \"ceiling_override_applied\": true,\n"
            "  \"experiment\": \"raise_exact_basic_tab_affordability_local_only\"\n"
            "}\n",
            year, month, day, staff_id, club_id, return_address,
            native_result, contract_c, native_ceiling, native_threshold,
            effective_ceiling, effective_threshold);

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
    uint32_t return_address = og_read_u32(entry_esp);
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

    /* At 0x4e0abd the caller pushes edi, then CALL pushes the return address.
     * The Basic-tab function's own ESP is therefore entry_esp + 8. The generated
     * code reads its qword local at [esp + 0x44] for the affordability comparison. */
    uint32_t caller_esp = entry_esp + 8u;
    uint32_t ceiling_ptr = caller_esp + 0x44u;
    double native_ceiling = og_read_f64(ceiling_ptr);
    double effective_ceiling = native_ceiling;

    if (native_result && native_ceiling > 0.0) {
        effective_ceiling = OG_TEST_LOCAL_WAGE_CEILING;
        og_write_f64(ceiling_ptr, effective_ceiling);
    }

    og_write_event(staff_id, club_id, return_address, native_result,
                   contract, native_ceiling, effective_ceiling);

    fprintf(stderr,
            "owner-game: affordability test: staff %d -> club %d native_ceiling=%.3f effective=%.3f\n",
            staff_id, club_id, native_ceiling, effective_ceiling);
}
