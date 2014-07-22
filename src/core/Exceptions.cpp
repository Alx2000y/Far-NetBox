//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Common.h"
#include "Exceptions.h"
#include "TextsCore.h"
#include "HelpCore.h"
#include "Configuration.h"
#include "CoreMain.h"
//---------------------------------------------------------------------------
static bool WellKnownException(
  const Exception * E, UnicodeString * AMessage, const wchar_t ** ACounterName, Exception ** AClone, bool Rethrow)
{
  UnicodeString Message;
  const wchar_t * CounterName = nullptr;
  std::unique_ptr<Exception> Clone;

  bool Result = true;

  // EAccessViolation is EExternal
  if (NB_STATIC_DOWNCAST_CONST(EAccessViolation, E) != nullptr)
  {
    if (Rethrow)
    {
      throw EAccessViolation(E->Message);
    }
    Message = MainInstructions(LoadStr(ACCESS_VIOLATION_ERROR3));
    CounterName = L"AccessViolations";
    Clone.reset(new EAccessViolation(E->Message));
  }
  /*
  // EIntError and EMathError are EExternal
  else if ((NB_STATIC_DOWNCAST(EListError, E) != nullptr) ||
           (NB_STATIC_DOWNCAST(EStringListError, E) != nullptr) ||
           (NB_STATIC_DOWNCAST(EIntError, E) != nullptr) ||
           (NB_STATIC_DOWNCAST(EMathError, E) != nullptr) ||
           (NB_STATIC_DOWNCAST(EVariantError, E) != nullptr) ||
           (NB_STATIC_DOWNCAST(EInvalidOperation, E) != nullptr))
  {
    if (Rethrow)
    {
      throw EIntError(E->Message);
    }
    Message = E->Message;
    CounterName = L"InternalExceptions";
    Clone.reset(new EIntError(E->Message));
  }
  else if (NB_STATIC_DOWNCAST(EExternal, E) != nullptr)
  {
    if (Rethrow)
    {
      throw EExternal(E->Message);
    }
    Message = MainInstructions(E->Message);
    CounterName = L"ExternalExceptions";
    Clone.reset(new EExternal(E->Message));
  }
  else if (NB_STATIC_DOWNCAST(EHeapException, E) != nullptr)
  {
    if (Rethrow)
    {
      throw EHeapException(E->Message);
    }
    Message = MainInstructions(E->Message);
    CounterName = L"HeapExceptions";
    Clone.reset(new EHeapException(E->Message));
  }
  */
  else
  {
    Result = false;
  }

  if (Result)
  {
    if (AMessage != nullptr)
    {
      (*AMessage) = Message;
    }
    if (ACounterName != nullptr)
    {
      (*ACounterName) = CounterName;
    }
    if (AClone != nullptr)
    {
      (*AClone) = NOT_NULL(Clone.release());
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
static bool ExceptionMessage(const Exception * E, bool Count,
  bool Formatted, UnicodeString & Message, bool & InternalError)
{
  bool Result = true;
  const wchar_t * CounterName = nullptr;
  InternalError = false; // see also IsInternalException

  // this list has to be in sync with CloneException
  if (NB_STATIC_DOWNCAST_CONST(EAbort, E) != nullptr)
  {
    Result = false;
  }
  else if (WellKnownException(E, &Message, &CounterName, nullptr, false))
  {
    InternalError = true;
  }
  else if (E && E->Message.IsEmpty())
  {
    Result = false;
  }
  else if (E)
  {
    Message = E->Message;
  }

  if (!Formatted)
  {
    Message = UnformatMessage(Message);
  }

  if (InternalError)
  {
    Message = FMTLOAD(REPORT_ERROR, Message.c_str());
  }
/*
  if (Count && (CounterName != nullptr) && (Configuration->Usage != nullptr))
  {
    Configuration->Usage->Inc(CounterName);
  }
*/
  return Result;
}
//---------------------------------------------------------------------------
bool IsInternalException(Exception * E)
{
  // see also InternalError in ExceptionMessage
  return WellKnownException(E, NULL, NULL, NULL, false);
}
//---------------------------------------------------------------------------
bool ExceptionMessage(const Exception * E, UnicodeString & Message)
{
  bool InternalError;
  return ExceptionMessage(E, true, false, Message, InternalError);
}
//---------------------------------------------------------------------------
bool ExceptionMessageFormatted(const Exception * E, UnicodeString & Message)
{
  bool InternalError;
  return ExceptionMessage(E, true, true, Message, InternalError);
}
//---------------------------------------------------------------------------
bool ShouldDisplayException(Exception * E)
{
  UnicodeString Message;
  return ExceptionMessageFormatted(E, Message);
}
//---------------------------------------------------------------------------
TStrings * ExceptionToMoreMessages(Exception * E)
{
  TStrings * Result = nullptr;
  UnicodeString Message;
  if (ExceptionMessage(E, Message))
  {
    Result = new TStringList();
    Result->Add(Message);
    ExtException * ExtE = NB_STATIC_DOWNCAST(ExtException, E);
    if ((ExtE != nullptr) && (ExtE->GetMoreMessages() != nullptr))
    {
      Result->AddStrings(ExtE->GetMoreMessages());
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString GetExceptionHelpKeyword(Exception * E)
{
  UnicodeString HelpKeyword;
  ExtException * ExtE = NB_STATIC_DOWNCAST(ExtException, E);
  UnicodeString Message; // not used
  bool InternalError = false;
  if (ExtE != nullptr)
  {
    HelpKeyword = ExtE->GetHelpKeyword();
  }
  else if ((E != nullptr) && ExceptionMessage(E, false, false, Message, InternalError) &&
           InternalError)
  {
    HelpKeyword = HELP_INTERNAL_ERROR;
  }
  return HelpKeyword;
}
//---------------------------------------------------------------------------
UnicodeString MergeHelpKeyword(const UnicodeString & PrimaryHelpKeyword, const UnicodeString & SecondaryHelpKeyword)
{
  if (!PrimaryHelpKeyword.IsEmpty() &&
      !IsInternalErrorHelpKeyword(SecondaryHelpKeyword))
  {
    // we have to yet decide what we have both
    // PrimaryHelpKeyword and SecondaryHelpKeyword
    return PrimaryHelpKeyword;
  }
  else
  {
    return SecondaryHelpKeyword;
  }
}
//---------------------------------------------------------------------------
bool IsInternalErrorHelpKeyword(const UnicodeString & HelpKeyword)
{
  return
    (HelpKeyword == HELP_INTERNAL_ERROR);
}
//---------------------------------------------------------------------------
ExtException::ExtException(Exception * E) :
  Exception(L""),
  FMoreMessages(nullptr)
{
  AddMoreMessages(E);
  FHelpKeyword = GetExceptionHelpKeyword(E);
}
//---------------------------------------------------------------------------
ExtException::ExtException(Exception* E, const UnicodeString & Msg, const UnicodeString & HelpKeyword):
  Exception(Msg),
  FMoreMessages(nullptr)
{
  AddMoreMessages(E);
  FHelpKeyword = MergeHelpKeyword(HelpKeyword, GetExceptionHelpKeyword(E));
}
/*ExtException::ExtException(ExtException * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword):
  Exception(Msg),
  FMoreMessages(nullptr),
  FHelpKeyword()
{
  AddMoreMessages(E);
}*/
//---------------------------------------------------------------------------
ExtException::ExtException(Exception * E, int Ident) :
  Exception(E, Ident),
  FMoreMessages(nullptr),
  FHelpKeyword()
{
}
//---------------------------------------------------------------------------
ExtException::ExtException(const UnicodeString & Msg, Exception* E, const UnicodeString & HelpKeyword) :
  Exception(L""),
  FMoreMessages(nullptr)
{
  // "copy exception"
  AddMoreMessages(E);
  // and append message to the end to more messages
  if (!Msg.IsEmpty())
  {
    if (Message.IsEmpty())
    {
      Message = Msg;
    }
    else
    {
      if (FMoreMessages == nullptr)
      {
        FMoreMessages = new TStringList();
      }
      FMoreMessages->Append(UnformatMessage(Msg));
    }
  }
  FHelpKeyword = MergeHelpKeyword(GetExceptionHelpKeyword(E), HelpKeyword);
}
//---------------------------------------------------------------------------
ExtException::ExtException(const UnicodeString & Msg, const UnicodeString & MoreMessages,
  const UnicodeString & HelpKeyword) :
  Exception(Msg),
  FMoreMessages(nullptr),
  FHelpKeyword(HelpKeyword)
{
  if (!MoreMessages.IsEmpty())
  {
    FMoreMessages = TextToStringList(MoreMessages);
  }
}
//---------------------------------------------------------------------------
ExtException::ExtException(const UnicodeString & Msg, TStrings * MoreMessages,
  bool Own, const UnicodeString & HelpKeyword) :
  Exception(Msg),
  FMoreMessages(nullptr),
  FHelpKeyword(HelpKeyword)
{
  if (Own)
  {
    FMoreMessages = MoreMessages;
  }
  else
  {
    FMoreMessages = new TStringList();
    FMoreMessages->Assign(MoreMessages);
  }
}
//---------------------------------------------------------------------------
void ExtException::AddMoreMessages(const Exception * E)
{
  if (E != nullptr)
  {
    if (FMoreMessages == nullptr)
    {
      FMoreMessages = new TStringList();
    }

    const ExtException * ExtE = NB_STATIC_DOWNCAST_CONST(ExtException, E);
    if (ExtE != nullptr)
    {
      if (ExtE->GetMoreMessages() != nullptr)
      {
        FMoreMessages->Assign(ExtE->GetMoreMessages());
      }
    }

    UnicodeString Msg;
    ExceptionMessageFormatted(E, Msg);

    // new exception does not have own message, this is in fact duplication of
    // the exception data, but the exception class may being changed
    if (Message.IsEmpty())
    {
      Message = Msg;
    }
    else if (!Msg.IsEmpty())
    {
      FMoreMessages->Insert(0, UnformatMessage(Msg));
    }

    if (FMoreMessages->GetCount() == 0)
    {
      SAFE_DESTROY(FMoreMessages);
    }
  }
}
//---------------------------------------------------------------------------
ExtException::~ExtException() noexcept
{
  SAFE_DESTROY(FMoreMessages);
  FMoreMessages = nullptr;
}
//---------------------------------------------------------------------------
ExtException * ExtException::Clone()
{
  return new ExtException(this, L"");
}
//---------------------------------------------------------------------------
UnicodeString SysErrorMessageForError(int LastError)
{
  UnicodeString Result;
  if (LastError != 0)
  {
    Result = FORMAT(L"System Error.  Code: %d.\r\n%s", LastError, SysErrorMessage(LastError).c_str());
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString LastSysErrorMessage()
{
  return SysErrorMessageForError(GetLastError());
}
//---------------------------------------------------------------------------
EOSExtException::EOSExtException(const UnicodeString & Msg) :
  ExtException(Msg, LastSysErrorMessage())
{
}
//---------------------------------------------------------------------------
EOSExtException::EOSExtException(const UnicodeString & Msg, int LastError) :
  ExtException(Msg, SysErrorMessageForError(LastError))
{
}
//---------------------------------------------------------------------------
EFatal::EFatal(Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword) :
  ExtException(Msg, E, HelpKeyword),
  FReopenQueried(false)
{
  EFatal * F = NB_STATIC_DOWNCAST(EFatal, E);
  if (F != nullptr)
  {
    FReopenQueried = F->GetReopenQueried();
  }
}
//---------------------------------------------------------------------------
ExtException * EFatal::Clone()
{
  return new EFatal(this, L"");
}
//---------------------------------------------------------------------------
ExtException * ESshTerminate::Clone()
{
  return new ESshTerminate(this, L"", Operation);
}
//---------------------------------------------------------------------------
ECallbackGuardAbort::ECallbackGuardAbort() : EAbort(L"callback abort")
{
}
//---------------------------------------------------------------------------
Exception * CloneException(Exception * E)
{
  Exception * Result;
  // this list has to be in sync with ExceptionMessage
  ExtException * Ext = NB_STATIC_DOWNCAST(ExtException, E);
  if (Ext != nullptr)
  {
    Result = Ext->Clone();
  }
  else if (NB_STATIC_DOWNCAST(ECallbackGuardAbort, E) != nullptr)
  {
    Result = new ECallbackGuardAbort();
  }
  else if (NB_STATIC_DOWNCAST(EAbort, E) != nullptr)
  {
    Result = new EAbort(E->Message);
  }
  else if (WellKnownException(E, nullptr, nullptr, &Result, false))
  {
    // noop
  }
  else
  {
    Result = new Exception(E->Message);
  }
  return Result;
}
//---------------------------------------------------------------------------
void RethrowException(Exception * E)
{
  // this list has to be in sync with ExceptionMessage
  if (NB_STATIC_DOWNCAST(EFatal, E) != nullptr)
  {
    throw EFatal(E, L"");
  }
  else if (NB_STATIC_DOWNCAST(ECallbackGuardAbort, E) != nullptr)
  {
    throw ECallbackGuardAbort();
  }
  else if (NB_STATIC_DOWNCAST(EAbort, E) != nullptr)
  {
    throw EAbort(E->Message);
  }
  else if (WellKnownException(E, nullptr, nullptr, nullptr, true))
  {
    // noop, should never get here
  }
  else
  {
    throw ExtException(E, L"");
  }
}
//------------------------------------------------------------------------------
NB_IMPLEMENT_CLASS(ExtException, NB_GET_CLASS_INFO(Exception), nullptr);
NB_IMPLEMENT_CLASS(EFatal, NB_GET_CLASS_INFO(ExtException), nullptr);
NB_IMPLEMENT_CLASS(ESshFatal, NB_GET_CLASS_INFO(EFatal), nullptr);
NB_IMPLEMENT_CLASS(EOSExtException, NB_GET_CLASS_INFO(ExtException), nullptr);
NB_IMPLEMENT_CLASS(ESshTerminate, NB_GET_CLASS_INFO(EFatal), nullptr);
NB_IMPLEMENT_CLASS(ECallbackGuardAbort, NB_GET_CLASS_INFO(EAbort), nullptr);
NB_IMPLEMENT_CLASS(ESsh, NB_GET_CLASS_INFO(ExtException), nullptr);
NB_IMPLEMENT_CLASS(ETerminal, NB_GET_CLASS_INFO(ExtException), nullptr);
NB_IMPLEMENT_CLASS(ECommand, NB_GET_CLASS_INFO(ExtException), nullptr);
NB_IMPLEMENT_CLASS(EScp, NB_GET_CLASS_INFO(ExtException), nullptr);
NB_IMPLEMENT_CLASS(ESkipFile, NB_GET_CLASS_INFO(ExtException), nullptr);
NB_IMPLEMENT_CLASS(EFileSkipped, NB_GET_CLASS_INFO(ESkipFile), nullptr);
//------------------------------------------------------------------------------
