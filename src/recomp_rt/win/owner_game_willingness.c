/* Owner Game player-willingness observer.
 *
 * Observational only: this file does not change CM01/02's transfer or player-interest decisions.
 * It watches the native staff-interest score for the active human club and, when CM posts its
 * "player rejects move" news, records the exact rejection plus the most recent matching interest
 * score/reason bits. This is the baseline seam for a later owner-project influence experiment.
 */
#include "win.h"
#include "host.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define OG_PLAYER_MANAGER       0x00b74c26u
#define OG_PLAYER_SLOT          0x00b77c7au
#define OG_STAFF_CLUB           0x39u
#define OG_STAFF_SECOND_NAME    0x08u
#define OG_STAFF_COMMON_NAME    0x0cu
#define OG_CLUB_NAME            0x04u
#define OG_CLUB_COUNT           0x00ae235cu
#define OG_CLUB_BASE_PTR        0x00ae23b4u
#define OG_STAFF_BASE_PTR       0x00ae23bcu
#define OG_CLUB_STRIDE          0x245u
#define OG_STAFF_STRIDE         0x6eu

/* TRANSFER_OFFER fields used by the original transfer code. */
#define OG_OFFER_BUYING_CLUB_ID 0x08u
#define OG_OFFER_STAFF_ID       0x0cu

typedef struct {
    int valid;
    int32_t staff_id;
    int32_t club_id;
    double score;
    uint32_t reason_bits;
    int check_wages;
    int mode;
    int squad_status;
} OgInterestObservation;

static OgInterestObservation og_interest;

static int og_memory_ok(uint32_t p, uint32_t bytes)
{
    if (p < 0x10000u || p >= 0xff000000u) return 0;
    return bytes <= 0xff000000u - p;
}

static void og_copy_string(char *out, size_t room, uint32_t at)
{
    size_t n = 0;
    if (!room) return;
    if (!at || !og_memory_ok(at, 1)) {
        out[0] = 0;
        return;
    }
    while (n + 1 < room && n < 64 && og_memory_ok(at + (uint32_t)n, 1)) {
        unsigned char c = g_mem[at + (uint32_t)n];
        if (!c) break;
        if (c < 32 || c == '"' || c == '\\') c = ' ';
        out[n++] = (char)c;
    }
    out[n] = 0;
}

static void og_copy_staff_name(char *out, size_t room, uint32_t staff)
{
    if (!room) return;
    out[0] = 0;
    if (!og_memory_ok(staff, OG_STAFF_COMMON_NAME + 4)) return;
    uint32_t common = rd32(g_mem, staff + OG_STAFF_COMMON_NAME);
    uint32_t second = rd32(g_mem, staff + OG_STAFF_SECOND_NAME);
    uint32_t at = common && og_memory_ok(common, 1) && g_mem[common] ? common : second;
    og_copy_string(out, room, at);
}

static uint32_t og_current_human_club(int32_t *club_id)
{
    if (club_id) *club_id = -1;
    if (!g_mem) return 0;
    int slot = (int16_t)rd16(g_mem, OG_PLAYER_SLOT);
    if (slot < 0 || slot >= 16) return 0;
    uint32_t manager_at = OG_PLAYER_MANAGER + (uint32_t)slot * 0x300u;
    if (!og_memory_ok(manager_at, 4)) return 0;
    uint32_t manager = rd32(g_mem, manager_at);
    if (!og_memory_ok(manager, OG_STAFF_CLUB + 4)) return 0;
    uint32_t club = rd32(g_mem, manager + OG_STAFF_CLUB);
    if (!og_memory_ok(club, OG_CLUB_NAME + 4)) return 0;
    int32_t id = (int32_t)rd32(g_mem, club);
    int32_t count = (int32_t)rd32(g_mem, OG_CLUB_COUNT);
    if (count <= 0 || id < 0 || id >= count) return 0;
    if (club_id) *club_id = id;
    return club;
}

static uint32_t og_staff_from_id(int32_t id)
{
    if (id < 0 || id > 200000) return 0;
    uint32_t base = rd32(g_mem, OG_STAFF_BASE_PTR);
    uint64_t at64 = (uint64_t)base + (uint64_t)(uint32_t)id * OG_STAFF_STRIDE;
    if (at64 > 0xffffffffu) return 0;
    uint32_t at = (uint32_t)at64;
    return og_memory_ok(at, OG_STAFF_CLUB + 4) ? at : 0;
}

static int og_replace_text_file(const char *name, const char *text)
{
    if (!win_user_dir[0]) return 0;
    char path[1200], tmp[1200];
    snprintf(path, sizeof path, "%s/%s", win_user_dir, name);
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = host_fopen(tmp, "wb");
    if (!f) return 0;
    int ok = fputs(text, f) >= 0 && fflush(f) == 0;
    if (ok) host_sync(f);
    fclose(f);
    if (!ok || host_replace(tmp, path)) {
        host_unlink(tmp);
        return 0;
    }
    return 1;
}

static void og_write_rejection_event(uint32_t offer, int reason)
{
    if (!og_memory_ok(offer, OG_OFFER_STAFF_ID + 4)) return;

    int32_t buying_club_id = (int32_t)rd32(g_mem, offer + OG_OFFER_BUYING_CLUB_ID);
    int32_t staff_id = (int32_t)rd32(g_mem, offer + OG_OFFER_STAFF_ID);
    int32_t human_club_id = -1;
    uint32_t human_club = og_current_human_club(&human_club_id);
    if (!human_club || buying_club_id != human_club_id) return;

    uint32_t staff = og_staff_from_id(staff_id);
    char staff_name[80], club_name[80], text[1800];
    og_copy_staff_name(staff_name, sizeof staff_name, staff);
    og_copy_string(club_name, sizeof club_name, human_club + OG_CLUB_NAME);

    int matched = og_interest.valid && og_interest.staff_id == staff_id &&
                  og_interest.club_id == human_club_id;
    int year = 0, month = 0, day = 0;
    win_game_date(&year, &month, &day);

    snprintf(text, sizeof text,
             "{\n"
             "  \"bridge_version\": \"0.5-poc\",\n"
             "  \"event\": \"player_willingness_rejection\",\n"
             "  \"date\": \"%04d-%02d-%02d\",\n"
             "  \"staff_id\": %d,\n"
             "  \"staff_name\": \"%s\",\n"
             "  \"human_club_id\": %d,\n"
             "  \"human_club_name\": \"%s\",\n"
             "  \"rejection_reason_code\": %d,\n"
             "  \"interest_observed\": %s,\n"
             "  \"interest_score\": %.6f,\n"
             "  \"interest_reason_bits\": %u,\n"
             "  \"check_wages\": %d,\n"
             "  \"mode\": %d,\n"
             "  \"squad_status\": %d,\n"
             "  \"result\": \"cm_rejected_move_before_contract_terms\"\n"
             "}\n",
             year, month, day, staff_id, staff_name,
             human_club_id, club_name, reason,
             matched ? "true" : "false",
             matched ? og_interest.score : 0.0,
             matched ? og_interest.reason_bits : 0u,
             matched ? og_interest.check_wages : -1,
             matched ? og_interest.mode : -1,
             matched ? og_interest.squad_status : -1);

    og_replace_text_file("owner-game-willingness-event.json", text);
    fprintf(stderr,
            "owner-game: CM rejected move: staff %d (%s) -> club %d (%s), reason=%d, interest=%s score=%.3f bits=%u\n",
            staff_id, staff_name, human_club_id, club_name, reason,
            matched ? "matched" : "not-seen", matched ? og_interest.score : 0.0,
            matched ? og_interest.reason_bits : 0u);
}

/* Native player/staff interest scorer:
 * thiscall(Shortlist_Manager *this, Staff *staff, Club *club, int check_wages,
 *          char mode, int squad_status, uint *out_reasons), returns float in ST(0).
 * Observe only calls for the active human club. Reading ST(0) does not pop or change CM's result. */
void o_00846920(void);
RT_WRAP(00846920, owner_game_staff_interest)
{
    uint32_t staff = rt_arg(0);
    uint32_t club = rt_arg(1);
    int check_wages = (int32_t)rt_arg(2);
    int mode = (int8_t)rt_arg(3);
    int squad_status = (int32_t)rt_arg(4);
    uint32_t out_reasons = rt_arg(5);
    int32_t human_club_id = -1;
    uint32_t human_club = og_current_human_club(&human_club_id);
    int observe = human_club && club == human_club && og_memory_ok(staff, 4);
    int32_t staff_id = observe ? (int32_t)rd32(g_mem, staff) : -1;

    o_00846920();
    if (rt_unwinding || !observe) return;

    og_interest.valid = 1;
    og_interest.staff_id = staff_id;
    og_interest.club_id = human_club_id;
    og_interest.score = fpu.st[fpu.top];
    og_interest.reason_bits = out_reasons && og_memory_ok(out_reasons, 4) ? rd32(g_mem, out_reasons) : 0u;
    og_interest.check_wages = check_wages;
    og_interest.mode = mode;
    og_interest.squad_status = squad_status;
}

/* Exact CM event behind the headline "<player> rejects move to <club>".
 * thiscall(TRANSFER_MANAGER *this, TRANSFER_OFFER *offer, int reason). */
void o_008d8310(void);
RT_WRAP(008d8310, owner_game_player_rejects_move)
{
    uint32_t offer = rt_arg(0);
    int reason = (int32_t)rt_arg(1);
    o_008d8310();
    if (rt_unwinding) return;
    og_write_rejection_event(offer, reason);
}
