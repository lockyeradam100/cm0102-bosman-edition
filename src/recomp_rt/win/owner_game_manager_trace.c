/* Owner Game Manager Desk: native manager-market integration seam.
 *
 * This is an OPT-IN, non-mutating trace of the native ManagerManager.
 * It proves where real appointments, departures, vacancies and board support
 * requests occur. It does not claim that an owner has hired/fired a manager,
 * compute compensation, or persist an owner-only hiring transaction.
 *
 * RECOMP_OWNER_MANAGER_TRACE=1 writes native observations in the isolated
 * user folder; the ordinary CM manager simulation is never overridden.
 * No offer, staff or club pointer is saved outside the running process.
 *
 * The next Manager Desk milestone can issue properly authorised actions
 * through these native entrypoints after compensation, staff contracts and
 * atomic career persistence are verified. Do not implement replacement by
 * editing a UI name or by pretending a void native call succeeded.
 */
#include "win.h"
#include "host.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OG_MANAGER_TRACE_FILE "owner-game-manager-native.log"
#define OG_STAFF_CLUB         0x39u

static int og_manager_trace_enabled(void)
{
    static int enabled = -1;
    if (enabled < 0) {
        const char *v = getenv("RECOMP_OWNER_MANAGER_TRACE");
        enabled = v && strcmp(v, "1") == 0;
    }
    return enabled;
}

static int og_manager_address_ok(uint32_t address, uint32_t length)
{
    return g_mem && address >= 0x10000u && address < 0xff000000u &&
           length <= 0xff000000u - address;
}

static int32_t og_manager_entity_id(uint32_t address)
{
    return og_manager_address_ok(address, 4)
           ? (int32_t)rd32(g_mem, address) : -1;
}

static int32_t og_manager_staff_club_id(uint32_t staff)
{
    if (!og_manager_address_ok(staff, OG_STAFF_CLUB + 4)) return -1;
    return og_manager_entity_id(rd32(g_mem, staff + OG_STAFF_CLUB));
}

static void og_manager_record(const char *event, int32_t club_id,
                              int32_t staff_id, int32_t old_club_id,
                              int detail, const char *evidence)
{
    if (!og_manager_trace_enabled() || !win_user_dir[0]) return;

    int year = 0, month = 0, day = 0;
    win_game_date(&year, &month, &day);
    char path[1200];
    snprintf(path, sizeof path, "%s/%s", win_user_dir, OG_MANAGER_TRACE_FILE);

    FILE *file = host_fopen(path, "ab");
    if (file) {
        fprintf(file,
                "%04d-%02d-%02d event=%s club=%d staff=%d previous_club=%d "
                "detail=%d evidence=%s\n",
                year, month, day, event, club_id, staff_id,
                old_club_id, detail, evidence);
        if (fflush(file) == 0) host_sync(file);
        fclose(file);
    }
    fprintf(stderr,
            "owner-game: manager %s club=%d staff=%d previous_club=%d "
            "detail=%d evidence=%s\n",
            event, club_id, staff_id, old_club_id, detail, evidence);
}

/* Underlying CM method creates a vacancy and seeds its candidate list.
 * Recording a call does not establish that a replacement was selected. */
void o_0069ba50(void);
RT_WRAP(0069ba50, owner_game_manager_native_vacancy)
{
    if (!og_manager_trace_enabled()) {
        o_0069ba50();
        return;
    }
    uint32_t club = rt_arg(0);
    uint32_t previous = rt_arg(1);
    int32_t club_id = og_manager_entity_id(club);
    int32_t staff_id = og_manager_entity_id(previous);
    o_0069ba50();
    if (rt_unwinding) return;
    og_manager_record("vacancy_processed", club_id, staff_id,
                      -1, 0, "native_method_completed");
}

/* Native method moves the new manager out of existing roles, opens vacancies
 * in affected clubs, updates job links and emits game news. We only classify
 * the appointment as verified if Staff::club points back at target club. */
void o_0069ebc0(void);
RT_WRAP(0069ebc0, owner_game_manager_native_appointment)
{
    if (!og_manager_trace_enabled()) {
        o_0069ebc0();
        return;
    }
    uint32_t club = rt_arg(0);
    uint32_t staff = rt_arg(1);
    int32_t club_id = og_manager_entity_id(club);
    int32_t staff_id = og_manager_entity_id(staff);
    int32_t before = og_manager_staff_club_id(staff);
    o_0069ebc0();
    if (rt_unwinding) return;
    int32_t after = og_manager_staff_club_id(staff);
    const char *event = after == club_id && club_id >= 0
                        ? "appointment_staff_club_verified"
                        : "appointment_called_unclassified";
    og_manager_record(event, club_id, staff_id, before,
                      after, "native_staff_club_postcheck");
}

/* The original removes the actual incumbent and contract, changes staff/job
 * links and queues departure news. Do not replace the dismissal contract or
 * severance calculations with fake sidecar transactions. */
void o_0069b790(void);
RT_WRAP(0069b790, owner_game_manager_native_departure)
{
    if (!og_manager_trace_enabled()) {
        o_0069b790();
        return;
    }
    uint32_t club = rt_arg(0);
    int32_t club_id = og_manager_entity_id(club);
    int departure_type = (int)(uint8_t)rt_arg(1);
    int reason = (int)(uint8_t)rt_arg(2);
    o_0069b790();
    if (rt_unwinding) return;
    og_manager_record("departure_processed", club_id, -1, -1,
                      (departure_type << 8) | (reason & 0xff),
                      "native_method_completed_reason_not_owner_decision");
}

/* These requests cover support such as facilities or transfer/wage budgets.
 * Board approval is computed and posted as native news, not inferred from
 * the method's mere invocation. */
void o_006a8810(void);
RT_WRAP(006a8810, owner_game_manager_native_support)
{
    if (!og_manager_trace_enabled()) {
        o_006a8810();
        return;
    }
    uint32_t club = rt_arg(0);
    int32_t club_id = og_manager_entity_id(club);
    int request_id = (int)(uint8_t)rt_arg(1);
    o_006a8810();
    if (rt_unwinding) return;
    og_manager_record("support_request_processed", club_id, -1, -1,
                      request_id, "native_decision_unclassified");
}

/* Observes the real daily manager-market processing boundary. This marker
 * ensures an apparently passing parity test genuinely executed the adapter
 * rather than testing an unused file. No native method is called twice. */
void o_0068f9c0(void);
RT_WRAP(0068f9c0, owner_game_manager_native_daily)
{
    o_0068f9c0();
    if (rt_unwinding || !og_manager_trace_enabled()) return;
    static int announced;
    if (!announced) {
        announced = 1;
        og_manager_record("daily_market_observed", -1, -1, -1, 0,
                          "native_update_completed");
    }
}
