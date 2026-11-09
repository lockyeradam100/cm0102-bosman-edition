/* Owner Game contract-offer lifecycle diagnostics.
 *
 * RECOMP_OWNER_CONTRACT_TRACE=1 enables read-only logging of native CM
 * add_contract_offer / remove_contract_offer for ANY staff or player targeted
 * by the human-controlled club, not just the previous Figo/Aston Villa proof.
 * The ContractManager remains authoritative; neither offer nor response is
 * modified here. The absence of an offer does NOT mean a player rejected it.
 */
#include "win.h"
#include "host.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OG_FIND_CONTRACT_OFFER     0x004cc9e0u
#define OG_PLAYER_MANAGER         0x00b74c26u
#define OG_PLAYER_SLOT            0x00b77c7au
#define OG_STAFF_CLUB             0x39u
#define OG_STAFF_CONTRACT_STAFF   0x00u
#define OG_STAFF_CONTRACT_CLUB    0x04u
#define OG_TRACE_FILE             "owner-game-contract-submit.log"

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

static uint32_t call_this(uint32_t fn, uint32_t self, int nargs, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t result = rt_callback(fn, nargs, args);
    if (!rt_unwinding) cpu = saved;
    return result;
}

static uint32_t find_offer(uint32_t manager, int32_t staff_id, int32_t club_id)
{
    uint32_t args[2] = {(uint32_t)staff_id, (uint32_t)club_id};
    return call_this(OG_FIND_CONTRACT_OFFER, manager, 2, args);
}

static void record(const char *event, uint32_t caller, int32_t staff_id,
                   int32_t club_id, uint32_t before, uint32_t after,
                   uint32_t arg4, uint32_t schedule, uint32_t arg6)
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
            "pending_before=%s pending_after=%s offer_before=0x%08x "
            "offer_after=0x%08x arg4=%u schedule=%u arg6=%u\n",
            year, month, day, event, staff_id, club_id, caller,
            before ? "yes" : "no", after ? "yes" : "no", before, after,
            arg4, schedule, arg6);
    if (fflush(file) == 0) host_sync(file);
    fclose(file);
    fprintf(stderr, "owner-game: contract %s staff=%d club=%d "
            "pending %d->%d schedule=%u\n",
            event, staff_id, club_id, before != 0, after != 0, schedule);
}

/* Native ContractManager::add_contract_offer(STAFF_CONTRACT*, ...).
 * arg5 contains the schedule-offer-decision flag in the generated function.
 */
void o_004cc490(void);
RT_WRAP(004cc490, owner_game_trace_add_contract_offer)
{
    if (!trace_enabled()) { o_004cc490(); return; }

    uint32_t caller = read_u32(cpu.esp);
    uint32_t manager = cpu.ecx;
    uint32_t offer = rt_arg(0);
    uint32_t arg4 = rt_arg(4), schedule = rt_arg(5), arg6 = rt_arg(6);
    int32_t staff_id = -1, club_id = -1;
    if (address_ok(offer, OG_STAFF_CONTRACT_CLUB + 4)) {
        staff_id = (int32_t)read_u32(offer + OG_STAFF_CONTRACT_STAFF);
        club_id = (int32_t)read_u32(offer + OG_STAFF_CONTRACT_CLUB);
    }
    int observe = staff_id >= 0 && club_id >= 0 &&
                  club_id == current_human_club_id();
    uint32_t before = observe ? find_offer(manager, staff_id, club_id) : 0;

    /* Always leave the native game responsible for handling the offer. */
    o_004cc490();
    if (rt_unwinding || !observe) return;
    uint32_t after = find_offer(manager, staff_id, club_id);
    if (rt_unwinding) return;
    record("native_add", caller, staff_id, club_id, before, after,
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
    uint32_t manager = cpu.ecx;
    uint32_t staff = rt_arg(0), club = rt_arg(1);
    int32_t staff_id = address_ok(staff, 4) ? (int32_t)read_u32(staff) : -1;
    int32_t club_id = address_ok(club, 4) ? (int32_t)read_u32(club) : -1;
    int observe = staff_id >= 0 && club_id >= 0 &&
                  club_id == current_human_club_id();
    uint32_t before = observe ? find_offer(manager, staff_id, club_id) : 0;

    o_004cca70();
    if (rt_unwinding || !observe) return;
    uint32_t after = find_offer(manager, staff_id, club_id);
    if (rt_unwinding) return;
    record("native_remove", caller, staff_id, club_id, before, after,
           0, 0, 0);
}
