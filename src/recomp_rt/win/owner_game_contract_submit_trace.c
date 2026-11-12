/* Owner Game contract-offer lifecycle diagnostics.
 *
 * RECOMP_OWNER_CONTRACT_TRACE=1 enables read-only logging of native CM
 * add_contract_offer / remove_contract_offer for ANY staff or player targeted
 * by the human-controlled club, not just the previous Figo/Aston Villa proof.
 * The ContractManager remains authoritative. This diagnostic deliberately
 * DOES NOT query ContractManager::find_contract_offer from inside mutation
 * hooks, because nested native callbacks may alter the saved contract state.
 * Event logs prove the native call occurred; they do NOT prove persistence,
 * acceptance or rejection. A separate safe-point readback is required.
 */
#include "win.h"
#include "host.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OG_PLAYER_MANAGER         0x00b74c26u
#define OG_PLAYER_SLOT            0x00b77c7au
#define OG_STAFF_CLUB             0x39u
#define OG_STAFF_CONTRACT_STAFF   0x00u
#define OG_STAFF_CONTRACT_CLUB    0x04u
#define OG_TRACE_FILE             "owner-game-contract-submit.log"
#define OG_MAX_WATCHED            96u

/* Ephemeral diagnostic watchlist. No pointers or fake offer statuses are
 * written into a football save; the engine always owns pending offers. */
typedef struct {
    int32_t staff_id;
    int32_t club_id;
} OgWatchedOffer;

static OgWatchedOffer watched[OG_MAX_WATCHED];
static size_t watched_count;
static size_t next_replace;
static int trace_announced;

static void watch_offer(int32_t staff_id, int32_t club_id)
{
    if (staff_id < 0 || club_id < 0) return;
    for (size_t i = 0; i < watched_count; ++i)
        if (watched[i].staff_id == staff_id && watched[i].club_id == club_id)
            return;
    size_t slot = watched_count < OG_MAX_WATCHED
                    ? watched_count++
                    : (next_replace++ % OG_MAX_WATCHED);
    watched[slot].staff_id = staff_id;
    watched[slot].club_id = club_id;
}


static int trace_enabled(void)
{
    static int state = -1;
    if (state < 0) {
        const char *v = getenv("RECOMP_OWNER_CONTRACT_TRACE");
        state = v && strcmp(v, "1") == 0;
    }
    return state;
}

static int address_ok(uint32_t address, uint32_t bytes)
{
    if (!g_mem || address < 0x10000u || address >= 0xff000000u) return 0;
    return bytes <= 0xff000000u - address;
}

static uint32_t read_u32(uint32_t address)
{
    return address_ok(address, 4) ? rd32(g_mem, address) : 0;
}

static int32_t current_human_club_id(void)
{
    if (!g_mem) return -1;
    int32_t slot = (int16_t)rd16(g_mem, OG_PLAYER_SLOT);
    if (slot < 0 || slot >= 16) return -1;
    uint32_t manager = read_u32(OG_PLAYER_MANAGER + (uint32_t)slot * 0x300u);
    if (!address_ok(manager, OG_STAFF_CLUB + 4)) return -1;
    uint32_t club = read_u32(manager + OG_STAFF_CLUB);
    if (!address_ok(club, 4)) return -1;
    return (int32_t)read_u32(club);
}

static void record(const char *event, uint32_t caller, int32_t staff_id,
                   int32_t club_id, uint32_t arg4, uint32_t schedule,
                   uint32_t arg6)
{
    if (!win_user_dir[0]) return;
    char path[1200];
    int year = 0, month = 0, day = 0;
    win_game_date(&year, &month, &day);
    snprintf(path, sizeof path, "%s/%s", win_user_dir, OG_TRACE_FILE);
    FILE *file = host_fopen(path, "ab");
    if (!file) return;
    fprintf(file,
            "%04d-%02d-%02d event=%s staff=%d club=%d caller=0x%08x "
            "arg4=%u schedule=%u arg6=%u pending_status=unverified\n",
            year, month, day, event, staff_id, club_id, caller,
            arg4, schedule, arg6);
    if (fflush(file) == 0) host_sync(file);
    fclose(file);
    fprintf(stderr,
            "owner-game: contract %s staff=%d club=%d schedule=%u "
            "pending_status=unverified\n",
            event, staff_id, club_id, schedule);
}

/* Native ContractManager::add_contract_offer(STAFF_CONTRACT*, ...).
 * arg5 contains the schedule-offer-decision flag in the generated function.
 */
void o_004cc490(void);
RT_WRAP(004cc490, owner_game_trace_add_contract_offer)
{
    if (!trace_enabled()) { o_004cc490(); return; }

    uint32_t caller = read_u32(cpu.esp);
    uint32_t offer = rt_arg(0);
    uint32_t arg4 = rt_arg(4), schedule = rt_arg(5), arg6 = rt_arg(6);
    int32_t staff_id = -1, club_id = -1;
    if (address_ok(offer, OG_STAFF_CONTRACT_CLUB + 4)) {
        staff_id = (int32_t)read_u32(offer + OG_STAFF_CONTRACT_STAFF);
        club_id = (int32_t)read_u32(offer + OG_STAFF_CONTRACT_CLUB);
    }
    int observe = staff_id >= 0 && club_id >= 0 &&
                  club_id == current_human_club_id();

    /* Do not call find_contract_offer from inside the native mutation.
     * Even an ostensibly read-only rt_callback can perturb the native
     * contract manager's state or its execution context. Observe calls
     * only. Whether a pending offer exists requires independent readback. */
    o_004cc490();
    if (rt_unwinding || !observe) return;
    watch_offer(staff_id, club_id);
    record("native_add_called", caller, staff_id, club_id,
           arg4, schedule, arg6);
}

/* Native ContractManager::remove_contract_offer(Staff*, Club*).
 * Removal may mean replacement, withdrawal, expiry or final resolution.
 */
void o_004cca70(void);
RT_WRAP(004cca70, owner_game_trace_remove_contract_offer)
{
    if (!trace_enabled()) { o_004cca70(); return; }

    uint32_t caller = read_u32(cpu.esp);
    uint32_t staff = rt_arg(0), club = rt_arg(1);
    int32_t staff_id = address_ok(staff, 4) ? (int32_t)read_u32(staff) : -1;
    int32_t club_id = address_ok(club, 4) ? (int32_t)read_u32(club) : -1;
    int observe = staff_id >= 0 && club_id >= 0 &&
                  club_id == current_human_club_id();

    o_004cca70();
    if (rt_unwinding || !observe) return;
    watch_offer(staff_id, club_id);
    record("native_remove_called", caller, staff_id, club_id, 0, 0, 0);
}


/* Observe the native daily ContractManager update without looking up offers
 * inside its mutation loop. Doing a nested native callback from a hook can
 * alter the contract data and violate observational parity. The daily
 * marker only establishes that a watched offer has been through another
 * CM day: it DOES NOT claim a pending/accepted/rejected state. */
void o_004c7ac0(void);
RT_WRAP(004c7ac0, owner_game_trace_daily_contracts)
{
    if (!trace_enabled()) {
        o_004c7ac0();
        return;
    }
    if (!trace_announced) {
        trace_announced = 1;
        fprintf(stderr,
                "owner-game: contract lifecycle trace active at native daily boundary\n");
    }
    o_004c7ac0();
    if (rt_unwinding) return;
    int32_t club = current_human_club_id();
    if (club < 0) return;
    for (size_t i = 0; i < watched_count; ++i) {
        if (watched[i].club_id != club) continue;
        record("native_daily_pass", 0, watched[i].staff_id,
               watched[i].club_id, 0, 0, 0);
    }
}
