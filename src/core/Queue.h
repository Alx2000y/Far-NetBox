//---------------------------------------------------------------------------
#ifndef QueueH
#define QueueH
//---------------------------------------------------------------------------
#include "Terminal.h"
#include "FileOperationProgress.h"
//---------------------------------------------------------------------------
class TSimpleThread : public TObject, private boost::noncopyable
{
public:
  explicit /* __fastcall */ TSimpleThread();
  virtual void __fastcall Init();
  virtual /* __fastcall */ ~TSimpleThread();

  virtual void __fastcall Start();
  void __fastcall WaitFor(unsigned int Milliseconds = INFINITE);
  virtual void __fastcall Terminate() = 0;
  void __fastcall Close();
  bool __fastcall IsFinished();

protected:
  HANDLE FThread;
  TThreadID FThreadId;
  bool FFinished;

  virtual void __fastcall Execute() = 0;
  virtual void __fastcall Finished();

public:
  static int __fastcall ThreadProc(void * Thread);

private:
  TSimpleThread(const TSimpleThread &);
  TSimpleThread & operator = (const TSimpleThread &);
};
//---------------------------------------------------------------------------
class TSignalThread : public TSimpleThread
{
public:
  virtual void __fastcall Init(bool LowPriority);
  virtual void __fastcall Start();
  virtual void __fastcall Terminate();
  void __fastcall TriggerEvent();

protected:
  HANDLE FEvent;
  bool FTerminated;

  explicit /* __fastcall */ TSignalThread();
  virtual /* __fastcall */ ~TSignalThread();

  bool __fastcall WaitForEvent();
  int __fastcall WaitForEvent(unsigned int Timeout);
  virtual void __fastcall Execute();
  virtual void __fastcall ProcessEvent() = 0;
};
//---------------------------------------------------------------------------
class TTerminal;
class TQueueItem;
class TTerminalQueue;
class TQueueItemProxy;
class TTerminalQueueStatus;
//---------------------------------------------------------------------------
DEFINE_CALLBACK_TYPE1(TQueueListUpdateEvent, void,
  TTerminalQueue * /* Queue */);
DEFINE_CALLBACK_TYPE2(TQueueItemUpdateEvent, void,
  TTerminalQueue * /* Queue */, TQueueItem * /* Item */);
enum TQueueEvent { qeEmpty, qePendingUserAction };
DEFINE_CALLBACK_TYPE2(TQueueEventEvent, void,
  TTerminalQueue * /* Queue */, TQueueEvent /* Event */);
//---------------------------------------------------------------------------
class TTerminalQueue : public TSignalThread
{
friend class TQueueItem;
friend class TQueueItemProxy;

public:
  explicit /* __fastcall */ TTerminalQueue(TTerminal * Terminal, TConfiguration * Configuration);
  virtual /* __fastcall */ ~TTerminalQueue();

  virtual void __fastcall Init();
  void __fastcall AddItem(TQueueItem * Item);
  TTerminalQueueStatus * __fastcall CreateStatus(TTerminalQueueStatus * Current);
  void __fastcall Idle();

  int __fastcall GetTransfersLimit() { return FTransfersLimit; }
  bool __fastcall GetEnabled() const { return FEnabled; }
  TQueryUserEvent & __fastcall GetOnQueryUser() { return FOnQueryUser; }
  void __fastcall SetOnQueryUser(TQueryUserEvent value) { FOnQueryUser = value; }
  TPromptUserEvent & __fastcall GetOnPromptUser() { return FOnPromptUser; }
  void __fastcall SetOnPromptUser(TPromptUserEvent value) { FOnPromptUser = value; }
  TExtendedExceptionEvent & __fastcall GetOnShowExtendedException() { return FOnShowExtendedException; }
  void __fastcall SetOnShowExtendedException(TExtendedExceptionEvent value) { FOnShowExtendedException = value; }
  TQueueListUpdateEvent & __fastcall GetOnListUpdate() { return FOnListUpdate; }
  void __fastcall SetOnListUpdate(TQueueListUpdateEvent value) { FOnListUpdate = value; }
  TQueueItemUpdateEvent & __fastcall GetOnQueueItemUpdate() { return FOnQueueItemUpdate; }
  void __fastcall SetOnQueueItemUpdate(TQueueItemUpdateEvent value) { FOnQueueItemUpdate = value; }
  TQueueEventEvent & __fastcall GetOnEvent() { return FOnEvent; }
  void __fastcall SetOnEvent(TQueueEventEvent value) { FOnEvent = value; }

protected:
  friend class TTerminalItem;
  friend class TQueryUserAction;
  friend class TPromptUserAction;
  friend class TShowExtendedExceptionAction;

  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TExtendedExceptionEvent FOnShowExtendedException;
  TQueueItemUpdateEvent FOnQueueItemUpdate;
  TQueueListUpdateEvent FOnListUpdate;
  TQueueEventEvent FOnEvent;
  TTerminal * FTerminal;
  TConfiguration * FConfiguration;
  TSessionData * FSessionData;
  TList * FItems;
  int FItemsInProcess;
  TCriticalSection * FItemsSection;
  int FFreeTerminals;
  TList * FTerminals;
  TList * FForcedItems;
  int FTemporaryTerminals;
  int FOverallTerminals;
  int FTransfersLimit;
  bool FEnabled;
  TDateTime FIdleInterval;
  TDateTime FLastIdle;
  TTerminalQueue * Self;

public:
  TQueueItem * __fastcall GetItem(int Index);
  bool __fastcall ItemGetData(TQueueItem * Item, TQueueItemProxy * Proxy);
  bool __fastcall ItemProcessUserAction(TQueueItem * Item, void * Arg);
  bool __fastcall ItemMove(TQueueItem * Item, TQueueItem * BeforeItem);
  bool __fastcall ItemExecuteNow(TQueueItem * Item);
  bool __fastcall ItemDelete(TQueueItem * Item);
  bool __fastcall ItemPause(TQueueItem * Item, bool Pause);
  bool __fastcall ItemSetCPSLimit(TQueueItem * Item, unsigned long CPSLimit);

  void __fastcall RetryItem(TQueueItem * Item);
  void __fastcall DeleteItem(TQueueItem * Item);

  virtual void __fastcall ProcessEvent();
  void __fastcall TerminalFinished(TTerminalItem * TerminalItem);
  bool __fastcall TerminalFree(TTerminalItem * TerminalItem);

  void __fastcall DoQueueItemUpdate(TQueueItem * Item);
  void __fastcall DoListUpdate();
  void __fastcall DoEvent(TQueueEvent Event);

public:
  void __fastcall SetMasks(const UnicodeString value);
  void __fastcall SetTransfersLimit(int value);
  void __fastcall SetEnabled(bool value);
  bool __fastcall GetIsEmpty();
private:
  TTerminalQueue(const TTerminalQueue &);
  TTerminalQueue & operator = (const TTerminalQueue &);
};
//---------------------------------------------------------------------------
class TQueueItem : public TObject
{
friend class TTerminalQueue;
friend class TTerminalItem;

public:
  enum TStatus {
    qsPending, qsConnecting, qsProcessing, qsPrompt, qsQuery, qsError,
    qsPaused, qsDone };
  struct TInfo
  {
    TFileOperation Operation;
    TOperationSide Side;
    UnicodeString Source;
    UnicodeString Destination;
    UnicodeString ModifiedLocal;
    UnicodeString ModifiedRemote;
  };

  static bool __fastcall IsUserActionStatus(TStatus Status);

  TStatus __fastcall GetStatus();
  HANDLE __fastcall GetCompleteEvent() { return FCompleteEvent; }
  void __fastcall SetCompleteEvent(HANDLE value) { FCompleteEvent = value; }

protected:
  TStatus FStatus;
  TCriticalSection * FSection;
  TTerminalItem * FTerminalItem;
  TFileOperationProgressType * FProgressData;
  TQueueItem::TInfo * FInfo;
  TTerminalQueue * FQueue;
  HANDLE FCompleteEvent;
  long FCPSLimit;
  TQueueItem * Self;

  explicit /* __fastcall */ TQueueItem();
  virtual /* __fastcall */ ~TQueueItem();

public:
  void __fastcall SetMasks(const UnicodeString value);
  void __fastcall SetStatus(TStatus Status);
  void __fastcall SetProgress(TFileOperationProgressType & ProgressData);
  void __fastcall GetData(TQueueItemProxy * Proxy);
  void __fastcall SetCPSLimit(unsigned long CPSLimit);
private:
  void __fastcall Execute(TTerminalItem * TerminalItem);
  virtual void __fastcall DoExecute(TTerminal * Terminal) = 0;
  virtual UnicodeString __fastcall StartupDirectory() = 0;
};
//---------------------------------------------------------------------------
class TQueueItemProxy : public TObject
{
friend class TQueueItem;
friend class TTerminalQueueStatus;
friend class TTerminalQueue;

public:
  bool __fastcall Update();
  bool __fastcall ProcessUserAction();
  bool __fastcall Move(bool Sooner);
  bool __fastcall Move(TQueueItemProxy * BeforeItem);
  bool __fastcall ExecuteNow();
  bool __fastcall Delete();
  bool __fastcall Pause();
  bool __fastcall Resume();
  bool __fastcall SetCPSLimit(unsigned long CPSLimit);

  TFileOperationProgressType * __fastcall GetProgressData();
  TQueueItem::TInfo * __fastcall GetInfo() { return FInfo; }
  TQueueItem::TStatus __fastcall GetStatus() const { return FStatus; }
  bool __fastcall GetProcessingUserAction() const { return FProcessingUserAction; }
  int __fastcall GetIndex();
  void * __fastcall GetUserData() { return FUserData; }
  void __fastcall SetUserData(void * value) { FUserData = value; }

private:
  TFileOperationProgressType * FProgressData;
  TQueueItem::TStatus FStatus;
  TTerminalQueue * FQueue;
  TQueueItem * FQueueItem;
  TTerminalQueueStatus * FQueueStatus;
  TQueueItem::TInfo * FInfo;
  bool FProcessingUserAction;
  void * FUserData;
  TQueueItemProxy * Self;

  explicit /* __fastcall */ TQueueItemProxy(TTerminalQueue * Queue, TQueueItem * QueueItem);
  virtual /* __fastcall */ ~TQueueItemProxy();
public:
  void __fastcall SetMasks(const UnicodeString value);
};
//---------------------------------------------------------------------------
class TTerminalQueueStatus
{
friend class TTerminalQueue;
friend class TQueueItemProxy;

public:
  virtual /* __fastcall */ ~TTerminalQueueStatus();

  TQueueItemProxy * __fastcall FindByQueueItem(TQueueItem * QueueItem);

  int __fastcall GetCount();
  int __fastcall GetActiveCount();
  TQueueItemProxy * __fastcall GetItem(int Index);

protected:
  /* __fastcall */ TTerminalQueueStatus();

  void __fastcall Add(TQueueItemProxy * ItemProxy);
  void __fastcall Delete(TQueueItemProxy * ItemProxy);
  void __fastcall ResetStats();

private:
  TList * FList;
  int FActiveCount;

public:
  void __fastcall SetMasks(const UnicodeString value);
};
//---------------------------------------------------------------------------
class TLocatedQueueItem : public TQueueItem
{
protected:
  explicit /* __fastcall */ TLocatedQueueItem(TTerminal * Terminal);
  virtual /* __fastcall */ ~TLocatedQueueItem() {}

  virtual void __fastcall DoExecute(TTerminal * Terminal);
  virtual UnicodeString __fastcall StartupDirectory();

private:
  UnicodeString FCurrentDir;
};
//---------------------------------------------------------------------------
class TTransferQueueItem : public TLocatedQueueItem
{
public:
  explicit /* __fastcall */ TTransferQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, int Params, TOperationSide Side);
  virtual /* __fastcall */ ~TTransferQueueItem();

protected:
  TStrings * FFilesToCopy;
  UnicodeString FTargetDir;
  TCopyParamType * FCopyParam;
  int FParams;
};
//---------------------------------------------------------------------------
class TUploadQueueItem : public TTransferQueueItem
{
public:
  explicit /* __fastcall */ TUploadQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, int Params);
  virtual /* __fastcall */ ~TUploadQueueItem() {}
protected:
  virtual void __fastcall DoExecute(TTerminal * Terminal);
};
//---------------------------------------------------------------------------
class TDownloadQueueItem : public TTransferQueueItem
{
public:
  explicit /* __fastcall */ TDownloadQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, int Params);
  virtual /* __fastcall */ ~TDownloadQueueItem() {}
protected:
  virtual void __fastcall DoExecute(TTerminal * Terminal);
};
//---------------------------------------------------------------------------
class TUserAction;
class TTerminalThread : public TSignalThread
{
public:
  explicit /* __fastcall */ TTerminalThread(TTerminal * Terminal);
  virtual void __fastcall Init();
  virtual /* __fastcall */ ~TTerminalThread();

  void __fastcall TerminalOpen();
  void __fastcall TerminalReopen();

  void __fastcall Cancel();
  void __fastcall Idle();

  TNotifyEvent & __fastcall GetOnIdle() { return FOnIdle; }
  void __fastcall SetOnIdle(TNotifyEvent Value) { FOnIdle = Value; }
  bool __fastcall GetCancelling() const { return FCancel; };

protected:
  virtual void __fastcall ProcessEvent();

private:
  TTerminal * FTerminal;

  TInformationEvent FOnInformation;
  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TExtendedExceptionEvent FOnShowExtendedException;
  TDisplayBannerEvent FOnDisplayBanner;
  TNotifyEvent FOnChangeDirectory;
  TReadDirectoryEvent FOnReadDirectory;
  TNotifyEvent FOnStartReadDirectory;
  TReadDirectoryProgressEvent FOnReadDirectoryProgress;

  TNotifyEvent FOnIdle;

  TNotifyEvent FAction;
  HANDLE FActionEvent;
  TUserAction * FUserAction;

  Exception * FException;
  Exception * FIdleException;
  bool FCancel;
  bool FCancelled;
  bool FPendingIdle;

  DWORD FMainThread;
  TCriticalSection * FSection;
  TTerminalThread * Self;

  void __fastcall WaitForUserAction(TUserAction * UserAction);
  void __fastcall RunAction(TNotifyEvent Action);

  static void __fastcall SaveException(Exception & E, Exception *& Exception);
  static void __fastcall Rethrow(Exception *& Exception);
  void __fastcall FatalAbort();
  void __fastcall CheckCancel();

  void /* __fastcall */ TerminalOpenEvent(TObject * Sender);
  void /* __fastcall */ TerminalReopenEvent(TObject * Sender);

  void /* __fastcall */ TerminalInformation(
    TTerminal * Terminal, const UnicodeString & Str, bool Status, int Phase);
  void /* __fastcall */ TerminalQueryUser(TObject * Sender,
    const UnicodeString & Query, TStrings * MoreMessages, unsigned int Answers,
    const TQueryParams * Params, unsigned int & Answer, TQueryType Type, void * Arg);
  void /* __fastcall */ TerminalPromptUser(TTerminal * Terminal, TPromptKind Kind,
    const UnicodeString & Name, const UnicodeString & Instructions,
    TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
  void /* __fastcall */ TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void /* __fastcall */ TerminalDisplayBanner(TTerminal * Terminal,
    UnicodeString SessionName, const UnicodeString & Banner,
    bool & NeverShowAgain, int Options);
  void /* __fastcall */ TerminalChangeDirectory(TObject * Sender);
  void /* __fastcall */ TerminalReadDirectory(TObject * Sender, Boolean ReloadOnly);
  void /* __fastcall */ TerminalStartReadDirectory(TObject * Sender);
  void /* __fastcall */ TerminalReadDirectoryProgress(TObject * Sender, int Progress, bool & Cancel);

private:
  TTerminalThread(const TTerminalThread &);
  TTerminalThread & operator = (const TTerminalThread &);
};
//---------------------------------------------------------------------------
#endif
