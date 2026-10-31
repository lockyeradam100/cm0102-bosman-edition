/* Owner Game live bridge POC.
 *
 * This module deliberately owns no football state. It reads the current human manager's club from the
 * recompiled CM01/02 memory, exposes a small state file in the Bosman user folder, and accepts a one-shot
 * owner capital command. The command is applied directly to the club's real Finance record; CM remains the
 * authority and derives the transfer budget from that balance in its normal Finance::get_transfer_budget.
 *
 * It also observes CM's own contract-offer resolver. When the human club has an offer pending for the staff
 * member being decided, the bridge records whether CM's agreed offer is that human-club offer. This is the
 * A-side of the first football-decision interception proof: observe CM unchanged before adding any override.
 *
 * POC command file (owner-game-command.txt):
 *   inject_cash <session_hex> <club_id> <amount>
 *
 * Commands are tied to this process session and the exact current club, then deleted after one read. This
 * prevents a stale command from being replayed after a restart or against a different club/save.
 */
#include "win.h"
#include "host.h"

#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OG_PLAYER_MANAGER      0x00b74c26u   /* manager pointer per player slot, slots 0x300 bytes apart */
#define OG_PLAYER_SLOT         0x00b77c7au   /* s16: player slot whose turn is running */
#define OG_CLUB_COUNT          0x00ae235cu   /* number of club records */
#define OG_FINANCE_TABLE       0x00ae2a38u   /* FinanceTable * */
#define OG_STAFF_CLUB          0x39u         /* Staff +0x39: Club * */
#define OG_STAFF_SECOND_NAME   0x08u         /* Staff +0x08: second-name string pointer */
#define OG_STAFF_COMMON_NAME   0x0cu         /* Staff +0x0c: common-name string pointer */
#define OG_CLUB_NAME           0x04u         /* Club +4: long name */
#define OG_FINANCE_CLUB        0x08u         /* Finance +8: club id */
#define OG_GET_FINANCE         0x005a0590u   /* FinanceTable::get_club_finance(Club *) */
#define OG_FIND_CONTRACT_OFFER 0x004cc9e0u   /* ContractManager::find_contract_offer(staff_id, club_id) */
#define OG_FIND_AGREED_OFFER   0x004cca30u   /* ContractManager::find_agreed_contract_offer(staff_id) */
#define OG_MAX_INJECTION       1000000000LL  /* POC guardrail: £1bn per command */

static uint64_t og_session;

typedef struct {
    int ready;
    int slot;
    uint32_t manager;
    uint32_t club;
    int32_t club_id;
    uint32_t finance;
    int64_t balance;
    int year, month, day;
    char club_name[80];
} OgState;

static int memory_ok(uint32_t p, uint32_t bytes)
{
    if (p < 0x10000u || p >= 0xff000000u) return 0;
    return bytes <= 0xff000000u - p;
}

static uint32_t call_this(uint32_t fn, uint32_t self, int nargs, const uint32_t *args)
{
    Cpu saved = cpu;
    cpu.ecx = self;
    uint32_t r = rt_callback(fn, nargs, args);
    if (!rt_unwinding) cpu = saved;
    return r;
}

static void copy_club_name(char *out, size_t room, uint32_t at)
{
    if (!room) return;
    size_t n = 0;
    while (n + 1 < room && n < 64 && memory_ok(at + (uint32_t)n, 1)) {
        unsigned char c = g_mem[at + (uint32_t)n];
        if (!c) break;
        /* Keep the state file valid JSON without needing a general JSON encoder. */
        if (c < 32 || c == '"' || c == '\\') c = ' ';
        out[n++] = (char)c;
    }
    out[n] = 0;
}

static void copy_staff_name(char *out, size_t room, uint32_t staff)
{
    if (!room) return;
    out[0] = 0;
    if (!memory_ok(staff, OG_STAFF_COMMON_NAME + 4)) return;
    uint32_t common = rd32(g_mem, staff + OG_STAFF_COMMON_NAME);
    uint32_t second = rd32(g_mem, staff + OG_STAFF_SECOND_NAME);
    uint32_t at = common && memory_ok(common, 1) && g_mem[common] ? common : second;
    if (!at || !memory_ok(at, 1)) return;
    copy_club_name(out, room, at); /* same bounded/sanitised string copy */
}

static int read_state(OgState *s)
{
    memset(s, 0, sizeof *s);
    s->slot = -1;
    if (!g_mem) return 0;

    int slot = (int16_t)rd16(g_mem, OG_PLAYER_SLOT);
    if (slot < 0 || slot >= 16) return 0;
    uint32_t manager_slot = OG_PLAYER_MANAGER + (uint32_t)slot * 0x300u;
    if (!memory_ok(manager_slot, 4)) return 0;

    uint32_t manager = rd32(g_mem, manager_slot);
    if (!memory_ok(manager, OG_STAFF_CLUB + 4)) return 0;
    uint32_t club = rd32(g_mem, manager + OG_STAFF_CLUB);
    if (!memory_ok(club, 0x82u)) return 0;

    int32_t club_count = (int32_t)rd32(g_mem, OG_CLUB_COUNT);
    int32_t club_id = (int32_t)rd32(g_mem, club);
    if (club_count <= 0 || club_id < 0 || club_id >= club_count) return 0;

    uint32_t table = rd32(g_mem, OG_FINANCE_TABLE);
    if (!memory_ok(table, 4)) return 0;
    uint32_t args[1] = {club};
    uint32_t finance = call_this(OG_GET_FINANCE, table, 1, args);
    if (rt_unwinding || !memory_ok(finance, OG_FINANCE_CLUB + 4)) return 0;
    if ((int32_t)rd32(g_mem, finance + OG_FINANCE_CLUB) != club_id) return 0;

    s->ready = 1;
    s->slot = slot;
    s->manager = manager;
    s->club = club;
    s->club_id = club_id;
    s->finance = finance;
    s->balance = (int64_t)rd64(g_mem, finance);
    win_game_date(&s->year, &s->month, &s->day);
    copy_club_name(s->club_name, sizeof s->club_name, club + OG_CLUB_NAME);
    return 1;
}

static void bridge_path(char *out, size_t room, const char *name)
{
    snprintf(out, room, "%s/%s", win_user_dir, name);
}

static int replace_text_file(const char *name, const char *text)
{
    if (!win_user_dir[0]) return 0;
    char path[1200], tmp[1200];
    bridge_path(path, sizeof path, name);
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

static void write_state(void)
{
    OgState s;
    char text[1024];
    if (!og_session) {
        og_session = host_unix_time_us() ^ (uint64_t)(uintptr_t)g_mem ^ 0x4f574e455247414dULL;
        if (!og_session) og_session = 1;
    }

    if (!read_state(&s)) {
        snprintf(text, sizeof text,
                 "{\n"
                 "  \"bridge_version\": \"0.4-poc\",\n"
                 "  \"session\": \"%016llx\",\n"
                 "  \"ready\": false\n"
                 "}\n",
                 (unsigned long long)og_session);
        replace_text_file("owner-game-state.json", text);
        return;
    }

    snprintf(text, sizeof text,
             "{\n"
             "  \"bridge_version\": \"0.4-poc\",\n"
             "  \"session\": \"%016llx\",\n"
             "  \"ready\": true,\n"
             "  \"date\": \"%04d-%02d-%02d\",\n"
             "  \"player_slot\": %d,\n"
             "  \"club_id\": %d,\n"
             "  \"club_name\": \"%s\",\n"
             "  \"balance\": %lld,\n"
             "  \"command\": \"inject_cash %016llx %d 5000000\"\n"
             "}\n",
             (unsigned long long)og_session,
             s.year, s.month, s.day, s.slot, s.club_id, s.club_name,
             (long long)s.balance, (unsigned long long)og_session, s.club_id);
    replace_text_file("owner-game-state.json", text);
}

static void write_result(const char *status, const char *message, const OgState *s,
                         int64_t amount, int64_t before, int64_t after)
{
    char text[1200];
    snprintf(text, sizeof text,
             "{\n"
             "  \"bridge_version\": \"0.4-poc\",\n"
             "  \"session\": \"%016llx\",\n"
             "  \"status\": \"%s\",\n"
             "  \"message\": \"%s\",\n"
             "  \"club_id\": %d,\n"
             "  \"club_name\": \"%s\",\n"
             "  \"amount\": %lld,\n"
             "  \"before\": %lld,\n"
             "  \"after\": %lld\n"
             "}\n",
             (unsigned long long)og_session, status, message,
             s ? s->club_id : -1, s ? s->club_name : "",
             (long long)amount, (long long)before, (long long)after);
    replace_text_file("owner-game-result.json", text);
}

static void write_contract_event(const OgState *s, uint32_t staff, uint32_t human_offer,
                                 uint32_t agreed_offer, uint32_t human_offer_after)
{
    char name[80], text[1400];
    copy_staff_name(name, sizeof name, staff);
    int32_t staff_id = memory_ok(staff, 4) ? (int32_t)rd32(g_mem, staff) : -1;
    int accepted = agreed_offer && (agreed_offer == human_offer || agreed_offer == human_offer_after);
    snprintf(text, sizeof text,
             "{\n"
             "  \"bridge_version\": \"0.4-poc\",\n"
             "  \"session\": \"%016llx\",\n"
             "  \"date\": \"%04d-%02d-%02d\",\n"
             "  \"staff_id\": %d,\n"
             "  \"staff_name\": \"%s\",\n"
             "  \"human_club_id\": %d,\n"
             "  \"human_club_name\": \"%s\",\n"
             "  \"human_offer_before\": %u,\n"
             "  \"human_offer_after\": %u,\n"
             "  \"agreed_offer\": %u,\n"
             "  \"accepted_human_club\": %s,\n"
             "  \"result\": \"%s\"\n"
             "}\n",
             (unsigned long long)og_session,
             s->year, s->month, s->day, staff_id, name,
             s->club_id, s->club_name,
             human_offer, human_offer_after, agreed_offer,
             accepted ? "true" : "false",
             accepted ? "cm_accepted_human_club_offer" : "cm_did_not_accept_human_club_offer");
    replace_text_file("owner-game-contract-event.json", text);
    fprintf(stderr, "owner-game: CM contract decision: staff %d (%s), club %d (%s), accepted=%d\n",
            staff_id, name, s->club_id, s->club_name, accepted);
}

static void process_command(void)
{
    if (!win_user_dir[0]) return;
    char path[1200], line[256];
    bridge_path(path, sizeof path, "owner-game-command.txt");
    FILE *f = host_fopen(path, "rb");
    if (!f) return;
    line[0] = 0;
    fgets(line, sizeof line, f);
    fclose(f);
    /* One read, one attempt. Even malformed or stale commands cannot replay forever. */
    host_unlink(path);

    char action[32] = "";
    unsigned long long session = 0;
    int club_id = -1;
    long long amount_ll = 0;
    if (sscanf(line, "%31s %llx %d %lld", action, &session, &club_id, &amount_ll) != 4 ||
        strcmp(action, "inject_cash")) {
        write_result("rejected", "invalid command", NULL, 0, 0, 0);
        return;
    }

    OgState s;
    if (!read_state(&s)) {
        write_result("rejected", "no active human club", NULL, (int64_t)amount_ll, 0, 0);
        return;
    }
    if ((uint64_t)session != og_session) {
        write_result("rejected", "stale process session", &s, (int64_t)amount_ll, s.balance, s.balance);
        return;
    }
    if (club_id != s.club_id) {
        write_result("rejected", "wrong club", &s, (int64_t)amount_ll, s.balance, s.balance);
        return;
    }
    if (amount_ll <= 0 || amount_ll > OG_MAX_INJECTION) {
        write_result("rejected", "amount outside POC guardrail", &s, (int64_t)amount_ll, s.balance, s.balance);
        return;
    }

    int64_t amount = (int64_t)amount_ll;
    int64_t before = s.balance;
    if (before > INT64_MAX - amount) {
        write_result("rejected", "balance overflow", &s, amount, before, before);
        return;
    }
    int64_t after = before + amount;
    wr64(g_mem, s.finance, (uint64_t)after);
    int64_t verify = (int64_t)rd64(g_mem, s.finance);
    if (verify != after) {
        write_result("error", "finance write did not verify", &s, amount, before, verify);
        return;
    }

    fprintf(stderr, "owner-game: injected %lld into club %d (%s): %lld -> %lld\n",
            (long long)amount, s.club_id, s.club_name, (long long)before, (long long)after);
    s.balance = after;
    write_result("applied", "owner capital injected into live CM finance", &s, amount, before, after);
}

/* Bosman already owns the one wrapper around game_run_human_turn in testrun.c. Keep that wrapper canonical
 * and let it call these ordinary hooks before and after the original human turn. */
void owner_game_before_human_turn(void)
{
    write_state();
}

void owner_game_after_human_turn(void)
{
    process_command();
    write_state();
}

/* CM's own contract resolver: thiscall(CONTRACT_MANAGER *this, Staff *staff). For the observational A-side,
 * do not alter any state before CM runs. If the human club has a pending contract offer for this staff member,
 * remember its pointer, let CM make the decision, then ask CM which offer (if any) is agreed and record whether
 * that is the human club's offer. */
void o_004ce4e0(void);
RT_WRAP(004ce4e0, owner_game_contract_decision)
{
    uint32_t contract_manager = cpu.ecx;
    uint32_t staff = rt_arg(0);
    OgState s;
    uint32_t human_offer = 0;
    int observe = 0;

    if (staff && memory_ok(staff, 4) && read_state(&s)) {
        int32_t staff_id = (int32_t)rd32(g_mem, staff);
        uint32_t args[2] = {(uint32_t)staff_id, (uint32_t)s.club_id};
        human_offer = call_this(OG_FIND_CONTRACT_OFFER, contract_manager, 2, args);
        if (!rt_unwinding && human_offer) observe = 1;
    }

    o_004ce4e0();
    if (rt_unwinding || !observe) return;

    int32_t staff_id = (int32_t)rd32(g_mem, staff);
    uint32_t find_args[2] = {(uint32_t)staff_id, (uint32_t)s.club_id};
    uint32_t after_offer = call_this(OG_FIND_CONTRACT_OFFER, contract_manager, 2, find_args);
    if (rt_unwinding) return;
    uint32_t agreed_args[1] = {(uint32_t)staff_id};
    uint32_t agreed = call_this(OG_FIND_AGREED_OFFER, contract_manager, 1, agreed_args);
    if (rt_unwinding) return;
    write_contract_event(&s, staff, human_offer, agreed, after_offer);
}
