/*
 * UAE - the Un*x Amiga Emulator
 *
 * HTML user interface via Pepper used under Native Client.
 *
 * Messages from the HTML UI are dispatched via PostMessage to the Native
 * Client module.
 */

#include "gui.h"

#include "inputdevice.h"
#include "options.h"
#include "sysdeps.h"
#include "threaddep/thread.h"
#include "uae.h"
#include "writelog.h"

static uae_sem_t gui_sem;           // For mutual exclusion on pref settings
static smp_comm_pipe from_gui_pipe; // For sending messages from the GUI to UAE

static char *new_disk_string[4];
static char *gui_romname;

/*
 * Supported messages. Sent from the GUI to UAE via from_gui_pipe.
 */
enum uae_commands {
    UAECMD_START,
    UAECMD_STOP,
    UAECMD_QUIT,
    UAECMD_RESET,
    UAECMD_PAUSE,
    UAECMD_RESUME,
    UAECMD_DEBUG,
    UAECMD_SAVE_CONFIG,
    UAECMD_EJECTDISK,
    UAECMD_INSERTDISK,
    UAECMD_SELECT_ROM,
    UAECMD_SAVESTATE_LOAD,
    UAECMD_SAVESTATE_SAVE
};

/* handle_message()
 *
 * This is called from the GUI when a GUI event happened. Specifically,
 * HandleMessage (PPP_Messaging) forwards dispatched UI action
 * messages posted from JavaScript to handle_message().
 */
int handle_message(const char* msg) {
    // Grammar for messages from the UI:
    //
    // message ::= 'insert' drive fileURL
    //           | 'rom' fileURL
    //           | 'connect' port input
    //           | 'eject' drive
    //           | 'reset'
    // device  ::= 'kickstart' | drive
    // drive   ::= 'df0' | 'df1'
    // port    ::= 'port0' | 'port1'
    // input   ::= 'mouse' | 'joy0' | 'joy1' | 'kbd0' | 'kbd1'
    // fileURL ::= <a URL of the form file://>

    DEBUG_LOG("%s\n", msg);

    // TODO(cstefansen): scan the string instead of these shenanigans.

    // Copy to non-const buffer
    char buf[1024];
    (void) strncpy(buf, msg, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0'; // Ensure NUL termination

    // Tokenize message up to 3 tokens (max given the grammar)
    int i = 0;
    char *t[3], *token, *rest = NULL, *sep = " ";

    for (token = strtok_r(buf, sep, &rest);
         token != NULL && i <= 3;
         token = strtok_r(NULL, sep, &rest), ++i) {
        t[i] = token;
    }

    // Pipe message to UAE main thread
    if (i == 1 && !strcmp(t[0], "reset")) {
        write_comm_pipe_int(&from_gui_pipe, UAECMD_RESET, 1);
    } else if (i == 2 && !strcmp(t[0], "eject")) {
        int drive_num;
        if (!strcmp(t[1], "df0")) {
            drive_num = 0;
        } else if (!strcmp(t[1], "df1")) {
            drive_num = 1;
        } else {
            return -1;
        }
        write_comm_pipe_int(&from_gui_pipe, UAECMD_EJECTDISK, 0);
        write_comm_pipe_int(&from_gui_pipe, drive_num, 1);
    } else if (i == 3 && !strcmp(t[0], "insert")) {
        int drive_num;
        if (!strcmp(t[1], "df0")) {
            drive_num = 0;
        } else if (!strcmp(t[1], "df1")) {
            drive_num = 1;
        } else {
            return -1;
        }
        uae_sem_wait(&gui_sem);
        if (new_disk_string[drive_num] != 0)
            free (new_disk_string[drive_num]);
        new_disk_string[drive_num] = strdup(t[2]);
        uae_sem_post(&gui_sem);
        write_comm_pipe_int (&from_gui_pipe, UAECMD_INSERTDISK, 0);
        write_comm_pipe_int (&from_gui_pipe, drive_num, 1);
    } else if (i == 2 && !strcmp(t[0], "rom")) {
        uae_sem_wait(&gui_sem);
        if (gui_romname != 0)
            free (gui_romname);
        gui_romname = strdup(t[1]);
        uae_sem_post(&gui_sem);
        write_comm_pipe_int(&from_gui_pipe, UAECMD_SELECT_ROM, 0);
    } else if (i == 3 && !strcmp(t[0], "connect")) {
        int port_num;
        if (!strcmp(t[1], "port0")) {
            port_num = 0;
        } else if (!strcmp(t[1], "port1")) {
            port_num = 1;
        } else {
            return -1;
        }

        int input_device =
                !strcmp(t[2], "mouse") ? JSEM_MICE :
                !strcmp(t[2], "joy0") ? JSEM_JOYS :
                !strcmp(t[2], "joy1") ? JSEM_JOYS + 1 :
                !strcmp(t[2], "kbd0") ? JSEM_KBDLAYOUT + 1 :
                !strcmp(t[2], "kbd1") ? JSEM_KBDLAYOUT + 2 :
                JSEM_END;

        changed_prefs.jports[port_num].id = input_device;
        if (changed_prefs.jports[port_num].id != currprefs.jports[port_num].id) {
            // It's a little fishy that the typical way to update input
            // devices doesn't use the comm pipe.
            inputdevice_updateconfig (&changed_prefs);
            inputdevice_config_change();
        }
    } else {
        return -1;
    }
    return 0;
}

/* TODO(cstefansen): Factor out general descriptions like the following to
 * gui.h.
 */
/*
 * gui_init()
 *
 * This is called from the main UAE thread to tell the GUI to initialize.
 * To indicate failure to initialize, return -1.
 */
int gui_init (void)
{
    init_comm_pipe (&from_gui_pipe, 20 /* size */, 1 /* chunks */);
    uae_sem_init (&gui_sem, 0, 1);
    return 0;
}

/*
 * gui_update()
 *
 * This is called from the main UAE thread to tell the GUI to update itself
 * using the current state of currprefs. This function will block
 * until it receives a message from the GUI telling it that the update
 * is complete.
 */
int gui_update (void)
{
    DEBUG_LOG("gui_update() unimplemented for HTML GUI.\n");
    return 0;
}

/*
 * gui_exit()
 *
 * This called from the main UAE thread to tell the GUI to quit gracefully.
 */
void gui_exit (void) {}

/*
 * gui_led()
 *
 * Called from the main UAE thread to inform the GUI
 * of disk activity so that indicator LEDs may be refreshed.
 */
void gui_led (int num, int on) {}

/*
 * gui_handle_events()
 *
 * This is called from the main UAE thread to process events sent from
 * the GUI thread.
 *
 * If the UAE emulation proper is not running yet or is paused,
 * this loops continuously waiting for and responding to events
 * until the emulation is started or resumed, respectively. When
 * the emulation is running, this is called periodically from
 * the main UAE event loop.
 */
void gui_handle_events (void)
{
    // read GUI command if any

    // process it, e.g., call uae_reset()
    while (comm_pipe_has_data (&from_gui_pipe)) {
        int cmd = read_comm_pipe_int_blocking (&from_gui_pipe);
        printf("gui_handle_events: %i\n", cmd);

        switch (cmd) {
        case UAECMD_EJECTDISK: {
            int n = read_comm_pipe_int_blocking (&from_gui_pipe);
            uae_sem_wait (&gui_sem);
            changed_prefs.floppyslots[n].df[0] = '\0';
            uae_sem_post (&gui_sem);
// TODO(cstefansen): Wire up notifications from UAE to GUI
//            if (pause_uae) {
//                /* When UAE is running it will notify the GUI when a disk has been inserted
//                 * or removed itself. When UAE is paused, however, we need to do this ourselves
//                 * or the change won't be realized in the GUI until UAE is resumed */
//                write_comm_pipe_int (&to_gui_pipe, GUICMD_DISKCHANGE, 0);
//                write_comm_pipe_int (&to_gui_pipe, n, 1);
//            }
            break;
        }
        case UAECMD_INSERTDISK: {
            int n = read_comm_pipe_int_blocking (&from_gui_pipe);
            uae_sem_wait (&gui_sem);
            strncpy (changed_prefs.floppyslots[n].df, new_disk_string[n], 255);
            free (new_disk_string[n]);
            new_disk_string[n] = 0;
            changed_prefs.floppyslots[n].df[255] = '\0';
            uae_sem_post (&gui_sem);
        }
// TODO(cstefansen): Wire up notifications from UAE to GUI
//            if (pause_uae) {
//                /* When UAE is running it will notify the GUI when a disk has been inserted
//                 * or removed itself. When UAE is paused, however, we need to do this ourselves
//                 * or the change won't be realized in the GUI until UAE is resumed */
//                write_comm_pipe_int (&to_gui_pipe, GUICMD_DISKCHANGE, 0);
//                write_comm_pipe_int (&to_gui_pipe, n, 1);
//            }
            break;
        case UAECMD_RESET:
            uae_reset(0);
            break;
// TODO(cstefansen): Implement pausing/resuming.
//        case UAECMD_PAUSE:
//            pause_uae = 1;
//            uae_pause ();
//            break;
//        case UAECMD_RESUME:
//            pause_uae = 0;
//            uae_resume ();
//            break;
        case UAECMD_SELECT_ROM:
            uae_sem_wait (&gui_sem);
            strncpy (changed_prefs.romfile, gui_romname, 255);
            changed_prefs.romfile[255] = '\0';
            free (gui_romname);
            gui_romname = 0;
            uae_sem_post (&gui_sem);
            break;
        default:
            break;
        }
    }
}

/*
 * gui_filename()
 *
 * This is called from the main UAE thread to inform
 * the GUI that a floppy disk has been inserted or ejected.
 */
void gui_filename (int num, const char *name) {}

/* gui_fps()
 *
 * This is called from the main UAE thread to provide the GUI with
 * the most recent FPS and idle numbers.
 */
void gui_fps (int fps, int idle) {
    gui_data.fps  = fps;
    gui_data.idle = idle;
}

/* gui_lock() */
void gui_lock (void) {}

/* gui_unlock() */
void gui_unlock (void) {}

/* gui_flicker_led()
 *
 * This is called from the main UAE thread to tell the GUI that a particular
 * drive LED should flicker to indicate I/O activity.
 */
void gui_flicker_led (int led, int unitnum, int status) {}

/* gui_disk_image_change() */
void gui_disk_image_change (int unitnum, const TCHAR *name, bool writeprotected) {}

/* gui_display() */
void gui_display (int shortcut) {}

/* gui_gameport_button_change() */
void gui_gameport_button_change (int port, int button, int onoff) {}

/* gui_gameport_axis_change */
void gui_gameport_axis_change (int port, int axis, int state, int max) {}

/* gui_message() */
void gui_message (const char *format,...) {}
