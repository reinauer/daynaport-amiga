#!/usr/bin/env python3
"""Exercise the driver's actual command and readiness code with failed I/O.

Extract the OS-independent bodies into a host harness; mock Exec's DoIO,
timer and SANA event delivery. No Amiga or network connection is required.
"""
from pathlib import Path
import subprocess
import tempfile
import re

ROOT = Path(__file__).resolve().parents[1]
scsi = (ROOT / "scsiwifi.c").read_text()
device = (ROOT / "device.c").read_text()
header = (ROOT / "scsiwifi.h").read_text()


def function(source, name):
    start = re.search(r"^(?:static )?(?:void|LONG|enum SCSIWifi_LinkState) " +
                      re.escape(name) + r"\(", source, re.M).start()
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end] + "\n"


defs = scsi[scsi.index("#define SCSI_INQUIRY"):scsi.index("#define INQUIRE_BUFFER_SIZE")]
macro = scsi[scsi.index("#define SCSI_PREPCMD"):scsi.index("// Internal SCSI device data")]
internal = scsi[scsi.index("struct SCSIDevice {"):scsi.index("#define SysBase")]
result = header[header.index("struct SCSIWifi_Result {"):header.index("// Internal SCSI device data")]
net = header[header.index("struct STRUCT_PACKED SCSIWifi_NetworkEntry {"):header.index("// These structs")]
poll = device[device.index("\t\tGetSysTime(&timeWifiCheck);\n\t\t/* The packet gate"):
              device.index('\n\t\t/* MULTICAST=1: give')]

harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef uint16_t UWORD, USHORT;
typedef uint8_t UBYTE;
typedef void *APTR, *SCSIWIFIDevice;
typedef int BOOL;
#define TRUE 1
#define FALSE 0
#define STRUCT_PACKED __attribute__((packed))
#define SCSIF_READ 1
#define SCSIF_WRITE 0
#define SCSIF_AUTOSENSE 2
#define HFERR_BadStatus 45
#define S2EVENT_ONLINE 1
#define S2EVENT_OFFLINE 2
struct SCSICmd {
    APTR scsi_Data;
    UBYTE *scsi_Command;
    ULONG scsi_Length, scsi_Actual, scsi_SenseActual;
    UBYTE scsi_Status, scsi_Flags;
};
struct IOStdReq { APTR io_Data; LONG io_Error; };
struct IORequest { APTR io_Data; LONG io_Error; };
'''+ defs + macro + result + internal + net + r'''
typedef struct SCSIDevice *LSCSIDevice;
static struct SCSIDevice adapter;
static struct IOStdReq ior;
static UBYTE cdb[12];
static LONG now, ready_at, info_io_error, info_status, enable_failures;
static int short_write, info_calls, enable_calls, disable_calls, unsupported;
static ULONG reply_length;
static LONG DoIO(struct IORequest *req) {
    struct SCSICmd *cmd = req->io_Data;
    LONG err = 0;
    cmd->scsi_Status = 0;
    cmd->scsi_Actual = cmd->scsi_Length;
    if (cmd->scsi_Command[0] == SCSI_NETWORK_WIFI_ENABLE) {
        if (cmd->scsi_Command[5]) {
            ++enable_calls;
            if (enable_failures) { --enable_failures; err = 20; }
        } else ++disable_calls;
    } else if (cmd->scsi_Command[0] == SCSI_NETWORK_WIFI_CMD &&
               cmd->scsi_Command[1] == SCSI_NETWORK_WIFI_OPT_INFO) {
        ++info_calls;
        UBYTE *data = cmd->scsi_Data;
        struct SCSIWifi_NetworkEntry net = {0};
        strcpy(net.ssid, "test");
        net.rssi = now >= ready_at ? -45 : 0;
        data[0] = 0; data[1] = sizeof(net);
        memcpy(data + 2, &net, sizeof(net));
        cmd->scsi_Actual = reply_length;
        err = info_io_error;
        cmd->scsi_Status = info_status;
        if (unsupported) {
            err = HFERR_BadStatus;
            cmd->scsi_Status = 2;
            cmd->scsi_SenseActual = 14;
            adapter.senseData[0] = 0x70;
            adapter.senseData[2] = 5;
            adapter.senseData[12] = 0x20;
            adapter.senseData[13] = 0;
        }
    } else if (cmd->scsi_Command[0] == SCSI_NETWORK_WIFI_WRITEFRAME) {
        err = info_io_error;
        if (short_write) --cmd->scsi_Actual;
    }
    req->io_Error = err;
    return err;
}
LONG SCSIWifi_getNetwork(SCSIWIFIDevice, struct SCSIWifi_NetworkEntry *);
'''
for name in ["runCommand", "SCSIWifi_getResult", "SCSIWifi_getNetwork",
             "SCSIWifi_linkState", "SCSIWifi_enable", "SCSIWifi_sendFrame",
             "SCSIWifi_AmigaNetSendFrames"]:
    harness += function(scsi, name)

harness += r'''
struct timeval_fake { ULONG tv_secs; };
struct devbase {
    int db_quit;
    USHORT db_online, db_currentWifiState, db_mcastPending, db_mcastCount;
    int db_mcastSem;
    struct { struct timeval_fake LastStart; } db_DevStats;
};
#define DEVBASEP struct devbase *db
static struct devbase db_value;
static int last_event, events, rejected, errors;
static void logMessage(struct devbase *db, const char *text) { (void)db; (void)text; }
static void logScsiError(struct devbase *db, const char *text, SCSIWIFIDevice dev) {
    (void)db; (void)text; (void)dev; ++errors;
}
static void Delay(ULONG ticks) { now += ticks / 50; }
'''+function(device, "prepareLink")+r'''
static USHORT currentWifiState, packetEnabled, lastWifiStatus, fastPollsLeft;
static ULONG lastEnableAttempt;
static BOOL enableAttempted;
static struct timeval_fake timeLastWifiCheck, timeWifiCheck;
static void GetSysTime(struct timeval_fake *t) { t->tv_secs = now; }
static void ObtainSemaphore(int *s) { (void)s; }
static void ReleaseSemaphore(int *s) { (void)s; }
static void rejectAllPackets(struct devbase *db) { (void)db; ++rejected; }
static void DoEvent(struct devbase *db, int event) { (void)db; ++events; last_event = event; }
static void step(void) {
    struct devbase *db = &db_value;
    SCSIWIFIDevice scsiDevice = &adapter;
    USHORT shouldBeEnabled = db->db_online;
'''+poll+r'''
}
static void reset(void) {
    memset(&adapter, 0, sizeof(adapter));
    memset(&db_value, 0, sizeof(db_value));
    adapter.SCSIReq = &ior;
    adapter.scsiCommand = cdb;
    adapter.Cmd.scsi_Command = cdb;
    ior.io_Data = &adapter.Cmd;
    now = ready_at = info_io_error = info_status = enable_failures = 0;
    short_write = info_calls = enable_calls = disable_calls = unsupported = 0;
    reply_length = sizeof(struct SCSIWifi_NetworkEntry) + 2;
    errors = events = last_event = rejected = 0;
    currentWifiState = packetEnabled = lastWifiStatus = 1;
    db_value.db_online = db_value.db_currentWifiState = 1;
    lastEnableAttempt = 0; enableAttempted = FALSE;
    timeLastWifiCheck.tv_secs = 0;
}
int main(void) {
    UBYTE frame[60] = {0};
    reset(); enable_failures = 1;
    assert(!SCSIWifi_enable(&adapter, 1)); /* io_Error with GOOD status */
    assert(adapter.result.ioError == 20 && adapter.result.status == 0);
    assert(SCSIWifi_enable(&adapter, 1));
    reset(); info_io_error = 20;
    assert(!SCSIWifi_sendFrame(&adapter, frame, sizeof(frame)));
    assert(!SCSIWifi_AmigaNetSendFrames(&adapter, frame, sizeof(frame)));
    info_io_error = 0; short_write = 1;
    assert(!SCSIWifi_sendFrame(&adapter, frame, sizeof(frame)));
    assert(!SCSIWifi_AmigaNetSendFrames(&adapter, frame, sizeof(frame)));
    short_write = 0;
    assert(SCSIWifi_sendFrame(&adapter, frame, sizeof(frame)));
    reset(); reply_length = 4;
    assert(SCSIWifi_linkState(&adapter) == swlsError);
    assert(!adapter.linkInfoUnsupported);
    reset(); unsupported = 1;
    assert(SCSIWifi_linkState(&adapter) == swlsUnsupported);
    assert(SCSIWifi_linkState(&adapter) == swlsUnsupported && info_calls == 1);
    reset(); info_status = 2;
    assert(SCSIWifi_linkState(&adapter) == swlsError);
    assert(!adapter.linkInfoUnsupported);
    reset(); info_io_error = 20;
    assert(SCSIWifi_linkState(&adapter) == swlsError);
    assert(!adapter.linkInfoUnsupported);
    reset(); ready_at = 6;
    assert(prepareLink(&db_value, &adapter));
    assert(now == 6 && enable_calls == 1 && disable_calls == 0);
    reset(); enable_failures = 2;
    assert(prepareLink(&db_value, &adapter));
    assert(now == 2 && enable_calls == 3 && errors == 1);
    reset(); ready_at = 100;
    assert(!prepareLink(&db_value, &adapter));
    assert(now == 40 && enable_calls == 1 && disable_calls == 1);
    reset(); info_io_error = 20;
    assert(!prepareLink(&db_value, &adapter));
    assert(now == 40 && disable_calls == 1);
    reset(); db_value.db_quit = 1;
    assert(!prepareLink(&db_value, &adapter));
    assert(now == 0 && enable_calls == 0);
    reset(); unsupported = 1;
    assert(prepareLink(&db_value, &adapter) && now == 0);
    reset(); now = 5; ready_at = 10;
    step(); assert(!db_value.db_currentWifiState && last_event == S2EVENT_OFFLINE);
    assert(packetEnabled && disable_calls == 0 && rejected == 1);
    now = 10; step();
    assert(db_value.db_currentWifiState && last_event == S2EVENT_ONLINE);
    assert(enable_calls == 0 && disable_calls == 0);
    reset(); now = 5; info_io_error = 20;
    step(); assert(!db_value.db_currentWifiState && errors == 1);
    now = 6; info_io_error = 0; step();
    assert(db_value.db_currentWifiState);
    reset(); db_value.db_online = 0; step();
    assert(!packetEnabled && !db_value.db_currentWifiState && disable_calls == 1);
    db_value.db_online = 1; enable_failures = 1; ++now; step();
    assert(!packetEnabled && !db_value.db_currentWifiState);
    step(); assert(enable_calls == 1); /* retry is rate limited */
    ++now; step(); assert(packetEnabled && db_value.db_currentWifiState);
    reset(); db_value.db_currentWifiState = 0; step(); /* rapid offline/online */
    assert(db_value.db_currentWifiState && last_event == S2EVENT_ONLINE);
    puts("PASS: transport errors, short writes, startup waits, timeout, wired fallback, link recovery and online retries");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="scsidayna-readiness-test-") as tmp:
    source = Path(tmp) / "test.c"
    binary = Path(tmp) / "test"
    source.write_text(harness)
    subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
