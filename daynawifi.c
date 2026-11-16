/*
 * daynawifi - the Wi-Fi of a BlueSCSI/ZuluSCSI DaynaPORT, from a Shell.
 *
 *   daynawifi            what the adapter says it is joined to, and its signal
 *   daynawifi REJOIN     reset its packet buffer and join the network again -
 *                        for an adapter that says "connected" while the router
 *                        has dropped it (no packets come in, and nothing short
 *                        of a power cycle used to bring it back)
 *   daynawifi SCAN       the networks it can see
 *
 * The adapter, its SCSI driver and ID, and the network name and key come from
 * ENV:scsidayna.prefs (SSID= and KEY= lines, as for AUTOCONNECT); SSID= and
 * KEY= on the command line override them. Stop the TCP/IP stack first when it
 * is using the adapter (the driver polls the adapter while a stack is up).
 *
 * Same licence as scsidayna.device (licence.txt).
 */
#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <dos/rdargs.h>
#include <stdarg.h>
#include <string.h>
#include "scsiwifi.h"

#ifdef __GNUC__
__attribute__((used))
#endif
static const char version[] = "$VER: daynawifi 1.0 (24.10.2026)";

struct Library *UtilityBase;

/* Printf() with a plain char format; numbers must be passed as LONG (%ld) */
static void say(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VPrintf((CONST_STRPTR)fmt, (APTR)ap);
    va_end(ap);
}

/* bounded copy of a command line argument, always NUL-terminated */
static void copyArg(char *dest, const char *src, ULONG size)
{
    ULONG i;
    for (i = 0; i + 1 < size && src[i]; i++)
        dest[i] = src[i];
    dest[i] = '\0';
}

static void show(SCSIWIFIDevice dev)
{
    struct SCSIWifi_NetworkEntry net;
    struct SCSIWifi_MACAddress mac;
    memset(&net, 0, sizeof(net));
    if (SCSIWifi_getNetwork(dev, &net) && net.rssi)
        say("joined to \"%s\", signal %ld dB, channel %ld\n", (LONG)net.ssid, (LONG)net.rssi, (LONG)net.channel);
    else
        PutStr("not joined to any network\n");
    if (SCSIWifi_getMACAddress(dev, &mac) && mac.valid)
        say("adapter MAC %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n", (LONG)mac.address[0], (LONG)mac.address[1],
               (LONG)mac.address[2], (LONG)mac.address[3], (LONG)mac.address[4], (LONG)mac.address[5]);
}

static int scan(SCSIWIFIDevice dev)
{
    enum SCSIWifi_ScanStatus st;
    struct SCSIWifi_ScanResults *res;
    int i, t;
    if (!SCSIWifi_scan(dev, &st)) { PutStr("the adapter refused to scan\n"); return 10; }
    for (t = 0; t < 30; t++) {
        Delay(25);
        if (!SCSIWifi_scanComplete(dev, &st) || st != swssBusy) break;
    }
    res = AllocVec(sizeof(*res), MEMF_PUBLIC | MEMF_CLEAR);
    if (!res) return 20;
    if (SCSIWifi_getScanResults(dev, res)) {
        say("%ld network%s:\n", (LONG)res->count, (LONG)(res->count == 1 ? "" : "s"));
        for (i = 0; i < res->count && i < SCSIWIFI_MAX_NETWORK_COUNT; i++)
            say("  %-32s %4ld dB  ch %ld\n", (LONG)res->networks[i].ssid, (LONG)res->networks[i].rssi,
                   (LONG)res->networks[i].channel);
    } else
        PutStr("no scan results\n");
    FreeVec(res);
    return 0;
}

int main(void)
{
    LONG args[4] = { 0 };
    struct RDArgs *rda;
    struct ScsiDaynaSettings s;
    struct SCSIDevice_OpenData od;
    enum SCSIWifi_OpenResult err;
    SCSIWIFIDevice dev;
    int rc = 0;

    rda = ReadArgs("REJOIN/S,SCAN/S,SSID/K,KEY/K", args, NULL);
    if (!rda) { PrintFault(IoErr(), "daynawifi"); return 10; }
    UtilityBase = OpenLibrary("utility.library", 37);
    if (!UtilityBase) { FreeArgs(rda); return 20; }

    SCSIWifi_defaultSettings(&s);
    if (!SCSIWifi_loadSettings((void *)UtilityBase, (void *)DOSBase, &s))
        say("no ENV:scsidayna.prefs - using the defaults (%s)\n", (LONG)s.deviceName);
    if (args[2]) copyArg(s.ssid, (const char *)args[2], sizeof(s.ssid));
    if (args[3]) copyArg(s.key, (const char *)args[3], sizeof(s.key));

    memset(&od, 0, sizeof(od));
    od.sysBase = SysBase;
    od.utilityBase = (struct UtilityBase *)UtilityBase;
    od.dosBase = (struct DosBase *)DOSBase;
    od.scsiMode = s.scsiMode;
    od.deviceDriverName = s.deviceName;
    if (s.deviceID >= 0 && s.deviceID <= 7) {
        od.deviceID = s.deviceID;
        dev = SCSIWifi_open(&od, &err);
    } else {
        /* DEVICEID=-1: the adapter is on whichever unit answers as one (as the driver looks) */
        int id;
        for (id = 0, dev = NULL; id < 8 && !dev; id++) {
            od.deviceID = id;
            dev = SCSIWifi_open(&od, &err);
        }
        if (dev) say("adapter on %s unit %ld\n", (LONG)s.deviceName, (LONG)od.deviceID);
    }
    if (!dev) {
        say("no DaynaPORT Wi-Fi adapter on %s (unit %ld): open result %ld\n", (LONG)s.deviceName, (LONG)s.deviceID, (LONG)err);
        rc = 10;
    } else if (args[1]) {
        rc = scan(dev);
    } else if (args[0]) {
        struct SCSIWifi_JoinRequest req;
        struct SCSIWifi_NetworkEntry net;
        int t;
        if (!s.ssid[0]) {
            PutStr("no SSID: put SSID= and KEY= in ENV:scsidayna.prefs, or give them here\n");
            rc = 10;
        } else {
            PutStr("before: "); show(dev);
            /* enable off/on resets the adapter's packet buffer (scsiwifi.h) */
            SCSIWifi_enable(dev, 0);
            Delay(25);
            SCSIWifi_enable(dev, 1);
            Delay(25);
            memset(&req, 0, sizeof(req));
            /* s.ssid/s.key are NUL-terminated within the same 64 bytes */
            strcpy(req.ssid, s.ssid);
            strcpy(req.key, s.key);
            say("joining \"%s\"\n", (LONG)req.ssid);
            if (!SCSIWifi_joinNetwork(dev, &req))
                PutStr("the adapter refused the join request\n");
            for (t = 0, rc = 5; t < 40; t++) {         /* up to 40 s */
                Delay(50);
                memset(&net, 0, sizeof(net));
                if (SCSIWifi_getNetwork(dev, &net) && net.rssi && !strcmp(net.ssid, req.ssid)) {
                    say("joined after %ld s: ", (LONG)(t + 1)); show(dev);
                    rc = 0;
                    break;
                }
            }
            if (rc) PutStr("not joined after 40 s\n");
        }
    } else
        show(dev);
    if (dev) SCSIWifi_close(dev);
    CloseLibrary((struct Library *)UtilityBase);
    FreeArgs(rda);
    return rc;
}
