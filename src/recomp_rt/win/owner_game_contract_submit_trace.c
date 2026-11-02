/* Owner Game contract persistence trace.
 *
 * Build 26 proved that the Figo -> Aston Villa negotiation screen can be opened with owner wage
 * authority, but a submitted offer did not survive as a native pending CM contract offer. This
 * diagnostic observes the two authoritative ContractManager mutations without changing either:
 *
 *   - CONTRACT_MANAGER::add_contract_offer (0x4cc490)
 *   - CONTRACT_MANAGER::remove_contract_offer (0x4cca70)
 *
 * Only the exact Figo (staff 1730) -> Aston Villa (club 374) proof pair is logged. After each native
 * mutation, CM itself is queried with find_contract_offer so the trace distinguishes "never added",
 * "added and persisted", and "later removed". Football state remains entirely native CM state.
 */
#include "win.h"
#include "host.h"

#include <stdint.h>
#include <stdio.h>

#define OG_TRACE_STAFF_ID              1730
#define OG_TRACE_CLUB_ID               374
#define OG_FIND_CONTRACT_OFFER         0x004cc9e0u
#define OG_STAFF_CONTRACT_STAFF_ID     0x00u
#define OG_STAFF_CONTRACT_CLUB_ID      0x04u
#define OG_ADD_FROM_STAFF_CONFIRM      0x004e52d2u
#define OG_TRACE_FILE                  "owner-game-contract-submit.log"

static int og_trace_memory_ok(uint32_t p, uint32_t bytes)
{
    if (p < 0x10000u || p >= 0xff000000u) return 0;
    return bytes <= 0xff000000u - p;
}

static uint32_t og_trace_u32(uint32_t p)
{
    return og_trace_memory_ok(p, 4) ? rd32(g_mem, p) : 0u;
}

static uint32_t og_trace_call_this(uint32_t fn, uint32_t self, int nargs, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t result = rt_callback(fn, nargs, args);
    if (!rt_unwinding) cpu = saved;
    return result;
}

static uint32_t og_trace_find_offer(uint32_t contract_manager, int32_t staff_id, int32_t club_id)
{
    uint32_t args[2] = {(uint32_t)staff_id, (uint32_t)club_id};
    return og_trace_call_this(OG_FIND_CONTRACT_OFFER, contract_manager, 2, args);
}

static void og_trace_write(const char *event,
                           uint32_t return_address,
                           int32_t staff_id,
                           int32_t club_id,
                           uint32_t pending_before,
                           uint32_t pending_after,
                           uint32_t arg4,
                           uint32_t arg5,
                           uint32_t arg6)
{
    if (!win_user_dir[0]) return;

    char path[1200];
    int year = 0, month = 0, day = 0;
    win_game_date(&year, &month, &day);
    snprintf(path, sizeof path, "%s/%s", win_user_dir, OG_TRACE_FILE);

    FILE *f = host_fopen(path, "ab");
    if (!f) return;
    fprintf(f,
            "%04d-%02d-%02d event=%s staff=%d club=%d return=0x%08x "
            "pending_before=0x%08x pending_after=0x%08x arg4=%u arg5_schedule=%u arg6=%u result=%s\n",
            year, month, day, event, staff_id, club_id, return_address,
            pending_before, pending_after, arg4, arg5, arg6,
            pending_after ? "pending_offer_present" : "pending_offer_absent");
    if (fflush(f) == 0) host_sync(f);
    fclose(f);

    fprintf(stderr,
            "owner-game: contract trace %s staff=%d club=%d return=0x%08x pending 0x%08x -> 0x%08x schedule=%u\n",
            event, staff_id, club_id, return_address,
            pending_before, pending_after, arg5);
}

/* thiscall CONTRACT_MANAGER::add_contract_offer(
 *     STAFF_CONTRACT *offer, ..., ..., ..., uint32_t arg4, uint32_t schedule_decision, uint32_t arg6)
 *
 * The generated native function proves arg0 is copied into CM-owned pending state and arg5 controls
 * schedule_offer_decision. The normal staff-offer confirmation path calls this from 0x4e52d2 with
 * arg5 == 1.
 */
void o_004cc490(void);
RT_WRAP(004cc490, owner_game_trace_add_contract_offer)
{
    uint32_t entry_esp = cpu.esp;
    uint32_t return_address = og_trace_u32(entry_esp);
    uint32_t contract_manager = cpu.ecx;
    uint32_t offer = rt_arg(0);
    uint32_t arg4 = rt_arg(4);
    uint32_t arg5 = rt_arg(5);
    uint32_t arg6 = rt_arg(6);

    int32_t staff_id = -1;
    int32_t club_id = -1;
    if (og_trace_memory_ok(offer, OG_STAFF_CONTRACT_CLUB_ID + 4)) {
        staff_id = (int32_t)rd32(g_mem, offer + OG_STAFF_CONTRACT_STAFF_ID);
        club_id = (int32_t)rd32(g_mem, offer + OG_STAFF_CONTRACT_CLUB_ID);
    }

    int exact_proof = staff_id == OG_TRACE_STAFF_ID &&
                      club_id == OG_TRACE_CLUB_ID &&
                      return_address == OG_ADD_FROM_STAFF_CONFIRM;
    uint32_t pending_before = 0;
    if (exact_proof)
        pending_before = og_trace_find_offer(contract_manager, staff_id, club_id);

    o_004cc490();
    if (rt_unwinding || !exact_proof) return;

    uint32_t pending_after = og_trace_find_offer(contract_manager, staff_id, club_id);
    if (rt_unwinding) return;
    og_trace_write("add_contract_offer", return_address, staff_id, club_id,
                   pending_before, pending_after, arg4, arg5, arg6);
}

/* thiscall CONTRACT_MANAGER::remove_contract_offer(Staff *staff, Club *club).
 * This is also called by the native staff-offer confirmation handler, so logging it catches the
 * alternative path where CM deliberately removes/replaces an existing pending offer.
 */
void o_004cca70(void);
RT_WRAP(004cca70, owner_game_trace_remove_contract_offer)
{
    uint32_t entry_esp = cpu.esp;
    uint32_t return_address = og_trace_u32(entry_esp);
    uint32_t contract_manager = cpu.ecx;
    uint32_t staff = rt_arg(0);
    uint32_t club = rt_arg(1);

    int32_t staff_id = og_trace_memory_ok(staff, 4) ? (int32_t)rd32(g_mem, staff) : -1;
    int32_t club_id = og_trace_memory_ok(club, 4) ? (int32_t)rd32(g_mem, club) : -1;
    int exact_proof = staff_id == OG_TRACE_STAFF_ID && club_id == OG_TRACE_CLUB_ID;

    uint32_t pending_before = 0;
    if (exact_proof)
        pending_before = og_trace_find_offer(contract_manager, staff_id, club_id);

    o_004cca70();
    if (rt_unwinding || !exact_proof) return;

    uint32_t pending_after = og_trace_find_offer(contract_manager, staff_id, club_id);
    if (rt_unwinding) return;
    og_trace_write("remove_contract_offer", return_address, staff_id, club_id,
                   pending_before, pending_after, 0, 0, 0);
}
