/* Owner Game contract-terms gate experiment.
 *
 * Build 19 proved the exact Figo -> Aston Villa Basic-tab call already returns native
 * will_negotiate = 1. Live caller values plus generated-source tracing then proved two later
 * Basic-tab gates:
 *
 *   1) affordability:
 *      contract_field_c = 154962
 *      local wage ceiling = 75080.0
 *      multiplier = 1.5
 *      native threshold = 112620.0 -> refusal
 *
 *   2) strength of interest after affordability:
 *      local interest = 1.0
 *      shortcut threshold = 2.75
 *      values above 2.75 jump directly to the real terms-rendering path.
 *
 * Build 21 is an exact-pair controlled test only. For Luis Figo (staff 1730) considering
 * Aston Villa (club 374), and only at the Basic-tab caller after CM has natively returned
 * will_negotiate = 1, it changes two caller locals:
 *
 *      wage ceiling 75080 -> 120000  (affordability threshold 112620 -> 180000)
 *      interest      1.0  -> 3.0     (clears the >2.75 terms shortcut)
 *
 * It does not change the contract object itself, Figo's wage figure, bonuses, clauses, club
 * permission, transfer fee or any later accept/reject decision.
 */
#include "win.h"
#include "host.h"

#include <stdint.h>
#include <stdio.h>

#define OG_TEST_STAFF_ID               1730
#define OG_TEST_CLUB_ID                374
#define OG_BASIC_TAB_RETURN            0x004e0ac2u
#define OG_CONTRACT_STAFF_ID           0x00u
#define OG_CONTRACT_CLUB_ID            0x04u
#define OG_TEST_LOCAL_WAGE_CEILING     120000.0
#define OG_AFFORDABILITY_MULTIPLIER    1.5
#define OG_TEST_LOCAL_INTEREST_SCORE   3.0f
#define OG_TERMS_INTEREST_THRESHOLD    2.75

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

static float og_read_f32(uint32_t p)
{
    return og_negotiate_memory_ok(p, 4) ? rdf32(g_mem, p) : 0.0f;
}

static void og_write_f64(uint32_t p, double value)
{
    if (og_negotiate_memory_ok(p, 8))
        wrf64(g_mem, p, value);
}

static void og_write_f32(uint32_t p, float value)
{
    if (og_negotiate_memory_ok(p, 4))
        wrf32(g_mem, p, value);
}

static void og_write_event(int32_t staff_id, int32_t club_id,
                           uint32_t return_address, int native_result,
                           uint32_t contract,
                           double native_ceiling, double effective_ceiling,
                           float native_interest, float effective_interest)
{
    if (!win_user_dir[0]) return;

    char path[1200], tmp[1200];
    int year = 0, month = 0, day = 0;
    win_game_date(&year, &month, &day);

    uint32_t contract_c = og_read_u32(contract + 0x0cu);
    double native_affordability = native_ceiling * OG_AFFORDABILITY_MULTIPLIER;
    double effective_affordability = effective_ceiling * OG_AFFORDABILITY_MULTIPLIER;

    snprintf(path, sizeof path, "%s/%s", win_user_dir,
             "owner-game-contract-terms-gates.json");
    snprintf(tmp, sizeof tmp, "%s.tmp", path);

    FILE *f = host_fopen(tmp, "wb");
    if (!f) return;

    fprintf(f,
            "{\n"
            "  \"bridge_version\": \"1.2-poc\",\n"
            "  \"event\": \"contract_terms_gate_experiment\",\n"
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
            "  \"native_local_interest_score\": %.3f,\n"
            "  \"terms_interest_threshold\": %.3f,\n"
            "  \"effective_local_interest_score\": %.3f,\n"
            "  \"gate_overrides_applied\": true,\n"
            "  \"experiment\": \"exact_basic_tab_affordability_plus_interest_terms_path\"\n"
            "}\n",
            year, month, day, staff_id, club_id, return_address,
            native_result, contract_c,
            native_ceiling, native_affordability,
            effective_ceiling, effective_affordability,
            (double)native_interest, OG_TERMS_INTEREST_THRESHOLD,
            (double)effective_interest);

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
     * code subsequently reads:
     *   qword [esp + 0x44] for the affordability ceiling, and
     *   float [esp + 0x20] for the contract-screen interest score. */
    uint32_t caller_esp = entry_esp + 8u;
    uint32_t ceiling_ptr = caller_esp + 0x44u;
    uint32_t interest_ptr = caller_esp + 0x20u;

    double native_ceiling = og_read_f64(ceiling_ptr);
    float native_interest = og_read_f32(interest_ptr);
    double effective_ceiling = native_ceiling;
    float effective_interest = native_interest;

    if (native_result && native_ceiling > 0.0) {
        effective_ceiling = OG_TEST_LOCAL_WAGE_CEILING;
        og_write_f64(ceiling_ptr, effective_ceiling);

        effective_interest = OG_TEST_LOCAL_INTEREST_SCORE;
        og_write_f32(interest_ptr, effective_interest);
    }

    og_write_event(staff_id, club_id, return_address, native_result,
                   contract, native_ceiling, effective_ceiling,
                   native_interest, effective_interest);

    fprintf(stderr,
            "owner-game: terms-gate test: staff %d -> club %d wage %.3f->%.3f interest %.3f->%.3f\n",
            staff_id, club_id,
            native_ceiling, effective_ceiling,
            (double)native_interest, (double)effective_interest);
}
