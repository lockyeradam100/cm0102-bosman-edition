/* Owner Game negotiation-room vertical slice.
 *
 * CM remains authoritative for the player's demand and final contract decision. This module only
 * controls the owner layer around the exact Figo -> Aston Villa proof negotiation:
 *
 *   - hold position: keep the proven £120k contract-screen authority from Build 22;
 *   - owner exception: authorise £160k for this negotiation only;
 *   - take time / final offer / walk away / reopen: persist the Owner Game negotiation phase;
 *   - preserve CM's native will-negotiate result and later accept/reject resolver.
 *
 * The sidecar control file is deliberately prototype-only. A later Owner Game UI can drive the
 * same state without changing the football-engine seam.
 */
#include "win.h"
#include "host.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OG_TEST_STAFF_ID               1730
#define OG_TEST_CLUB_ID                374
#define OG_BASIC_TAB_RETURN            0x004e0ac2u
#define OG_CONTRACT_STAFF_ID           0x00u
#define OG_CONTRACT_CLUB_ID            0x04u
#define OG_HOLD_WAGE_CEILING           120000.0
#define OG_EXCEPTION_WAGE_CEILING      160000.0
#define OG_AFFORDABILITY_MULTIPLIER    1.5
#define OG_TERMS_INTEREST_SCORE        3.0f
#define OG_TERMS_INTEREST_THRESHOLD    2.75
#define OG_CONTROL_FILE                "owner-game-negotiation-control.txt"
#define OG_STATE_FILE                  "owner-game-negotiation.json"

typedef struct {
    const char *phase;
    int owner_exception;
    int reopened;
} OgNegotiationControl;

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
    if (og_negotiate_memory_ok(p, 8)) wrf64(g_mem, p, value);
}

static void og_write_f32(uint32_t p, float value)
{
    if (og_negotiate_memory_ok(p, 4)) wrf32(g_mem, p, value);
}

static void og_trim_line(char *s)
{
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\r' || s[n - 1] == '\n' || s[n - 1] == ' ' || s[n - 1] == '\t'))
        s[--n] = 0;
}

static OgNegotiationControl og_read_control(void)
{
    OgNegotiationControl c = {"active", 0, 0};
    if (!win_user_dir[0]) return c;

    char path[1200], line[128];
    snprintf(path, sizeof path, "%s/%s", win_user_dir, OG_CONTROL_FILE);
    FILE *f = host_fopen(path, "rb");
    if (!f) return c;

    line[0] = 0;
    fgets(line, sizeof line, f);
    fclose(f);
    og_trim_line(line);

    if (!strcmp(line, "active_exception")) {
        c.phase = "active";
        c.owner_exception = 1;
    } else if (!strcmp(line, "dormant")) {
        c.phase = "dormant";
    } else if (!strcmp(line, "final_offer_exception")) {
        c.phase = "final_offer";
        c.owner_exception = 1;
    } else if (!strcmp(line, "walked_away")) {
        c.phase = "walked_away";
    } else if (!strcmp(line, "reopened_hold")) {
        c.phase = "active";
        c.reopened = 1;
    } else if (!strcmp(line, "reopened_exception")) {
        c.phase = "active";
        c.owner_exception = 1;
        c.reopened = 1;
    } else {
        c.phase = "active";
    }

    return c;
}

static void og_write_state(const OgNegotiationControl *control,
                           int32_t staff_id, int32_t club_id,
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

    snprintf(path, sizeof path, "%s/%s", win_user_dir, OG_STATE_FILE);
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = host_fopen(tmp, "wb");
    if (!f) return;

    fprintf(f,
            "{\n"
            "  \"bridge_version\": \"1.4-poc\",\n"
            "  \"event\": \"owner_negotiation_room\",\n"
            "  \"date\": \"%04d-%02d-%02d\",\n"
            "  \"staff_id\": %d,\n"
            "  \"staff_name\": \"Figo\",\n"
            "  \"human_club_id\": %d,\n"
            "  \"human_club_name\": \"Aston Villa\",\n"
            "  \"phase\": \"%s\",\n"
            "  \"reopened\": %s,\n"
            "  \"owner_exception_approved\": %s,\n"
            "  \"owner_wage_authority\": %.0f,\n"
            "  \"return_address\": \"0x%08x\",\n"
            "  \"native_will_negotiate\": %d,\n"
            "  \"contract_field_c\": %u,\n"
            "  \"native_local_wage_ceiling\": %.3f,\n"
            "  \"native_affordability_threshold\": %.3f,\n"
            "  \"effective_local_wage_ceiling\": %.3f,\n"
            "  \"effective_affordability_threshold\": %.3f,\n"
            "  \"native_local_interest_score\": %.3f,\n"
            "  \"effective_local_interest_score\": %.3f,\n"
            "  \"terms_interest_threshold\": %.3f,\n"
            "  \"player_demand_and_final_decision\": \"native_cm\"\n"
            "}\n",
            year, month, day, staff_id, club_id,
            control->phase,
            control->reopened ? "true" : "false",
            control->owner_exception ? "true" : "false",
            effective_ceiling,
            return_address, native_result, contract_c,
            native_ceiling, native_affordability,
            effective_ceiling, effective_affordability,
            (double)native_interest, (double)effective_interest,
            OG_TERMS_INTEREST_THRESHOLD);

    int ok = fflush(f) == 0;
    if (ok) host_sync(f);
    fclose(f);
    if (!ok || host_replace(tmp, path)) host_unlink(tmp);
}

/* CONTRACT_MANAGER::will_negotiate_offer(this, STAFF_CONTRACT *offer), returns native 1/0 in EAX. */
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

    const char *proof_mode = getenv("RECOMP_OWNER_FIGO_PROOF");
    int exact_proof = proof_mode && !strcmp(proof_mode, "1") &&
                      return_address == OG_BASIC_TAB_RETURN &&
                      staff_id == OG_TEST_STAFF_ID &&
                      club_id == OG_TEST_CLUB_ID;

    o_004d4660();
    if (rt_unwinding || !exact_proof) return;

    int native_result = cpu.eax ? 1 : 0;
    uint32_t caller_esp = entry_esp + 8u;
    uint32_t ceiling_ptr = caller_esp + 0x44u;
    uint32_t interest_ptr = caller_esp + 0x20u;

    double native_ceiling = og_read_f64(ceiling_ptr);
    float native_interest = og_read_f32(interest_ptr);
    double effective_ceiling = native_ceiling;
    float effective_interest = native_interest;
    OgNegotiationControl control = og_read_control();

    if (native_result && native_ceiling > 0.0) {
        effective_ceiling = control.owner_exception
                            ? OG_EXCEPTION_WAGE_CEILING
                            : OG_HOLD_WAGE_CEILING;
        effective_interest = OG_TERMS_INTEREST_SCORE;
        og_write_f64(ceiling_ptr, effective_ceiling);
        og_write_f32(interest_ptr, effective_interest);
    }

    og_write_state(&control, staff_id, club_id, return_address, native_result,
                   contract, native_ceiling, effective_ceiling,
                   native_interest, effective_interest);

    fprintf(stderr,
            "owner-game: negotiation room: phase=%s exception=%d staff %d -> club %d wage %.3f->%.3f interest %.3f->%.3f\n",
            control.phase, control.owner_exception,
            staff_id, club_id,
            native_ceiling, effective_ceiling,
            (double)native_interest, (double)effective_interest);
}
