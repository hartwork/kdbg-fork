// $Id$

// Copyright by Johannes Sixt
// This file is under GPL, the GNU General Public Licence

#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <qtimer.h>
#include <qdict.h>
#include "valarray.h"
#include "envvar.h"
#include "exprwnd.h"			/* some compilers require this */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

class ExprWnd;
class VarTree;
class ProgramTypeTable;
class KTreeViewItem;
class KConfig;
class KSimpleConfig;
class QListBox;
class RegisterInfo;
class DebuggerDriver;
class CmdQueueItem;
class Breakpoint;
class KProcess;


class KDebugger : public QObject
{
    Q_OBJECT
public:
    KDebugger(QWidget* parent,		/* will be used as the parent for dialogs */
	      ExprWnd* localVars,
	      ExprWnd* watchVars,
	      QListBox* backtrace,
	      DebuggerDriver* driver
	      );
    ~KDebugger();
    
    /**
     * This function starts to debug the specified executable.
     * @return false if an error occurs, in particular if a program is
     * currently being debugged
     */
    bool debugProgram(const QString& executable);

    /**
     * Queries the user for an executable file and debugs it. If a program
     * is currently being debugged, it is terminated first.
     */
    void fileExecutable();

    /**
     * Queries the user for a core file and uses it to debug the active
     * program
     */
    void fileCoreFile();

    /**
     * Runs the program or continues it if it is stopped at a breakpoint.
     */
    void programRun();

    /**
     * Attaches to a process and debugs it.
     */
    void programAttach();

    /**
     * Restarts the debuggee.
     */
    void programRunAgain();

    /**
     * Performs a single-step, possibly stepping into a function call.
     */
    void programStep();

    /**
     * Performs a single-step, stepping over a function call.
     */
    void programNext();

    /**
     * Runs the program until it returns from the current function.
     */
    void programFinish();

    /**
     * Kills the program (removes it from memory).
     */
    void programKill();

    /**
     * Interrupts the program if it is currently running.
     */
    void programBreak();

    /**
     * Queries the user for program arguments.
     */
    void programArgs();

    /**
     * Setup remote debugging device
     */
    void setRemoteDevice(const QString& remoteDevice) { m_remoteDevice = remoteDevice; }

    /**
     * Run the debuggee until the specified line in the specified file is
     * reached.
     * 
     * @return false if the command was not executed, e.g. because the
     * debuggee is running at the moment.
     */
    bool runUntil(const QString& fileName, int lineNo);

    /**
     * Set a breakpoint.
     *
     * @param fileName The source file in which to set the breakpoint.
     * @param lineNo The zero-based line number.
     * @param temporary Specifies whether this is a temporary breakpoint
     * @return false if the command was not executed, e.g. because the
     * debuggee is running at the moment.
     */
    bool setBreakpoint(QString fileName, int lineNo, bool temporary);

    /**
     * Enable or disable a breakpoint at the specified location.
     * 
     * @param fileName The source file in which the breakpoint is.
     * @param lineNo The zero-based line number.
     * @return false if the command was not executed, e.g. because the
     * debuggee is running at the moment.
     */
    bool enableDisableBreakpoint(QString fileName, int lineNo);

    /**
     * Tells whether one of the single stepping commands can be invoked
     * (step, next, finish, until, also run).
     */
    bool canSingleStep();

    /**
     * Tells whether a breakpoints can be set, deleted, enabled, or disabled.
     */
    bool canChangeBreakpoints();

    /**
     * Tells whether the debuggee can be changed.
     */
    bool canChangeExecutable() { return isReady() && !m_programActive; }

    /**
     * Add a watch expression.
     */
    void addWatch(const QString& expr);

    /**
     * Retrieves the current status message.
     */
    const QString& statusMessage() const { return m_statusMessage; }

    /**
     * Is the debugger ready to receive another high-priority command?
     */
    bool isReady() const;

    /**
     * Is the debuggee running (not just active)?
     */
    bool isProgramRunning() { return m_haveExecutable && m_programRunning; }

    /**
     * Do we have an executable set?
     */
    bool haveExecutable() { return m_haveExecutable; }

    /**
     * Is the debuggee active, i.e. was it started by the debugger?
     */
    bool isProgramActive() { return m_programActive; }

    /** The list of breakpoints. */
    int numBreakpoints() const { return m_brkpts.size(); }
    const Breakpoint* breakpoint(int i) const { return m_brkpts[i]; }

    const QString& executable() const { return m_executable; }

    void setCoreFile(const QString& corefile) { m_corefile = corefile; }

    /**
     * Sets the command to invoke gdb. If cmd is the empty string, the
     * default is substituted.
     */
    void setDebuggerCmd(const ValArray<QString>& cmd)
    { m_debuggerCmd = cmd; }

    /** Sets the terminal that is to be used by the debugger. */
    void setTerminal(const QString& term) { m_inferiorTerminal = term; }

    /** Returns the debugger driver. */
    DebuggerDriver* driver() { return m_d; }

    // settings
    void saveSettings(KConfig*);
    void restoreSettings(KConfig*);

protected:
    QString m_inferiorTerminal;
    ValArray<QString> m_debuggerCmd;
    bool startGdb();
    void stopGdb();
    void writeCommand();
    
    QList<VarTree> m_watchEvalExpr;	/* exprs to evaluate for watch windows */
    QArray<Breakpoint*> m_brkpts;

protected slots:
    void parse(CmdQueueItem* cmd, const char* output);
protected:
    VarTree* parseExpr(const char* output, bool wantErrorValue);
    void handleRunCommands(const char* output);
    void updateAllExprs();
    void updateProgEnvironment(const QString& args, const QString& wd,
			       const QDict<EnvVar>& newVars);
    void parseLocals(const char* output, QList<VarTree>& newVars);
    void handleLocals(const char* output);
    bool handlePrint(CmdQueueItem* cmd, const char* output);
    void handleBacktrace(const char* output);
    void handleFrameChange(const char* output);
    void handleFindType(CmdQueueItem* cmd, const char* output);
    void handlePrintStruct(CmdQueueItem* cmd, const char* output);
    void handleSharedLibs(const char* output);
    void handleRegisters(const char* output);
    void evalExpressions();
    void evalInitialStructExpression(VarTree* var, ExprWnd* wnd, bool immediate);
    void evalStructExpression(VarTree* var, ExprWnd* wnd, bool immediate);
    void exprExpandingHelper(ExprWnd* wnd, KTreeViewItem* item, bool& allow);
    void dereferencePointer(ExprWnd* wnd, VarTree* var, bool immediate);
    void determineType(ExprWnd* wnd, VarTree* var);
    void removeExpr(ExprWnd* wnd, VarTree* var);
    CmdQueueItem* loadCoreFile();

    Breakpoint* breakpointByFilePos(QString file, int lineNo);
    void newBreakpoint(const char* output);
    void updateBreakList(const char* output);
    bool haveTemporaryBP() const;
    void saveBreakpoints(KSimpleConfig* config);
    void restoreBreakpoints(KSimpleConfig* config);

    QString m_lastDirectory;		/* the dir of the most recently opened file */
    bool m_haveExecutable;		/* has an executable been specified */
    bool m_programActive;		/* is the program active (possibly halting in a brkpt)? */
    bool m_programRunning;		/* is the program executing (not stopped)? */
    bool m_sharedLibsListed;		/* do we know the shared libraries loaded by the prog? */
    QString m_executable;
    QString m_corefile;
    QString m_attachedPid;		/* user input of attaching to pid */
    QString m_programArgs;
    QString m_remoteDevice;
    unsigned m_runRedirect;
    QString m_programWD;		/* working directory of gdb */
    QDict<EnvVar> m_envVars;		/* environment variables set by user */
    QStrList m_sharedLibs;		/* shared libraries used by program */
    ProgramTypeTable* m_typeTable;	/* known types used by the program */
    KSimpleConfig* m_programConfig;	/* program-specific settings (brkpts etc) */
    void saveProgramSettings();
    void restoreProgramSettings();

    // debugger process
    DebuggerDriver* m_d;
    bool m_explicitKill;		/* whether we are killing gdb ourselves */

    QString m_statusMessage;

protected slots:
    void gdbExited(KProcess*);
    void slotInferiorRunning();
    void backgroundUpdate();
    void gotoFrame(int);
    void slotLocalsExpanding(KTreeViewItem*, bool&);
    void slotWatchExpanding(KTreeViewItem*, bool&);
    void slotUpdateAnimation();
    void slotDeleteWatch();
    
signals:
    /**
     * This signal is emitted before the debugger is started. The slot is
     * supposed to set up m_inferiorTerminal.
     */
    void debuggerStarting();

    /**
     * This signal is emitted whenever a part of the debugger needs to
     * highlight the specfied source code line (e.g. when the program
     * stops).
     * 
     * @param file specifies the file; this is not necessarily a full path
     * name, and if it is relative, you won't know relative to what, you
     * can only guess.
     * @param lineNo specifies the line number (0-based!) (this may be
     * negative, in which case the file should be activated, but the line
     * should NOT be changed).
     */
    void activateFileLine(const QString& file, int lineNo);

    /**
     * This signal is emitted when a line decoration item (those thingies
     * that indicate breakpoints) must be changed.
     */
    void lineItemsChanged();

    /**
     * This signal is a special case of @ref #lineItemsChanged because it
     * indicates that only the program counter has changed.
     *
     * @param filename specifies the filename where the program stopped
     * @param lineNo specifies the line number (zero-based); it can be -1
     * if it is unknown
     * @param frameNo specifies the frame number: 0 is the innermost frame,
     * positive numbers are frames somewhere up the stack (indicates points
     * where a function was called); the latter cases should be indicated
     * differently in the source window.
     */
    void updatePC(const QString& filename, int lineNo, int frameNo);

    /**
     * This signal is emitted when gdb detects that the executable has been
     * updated, e.g. recompiled. (You usually need not handle this signal
     * if you are the editor which changed the executable.)
     */
    void executableUpdated();

    /**
     * This signal is emitted when the animated icon should advance to the
     * next picture.
     */
    void animationTimeout();

    /**
     * Indicates that a new status message is available.
     */
    void updateStatusMessage();

    /**
     * Indicates that the internal state of the debugger has changed, and
     * that this will very likely have an impact on the UI.
     */
    void updateUI();

    /**
     * Indicates that the list of breakpoints has possibly changed.
     */
    void breakpointsChanged();

    /**
     * Indicates that the register values have possibly changed.
     */
    void registersChanged(QList<RegisterInfo>&);

protected:
    ExprWnd& m_localVariables;
    ExprWnd& m_watchVariables;
    QListBox& m_btWindow;

    // animation
    QTimer m_animationTimer;
    int m_animationInterval;
    
    // implementation helpers
protected:
    void startAnimation(bool fast);
    void stopAnimation();

    QWidget* parentWidget() { return static_cast<QWidget*>(parent()); }

    friend class BreakpointTable;
};

#endif // DEBUGGER_H
