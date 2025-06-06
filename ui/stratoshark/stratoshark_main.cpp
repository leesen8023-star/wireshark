/* stratoshark_main.cpp
 *
 * Stratoshark - System call and event log analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <config.h>
#define WS_LOG_DOMAIN  LOG_DOMAIN_MAIN

#include <locale.h>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#include <wchar.h>
#include <shellapi.h>
#include <wsutil/console_win32.h>
#endif

#include <ws_exit_codes.h>
#include <wsutil/application_flavor.h>
#include <wsutil/clopts_common.h>
#include <wsutil/cmdarg_err.h>
#include <ui/urls.h>
#include <wsutil/time_util.h>
#include <wsutil/filesystem.h>
#include <wsutil/privileges.h>
#include <wsutil/socket.h>
#include <wsutil/wslog.h>
#ifdef HAVE_PLUGINS
#include <wsutil/plugins.h>
#endif
#include <wsutil/please_report_bug.h>
#include <wsutil/unicode-utils.h>
#include <wsutil/version_info.h>

#include <epan/addr_resolv.h>
#include <epan/ex-opt.h>
#include <epan/tap.h>
#include <epan/stat_tap_ui.h>
#include <epan/column.h>
#include <epan/disabled_protos.h>
#include <epan/prefs.h>

#ifdef HAVE_KERBEROS
#include <epan/packet.h>
#include <epan/asn1.h>
#include <epan/dissectors/packet-kerberos.h>
#endif

#include <wsutil/codecs.h>

#include <extcap.h>

/* general (not Qt specific) */
#include "file.h"
#include "epan/color_filters.h"

#include "epan/rtd_table.h"
#include "epan/srt_table.h"

#include "ui/alert_box.h"
#include "ui/iface_lists.h"
#include "ui/language.h"
#include "ui/persfilepath_opt.h"
#include "ui/recent.h"
#include "ui/simple_dialog.h"
#include "ui/util.h"
#include "ui/dissect_opts.h"
#include "ui/commandline.h"
#include "ui/capture_ui_utils.h"
#include "ui/preference_utils.h"
#include "ui/software_update.h"
#include "ui/taps.h"

#include "ui/qt/conversation_dialog.h"
#include "ui/qt/utils/color_utils.h"
#include "ui/qt/coloring_rules_dialog.h"
#include "ui/qt/endpoint_dialog.h"
#include "ui/qt/glib_mainloop_on_qeventloop.h"
#include "ui/stratoshark/stratoshark_main_window.h"
#include "ui/qt/simple_dialog.h"
#include "ui/qt/simple_statistics_dialog.h"
#include <ui/qt/widgets/splash_overlay.h>
#include "ui/stratoshark/stratoshark_application.h"

#include "capture/capture-pcap-util.h"

#include <QMessageBox>
#include <QScreen>

#ifdef _WIN32
#  include "capture/capture-wpcap.h"
#  include <wsutil/file_util.h>
#endif /* _WIN32 */

#include "epan/crypt/dot11decrypt_ws.h"

/* Handle the addition of View menu items without request */
#if defined(Q_OS_MAC)
#include <ui/macosx/cocoa_bridge.h>
#endif

#include <ui/qt/utils/qt_ui_utils.h>

//#define DEBUG_STARTUP_TIME 1

/* update the main window */
void main_window_update(void)
{
    StratosharkApplication::processEvents();
}

void exit_application(int status) {
    if (ssApp) {
        ssApp->quit();
    }
    exit(status);
}

/*
 * Report an error in command-line arguments.
 *
 * On Windows, Stratoshark is built for the Windows subsystem, and runs
 * without a console, so we create a console on Windows to receive the
 * output.
 *
 * See create_console(), in ui/win32/console_win32.c, for an example
 * of code to check whether we need to create a console.
 *
 * On UN*Xes:
 *
 *  If Stratoshark is run from the command line, its output either goes
 *  to the terminal or to wherever the standard error was redirected.
 *
 *  If Stratoshark is run by executing it as a remote command, e.g. with
 *  ssh, its output either goes to whatever socket was set up for the
 *  remote command's standard error or to wherever the standard error
 *  was redirected.
 *
 *  If Stratoshark was run from the GUI, e.g. by double-clicking on its
 *  icon or on a file that it opens, there are no guarantees as to
 *  where the standard error went.  It could be going to /dev/null
 *  (current macOS), or to a socket to systemd for the journal, or
 *  to a log file in the user's home directory, or to the "console
 *  device" ("workstation console"), or....
 *
 *  Part of determining that, at least for locally-run Stratoshark,
 *  is to try to open /dev/tty to determine whether the process
 *  has a controlling terminal.  (It fails, at a minimum, for
 *  Stratoshark launched from the GUI under macOS, Ubuntu with GNOME,
 *  and Ubuntu with KDE; in all cases, an attempt to open /dev/tty
 *  fails with ENXIO.)  If it does have a controlling terminal,
 *  write to the standard error, otherwise assume that the standard
 *  error might not go anywhere that the user will be able to see.
 *  That doesn't handle the "run by ssh" case, however; that will
 *  not have a controlling terminal.  (This means running it by
 *  remote execution, not by remote login.)  Perhaps there's an
 *  environment variable to check there.
 */
// xxx copied from ../gtk/main.c
static void
stratoshark_cmdarg_err(const char *fmt, va_list ap)
{
#ifdef _WIN32
    create_console();
#endif
    fprintf(stderr, "stratoshark: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

/*
 * Report additional information for an error in command-line arguments.
 * Creates a console on Windows.
 */
// xxx copied from ../gtk/main.c
static void
stratoshark_cmdarg_err_cont(const char *fmt, va_list ap)
{
#ifdef _WIN32
    create_console();
#endif
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

void
gather_wireshark_qt_compiled_info(feature_list l)
{
#ifdef QT_VERSION
    with_feature(l, "Qt %s", QT_VERSION_STR);
#else
    with_feature(l, "Qt (version unknown)");
#endif
    gather_caplibs_compile_info(l);
    epan_gather_compile_info(l);
#ifdef QT_MULTIMEDIA_LIB
    with_feature(l, "QtMultimedia");
#else
    without_feature(l, "QtMultimedia");
#endif

    const char *update_info = software_update_info();
    if (update_info) {
        with_feature(l, "automatic updates using %s", update_info);
    } else {
        without_feature(l, "automatic updates");
    }

#ifdef HAVE_MINIZIP
    with_feature(l, "Minizip");
#else
    without_feature(l, "Minizip");
#endif

#ifdef HAVE_SINSP
    with_feature(l, "falcosecurity-libs sinsp/scap %d.%d.%d", SINSP_VERSION_MAJOR, SINSP_VERSION_MINOR, SINSP_VERSION_MICRO);
#else
    without_feature(l, "falcosecurity-libs sinsp/scap");
#endif
}

void
gather_wireshark_runtime_info(feature_list l)
{
    with_feature(l, "Qt %s", qVersion());
#ifdef HAVE_LIBPCAP
    gather_caplibs_runtime_info(l);
#endif
    epan_gather_runtime_info(l);

    if (mainApp) {
        // Display information
        const char *display_mode = ColorUtils::themeIsDark() ? "dark" : "light";
        with_feature(l, "%s display mode", display_mode);

        int hidpi_count = 0;
        foreach (QScreen *screen, mainApp->screens()) {
            if (screen->devicePixelRatio() > 1.0) {
                hidpi_count++;
            }
        }
        if (hidpi_count == mainApp->screens().count()) {
            with_feature(l, "HiDPI");
        } else if (hidpi_count) {
            with_feature(l, "mixed DPI");
        } else {
            without_feature(l, "HiDPI");
        }
    }
}

static void
qt_log_message_handler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    enum ws_log_level log_level = LOG_LEVEL_DEBUG;

    switch (type) {
    case QtInfoMsg:
        log_level = LOG_LEVEL_INFO;
        break;
    // We want qDebug() messages to show up at our default log level.
    case QtDebugMsg:
    case QtWarningMsg:
        log_level = LOG_LEVEL_WARNING;
        break;
    case QtCriticalMsg:
        log_level = LOG_LEVEL_CRITICAL;
        break;
    case QtFatalMsg:
        log_level = LOG_LEVEL_ERROR;
        break;
    default:
        break;
    }
    ws_log(LOG_DOMAIN_QTUI, log_level, "%s", qUtf8Printable(msg));
}

#ifdef HAVE_LIBPCAP
/*  Check if there's something important to tell the user during startup.
 *  We want to do this *after* showing the main window so that any windows
 *  we pop up will be above the main window.
 */
static void
check_and_warn_user_startup()
{
    char                *cur_user, *cur_group;

    /* Tell the user not to run as root. */
    if (running_with_special_privs() && recent.privs_warn_if_elevated) {
        cur_user = get_cur_username();
        cur_group = get_cur_groupname();
        simple_message_box(ESD_TYPE_WARN, &recent.privs_warn_if_elevated,
        "Running as user \"%s\" and group \"%s\".\n"
        "This could be dangerous.\n\n"
        "If you're running Stratoshark this way in order to perform live capture, "
        "you may want to be aware that there is a better way documented at\n"
        WS_WIKI_URL("CaptureSetup/CapturePrivileges"), cur_user, cur_group);
        g_free(cur_user);
        g_free(cur_group);
    }
}
#endif

#ifdef _WIN32
// Try to avoid library search path collisions. QCoreApplication will
// search QT_INSTALL_PREFIX/plugins for platform DLLs before searching
// the application directory. If
//
// - You have Qt version 5.x.y installed in the default location
//   (C:\Qt\5.x) on your machine.
//
// and
//
// - You install Stratoshark that was built on a machine with Qt version
//   5.x.z installed in the default location.
//
// Qt5Core.dll will load qwindows.dll from your local C:\Qt\5.x\...\plugins
// directory. This may not be compatible with qwindows.dll from that
// same path on the build machine. At any rate, loading DLLs from paths
// you don't control is ill-advised. We work around this by removing every
// path except our application directory.

static inline void
win32_reset_library_path(void)
{
    QString app_path = QDir(get_progfile_dir()).path();
    foreach (QString path, QCoreApplication::libraryPaths()) {
        QCoreApplication::removeLibraryPath(path);
    }
    QCoreApplication::addLibraryPath(app_path);
}
#endif

#ifdef Q_OS_MAC
// Try to work around
//
//     https://gitlab.com/wireshark/wireshark/-/issues/17075
//
// aka
//
//     https://bugreports.qt.io/browse/QTBUG-87014
//
// The fix at
//
//     https://codereview.qt-project.org/c/qt/qtbase/+/322228/3/src/plugins/platforms/cocoa/qnsview_drawing.mm
//
// enables layer backing if we're running on Big Sur OR we're running on
// Catalina AND we were built with the Catalina SDK. Enable layer backing
// here by setting QT_MAC_WANTS_LAYER=1, but only if we're running on Big
// Sur and our version of Qt doesn't have a fix for QTBUG-87014.
#include <QOperatingSystemVersion>
static inline void
macos_enable_layer_backing(void)
{
    // At the time of this writing, the QTBUG-87014 for layerEnabledByMacOS is...
    //
    // ...in https://github.com/qt/qtbase/blob/5.15/src/plugins/platforms/cocoa/qnsview_drawing.mm
    // ...not in https://github.com/qt/qtbase/blob/5.15.2/src/plugins/platforms/cocoa/qnsview_drawing.mm
    // ...not in https://github.com/qt/qtbase/blob/6.0/src/plugins/platforms/cocoa/qnsview_drawing.mm
    // ...not in https://github.com/qt/qtbase/blob/6.0.0/src/plugins/platforms/cocoa/qnsview_drawing.mm
    //
    // We'll assume that it will be fixed in 5.15.3, 6.0.1, and >= 6.1
    // Note that we only ship LTS versions of Qt with our macOS packages.
    // Feel free to add other versions if needed.
#if  \
        ((QT_VERSION >= QT_VERSION_CHECK(5, 15, 0) &&  QT_VERSION < QT_VERSION_CHECK(5, 15, 3)) \
        || (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) &&  QT_VERSION < QT_VERSION_CHECK(6, 0, 1)) \
    )
    QOperatingSystemVersion os_ver = QOperatingSystemVersion::current();
    int major_ver = os_ver.majorVersion();
    int minor_ver = os_ver.minorVersion();
    if ( (major_ver == 10 && minor_ver >= 16) || major_ver >= 11 ) {
        if (qgetenv("QT_MAC_WANTS_LAYER").isEmpty()) {
            qputenv("QT_MAC_WANTS_LAYER", "1");
        }
    }
#endif
}
#endif

#ifdef HAVE_LIBPCAP
static GList *
capture_opts_get_interface_list(int *err _U_, char **err_str _U_)
{
    // Stratoshark only wants the IF_EXTCAP interfaces, so there's no point
    // in spawning dumpcap to retrieve the other types of interfaces.
#if 0
    if (mainApp) {
        GList *if_list = mainApp->getInterfaceList();
        if (if_list == NULL) {
            if_list = capture_interface_list(err, err_str, main_window_update);
            mainApp->setInterfaceList(if_list);
        }
        return if_list;
    }
    return capture_interface_list(err, err_str, main_window_update);
#endif
    return append_extcap_interface_list(NULL);
}
#endif

/* And now our feature presentation... [ fade to music ] */
int main(int argc, char *qt_argv[])
{
    StratosharkMainWindow *main_w;

#ifdef _WIN32
    LPWSTR              *wc_argv;
    int                  wc_argc;
#endif
    int                  ret_val = EXIT_SUCCESS;
    char               **argv = qt_argv;

    char                *rf_path;
    int                  rf_open_errno;
#ifdef HAVE_LIBPCAP
    char                *err_str, *err_str_secondary;
#endif
    char                *err_msg = NULL;
    df_error_t          *df_err = NULL;
    e_prefs             *prefs_p;

    QString              dfilter, read_filter;
#ifdef HAVE_LIBPCAP
    int                  caps_queries = 0;
#endif
    /* Start time in microseconds */
    uint64_t start_time = g_get_monotonic_time();

    /* Set the program name. */
    g_set_prgname("stratoshark");

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    /*
     * See:
     *
     *    issue #16908;
     *
     *    https://doc.qt.io/qt-5/qvector.html#maximum-size-and-out-of-memory-conditions
     *
     *    https://forum.qt.io/topic/114950/qvector-realloc-throwing-sigsegv-when-very-large-surface3d-is-rendered
     *
     * for why we're doing this; the widget we use for the packet list
     * uses QVector, so those limitations apply to it.
     *
     * Apparently, this will be fixed in Qt 6:
     *
     *    https://github.com/qt/qtbase/commit/215ca735341b9487826023a7983382851ce8bf26
     *
     *    https://github.com/qt/qtbase/commit/2a6cdec718934ca2cc7f6f9c616ebe62f6912123#diff-724f419b0bb0487c2629bb16cf534c4b268ddcee89b5177189b607f940cfd83dR192
     *
     * Hopefully QList won't cause any performance hits relative to
     * QVector.
     *
     * We pick 53 million records as a value that should avoid the problem;
     * see the Wireshark issue for why that value was chosen.
     */
    cf_set_max_records(53000000);
#endif

#ifdef Q_OS_MAC
    macos_enable_layer_backing();
#endif

    cmdarg_err_init(stratoshark_cmdarg_err, stratoshark_cmdarg_err_cont);

    /* Initialize log handler early so we can have proper logging during startup. */
    ws_log_init(vcmdarg_err);
    /* For backward compatibility with GLib logging and Wireshark 3.4. */
    ws_log_console_writer_set_use_stdout(true);

    qInstallMessageHandler(qt_log_message_handler);

#ifdef _WIN32
    restore_pipes();
#endif

#ifdef DEBUG_STARTUP_TIME
    ws_log_console_open = LOG_CONSOLE_OPEN_ALWAYS;
#endif /* DEBUG_STARTUP_TIME */

#if defined(Q_OS_MAC)
    /* Disable automatic addition of tab menu entries in view menu */
    CocoaBridge::cleanOSGeneratedMenuItems();
#endif

    /*
     * Set the C-language locale to the native environment and set the
     * code page to UTF-8 on Windows.
     */
#ifdef _WIN32
    setlocale(LC_ALL, ".UTF-8");
#else
    setlocale(LC_ALL, "");
#endif

    ws_tzset();

#ifdef _WIN32
    //
    // On Windows, QCoreApplication has its own WinMain(), which gets the
    // command line using GetCommandLineW(), breaks it into individual
    // arguments using CommandLineToArgvW(), and then "helpfully"
    // converts those UTF-16LE arguments into strings in the local code
    // page.
    //
    // We don't want that, because not all file names can be represented
    // in the local code page, so we do the same, but we convert the
    // strings into UTF-8.
    //
    wc_argv = CommandLineToArgvW(GetCommandLineW(), &wc_argc);
    if (wc_argv) {
        argc = wc_argc;
        argv = arg_list_utf_16to8(wc_argc, wc_argv);
        LocalFree(wc_argv);
    } /* XXX else bail because something is horribly, horribly wrong? */

    create_app_running_mutex();
#endif /* _WIN32 */

    /* Early logging command-line initialization. */
    ws_log_parse_args(&argc, argv, commandline_optstring(), commandline_long_options(), vcmdarg_err, WS_EXIT_INVALID_OPTION);
    ws_noisy("Finished log init and parsing command line log arguments");

    /*
     * Get credential information for later use, and drop privileges
     * before doing anything else.
     * Let the user know if anything happened.
     */
    init_process_policies();
    relinquish_special_privs_perm();

    /*
     * Attempt to get the pathname of the directory containing the
     * executable file.
     */
    /* configuration_init_error = */ configuration_init(argv[0]);
    set_application_flavor(APPLICATION_FLAVOR_STRATOSHARK);
    /* ws_log(NULL, LOG_LEVEL_DEBUG, "progfile_dir: %s", get_progfile_dir()); */

#ifdef _WIN32
    ws_init_dll_search_path();
    /* Load wpcap if possible. Do this before collecting the run-time version information */
    load_wpcap();

#endif /* _WIN32 */

    /* Get the compile-time version information string */
    ws_init_version_info("Stratoshark", gather_wireshark_qt_compiled_info,
                         gather_wireshark_runtime_info);

    init_report_alert_box("Stratoshark");

    /* Create the user profiles directory */
    if (create_profiles_dir(&rf_path) == -1) {
        simple_dialog(ESD_TYPE_WARN, ESD_BTN_OK,
                      "Could not create profiles directory\n\"%s\": %s.",
                      rf_path, g_strerror(errno));
        g_free (rf_path);
    }

    profile_store_persconffiles(true);
    recent_init();

    /* Read the profile independent recent file.  We have to do this here so we can */
    /* set the profile before it can be set from the command line parameter */
    if (!recent_read_static(&rf_path, &rf_open_errno)) {
        simple_dialog(ESD_TYPE_WARN, ESD_BTN_OK,
                      "Could not open common recent file\n\"%s\": %s.",
                      rf_path, g_strerror(rf_open_errno));
        g_free(rf_path);
    }

    ret_val = commandline_early_options(argc, argv);
    if (ret_val != EXIT_SUCCESS) {
        if (ret_val == WS_EXIT_NOW) {
            return 0;
        }

        return ret_val;
    }

#ifdef _WIN32
    win32_reset_library_path();
#endif

    // Handle DPI scaling on Windows. This causes problems in at least
    // one case on X11 and we don't yet support Android.
    // We do the equivalent on macOS by setting NSHighResolutionCapable
    // in Info.plist.
    // Note that this enables Windows 8.1-style Per-monitor DPI
    // awareness but not Windows 10-style Per-monitor v2 awareness.
    // https://doc.qt.io/qt-5/scalability.html
    // https://doc.qt.io/qt-5/highdpi.html
    // https://bugreports.qt.io/browse/QTBUG-53022 - The device pixel ratio is pretty much bogus on Windows.
    // https://bugreports.qt.io/browse/QTBUG-55510 - Windows have wrong size
    //
    // Deprecated in Qt6, which is Per-Monitor DPI Aware V2 by default.
    //    warning: 'Qt::AA_EnableHighDpiScaling' is deprecated: High-DPI scaling is always enabled.
    //    This attribute no longer has any effect.
#if defined(Q_OS_WIN) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    // This function must be called before creating the application object.
    // Qt::HighDpiScaleFactorRoundingPolicy::PassThrough is the default in Qt6,
    // so this doesn't have any effect (Round is the default in 5.14 & 5.15)
#if defined(Q_OS_WIN)
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    /* Create The Stratoshark app */
    StratosharkApplication ss_app(argc, qt_argv);

    /* initialize the funnel mini-api */
    // xxx qtshark
    //initialize_funnel_ops();

    Dot11DecryptInitContext(&dot11decrypt_ctx);

    QString cf_name;
    unsigned int in_file_type = WTAP_TYPE_AUTO;

    err_msg = ws_init_sockets();
    if (err_msg != NULL)
    {
        cmdarg_err("%s", err_msg);
        g_free(err_msg);
        cmdarg_err_cont("%s", please_report_bug());
        ret_val = WS_EXIT_INIT_FAILED;
        goto clean_exit;
    }

    /* Read the profile dependent (static part) of the recent file. */
    /* Only the static part of it will be read, as we don't have the gui now to fill the */
    /* recent lists which is done in the dynamic part. */
    /* We have to do this already here, so command line parameters can overwrite these values. */
    if (!recent_read_profile_static(&rf_path, &rf_open_errno)) {
        simple_dialog(ESD_TYPE_WARN, ESD_BTN_OK,
                      "Could not open recent file\n\"%s\": %s.",
                      rf_path, g_strerror(rf_open_errno));
        g_free(rf_path);
    }
    ssApp->applyCustomColorsFromRecent();

    // Initialize our language
    read_language_prefs();
    ssApp->loadLanguage(language);

    /* ws_log(LOG_DOMAIN_MAIN, LOG_LEVEL_DEBUG, "Translator %s", language); */

    // Init the main window (and splash)
    main_w = new(StratosharkMainWindow);
    main_w->show();
    // Setup GLib mainloop on Qt event loop to enable GLib and GIO watches
    GLibMainloopOnQEventLoop::setup(main_w);
    // We may not need a queued connection here but it would seem to make sense
    // to force the issue.
    main_w->connect(&ss_app, SIGNAL(openCaptureFile(QString,QString,uint)),
            main_w, SLOT(openCaptureFile(QString,QString,uint)));
    main_w->connect(&ss_app, &StratosharkApplication::openCaptureOptions,
            main_w, &StratosharkMainWindow::showCaptureOptionsDialog);

    /*
     * If we have a saved "last directory in which a file was opened"
     * in the recent file, set it as the one for the app.
     *
     * (do this after the path settings are processed)
     */
    if (recent.gui_fileopen_remembered_dir &&
        test_for_directory(recent.gui_fileopen_remembered_dir) == EISDIR) {
      set_last_open_dir(recent.gui_fileopen_remembered_dir);
    }

#ifdef DEBUG_STARTUP_TIME
    ws_log(LOG_DOMAIN_MAIN, LOG_LEVEL_INFO, "set_console_log_handler, elapsed time %" PRIu64 " us \n", g_get_monotonic_time() - start_time);
#endif

#ifdef HAVE_LIBPCAP
    /* Set the initial values in the capture options. This might be overwritten
       by preference settings and then again by the command line parameters. */
    capture_opts_init(&global_capture_opts, capture_opts_get_interface_list);
#endif

    /*
     * Libwiretap must be initialized before libwireshark is, so that
     * dissection-time handlers for file-type-dependent blocks can
     * register using the file type/subtype value for the file type.
     */
    wtap_init(true);

    splash_update(RA_DISSECTORS, NULL, NULL);
#ifdef DEBUG_STARTUP_TIME
    ws_log(LOG_DOMAIN_MAIN, LOG_LEVEL_INFO, "Calling epan init, elapsed time %" PRIu64 " us \n", g_get_monotonic_time() - start_time);
#endif
    /* Register all dissectors; we must do this before checking for the
       "-G" flag, as the "-G" flag dumps information registered by the
       dissectors, and we must do it before we read the preferences, in
       case any dissectors register preferences. */
    if (!epan_init(splash_update, NULL, true)) {
        SimpleDialog::displayQueuedMessages(main_w);
        ret_val = WS_EXIT_INIT_FAILED;
        goto clean_exit;
    }
#ifdef DEBUG_STARTUP_TIME
    /* epan_init resets the preferences */
    ws_log_console_open = LOG_CONSOLE_OPEN_ALWAYS;
    ws_log(LOG_DOMAIN_MAIN, LOG_LEVEL_INFO, "epan done, elapsed time %" PRIu64 " us \n", g_get_monotonic_time() - start_time);
#endif

    /* Register all audio codecs. */
    codecs_init();

    // Read the dynamic part of the recent file. This determines whether or
    // not the recent list appears in the main window so the earlier we can
    // call this the better.
    if (!recent_read_dynamic(&rf_path, &rf_open_errno)) {
        simple_dialog(ESD_TYPE_WARN, ESD_BTN_OK,
                      "Could not open recent file\n\"%s\": %s.",
                      rf_path, g_strerror(rf_open_errno));
        g_free(rf_path);
    }
    ssApp->refreshRecentCaptures();

    splash_update(RA_LISTENERS, NULL, NULL);
#ifdef DEBUG_STARTUP_TIME
    ws_log(LOG_DOMAIN_MAIN, LOG_LEVEL_INFO, "Register all tap listeners, elapsed time %" PRIu64 " us \n", g_get_monotonic_time() - start_time);
#endif
    /* Register all tap listeners; we do this before we parse the arguments,
       as the "-z" argument can specify a registered tap. */

    register_all_tap_listeners(tap_reg_listener);

    conversation_table_set_gui_info(init_conversation_table);
    endpoint_table_set_gui_info(init_endpoint_table);
//    srt_table_iterate_tables(register_service_response_tables, NULL);
//    rtd_table_iterate_tables(register_response_time_delay_tables, NULL);
    stat_tap_iterate_tables(register_simple_stat_tables, NULL);

    if (ex_opt_count("read_format") > 0) {
        in_file_type = open_info_name_to_type(ex_opt_get_next("read_format"));
    }

    splash_update(RA_PREFERENCES, NULL, NULL);
#ifdef DEBUG_STARTUP_TIME
    ws_log(LOG_DOMAIN_MAIN, LOG_LEVEL_INFO, "Calling module preferences, elapsed time %" PRIu64 " us \n", g_get_monotonic_time() - start_time);
#endif

    /* Read the preferences, but don't apply them yet. */
    prefs_p = ss_app.readConfigurationFiles(false);

    /* Now let's see if any of preferences were overridden at the command
     * line, and store them. We have to do this before applying the
     * preferences to the capture options.
     */
    commandline_override_prefs(argc, argv, true);

    /* Register the extcap preferences. We do this after seeing if the
     * capture_no_extcap preference is set in the configuration file
     * or command line. This will re-read the extcap specific preferences.
     */
#ifdef DEBUG_STARTUP_TIME
    ws_log(LOG_DOMAIN_MAIN, LOG_LEVEL_INFO, "Calling extcap_register_preferences, elapsed time %" PRIu64 " us \n", g_get_monotonic_time() - start_time);
#endif
    splash_update(RA_EXTCAP, NULL, NULL);
    extcap_register_preferences();

    /* Apply the extcap command line options now that the extcap preferences
     * are loaded.
     */
    commandline_options_apply_extcap();

    /* Some of the preferences affect the capture options. Apply those
     * before getting the other command line arguments, which can also
     * affect the capture options. The command line arguments should be
     * applied last to take precedence (at least until the user saves
     * preferences, or switches profiles.)
     */
    prefs_to_capture_opts();

    /* Now get our remaining args */
    commandline_other_options(argc, argv, true);

    /* Convert some command-line parameters to QStrings */
    cf_name = QString(commandline_get_cf_name());
    read_filter = QString(commandline_get_rfilter());
    dfilter = QString(commandline_get_dfilter());

    timestamp_set_type(recent.gui_time_format);
    timestamp_set_precision(recent.gui_time_precision);
    timestamp_set_seconds_type (recent.gui_seconds_format);

#ifdef HAVE_LIBPCAP
    caps_queries = commandline_get_caps_queries();

    if (commandline_is_start_capture() || caps_queries) {
        /* We're supposed to do a live capture or get a list of link-layer/timestamp
           types for a live capture device; if the user didn't specify an
           interface to use, pick a default. */
        ret_val = capture_opts_default_iface_if_necessary(&global_capture_opts,
        ((prefs_p->capture_device) && (*prefs_p->capture_device != '\0')) ? get_if_name(prefs_p->capture_device) : NULL);
        if (ret_val != 0) {
            goto clean_exit;
        }
    }

    /*
     * If requested, list the link layer types and/or time stamp types
     * and exit.
     */
    if (caps_queries) {
        unsigned i;

#ifdef _WIN32
        create_console();
#endif /* _WIN32 */
        /* Get the list of link-layer types for the capture devices. */
        ret_val = EXIT_SUCCESS;
        GList *if_cap_queries = NULL;
        if_cap_query_t *if_cap_query;
        GHashTable *capability_hash;
        for (i = 0; i < global_capture_opts.ifaces->len; i++) {
            interface_options *interface_opts;

            interface_opts = &g_array_index(global_capture_opts.ifaces, interface_options, i);
            if_cap_query = g_new(if_cap_query_t, 1);
            if_cap_query->name = interface_opts->name;
            if_cap_query->monitor_mode = interface_opts->monitor_mode;
            if_cap_query->auth_username = NULL;
            if_cap_query->auth_password = NULL;
            if_cap_queries = g_list_prepend(if_cap_queries, if_cap_query);
        }
        if_cap_queries = g_list_reverse(if_cap_queries);
        capability_hash = capture_get_if_list_capabilities(if_cap_queries, &err_str, &err_str_secondary, NULL);
        g_list_free_full(if_cap_queries, g_free);
        for (i = 0; i < global_capture_opts.ifaces->len; i++) {
            interface_options *interface_opts;
            if_capabilities_t *caps;
            interface_opts = &g_array_index(global_capture_opts.ifaces, interface_options, i);
            caps = static_cast<if_capabilities_t*>(g_hash_table_lookup(capability_hash, interface_opts->name));
            if (caps == NULL) {
                cmdarg_err("%s%s%s", err_str, err_str_secondary ? "\n" : "", err_str_secondary ? err_str_secondary : "");
                g_free(err_str);
                g_free(err_str_secondary);
                ret_val = WS_EXIT_INVALID_CAPABILITY;
                break;
            }
            ret_val = capture_opts_print_if_capabilities(caps, interface_opts,
                                                         caps_queries);
            if (ret_val != EXIT_SUCCESS) {
                break;
            }
        }
#ifdef _WIN32
        destroy_console();
#endif /* _WIN32 */
        g_hash_table_destroy(capability_hash);
        goto clean_exit;
    }

#ifdef DEBUG_STARTUP_TIME
    ws_log(LOG_DOMAIN_MAIN, LOG_LEVEL_INFO, "Calling fill_in_local_interfaces, elapsed time %" PRIu64 " us \n", g_get_monotonic_time() - start_time);
#endif
    splash_update(RA_INTERFACES, NULL, NULL);

    if (cf_name.isEmpty() && !prefs.capture_no_interface_load) {
        ssApp->scanLocalInterfaces(nullptr);
    }

    capture_opts_trim_snaplen(&global_capture_opts, MIN_PACKET_SIZE);
    capture_opts_trim_ring_num_files(&global_capture_opts);
#endif /* HAVE_LIBPCAP */

    /* Notify all registered modules that have had any of their preferences
       changed either from one of the preferences file or from the command
       line that their preferences have changed. */
#ifdef DEBUG_STARTUP_TIME
    ws_log(LOG_DOMAIN_MAIN, LOG_LEVEL_INFO, "Calling prefs_apply_all, elapsed time %" PRIu64 " us \n", g_get_monotonic_time() - start_time);
#endif
    prefs_apply_all();
    ColorUtils::setScheme(prefs.gui_color_scheme);
    ssApp->emitAppSignal(StratosharkApplication::ColorsChanged);
    ssApp->emitAppSignal(StratosharkApplication::PreferencesChanged);

#ifdef HAVE_LIBPCAP
    if ((global_capture_opts.num_selected == 0) &&
            (prefs.capture_device != NULL)) {
        unsigned i;
        interface_t *device;
        for (i = 0; i < global_capture_opts.all_ifaces->len; i++) {
            device = &g_array_index(global_capture_opts.all_ifaces, interface_t, i);
            if (!device->hidden && strcmp(device->display_name, prefs.capture_device) == 0) {
                device->selected = true;
                global_capture_opts.num_selected++;
                break;
            }
        }
    }
#endif

    /*
     * Enabled and disabled protocols and heuristic dissectors as per
     * command-line options.
     */
    if (!setup_enabled_and_disabled_protocols()) {
        ret_val = WS_EXIT_INVALID_OPTION;
        goto clean_exit;
    }

    build_column_format_array(&CaptureFile::globalCapFile()->cinfo, prefs_p->num_cols, true);
    ssApp->emitAppSignal(StratosharkApplication::ColumnsChanged); // We read "recent" widths above.
    ssApp->emitAppSignal(StratosharkApplication::RecentPreferencesRead); // Must be emitted after PreferencesChanged.

    ssApp->setMonospaceFont(prefs.gui_font_name);

    /* For update of WindowTitle (When use gui.window_title preference) */
    main_w->setMainWindowTitle();

    if (!color_filters_init(&err_msg, color_filter_add_cb)) {
        simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "%s", err_msg);
        g_free(err_msg);
    }

    ssApp->allSystemsGo();
    ws_log(LOG_DOMAIN_MAIN, LOG_LEVEL_INFO, "Stratoshark is up and ready to go, elapsed time %.3fs", (float) (g_get_monotonic_time() - start_time) / 1000000);
    SimpleDialog::displayQueuedMessages(main_w);

    /* User could specify filename, or display filter, or both */
    if (!dfilter.isEmpty())
        main_w->filterPackets(dfilter, false);
    if (!cf_name.isEmpty()) {
        if (main_w->openCaptureFile(cf_name, read_filter, in_file_type)) {

            char* jump_filter = commandline_get_jfilter();
            uint32_t go_to_packet = commandline_get_go_to_packet();

            /* Open stat windows; we do so after creating the main window,
               to avoid Qt warnings, and after successfully opening the
               capture file, so we know we have something to compute stats
               on, and after registering all dissectors, so that MATE will
               have registered its field array and we can have a tap filter
               with one of MATE's late-registered fields as part of the
               filter. */
            start_requested_stats();

            if (go_to_packet != 0) {
                /* Jump to the specified frame number, kept for backward
                   compatibility. */
                cf_goto_frame(CaptureFile::globalCapFile(), go_to_packet, false);
            } else if (jump_filter != NULL) {
                dfilter_t *jump_to_filter = NULL;
                /* try to compile given filter */
                if (!dfilter_compile(jump_filter, &jump_to_filter, &df_err)) {
                    // Similar code in MainWindow::mergeCaptureFile().
                    QMessageBox::warning(main_w, QObject::tr("Invalid Display Filter"),
                                         QObject::tr("The filter expression \"%1\" isn't a valid display filter.\n(%2).")
                                                 .arg(jump_filter, df_err->msg),
                                         QMessageBox::Ok);
                    df_error_free(&df_err);
                } else {
                    /* Filter ok, jump to the first packet matching the filter
                       conditions. Default search direction is forward, but if
                       option j was given, search backwards */
                    cf_find_packet_dfilter(CaptureFile::globalCapFile(), jump_to_filter, commandline_get_jump_direction(), false);
                }
            }
        }
    }
#ifdef HAVE_LIBPCAP
    else {
        if (commandline_is_start_capture()) {
            if (global_capture_opts.save_file != NULL) {
                /* Save the directory name for future file dialogs. */
                /* (get_dirname overwrites filename) */
                char *s = g_strdup(global_capture_opts.save_file);
                set_last_open_dir(get_dirname(s));
                g_free(s);
            }
            /* "-k" was specified; start a capture. */
            check_and_warn_user_startup();

            /* If no user interfaces were specified on the command line,
               copy the list of selected interfaces to the set of interfaces
               to use for this capture. */
            if (global_capture_opts.ifaces->len == 0)
                collect_ifaces(&global_capture_opts);
            CaptureFile::globalCapFile()->window = main_w;
            if (capture_start(&global_capture_opts, commandline_get_capture_comments(),
                              main_w->captureSession(), main_w->captureInfoData(),
                              main_window_update)) {
                /* The capture started.  Open stat windows; we do so after creating
                   the main window, to avoid GTK warnings, and after successfully
                   opening the capture file, so we know we have something to compute
                   stats on, and after registering all dissectors, so that MATE will
                   have registered its field array and we can have a tap filter with
                   one of MATE's late-registered fields as part of the filter. */
                start_requested_stats();
            }
        }
        /* if the user didn't supply a capture filter, use the one to filter out remote connections like SSH */
        if (!commandline_is_start_capture() && !global_capture_opts.default_options.cfilter) {
            global_capture_opts.default_options.cfilter = g_strdup(get_conn_cfilter());
        }
    }
#endif /* HAVE_LIBPCAP */

    // UAT and UI settings files used in configuration profiles which are used
    // in Qt dialogs are not registered during startup because they only get
    // loaded when the dialog is shown.  Register them here.
    profile_register_persconffile("io_graphs");
    profile_register_persconffile("import_hexdump.json");
    profile_register_persconffile("remote_hosts.json");

    profile_store_persconffiles(false);

    // If the ssApp->exec() event loop exits cleanly, we call
    // StratosharkApplication::cleanup().
    ret_val = ssApp->exec();
    ssApp = NULL;

    // Many widgets assume that they always have valid epan data, so this
    // must be called before epan_cleanup().
    // XXX We need to clean up the Lua GUI here. We currently paper over
    // this in FunnelStatistics::~FunnelStatistics, which leaks memory.
    delete main_w;

    recent_cleanup();
    epan_cleanup();

    extcap_cleanup();

    Dot11DecryptDestroyContext(&dot11decrypt_ctx);

    ws_cleanup_sockets();

#ifdef _WIN32
    /* For some unknown reason, the "atexit()" call in "create_console()"
       doesn't arrange that "destroy_console()" be called when we exit,
       so we call it here if a console was created. */
    destroy_console();
#endif /* _WIN32 */

clean_exit:
#ifdef HAVE_LIBPCAP
    capture_opts_cleanup(&global_capture_opts);
#endif
    col_cleanup(&CaptureFile::globalCapFile()->cinfo);
    codecs_cleanup();
    wtap_cleanup();
    free_progdirs();
    commandline_options_free();
    return ret_val;
}
