/*
  toolstart.c - minimal startup code for the freestanding Shell tools
  (GCC build only; VBCC uses its own startup).

  Must be the first object on the link line and is compiled with
  -fno-toplevel-reorder, so the jump below - placed before any #include,
  as headers can emit code too - is the first instruction of the first
  hunk. It sets up SysBase and DOSBase for the inline library calls, and
  replies the Workbench startup message if a tool is started from an icon
  (the tools are Shell-only and then just exit).

  Only exec calls are made here: <proto/dos.h> would emit its static
  varargs helpers into this object.
*/
__asm__(
"	.text\n"
"	.even\n"
"	.globl	__start\n"
"__start:\n"
"	jra	_toolstart\n"
);

#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <workbench/startup.h>
#include <proto/exec.h>

struct ExecBase *SysBase;
struct DosLibrary *DOSBase;

int main(void);

int toolstart(void)
{
	struct Process *me;
	struct WBStartup *wbmsg = NULL;
	int rc = 20;

	SysBase = *(struct ExecBase **)4;
	me = (struct Process *)FindTask(NULL);
	if (!me->pr_CLI) {
		WaitPort(&me->pr_MsgPort);
		wbmsg = (struct WBStartup *)GetMsg(&me->pr_MsgPort);
	}

	DOSBase = (struct DosLibrary *)OpenLibrary((CONST_STRPTR)"dos.library", 37);
	if (DOSBase) {
		if (!wbmsg)
			rc = main();
		CloseLibrary((struct Library *)DOSBase);
	}

	if (wbmsg) {
		Forbid();
		ReplyMsg(&wbmsg->sm_Message);
	}
	return rc;
}
