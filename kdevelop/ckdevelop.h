/***************************************************************************
                     ckdevelop.h - the mainclass in kdevelop   
                             -------------------                                         

    begin                : 20 Jul 1998                                        
    copyright            : (C) 1998 by Sandy Meier                         
    email                : smeier@rz.uni-potsdam.de                                     
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/

#ifndef CKDEVELOP_H
#define CKDEVELOP_H

//#ifdef HAVE_CONFIG_H
//#include <config.h>
//#endif

#include <iostream.h>

#include <qdir.h>
#include <qlist.h>
#include <qstring.h>
#include <qstrlist.h>
#include <qwhatsthis.h>
#include <qtimer.h>
#include <qprogressbar.h>

#include <kapp.h>
#include <kprocess.h>
#include <kiconloader.h>
#include <kfiledialog.h>
#include <kprogress.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdockwidget.h>
#include "./kwrite/kguicommand.h"

class QSplitter;
class KHTMLWidget;
class CDocBrowser;
class CClassView;
class DocTreeView;
class MakeView;
class GrepView;
class OutputView;
class CRealFileView;
class CLogFileView;
class KSwallowWidget;
class CAddExistingFileDlg;
class QListViewItem;
class CEditWidget;
class MdiFrame;
class QextMdiChildView;
class EditorView;
class DocBrowserView;
class WidgetsPropSplitView;
class DlgEdit;

#include "component.h"
#include "ctabctl.h"
#include "ckdevaccel.h"
#include "ctreehandler.h"
#include "cproject.h"
#include "cimport.h"
#include "structdef.h"
#include "resource.h"
#include "./print/cprintdlg.h"

class CParsedMethod;
class CParsedClass;

// Debugger classes
class VarViewer;
class DbgController;
class FrameStack;
class BreakpointManager;
class Breakpoint;
class Disassemble;
class DbgToolbar;
class COutputWidget;

/** the mainclass in kdevelop
  *@author Sandy Meier
  */
class CKDevelop : public KDockMainWindow {
  Q_OBJECT
public:
  /**constructor*/
  CKDevelop();
  /**destructor*/
  virtual ~CKDevelop();
  void initView();
  void initConnections();
  void initKeyAccel();
  void initMenuBar();
  void initToolBar();
  void initStatusBar();
  void initWhatsThis();
  void initProject();

  /** The complete file save as handling
   *  @return true if it succeeded
   */
  bool fileSaveAs();
  bool saveFile(QString abs_filename);

  void refreshTrees();
  void refreshTrees(TFileInfo *info);

  void setKeyAccel();
  void setToolmenuEntries();
	
  void initDlgEditor();
  

  /** sets the Main window caption for KDevelop */
  void setMainCaption(int tab_item=-1);
  			
  void newFile(bool add_to_project);
  /** read the projectfile from the disk*/
  bool readProjectFile(QString file);

  /** Add a file with a specified type to the project. 
   *  
   *  @param complete_filename   The absolute filename.
   *  @param type                Type of file.
   *  @param refresh             If to refresh the trees.
   *  @return true if a new subdir was added.
   */
  bool addFileToProject(QString complete_filename,ProjectFileType type,bool refreshTrees=true);
  void addRecentProject(const char* file);

  /** Switch the view to a certain file.
   * @param filename the absolute filename
   * @param bForceReload if true then enforce updating widget text from file
   * @param bShowModifiedBox if true no messagebox is shown, if the file was modified outside the editor
   */
  void switchToFile(QString filename, bool bForceReload=false,bool bShowModifiedBox=true); // filename = abs

  /** Switch to a certain line in a certain file.
   *  @param filename Absolute filename of the file to switch to.
   *  @param lineNo   The line in the file to switch to.
   */
  void switchToFile(QString filename, int lineNo);

  /** set the correct toolbar and menubar,if a process is running
    * @param enable if true than enable,otherwise disable
    */
  void setToolMenuProcess(bool enable);

  /** Get the current project. */
  CProject* getProject()                 {return m_prj;}

  /** do something more when resizing */
  void resizeEvent(QResizeEvent* rse);
  
 public slots:

  void enableCommand(int id_);
  void disableCommand(int id_);

  ////////////////////////
  // editor commands
  ///////////////////////

  void doCursorCommand(int cmdNum);
  void doEditCommand(int cmdNum);
  void doStateCommand(int cmdNum);

  ////////////////////////
  // FILE-Menu entries
  ///////////////////////
 
  /** generate a new file*/
  void slotFileNew();
  /**open a file*/
  void slotFileOpen();
  /** opens a file from the file_open_popup that is a delayed popup menu 
   *installed in the open file button of the toolbar */
  void slotFileOpen( int id_ );
  /** close the current file*/
  void slotFileClose();
  bool slotFileCloseAll();
  /** save the current file,if Untitled a dialog ask for a valid name*/
  void slotFileSave();
  /** save all files*/
  void slotFileSaveAll();
  /** save the current file under a different filename*/
  void slotFileSaveAs();
  /** opens the printing dialog */
  void slotFilePrint();
  /** quit kdevelop*/
  void slotFileQuit();
  /** called from the EditorView to inform the mainapplication*/
  void slotFileWasSaved(EditorView* editor);

  ////////////////////////
  // EDIT-Menu entries  (most of the slots are not needed any more (jochen))
  ///////////////////////
  /** Undo last editing step */
  void slotEditUndo();
  /** Redo last editing step */
  void slotEditRedo();
  void slotEditUndoHistory();
  /** cuts a selection to the clipboard */
  void slotEditCut();
  /** copies a selection to the clipboard */
  void slotEditCopy();
  /** inserts the clipboard contents to the cursor position */
  void slotEditPaste();
  /** inserts a file at the cursor position */
  void slotEditInsertFile();
  /** opens the search dialog for the editing widget */
  void slotEditSearch();
  /** repeat last search */
  void slotEditRepeatSearch();
  /** acts on grep to search the selected word by keyboard shortcut */
  void slotEditSearchText();
  /** search in files, use grep and find*/
  void slotEditSearchInFiles();
  /** called by popups in the edit and brwoser widgets to grep a string */
  void slotEditSearchInFiles(QString);
  /** runs ispell check on the actual editwidget */
  void slotEditSpellcheck();
  /** opens the search and replace dialog */
  void slotEditReplace();
  /** selects the whole editing widget text */
  void slotEditSelectAll();
  
  /** remove all text selections */
  void slotEditDeselectAll();
  
  ////////////////////////
  // VIEW-Menu entries
  ///////////////////////
  /** opens the goto line dialog */
  void slotViewGotoLine();
  /** jump to the next error, based on the make output*/
  void slotViewNextError();
  /** jump to the previews error, based on the make output*/
  void slotViewPreviousError();
  /** dis-/enables the treeview */
  void slotViewTTreeView();
  void showTreeView(bool show=true);
  /** dis-/enables the outputview */
  void slotViewTOutputView();
  void showOutputView(bool show=true);
  /** en-/disable the toolbar */
  void slotViewTStdToolbar();
  /** en-/disable the browser toolbar */
  void slotViewTBrowserToolbar();
  /** en-/disable the statusbar */
  void slotViewTStatusbar();
  /** en-/disable the MDI view taskbar */
  void slotViewTMDIViewTaskbar();
  /** refresh all trees and other widgets*/
  void slotViewRefresh();
  /** show/hide the VAR debugger view */
  void slotViewDebuggerViewsVar();
  /** show/hide the breakpoint debugger view */
  void slotViewDebuggerViewsBreakpoints();
  /** show/hide the framestack debugger view */
  void slotViewDebuggerViewsFrameStack();
  /** show/hide the disassemble debugger view */
  void slotViewDebuggerViewsDisassemble();
  /** show/hide the debugger view */
  void slotViewDebuggerViewsDebugger();

  ////////////////////////
  // PROJECT-Menu entries
  ///////////////////////
  /** generates a new project with KAppWizard*/
  void slotProjectNewAppl();
  /** opens a projectfile and close the old one*/
  void slotProjectOpen();
  /** opens a project file from the recent project menu in the project menu by getting the project entry and
   * calling slotProjectOpenCmdl()
   */
  void slotProjectOpenRecent(int id_);
  /** opens a project committed by comandline or kfm */
  void slotProjectOpenCmdl(QString prjfile);
  /** imports an automake project */
  void slotProjectImport();
  /** close the current project,return false if  canceled*/
  bool slotProjectClose();
  /** add a new file to the project-same as file new */
  void slotProjectAddNewFile();
  /** opens the add existing files dialog */
  void slotProjectAddExistingFiles();
  /** helper methods for slotProjectAddExistingFiles() */
  void slotAddExistingFiles();
  /** add a new po file to the project*/
  void slotProjectAddNewTranslationFile();
  /** remove a project file */
  void slotProjectRemoveFile();
  /** opens the New class dialog */
  void slotProjectNewClass();
  /** opens the properties dialog for the project files */
  void slotProjectFileProperties();
  /** opens the properties dialog for project files,rel_name is selected, used by RFV,LFV*/
  void slotShowFileProperties(QString rel_name);
  /** opens the project options dialog */
  void slotProjectOptions();
  void slotProjectMessages();
  void slotProjectAPI();
  void slotProjectManual();
  void slotProjectMakeDistSourceTgz();

  ////////////////////////
  // BUILD-Menu entries
  ///////////////////////
  /** compile the actual sourcefile using setted options */
  void slotBuildCompileFile();
  void slotBuildMake();
  //   void slotBuildMakeWith();
  void slotBuildRebuildAll();
  void slotBuildCleanRebuildAll();
  void slotBuildStop();
  void slotBuildRun();
  void slotBuildRunWithArgs();
  void slotBuildDebug();
  void slotBuildDistClean();
  void slotBuildAutoconf();
  void slotBuildConfigure();
  
  
  ////////////////////////
  // TOOLS-Menu entries
  ///////////////////////
  void slotToolsTool(int tool);

  ////////////////////////
  // OPTIONS-Menu entries
  ///////////////////////
  void slotOptionsEditor();
  void slotOptionsEditorColors();
  void slotOptionsSyntaxHighlightingDefaults();
  void slotOptionsSyntaxHighlighting();
  /** shows the Browser configuration dialog */
  void slotOptionsDocBrowser();
  /** shows the Tools-menu configuration dialog */
  void slotOptionsToolsConfigDlg();
  /** shows the spellchecker config dialog */
  void slotOptionsSpellchecker();
  /** shows the configuration dialog for enscript-printing */
  void slotOptionsConfigureEnscript();
  /** shows the configuration dialog for a2ps printing */
  void slotOptionsConfigureA2ps();
  /** show a configure-dialog for kdevelop*/
  void slotOptionsCustomize();
  /** show a configure-dialog for kdevelop*/
  void slotOptionsKDevelop();
  /** sets the make command after it is changed in the Setup dialog */
  void slotOptionsMake();
  /** dis-/enables autosaving by setting in the Setup dialog */
  void slotOptionsAutosave(bool);
  /** sets the autosaving time intervall */
  void slotOptionsAutosaveTime(int);
  /** dis-/enalbes autoswitch by setting bAutoswitch */
  void slotOptionsAutoswitch(bool);
  /** toggles between autoswitching to CV or LFV */
  void slotOptionsDefaultCV(bool);
  /** shows the Update dialog and sends output to the messages widget */
  void slotOptionsUpdateKDEDocumentation();
  /** shows the create search database dialog called by the setup button */
  void  slotOptionsCreateSearchDatabase();
  
  ////////////////////////
  // Plugin-Menu entries
  ///////////////////////
  void slotPluginPluginManager();

  ////////////////////////
  // BOOKMARKS-Menu entries
  ///////////////////////
  void slotBookmarksSet();
  void slotBookmarksAdd();
  void slotBookmarksClear();
  void slotBoomarksBrowserSelected(int);

  ////////////////////////
  // WINDOW-Menu entries
  ///////////////////////
  /* extends the QextMDI slot, docks all undocked KDockWidgets */
  void slotSwitchToChildframeMode();
  /* extends the QextMDI slot, undocks all docked KDockWidgets */
  void slotSwitchToToplevelMode();

  ////////////////////////
  // HELP-Menu entries
  ///////////////////////
  /** goes one page back in the documentation browser */
  void slotHelpBack();
  /** goes one page forward in the documentatio browser */
  void slotHelpForward();
  /** goes to the page in the history list by delayed popup menu on the 
   *  back-button on the browser toolbar */
  void slotHelpHistoryBack( int id_);
  /** goes to the page in the history list by delayed popup menu on the
   * forward-button on the browser toolbar */
  void slotHelpHistoryForward(int id_);
  /** reloads the currently opened page */
  void slotHelpBrowserReload();
  /** search marked text */
  void slotHelpSearchText();
  /** search marked text with a text string */
  void slotHelpSearchText(QString text);
  /** shows the Search for Help on.. dialog to insert a search expression */
  void slotHelpSearch();
  /** shows the C/C++-referenc */
  void slotHelpReference();
  /** shows the Qt-doc */
  void slotHelpQtLib();
  void showLibsDoc(const char *libname);
  /** shows the kdecore-doc */
  void slotHelpKDECoreLib();
  /** shows the kdeui-doc */
  void slotHelpKDEGUILib();
  /** shows the kfile-doc */
  void slotHelpKDEKFileLib();
  /** shows the khtml / khtmlw -doc */
  void slotHelpKDEHTMLLib();
  /** shows the API of the current project */
  void slotHelpAPI();
  /** shows the manual of the current project */
  void slotHelpManual();
  /** shows the KDevelop manual */
  void slotHelpContents();
  /** shows the Programming handbook */
  void slotHelpTutorial();
  /** shows the tip of the day */
  void slotHelpTipOfDay();
  /**  open the KDevelop Homepage with kfm/konqueror*/
  void slotHelpHomepage();
  /** shows the bug report dialog*/
  void slotHelpBugReport();
  /** shows the aboutbox of KDevelop */
  void slotHelpAbout();


  void slotSwitchFileRequest(const QString &filename,int linenumber);
  

  //////////////////////////////////
  // Classbrowser wizardbutton slots
  //////////////////////////////////
  /** View the class header */
  void slotClassbrowserViewClass();
  /** View the graphical classtree. */
  void slotClassbrowserViewTree();
  /** View the declaration of a class/method. */
  void slotClassbrowserViewDeclaration();
  /** View the definition of a class/method. */
  void slotClassbrowserViewDefinition();
  /** Add a new method to a class. */
  void slotClassbrowserNewMethod();
  /** Add a new attribute to a class. */
  void slotClassbrowserNewAttribute();
  
  ////////////////////////
  // All slots which are used if the user clicks or selects something in the view
  ///////////////////////
  /** swich construction for the toolbar icons, selecting the right slots */
  void slotToolbarClicked(int);
	
  ///////////// -- the methods for the treeview selection
  /** click action on LFV */
  void slotLogFileTreeSelected(QString file);
  /** click action on RFV */
  void slotRealFileTreeSelected(QString file);
  /** click action on DOC */
  void slotDocTreeSelected(const QString &filename);
  /** selection of classes in the browser toolbar */
  void slotClassChoiceCombo(int index);
  /** selection of methods in the browser toolbar */
  void slotMethodChoiceCombo(int index);
  /** add a file to the project */
  void slotAddFileToProject(QString abs_filename);
  void delFileFromProject(QString rel_filename);

  /////////some slots for VCS interaction
  
  void slotUpdateFileFromVCS(QString file);
  void slotCommitFileToVCS(QString file);
  void slotUpdateDirFromVCS(QString dir);
  void slotCommitDirToVCS(QString dir);

  //////////////// -- the methods for the statusbar items
  /** change the status message to text */
  void slotStatusMsg(const char *text);
  /** change the status message of the whole statusbar temporary */
  void slotStatusHelpMsg(const char *text);
  /** switch argument for Statusbar help entries on slot selection */
  void statusCallback(int id_);
  /** change Statusbar status of INS and OVR */
  void slotNewStatus();
  /** change copy & cut status */
  void slotMarkStatus();
//  void slotCPPMarkStatus(KWriteView *, bool);
//  void slotHEADERMarkStatus(KWriteView *, bool);
  void slotBROWSERMarkStatus(bool);
  /** recognize change of Clipboard data */
  void slotClipboardChanged();
  /** change Statusbar status of Line and Column */
  void slotNewLineColumn();
  void slotNewUndo();

/*  void slotShowC();
  void slotShowHeader();
  void slotShowHelp();
  void slotShowTools();*/

  void slotURLSelected(const QString &url, const QString&, int button);
  void slotDocumentDone();
  void slotURLonURL(const QString &url);

  void switchToKDevelop();

  void slotSearchReceivedStdout(KProcess* proc,char* buffer,int buflen);
  void slotProcessExited(KProcess *);
  void slotSearchProcessExited(KProcess*);
  
  //////////////// -- the methods for signals generated from the CV
  /** Add a method to a class. Brings up a dialog and lets the
   * user fill it out.
   * @param aClassName      The class to add the method to.
   */
  void slotCVAddMethod( const char *aClassName );

  /** Add a method to a class.
   * user fill it out.
   * @param aClassName      The class to add the method to.
   * @param aMethod         The method to add to the class.
   */
  void slotCVAddMethod( const char *aClassName, CParsedMethod *aMethod );

  /** Add an attribute to a class.
   * @param aClassName      The class to add the attribute to.
   */
  void slotCVAddAttribute( const char *aClassName );
  
  /** Delete an method.
   * @param aClassName Name of the class holding the method. NULL for global functions.
   * @param aMethodName Name of the method(with arguments) to delete.
   */
  void slotCVDeleteMethod( const char *aClassName,const char *aMethodName );

  /** The user wants to view the declaration of a method. 
   * @param className Name of the class holding the declaration. NULL for globals.
   * @param declName Name of the declaration item.
   * @param type Type of declaration item
   */
  void slotCVViewDeclaration( const char *className, 
                              const char *declName, 
                              THType type  );
  /** The user wants to view the definition of a method/attr...
   * @param className Name of the class holding the definition. NULL for globals.
   * @param declName Name of the definition item.
   * @param type Type of definition item.
   */
  void slotCVViewDefinition( const char *className, 
                             const char *declName, 
                             THType type  );

  /** */
  void slotMDIGetFocus(QextMdiChildView* item);

  /** synchronize the state of the debugger_views_menu with the real debugger view states (item checks) */
  void slotUpdateDebuggerViewsMenu();

protected: // Protected methods

  /** The user selected a class in the classcombo.
   * @param aName Name of the class.
   */
  void CVClassSelected( const char *aName );

  /** The user selected a method in the methodcombo.
   * @param aName Name of the method.
   */
  void CVMethodSelected( const char *aName );

  /** Goto the definition of the specified item.
   * @param className Name of the class holding the definition. NULL for globals.
   * @param declName Name of the definition item.
   * @param type Type of definition item.
   */
  void CVGotoDefinition( const char *className, 
                         const char *declName, 
                         THType type );

  /** Goto the declaration of the specified item.
   * @param className Name of the class holding the definition. NULL for globals.
   * @param declName Name of the definition item.
   * @param type Type of definition item.
   */
  void CVGotoDeclaration( const char *className, 
                          const char *declName, 
                          THType type );
  
  /** Returns the class for the supplied classname. 
   * @param className Name of the class to fetch.
   * @return Pointer to the class or NULL if not found.
   */
  CParsedClass *CVGetClass( const char *className );

  /** Update the class combo with all classes in alpabetical order. */
  void CVRefreshClassCombo();

  /** Update the method combo with the methods from the selected
   * class.
   * @param aClass Class to update the methodcombo from.
   */
  void CVRefreshMethodCombo( CParsedClass *aClass );

public: // Public methods

  /** a tool meth,used in the search engine*/
  int searchToolGetNumber(QString str);
  QString searchToolGetTitle(QString str);
  QString searchToolGetURL(QString str);
  // returns the current editor (mdi) or 0L
  EditorView* getCurrentEditorView();

  /** called if a new subdirs was added to the project, shows a messagebox and start autoconf...*/
  void newSubDir();
protected:
  /** reads all options and initializes values*/
  void readOptions();
  /** saves all options on queryExit() */
  void saveOptions();
  /** save the project of the current window and close the swallow widget. 
   * If project closing is cancelled, returns false */
  virtual bool queryClose();
  /** saves all options by calling saveOptions() */
  virtual bool queryExit();
  /** saves the currently opened project by the session manager and write 
   * the project file to the session config*/
  virtual void saveProperties(KConfig* );
  /** initializes the session windows and opens the projects of the last
   * session */
  virtual void readProperties(KConfig* );

private:
  //the menus for kdevelop main
  QPopupMenu* m_file_menu;				
  QPopupMenu* m_recent_projects_menu;
  KGuiCmdPopup* m_edit_menu;
  KGuiCmdPopup* m_view_menu;
  QPopupMenu* m_bookmarks_menu;
  QPopupMenu* m_doc_bookmarks;
  QPopupMenu* m_debugger_views_menu;

  QPopupMenu* m_project_menu;
  QPopupMenu* m_build_menu;
  QPopupMenu* m_tools_menu;
  QPopupMenu* m_options_menu;
  QPopupMenu* m_plugin_menu;
  QPopupMenu* m_help_menu;
    //  QWhatsThis* m_whats_this;
	
  QPopupMenu* m_history_prev;
  QPopupMenu* m_history_next;
  QPopupMenu* m_file_open_popup;
  /** Popup menu for the classbrowser wizard button. */
  QPopupMenu* m_classbrowser_popup;

  /** Tells if the next click on the classwizard toolbar button should show
   * the declaration or the definition of the selected item. */
  bool m_cv_decl_or_impl;

  QStrList m_file_open_list;	
  
  QStrList m_tools_exe;
  QStrList m_tools_entry;
  QStrList m_tools_argument;
  	
  KGuiCmdDispatcher *m_kdev_dispatcher;
  KMenuBar* m_kdev_menubar;
  KStatusBar* m_kdev_statusbar;

  DlgEdit* m_dlgedit;
  /** If this to true, the user get a beep after a 
   *  process,slotProcessExited() */
  bool m_beep;
  


  KIconLoader m_icon_loader;

  /** search with glimpse */
  KShellProcess m_search_process;
  /** at the moment only one project at the same time */
  CProject* m_prj;

  CKDevAccel *m_accel;
  KConfig* m_config;
  int m_act_outbuffer_len;

  QStrList m_recent_projects;
  // for the browser
  QStrList m_history_list;
  QStrList m_history_title_list;
  QStrList m_doc_bookmarks_list;
  QStrList m_doc_bookmarks_title_list;
	

  ///////////////////////////////
  //some widgets for the mainview
  ///////////////////////////////

  /** The tabbar for the trees. */
  KDockWidget* m_dockbase_t_tab_view;
  /** The tabbar for the output_widgets. */
  KDockWidget* m_dockbase_o_tab_view;

  KDockWidget* m_dockbase_mdi_main_frame;
  MdiFrame* m_mdi_main_frame;

  /** the current editor view or 0*/
  EditorView* m_editor_view;
  DocBrowserView* m_browser_view;

  
  //  CEditWidget* m_edit_widget; // a pointer to the actual editwidget
  //  CEditWidget* m_header_widget; // the editwidget for the headers/resources
  //CEditWidget* m_cpp_widget;    //  the editwidget for cpp files
  CDocBrowser* m_browser_widget;
  //  KSwallowWidget* m_swallow_widget;
 
  /** The classview. */
  KDockWidget* m_dockbase_class_tree;
  CClassView* m_class_tree;
  /** The logical fileview. */
  KDockWidget* m_dockbase_log_file_tree;
  CLogFileView* m_log_file_tree;
  /** The real fileview. */
  KDockWidget* m_dockbase_real_file_tree;
  CRealFileView* m_real_file_tree;
  /** The debugger's tree of local variables */
  KDockWidget* m_dockbase_var_viewer;
  VarViewer* m_var_viewer;
  /** The documentation tree. */
  KDockWidget* m_dockbase_doc_tree;
  DocTreeView* m_doc_tree;
  /** splitview, contains a WidgetsView and a PropertyView */
  KDockWidget* m_dockbase_widprop_split_view;
  WidgetsPropSplitView*  m_widprop_split_view;
  
	/** Output from the compiler ... */
	KDockWidget* m_dockbase_messages_widget;
	MakeView *m_messages_widget;
	/** Output from grep */
	KDockWidget* m_dockbase_grepview;
	GrepView *m_grepview;
	/** Output from the application */
	KDockWidget* m_dockbase_outputview;
	OutputView *m_outputview;
	/** Manages a list of breakpoints - Always active */
	KDockWidget* m_dockbase_brkptManager_view;
	BreakpointManager* m_brkptManager;
	/** Manages a frame stack list */
	KDockWidget* m_dockbase_frameStack_view;
	FrameStack* m_frameStack;
	/** show disassembled code being run */
	KDockWidget* m_dockbase_disassemble_view;
	Disassemble* m_disassemble;
	/** debug aid. Switch on using compile switch GDB_MONITOR
			or DBG_MONITOR */
	KDockWidget* m_dockbase_dbg_widget_view;			
	COutputWidget* m_dbg_widget;


  QString m_version;
  QString m_kdev_caption;
  bool m_project;

  bool  m_prev_was_search_result;

  // Autosaving elements
  /** The autosave timer. */
  QTimer* m_saveTimer;
  /** Tells if autosaving is enabled. */
  bool m_bAutosave;
  /** The autosave interval. */
  int m_saveTimeout;

  bool m_bAutoswitch;
  bool m_bDefaultCV;
  bool m_bKDevelop;

  QProgressBar* m_statProg;
  //some vars for the searchengine
  QString m_search_output;
  QString m_doc_search_display_text, m_doc_search_text;
  // for more then one job in proc;checked in slotProcessExited(KProcess* proc);
  // values are "run","make" "refresh";
  QString m_next_job;
  QString m_make_cmd;

  CAddExistingFileDlg* m_add_dlg;

  int m_lasttab;
  QString m_lastfile;
  bool m_useGlimpse;
  bool m_useHtDig;

  // modes of KDevelop
  bool m_bIsDebuggingInternal;
  bool m_bIsDebuggingExternal;
  bool m_bInternalDbgChosen;
};

#endif
