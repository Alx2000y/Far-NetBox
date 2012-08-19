//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#else
#include "stdafx.h"

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>
#endif

#include <stdio.h>
#include <lmcons.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <secext.h>

#include "Common.h"
#include "SessionInfo.h"
#include "Exceptions.h"
#include "TextsCore.h"
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
UnicodeString __fastcall DoXmlEscape(UnicodeString Str, bool NewLine)
{
  for (int i = 1; i <= Str.Length(); i++)
  {
    const wchar_t * Repl = NULL;
    switch (Str[i])
    {
      case L'&':
        Repl = L"amp;";
        break;

      case L'>':
        Repl = L"gt;";
        break;

      case L'<':
        Repl = L"lt;";
        break;

      case L'"':
        Repl = L"quot;";
        break;

      case L'\n':
        if (NewLine)
        {
          Repl = L"#10;";
        }
        break;

      case L'\r':
        Str.Delete(i, 1);
        i--;
        break;
    }

    if (Repl != NULL)
    {
      Str[i] = L'&';
      Str.Insert(Repl, i + 1);
      i += wcslen(Repl);
    }
  }
  return Str;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall XmlEscape(UnicodeString Str)
{
  return DoXmlEscape(Str, false);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall XmlAttributeEscape(UnicodeString Str)
{
  return DoXmlEscape(Str, true);
}
//---------------------------------------------------------------------------
TStrings * __fastcall ExceptionToMessages(Exception * E)
{
  TStrings * Result = NULL;
  UnicodeString Message;
  if (ExceptionMessage(E, Message))
  {
    Result = new TStringList();
    Result->Add(Message);
    ExtException * EE = dynamic_cast<ExtException *>(E);
    if ((EE != NULL) && (EE->GetMoreMessages() != NULL))
    {
      Result->AddStrings(EE->GetMoreMessages());
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// #pragma warn -inl
class TSessionActionRecord
{
public:
  explicit /* __fastcall */ TSessionActionRecord(TActionLog * Log, TLogAction Action) :
    FLog(Log),
    FAction(Action),
    FState(Opened),
    FRecursive(false),
    FErrorMessages(NULL),
    FNames(new TStringList()),
    FValues(new TStringList()),
    FFileList(NULL),
    FFile(NULL)
  {
    FLog->AddPendingAction(this);
  }

  /* __fastcall */ ~TSessionActionRecord()
  {
    delete FErrorMessages;
    delete FNames;
    delete FValues;
    delete FFileList;
    delete FFile;
  }

  void __fastcall Restart()
  {
    FState = Opened;
    FRecursive = false;
    delete FErrorMessages;
    FErrorMessages = NULL;
    delete FFileList;
    FFileList = NULL;
    delete FFile;
    FFile = NULL;
    FNames->Clear();
    FValues->Clear();
  }

  bool __fastcall Record()
  {
    bool Result = (FState != Opened);
    if (Result)
    {
      if (FLog->FLogging && (FState != Cancelled))
      {
        const wchar_t * Name = ActionName();
        UnicodeString Attrs;
        if (FRecursive)
        {
          Attrs = L" recursive=\"true\"";
        }
        FLog->AddIndented(FORMAT(L"<%s%s>", Name,  Attrs.c_str()));
        for (int Index = 0; Index < FNames->GetCount(); Index++)
        {
          UnicodeString Value = FValues->GetStrings(Index);
          if (Value.IsEmpty())
          {
            FLog->AddIndented(FORMAT(L"  <%s />", FNames->GetStrings(Index).c_str()));
          }
          else
          {
            FLog->AddIndented(FORMAT(L"  <%s value=\"%s\" />",
              FNames->GetStrings(Index).c_str(), XmlAttributeEscape(Value).c_str()));
          }
        }
        if (FFileList != NULL)
        {
          FLog->AddIndented(L"  <files>");
          for (int Index = 0; Index < FFileList->GetCount(); Index++)
          {
            TRemoteFile * File = FFileList->GetFiles(Index);

            FLog->AddIndented(L"    <file>");
            FLog->AddIndented(FORMAT(L"      <filename value=\"%s\" />", XmlAttributeEscape(File->GetFileName()).c_str()));
            FLog->AddIndented(FORMAT(L"      <type value=\"%s\" />", XmlAttributeEscape(File->GetType()).c_str()));
            if (!File->GetIsDirectory())
            {
              FLog->AddIndented(FORMAT(L"      <size value=\"%s\" />", Int64ToStr(File->GetSize()).c_str()));
            }
            FLog->AddIndented(FORMAT(L"      <modification value=\"%s\" />", StandardTimestamp(File->GetModification()).c_str()));
            FLog->AddIndented(FORMAT(L"      <permissions value=\"%s\" />", XmlAttributeEscape(File->GetRights()->GetText()).c_str()));
            FLog->AddIndented(L"    </file>");
          }
          FLog->AddIndented(L"  </files>");
        }
        if (FFile != NULL)
        {
          FLog->AddIndented(L"  <file>");
          FLog->AddIndented(FORMAT(L"    <type value=\"%s\" />", XmlAttributeEscape(FFile->GetType()).c_str()));
          if (!FFile->GetIsDirectory())
          {
            FLog->AddIndented(FORMAT(L"    <size value=\"%s\" />", Int64ToStr(FFile->GetSize()).c_str()));
          }
          FLog->AddIndented(FORMAT(L"    <modification value=\"%s\" />", StandardTimestamp(FFile->GetModification()).c_str()));
          FLog->AddIndented(FORMAT(L"    <permissions value=\"%s\" />", XmlAttributeEscape(FFile->GetRights()->GetText()).c_str()));
          FLog->AddIndented(L"  </file>");
        }
        if (FState == RolledBack)
        {
          if (FErrorMessages != NULL)
          {
            FLog->AddIndented(L"  <result success=\"false\">");
            FLog->AddMessages(L"    ", FErrorMessages);
            FLog->AddIndented(L"  </result>");
          }
          else
          {
            FLog->AddIndented(L"  <result success=\"false\" />");
          }
        }
        else
        {
          FLog->AddIndented(L"  <result success=\"true\" />");
        }
        FLog->AddIndented(FORMAT(L"</%s>", Name));
      }
      delete this;
    }
    return Result;
  }

  void __fastcall Commit()
  {
    Close(Committed);
  }

  void __fastcall Rollback(Exception * E)
  {
    assert(FErrorMessages == NULL);
    FErrorMessages = ExceptionToMessages(E);
    Close(RolledBack);
  }

  void __fastcall Cancel()
  {
    Close(Cancelled);
  }

  void __fastcall FileName(const UnicodeString & FileName)
  {
    Parameter(L"filename", FileName);
  }

  void __fastcall Destination(const UnicodeString & Destination)
  {
    Parameter(L"destination", Destination);
  }

  void __fastcall Rights(const TRights & Rights)
  {
    Parameter(L"permissions", Rights.GetText());
  }

  void __fastcall Modification(const TDateTime & DateTime)
  {
    Parameter(L"modification", StandardTimestamp(DateTime));
  }

  void __fastcall Recursive()
  {
    FRecursive = true;
  }

  void __fastcall Command(const UnicodeString & Command)
  {
    Parameter(L"command", Command);
  }

  void __fastcall AddOutput(UnicodeString Output, bool StdError)
  {
    const wchar_t * Name = (StdError ? L"erroroutput" : L"output");
    int Index = FNames->IndexOf(Name);
    if (Index >= 0)
    {
      FValues->PutString(Index, FValues->GetStrings(Index) + L"\r\n" + Output);
    }
    else
    {
      Parameter(Name, Output);
    }
  }

  void __fastcall FileList(TRemoteFileList * FileList)
  {
    if (FFileList == NULL)
    {
      FFileList = new TRemoteFileList();
    }
    FileList->DuplicateTo(FFileList);
  }

  void __fastcall File(TRemoteFile * File)
  {
    if (FFile != NULL)
    {
      delete FFile;
    }
    FFile = File->Duplicate(true);
  }

protected:
  enum TState { Opened, Committed, RolledBack, Cancelled };

  inline void __fastcall Close(TState State)
  {
    assert(FState == Opened);
    FState = State;
    FLog->RecordPendingActions();
  }

  const wchar_t * __fastcall ActionName()
  {
    switch (FAction)
    {
      case laUpload: return L"upload";
      case laDownload: return L"download";
      case laTouch: return L"touch";
      case laChmod: return L"chmod";
      case laMkdir: return L"mkdir";
      case laRm: return L"rm";
      case laMv: return L"mv";
      case laCall: return L"call";
      case laLs: return L"ls";
      case laStat: return L"stat";
      default: assert(false); return L"";
    }
  }

  void __fastcall Parameter(const UnicodeString & Name, const UnicodeString & Value = L"")
  {
    FNames->Add(Name);
    FValues->Add(Value);
  }

private:
  TActionLog * FLog;
  TLogAction FAction;
  TState FState;
  bool FRecursive;
  TStrings * FErrorMessages;
  TStrings * FNames;
  TStrings * FValues;
  TRemoteFileList * FFileList;
  TRemoteFile * FFile;
};
// #pragma warn .inl
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TSessionAction::TSessionAction(TActionLog *Log, TLogAction Action)
{
  if (Log->FLogging)
  {
    FRecord = new TSessionActionRecord(Log, Action);
  }
  else
  {
    FRecord = NULL;
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ TSessionAction::~TSessionAction()
{
  if (FRecord != NULL)
  {
    Commit();
  }
}
//---------------------------------------------------------------------------
void __fastcall TSessionAction::Restart()
{
  if (FRecord != NULL)
  {
    FRecord->Restart();
  }
}
//---------------------------------------------------------------------------
void __fastcall TSessionAction::Commit()
{
  if (FRecord != NULL)
  {
    TSessionActionRecord * Record = FRecord;
    FRecord = NULL;
    Record->Commit();
  }
}
//---------------------------------------------------------------------------
void __fastcall TSessionAction::Rollback(Exception * E)
{
  if (FRecord != NULL)
  {
    TSessionActionRecord * Record = FRecord;
    FRecord = NULL;
    Record->Rollback(E);
  }
}
//---------------------------------------------------------------------------
void __fastcall TSessionAction::Cancel()
{
  if (FRecord != NULL)
  {
    TSessionActionRecord * Record = FRecord;
    FRecord = NULL;
    Record->Cancel();
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TFileSessionAction::TFileSessionAction(TActionLog * Log, TLogAction Action) :
  TSessionAction(Log, Action)
{
};
//---------------------------------------------------------------------------
/* __fastcall */ TFileSessionAction::TFileSessionAction(
    TActionLog * Log, TLogAction Action, const UnicodeString & AFileName) :
  TSessionAction(Log, Action)
{
  FileName(AFileName);
};
//---------------------------------------------------------------------------
void __fastcall TFileSessionAction::FileName(const UnicodeString & FileName)
{
  if (FRecord != NULL)
  {
    FRecord->FileName(FileName);
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TFileLocationSessionAction::TFileLocationSessionAction(
    TActionLog * Log, TLogAction Action) :
  TFileSessionAction(Log, Action)
{
};
//---------------------------------------------------------------------------
/* __fastcall */ TFileLocationSessionAction::TFileLocationSessionAction(
    TActionLog * Log, TLogAction Action, const UnicodeString & FileName) :
  TFileSessionAction(Log, Action, FileName)
{
};
//---------------------------------------------------------------------------
void __fastcall TFileLocationSessionAction::Destination(const UnicodeString & Destination)
{
  if (FRecord != NULL)
  {
    FRecord->Destination(Destination);
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TUploadSessionAction::TUploadSessionAction(TActionLog * Log) :
  TFileLocationSessionAction(Log, laUpload)
{
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TDownloadSessionAction::TDownloadSessionAction(TActionLog * Log) :
  TFileLocationSessionAction(Log, laDownload)
{
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TChmodSessionAction::TChmodSessionAction(
    TActionLog * Log, const UnicodeString & FileName) :
  TFileSessionAction(Log, laChmod, FileName)
{
}
//---------------------------------------------------------------------------
void __fastcall TChmodSessionAction::Recursive()
{
  if (FRecord != NULL)
  {
    FRecord->Recursive();
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ TChmodSessionAction::TChmodSessionAction(
    TActionLog * Log, const UnicodeString & FileName, const TRights & ARights) :
  TFileSessionAction(Log, laChmod, FileName)
{
  Rights(ARights);
}
//---------------------------------------------------------------------------
void __fastcall TChmodSessionAction::Rights(const TRights & Rights)
{
  if (FRecord != NULL)
  {
    FRecord->Rights(Rights);
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ TTouchSessionAction::TTouchSessionAction(
    TActionLog * Log, const UnicodeString & FileName, const TDateTime & Modification) :
  TFileSessionAction(Log, laTouch, FileName)
{
  if (FRecord != NULL)
  {
    FRecord->Modification(Modification);
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ TMkdirSessionAction::TMkdirSessionAction(
    TActionLog * Log, const UnicodeString & FileName) :
  TFileSessionAction(Log, laMkdir, FileName)
{
}
//---------------------------------------------------------------------------
/* __fastcall */ TRmSessionAction::TRmSessionAction(
    TActionLog * Log, const UnicodeString & FileName) :
  TFileSessionAction(Log, laRm, FileName)
{
}
//---------------------------------------------------------------------------
void __fastcall TRmSessionAction::Recursive()
{
  if (FRecord != NULL)
  {
    FRecord->Recursive();
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ TMvSessionAction::TMvSessionAction(TActionLog * Log,
    const UnicodeString & FileName, const UnicodeString & ADestination) :
  TFileLocationSessionAction(Log, laMv, FileName)
{
  Destination(ADestination);
}
//---------------------------------------------------------------------------
/* __fastcall */ TCallSessionAction::TCallSessionAction(TActionLog * Log,
    const UnicodeString & Command, const UnicodeString & Destination) :
  TSessionAction(Log, laCall)
{
  if (FRecord != NULL)
  {
    FRecord->Command(Command);
    FRecord->Destination(Destination);
  }
}
//---------------------------------------------------------------------------
void __fastcall TCallSessionAction::AddOutput(const UnicodeString & Output, bool StdError)
{
  if (FRecord != NULL)
  {
    FRecord->AddOutput(Output, StdError);
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ TLsSessionAction::TLsSessionAction(TActionLog * Log,
    const UnicodeString & Destination) :
  TSessionAction(Log, laLs)
{
  if (FRecord != NULL)
  {
    FRecord->Destination(Destination);
  }
}
//---------------------------------------------------------------------------
void __fastcall TLsSessionAction::FileList(TRemoteFileList * FileList)
{
  if (FRecord != NULL)
  {
    FRecord->FileList(FileList);
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TStatSessionAction::TStatSessionAction(TActionLog * Log, const UnicodeString & FileName) :
  TFileSessionAction(Log, laStat, FileName)
{
}
//---------------------------------------------------------------------------
void __fastcall TStatSessionAction::File(TRemoteFile * File)
{
  if (FRecord != NULL)
  {
    FRecord->File(File);
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TSessionInfo::TSessionInfo()
{
  LoginTime = Now();
}
//---------------------------------------------------------------------------
TFileSystemInfo::TFileSystemInfo()
{
  memset(&IsCapable, (int)false, sizeof(IsCapable));
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
FILE * __fastcall OpenFile(UnicodeString LogFileName, TSessionData * SessionData, bool Append, UnicodeString & NewFileName)
{
  FILE * Result;
  UnicodeString ANewFileName = GetExpandedLogFileName(LogFileName, SessionData);
  // Result = _wfopen(ANewFileName.c_str(), (Append ? L"a" : L"w"));
  Result = _fsopen(W2MB(ANewFileName.c_str()).c_str(),
    Append ? "a" : "w", SH_DENYWR); // _SH_DENYNO); // 
  if (Result != NULL)
  {
    setvbuf(Result, NULL, _IONBF, BUFSIZ);
    NewFileName = ANewFileName;
  }
  else
  {
    throw Exception(FMTLOAD(LOG_OPENERROR, ANewFileName.c_str()));
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
const wchar_t *LogLineMarks = L"<>!.*";
/* __fastcall */ TSessionLog::TSessionLog(TSessionUI* UI, TSessionData * SessionData,
  TConfiguration * AConfiguration):
  TStringList()
{
  FCriticalSection = new TCriticalSection();
  FLogging = false;
  FConfiguration = AConfiguration;
  FParent = NULL;
  FUI = UI;
  FSessionData = SessionData;
  FFile = NULL;
  FLoggedLines = 0;
  FTopIndex = -1;
  FCurrentLogFileName = L"";
  FCurrentFileName = L"";
  FClosed = false;
  Self = this;
}
//---------------------------------------------------------------------------
/* __fastcall */ TSessionLog::~TSessionLog()
{
  FClosed = true;
  ReflectSettings();
  assert(FFile == NULL);
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::Lock()
{
  FCriticalSection->Enter();
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::Unlock()
{
  FCriticalSection->Leave();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TSessionLog::GetSessionName()
{
  assert(FSessionData != NULL);
  return FSessionData->GetSessionName();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TSessionLog::GetLine(Integer Index)
{
  return GetStrings(Index - FTopIndex);
}
//---------------------------------------------------------------------------
TLogLineType __fastcall TSessionLog::GetType(int Index)
{
  return static_cast<TLogLineType>(reinterpret_cast<size_t>(GetObjects(Index - FTopIndex)));
}
//---------------------------------------------------------------------------
void /* __fastcall */ TSessionLog::DoAddToParent(TLogLineType Type, UnicodeString Line)
{
  assert(FParent != NULL);
  FParent->Add(Type, Line);
}
//---------------------------------------------------------------------------
void /* __fastcall */ TSessionLog::DoAddToSelf(TLogLineType Type, UnicodeString Line)
{
  if (static_cast<int>(FTopIndex) < 0)
  {
    FTopIndex = 0;
  }

  TStringList::AddObject(Line, static_cast<TObject *>(reinterpret_cast<void *>(static_cast<size_t>(Type))));

  FLoggedLines++;

  if (LogToFile())
  {
    if (FFile == NULL)
    {
      OpenLogFile();
    }

    if (FFile != NULL)
    {
      unsigned short Y, M, D, H, N, S, MS;
      TDateTime DateTime = Now();
      DateTime.DecodeDate(Y, M, D);
      DateTime.DecodeTime(H, N, S, MS);
      UnicodeString dt = FORMAT(L" %04d-%02d-%02d %02d:%02d:%02d.%03d ", Y, M, D, H, N, S, MS);
      // UnicodeString Timestamp = FormatDateTime(L" yyyy-mm-dd hh:nn:ss.zzz ", Now());
      UnicodeString Timestamp = dt;
      UTF8String UtfLine = UTF8String(UnicodeString(LogLineMarks[Type]) + Timestamp + Line + "\n");
      // fwrite(UtfLine.c_str(), UtfLine.Length(), 1, (FILE *)FFile);
      fprintf_s(static_cast<FILE *>(FFile), "%s", const_cast<char *>(AnsiString(UtfLine).c_str()));
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::DoAdd(TLogLineType Type, UnicodeString Line,
  // void __fastcall (__closure *f)(TLogLineType Type, const UnicodeString & Line))
  TDoAddLogEvent Event)
{
  UnicodeString Prefix;

  if (!GetName().IsEmpty())
  {
    Prefix = L"[" + GetName() + L"] ";
  }

  while (!Line.IsEmpty())
  {
    Event(Type, Prefix + CutToChar(Line, L'\n', false));
  }
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::Add(TLogLineType Type, const UnicodeString & Line)
{
  assert(FConfiguration);
  if (GetLogging())
  {
    try
    {
      if (FParent != NULL)
      {
        DoAdd(Type, Line, fastdelegate::bind(&TSessionLog::DoAddToParent, this, _1, _2));
      }
      else
      {
        TGuard Guard(FCriticalSection);

        BeginUpdate();
        // try
        {
          BOOST_SCOPE_EXIT ( (&Self) )
          {
            Self->DeleteUnnecessary();
            Self->EndUpdate();
          } BOOST_SCOPE_EXIT_END
          DoAdd(Type, Line, fastdelegate::bind(&TSessionLog::DoAddToSelf, this, _1, _2));
        }
#ifndef _MSC_VER
        __finally
        {
          DeleteUnnecessary();

          EndUpdate();
        }
#endif
      }
    }
    catch (Exception &E)
    {
      // We failed logging, turn it off and notify user.
      FConfiguration->SetLogging(false);
      try
      {
        throw ExtException(&E, FMTLOAD(LOG_GEN_ERROR));
      }
      catch (Exception &E)
      {
        AddException(&E);
        FUI->HandleExtendedException(&E);
      }
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::AddException(Exception * E)
{
  if (E != NULL)
  {
    Add(llException, ExceptionLogString(E));
  }
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::ReflectSettings()
{
  TGuard Guard(FCriticalSection);

  bool ALogging =
    !FClosed &&
    ((FParent != NULL) || FConfiguration->GetLogging());

  if (FLogging != ALogging)
  {
    FLogging = ALogging;
    StateChange();
  }

  // if logging to file was turned off or log file was changed -> close current log file
  if ((FFile != NULL) &&
      (!LogToFile() || (FCurrentLogFileName != FConfiguration->GetLogFileName())))
  {
    CloseLogFile();
  }

  DeleteUnnecessary();
}
//---------------------------------------------------------------------------
bool __fastcall TSessionLog::LogToFile()
{
  return GetLogging() && FConfiguration->GetLogToFile() && (FParent == NULL);
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::CloseLogFile()
{
  if (FFile != NULL)
  {
    fclose(static_cast<FILE *>(FFile));
    FFile = NULL;
  }
  FCurrentLogFileName = L"";
  FCurrentFileName = L"";
  StateChange();
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::OpenLogFile()
{
  try
  {
    assert(FFile == NULL);
    assert(FConfiguration != NULL);
    FCurrentLogFileName = FConfiguration->GetLogFileName();
    FFile = OpenFile(FCurrentLogFileName, FSessionData, FConfiguration->GetLogFileAppend(), FCurrentFileName);
  }
  catch (Exception & E)
  {
    // We failed logging to file, turn it off and notify user.
    FCurrentLogFileName = L"";
    FCurrentFileName = L"";
    FConfiguration->SetLogFileName(UnicodeString());
    try
    {
      throw ExtException(&E, FMTLOAD(LOG_GEN_ERROR));
    }
    catch (Exception & E)
    {
      AddException(&E);
      FUI->HandleExtendedException(&E);
    }
  }
  StateChange();
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::StateChange()
{
  if (!FOnStateChange.empty())
  {
    FOnStateChange(this);
  }
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::DeleteUnnecessary()
{
  BeginUpdate();
  // try
  {
    BOOST_SCOPE_EXIT ( (&Self) )
    {
        Self->EndUpdate();
    } BOOST_SCOPE_EXIT_END
    if (!GetLogging() || (FParent != NULL))
    {
        Clear();
    }
    else
    {
      while (!FConfiguration->GetLogWindowComplete() && (GetCount() > FConfiguration->GetLogWindowLines()))
      {
        Delete(0);
        FTopIndex++;
      }
    }
  }
#ifndef _MSC_VER
  __finally
  {
    EndUpdate();
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::AddStartupInfo()
{
  if (GetLogging())
  {
    if (FParent != NULL)
    {
      // do not add session info for secondary session
      // (this should better be handled in the TSecondaryTerminal)
    }
    else
    {
      DoAddStartupInfo(FSessionData);
    }
  }
}
//---------------------------------------------------------------------------
void /* __fastcall */ TSessionLog::DoAddStartupInfo(TSessionData * Data)
{
  TGuard Guard(FCriticalSection);

  BeginUpdate();
  // try
  {
#ifdef _MSC_VER
    BOOST_SCOPE_EXIT ( (&Self) )
    {
      Self->DeleteUnnecessary();

      Self->EndUpdate();
    } BOOST_SCOPE_EXIT_END
#endif
    // #define ADF(S, F) DoAdd(llMessage, FORMAT(S, F), DoAddToSelf);
    #define ADF(S, ...) DoAdd(llMessage, FORMAT(S, __VA_ARGS__), fastdelegate::bind(&TSessionLog::DoAddToSelf, this, _1, _2));
    AddSeparator();
    ADF(L"NetBox %s (OS %s)", FConfiguration->GetVersionStr().c_str(), FConfiguration->GetOSVersionStr().c_str());
    THierarchicalStorage * Storage = FConfiguration->CreateScpStorage(false);
    // try
    {
#ifdef _MSC_VER
      BOOST_SCOPE_EXIT ( (&Storage) )
      {
        delete Storage;
      } BOOST_SCOPE_EXIT_END
#endif
      ADF(L"Configuration: %s", Storage->GetSource().c_str());
    }
#ifndef _MSC_VER
    __finally
    {
      delete Storage;
    }
#endif

    typedef BOOL (WINAPI * TGetUserNameEx)(EXTENDED_NAME_FORMAT NameFormat, LPWSTR lpNameBuffer, PULONG nSize);
    HINSTANCE Secur32 = LoadLibrary(L"secur32.dll");
    TGetUserNameEx GetUserNameEx =
      (Secur32 != NULL) ? reinterpret_cast<TGetUserNameEx>(GetProcAddress(Secur32, "GetUserNameExW")) : NULL;
    wchar_t UserName[UNLEN + 1];
    unsigned long UserNameSize = LENOF(UserName);
    if ((GetUserNameEx == NULL) || !GetUserNameEx(NameSamCompatible, (LPWSTR)UserName, &UserNameSize))
    {
      wcscpy(UserName, L"<Failed to retrieve username>");
    }
    ADF(L"Local account: %s", UserName);
    unsigned short Y, M, D, H, N, S, MS;
    TDateTime DateTime = Now();
    DateTime.DecodeDate(Y, M, D);
    DateTime.DecodeTime(H, N, S, MS);
    UnicodeString dt = FORMAT(L"%02d.%02d.%04d %02d:%02d:%02d", D, M, Y, H, N, S);
    // ADF(L"Login time: %s", FormatDateTime(L"dddddd tt", Now()).c_str());
    ADF(L"Login time: %s", dt.c_str());
    AddSeparator();
    ADF(L"Session name: %s (%s)", Data->GetSessionName().c_str(), Data->GetSource().c_str());
    ADF(L"Host name: %s (Port: %d)", Data->GetHostNameExpanded().c_str(), Data->GetPortNumber());
    ADF(L"User name: %s (Password: %s, Key file: %s)",
      Data->GetUserNameExpanded().c_str(), BooleanToEngStr(!Data->GetPassword().IsEmpty()).c_str(),
       BooleanToEngStr(!Data->GetPublicKeyFile().IsEmpty()).c_str())
    ADF(L"Tunnel: %s", BooleanToEngStr(Data->GetTunnel()).c_str());
    if (Data->GetTunnel())
    {
      ADF(L"Tunnel: Host name: %s (Port: %d)", Data->GetTunnelHostName().c_str(), Data->GetTunnelPortNumber());
      ADF(L"Tunnel: User name: %s (Password: %s, Key file: %s)",
        Data->GetTunnelUserName().c_str(), BooleanToEngStr(!Data->GetTunnelPassword().IsEmpty()).c_str(),
         BooleanToEngStr(!Data->GetTunnelPublicKeyFile().IsEmpty()).c_str());
        ADF(L"Tunnel: Local port number: %d", Data->GetTunnelLocalPortNumber());
    }
    ADF(L"Transfer Protocol: %s", Data->GetFSProtocolStr().c_str());
    ADF(L"Code Page: %d", Data->GetCodePageAsNumber());
    wchar_t * PingTypes = L"-NC";
    TPingType PingType;
    size_t PingInterval;
    if ((Data->GetFSProtocol() == fsFTP) || (Data->GetFSProtocol() == fsFTPS))
    {
      PingType = Data->GetFtpPingType();
      PingInterval = Data->GetFtpPingInterval();
    }
    else
    {
      PingType = Data->GetPingType();
      PingInterval = Data->GetPingInterval();
    }
    ADF(L"Ping type: %s, Ping interval: %d sec; Timeout: %d sec",
      UnicodeString(PingTypes[PingType]).c_str(), PingInterval, Data->GetTimeout());
    ADF(L"Proxy: %s%s", ProxyMethodList[Data->GetProxyMethod()],
      Data->GetProxyMethod() == pmSystem ?
        FORMAT(L" (%s)", ProxyMethodList[Data->GetActualProxyMethod()]).c_str() :
        L"")
    if (Data->GetProxyMethod() != ::pmNone)
    {
      ADF(L"HostName: %s (Port: %d); Username: %s; Passwd: %s",
        Data->GetProxyHost().c_str(), Data->GetProxyPort(),
         Data->GetProxyUsername().c_str(), BooleanToEngStr(!Data->GetProxyPassword().IsEmpty()).c_str());
      if (Data->GetProxyMethod() == pmTelnet)
      {
        ADF(L"Telnet command: %s", Data->GetProxyTelnetCommand().c_str());
      }
      if (Data->GetProxyMethod() == pmCmd)
      {
        ADF(L"Local command: %s", Data->GetProxyLocalCommand().c_str());
      }
    }
    wchar_t const * BugFlags = L"+-A";
    if (Data->GetUsesSsh())
    {
      ADF(L"SSH protocol version: %s; Compression: %s",
        Data->GetSshProtStr().c_str(), BooleanToEngStr(Data->GetCompression()).c_str());
      ADF(L"Bypass authentication: %s",
       BooleanToEngStr(Data->GetSshNoUserAuth()).c_str());
      ADF(L"Try agent: %s; Agent forwarding: %s; TIS/CryptoCard: %s; KI: %s; GSSAPI: %s",
         BooleanToEngStr(Data->GetTryAgent()).c_str(), BooleanToEngStr(Data->GetAgentFwd()).c_str(), BooleanToEngStr(Data->GetAuthTIS()).c_str(),
         BooleanToEngStr(Data->GetAuthKI()).c_str(), BooleanToEngStr(Data->GetAuthGSSAPI()).c_str());
      if (Data->GetAuthGSSAPI())
      {
        ADF(L"GSSAPI: Forwarding: %s; Server realm: %s",
          BooleanToEngStr(Data->GetGSSAPIFwdTGT()).c_str(), Data->GetGSSAPIServerRealm().c_str());
      }
      ADF(L"Ciphers: %s; Ssh2DES: %s",
        Data->GetCipherList().c_str(), BooleanToEngStr(Data->GetSsh2DES()).c_str());
      UnicodeString Bugs;
      for (int Index = 0; Index < BUG_COUNT; Index++)
      {
        Bugs += UnicodeString(BugFlags[Data->GetBug(static_cast<TSshBug>(Index))])+(Index<BUG_COUNT-1?L",":L"");
      }
      ADF(L"SSH Bugs: %s", Bugs.c_str());
      Bugs = L"";
      for (int Index = 0; Index < SFTP_BUG_COUNT; Index++)
      {
        Bugs += UnicodeString(BugFlags[Data->GetSFTPBug(static_cast<TSftpBug>(Index))])+(Index<SFTP_BUG_COUNT-1 ? L"," : L"");
      }
      ADF(L"SFTP Bugs: %s", Bugs.c_str());
      ADF(L"Return code variable: %s; Lookup user groups: %c",
         Data->GetDetectReturnVar() ? UnicodeString(L"Autodetect").c_str() : Data->GetReturnVar().c_str(),
         BugFlags[Data->GetLookupUserGroups()]);
      ADF(L"Shell: %s", Data->GetShell().IsEmpty() ? UnicodeString(L"default").c_str() : Data->GetShell().c_str());
      ADF(L"EOL: %d, UTF: %d", Data->GetEOLType(), Data->GetNotUtf());
      ADF(L"Clear aliases: %s, Unset nat.vars: %s, Resolve symlinks: %s",
         BooleanToEngStr(Data->GetClearAliases()).c_str(), BooleanToEngStr(Data->GetUnsetNationalVars()).c_str(),
         BooleanToEngStr(Data->GetResolveSymlinks()).c_str());
      ADF(L"LS: %s, Ign LS warn: %s, Scp1 Comp: %s",
         Data->GetListingCommand().c_str(),
         BooleanToEngStr(Data->GetIgnoreLsWarnings()).c_str(),
         BooleanToEngStr(Data->GetScp1Compatibility()).c_str());
    }
    if ((Data->GetFSProtocol() == fsFTP) || (Data->GetFSProtocol() == fsFTPS))
    {
      UnicodeString Ftps;
      switch (Data->GetFtps())
      {
        case ftpsImplicit:
          Ftps = L"Implicit SSL/TLS";
          break;

        case ftpsExplicitSsl:
          Ftps = L"Explicit SSL";
          break;

        case ftpsExplicitTls:
          Ftps = L"Explicit TLS";
          break;

        default:
          assert(Data->GetFtps() == ftpsNone);
          Ftps = L"None";
          break;
      }
      ADF(L"FTP: FTPS: %s; Passive: %s [Force IP: %c]",
         Ftps.c_str(), BooleanToEngStr(Data->GetFtpPasvMode()).c_str(),
         BugFlags[Data->GetFtpForcePasvIp()]);
    }
    ADF(L"Local directory: %s, Remote directory: %s, Update: %s, Cache: %s",
      (Data->GetLocalDirectory().IsEmpty() ? UnicodeString(L"default").c_str() : Data->GetLocalDirectory().c_str()),
       (Data->GetRemoteDirectory().IsEmpty() ? UnicodeString(L"home").c_str() : Data->GetRemoteDirectory().c_str()),
       BooleanToEngStr(Data->GetUpdateDirectories()).c_str(),
       BooleanToEngStr(Data->GetCacheDirectories()).c_str());
    ADF(L"Cache directory changes: %s, Permanent: %s",
       BooleanToEngStr(Data->GetCacheDirectoryChanges()).c_str(),
       BooleanToEngStr(Data->GetPreserveDirectoryChanges()).c_str());
    ADF(L"DST mode: %d", static_cast<int>(Data->GetDSTMode()));

    if ((Data->GetFSProtocol() == fsHTTP) || (Data->GetFSProtocol() == fsHTTPS))
    {
      ADF(L"Compression: %s",
        BooleanToEngStr(Data->GetCompression()).c_str());
    }

    AddSeparator();

    #undef ADF
  }
#ifndef _MSC_VER
  __finally
  {
    DeleteUnnecessary();

    EndUpdate();
  }
#endif
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::AddSeparator()
{
  Add(llMessage, L"--------------------------------------------------------------------------");
}
//---------------------------------------------------------------------------
int __fastcall TSessionLog::GetBottomIndex()
{
  return (GetCount() > 0 ? (GetTopIndex() + GetCount() - 1) : -1);
}
//---------------------------------------------------------------------------
bool __fastcall TSessionLog::GetLoggingToFile()
{
  assert((FFile == NULL) || LogToFile());
  return (FFile != NULL);
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::Clear()
{
  TGuard Guard(FCriticalSection);

  FTopIndex += GetCount();
  TStringList::Clear();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/* __fastcall */ TActionLog::TActionLog(TSessionUI* UI, TSessionData * SessionData,
  TConfiguration * Configuration)
{
  FCriticalSection = new TCriticalSection;
  FConfiguration = Configuration;
  FUI = UI;
  FSessionData = SessionData;
  FFile = NULL;
  FCurrentLogFileName = L"";
  FCurrentFileName = L"";
  FLogging = false;
  FClosed = false;
  FPendingActions = new TList();
  FIndent = L"  ";
  FInGroup = false;
  FEnabled = true;
}
//---------------------------------------------------------------------------
/* __fastcall */ TActionLog::~TActionLog()
{
  assert(FPendingActions->GetCount() == 0);
  delete FPendingActions;
  FClosed = true;
  ReflectSettings();
  assert(FFile == NULL);
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::Add(const UnicodeString & Line)
{
  assert(FConfiguration);
  if (FLogging)
  {
    try
    {
      TGuard Guard(FCriticalSection);
      if (FFile == NULL)
      {
        OpenLogFile();
      }

      if (FFile != NULL)
      {
        UTF8String UtfLine = UTF8String(Line);
        fwrite(UtfLine.c_str(), 1, UtfLine.Length(), (FILE *)FFile);
        fwrite("\n", 1, 1, (FILE *)FFile);
      }
    }
    catch (Exception &E)
    {
      // We failed logging, turn it off and notify user.
      FConfiguration->SetLogActions(false);
      try
      {
        throw ExtException(&E, FMTLOAD(LOG_GEN_ERROR));
      }
      catch (Exception &E)
      {
        FUI->HandleExtendedException(&E);
      }
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::AddIndented(const UnicodeString & Line)
{
  Add(FIndent + Line);
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::AddFailure(TStrings * Messages)
{
  AddIndented(L"<failure>");
  AddMessages(L"  ", Messages);
  AddIndented(L"</failure>");
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::AddFailure(Exception * E)
{
  TStrings * Messages = ExceptionToMessages(E);
  if (Messages != NULL)
  {
    // try
    {
#ifdef _MSC_VER
      BOOST_SCOPE_EXIT ( (&Messages) )
      {
        delete Messages;
      } BOOST_SCOPE_EXIT_END
#endif
      AddFailure(Messages);
    }
#ifndef _MSC_VER
    __finally
    {
      delete Messages;
    }
#endif
  }
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::AddMessages(UnicodeString Indent, TStrings * Messages)
{
  for (int Index = 0; Index < Messages->GetCount(); Index++)
  {
    AddIndented(
      FORMAT((Indent + L"<message>%s</message>").c_str(), XmlEscape(Messages->GetStrings(Index)).c_str()));
  }
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::ReflectSettings()
{
  TGuard Guard(FCriticalSection);

  bool ALogging =
    !FClosed && FConfiguration->GetLogActions() && GetEnabled();

  if (ALogging && !FLogging)
  {
    FLogging = true;
    Add(L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    Add(FORMAT(L"<session xmlns=\"http://winscp.net/schema/session/1.0\" name=\"%s\" start=\"%s\">",
      XmlAttributeEscape(FSessionData->GetSessionName()).c_str(), StandardTimestamp().c_str()));
  }
  else if (!ALogging && FLogging)
  {
    if (FInGroup)
    {
      EndGroup();
    }
    Add(L"</session>");
    CloseLogFile();
    FLogging = false;
  }

}
//---------------------------------------------------------------------------
void __fastcall TActionLog::CloseLogFile()
{
  if (FFile != NULL)
  {
    fclose((FILE *)FFile);
    FFile = NULL;
  }
  FCurrentLogFileName = L"";
  FCurrentFileName = L"";
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::OpenLogFile()
{
  try
  {
    assert(FFile == NULL);
    assert(FConfiguration != NULL);
    FCurrentLogFileName = FConfiguration->GetActionsLogFileName();
    FFile = OpenFile(FCurrentLogFileName, FSessionData, false, FCurrentFileName);
  }
  catch (Exception & E)
  {
    // We failed logging to file, turn it off and notify user.
    FCurrentLogFileName = L"";
    FCurrentFileName = L"";
    FConfiguration->SetLogActions(false);
    try
    {
      throw ExtException(&E, FMTLOAD(LOG_GEN_ERROR));
    }
    catch (Exception & E)
    {
      FUI->HandleExtendedException(&E);
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::AddPendingAction(TSessionActionRecord * Action)
{
  FPendingActions->Add(Action);
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::RecordPendingActions()
{
  while ((FPendingActions->GetCount() > 0) &&
         static_cast<TSessionActionRecord *>(FPendingActions->GetItem(0))->Record())
  {
    FPendingActions->Delete(0);
  }
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::BeginGroup(UnicodeString Name)
{
  assert(!FInGroup);
  FInGroup = true;
  assert(FIndent == L"  ");
  AddIndented(FORMAT(L"<group name=\"%s\" start=\"%s\">",
    XmlAttributeEscape(Name).c_str(), StandardTimestamp().c_str()));
  FIndent = L"    ";
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::EndGroup()
{
  assert(FInGroup);
  FInGroup = false;
  assert(FIndent == L"    ");
  FIndent = L"  ";
  AddIndented(L"</group>");
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::SetEnabled(bool value)
{
  if (GetEnabled() != value)
  {
    FEnabled = value;
    ReflectSettings();
  }
}
