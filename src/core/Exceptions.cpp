//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "Common.h"
#include "Exceptions.h"
#include "TextsCore.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
bool __fastcall ExceptionMessage(const Exception * E, UnicodeString & Message)
{
  bool Result = true;
  if (dynamic_cast<const EAbort *>(E) != NULL)
  {
    Result = false;
  }
  else if (dynamic_cast<const EAccessViolation*>(E) != NULL)
  {
    Message = LoadStr(ACCESS_VIOLATION_ERROR);
  }
  else if (E->GetMessage().IsEmpty())
  {
    Result = false;
  }
  else
  {
    Message = E->GetMessage();
  }
  return Result;
}
//---------------------------------------------------------------------------
TStrings * ExceptionToMoreMessages(Exception * E)
{
  TStrings * Result = NULL;
  UnicodeString Message;
  if (ExceptionMessage(E, Message))
  {
    Result = new TStringList();
    Result->Add(Message);
    ExtException * ExtE = dynamic_cast<ExtException *>(E);
    if (ExtE != NULL)
    {
      Result->AddStrings(ExtE->GetMoreMessages());
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
/* __fastcall */ ExtException::ExtException(Exception * E) :
  Exception(L""),
  FMoreMessages(NULL),
  FHelpKeyword()
{
  AddMoreMessages(E);
}
//---------------------------------------------------------------------------
/* __fastcall */ ExtException::ExtException(Exception* E, UnicodeString Msg):
  Exception(Msg),
  FMoreMessages(NULL),
  FHelpKeyword()
{
  AddMoreMessages(E);
}
/* __fastcall */ ExtException::ExtException(ExtException* E, UnicodeString Msg):
  Exception(Msg),
  FMoreMessages(NULL),
  FHelpKeyword()
{
  AddMoreMessages(E);
}
//---------------------------------------------------------------------------
/* __fastcall */ ExtException::ExtException(UnicodeString Msg, Exception* E) :
  Exception(L""),
  FMoreMessages(NULL),
  FHelpKeyword()
{
  // "copy exception"
  AddMoreMessages(E);
  // and append message to the end to more messages
  if (!Msg.IsEmpty())
  {
    if (FMessage.IsEmpty())
    {
      SetMessage(Msg);
    }
    else
    {
      if (FMoreMessages == NULL)
      {
        FMoreMessages = new TStringList();
      }
      FMoreMessages->Append(GetMessage());
    }
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ ExtException::ExtException(UnicodeString Msg, UnicodeString MoreMessages,
    UnicodeString HelpKeyword) :
  Exception(Msg),
  FMoreMessages(NULL),
  FHelpKeyword(HelpKeyword)
{
  if (!MoreMessages.IsEmpty())
  {
    FMoreMessages = new TStringList();
    FMoreMessages->SetText(MoreMessages);
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ ExtException::ExtException(UnicodeString Msg, TStrings* MoreMessages,
  bool Own, UnicodeString HelpKeyword) :
  Exception(Msg),
  FMoreMessages(NULL),
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
void __fastcall ExtException::AddMoreMessages(const Exception * E)
{
  if (E != NULL)
  {
    if (FMoreMessages == NULL)
    {
      FMoreMessages = new TStringList();
    }

    const ExtException * ExtE = dynamic_cast<const ExtException *>(E);
    if (ExtE != NULL)
    {
      if (!ExtE->GetHelpKeyword().IsEmpty())
      {
        // we have to yet decide what to do now
        assert(GetHelpKeyword().IsEmpty());

        FHelpKeyword = ExtE->GetHelpKeyword();
      }

      if (ExtE->GetMoreMessages() != NULL)
      {
        FMoreMessages->Assign(ExtE->GetMoreMessages());
      }
    }

    UnicodeString Msg;
    ExceptionMessage(E, Msg);

    // new exception does not have own message, this is in fact duplication of
    // the exception data, but the exception class may being changed
    if (GetMessage().IsEmpty())
    {
      SetMessage(Msg);
    }
    else if (!Msg.IsEmpty())
    {
      FMoreMessages->Insert(0, Msg);
    }

    if (FMoreMessages->GetCount() == 0)
    {
      delete FMoreMessages;
      FMoreMessages = NULL;
    }
  }
}
//---------------------------------------------------------------------------
/* __fastcall */ ExtException::~ExtException()
{
  delete FMoreMessages;
  FMoreMessages = NULL;
}
//---------------------------------------------------------------------------
ExtException * __fastcall ExtException::Clone()
{
  return new ExtException(this, L"");
}
//---------------------------------------------------------------------------
UnicodeString __fastcall LastSysErrorMessage()
{
  int LastError = GetLastError();
  UnicodeString Result;
  if (LastError != 0)
  {
    Result = FORMAT(L"System Error.  Code: %d.\r\n%s", LastError, SysErrorMessage(LastError).c_str());
  }
  return Result;
}
//---------------------------------------------------------------------------
/* __fastcall */ EOSExtException::EOSExtException(UnicodeString Msg) :
  ExtException(Msg, LastSysErrorMessage())
{
}
//---------------------------------------------------------------------------
/* __fastcall */ EFatal::EFatal(Exception * E, UnicodeString Msg) :
  ExtException(Msg, E),
  FReopenQueried(false)
{
  EFatal * F = dynamic_cast<EFatal *>(E);
  if (F != NULL)
  {
    FReopenQueried = F->FReopenQueried;
  }
}
//---------------------------------------------------------------------------
ExtException * __fastcall EFatal::Clone()
{
  return new EFatal(this, L"");
}
//---------------------------------------------------------------------------
ExtException * __fastcall ESshTerminate::Clone()
{
  return new ESshTerminate(this, L"", Operation);
}
//---------------------------------------------------------------------------
/* __fastcall */ ECallbackGuardAbort::ECallbackGuardAbort() : EAbort(L"callback abort")
{
}
//---------------------------------------------------------------------------
Exception * __fastcall CloneException(Exception * E)
{
  ExtException * Ext = dynamic_cast<ExtException *>(E);
  if (Ext != NULL)
  {
    return Ext->Clone();
  }
  else if (dynamic_cast<ECallbackGuardAbort *>(E) != NULL)
  {
    return new ECallbackGuardAbort();
  }
  else if (dynamic_cast<EAbort *>(E) != NULL)
  {
    return new EAbort(E->GetMessage());
  }
  else
  {
    return new Exception(E->GetMessage());
  }
}
//---------------------------------------------------------------------------
void __fastcall RethrowException(Exception * E)
{
  if (dynamic_cast<EFatal *>(E) != NULL)
  {
    throw EFatal(E, L"");
  }
  else if (dynamic_cast<ECallbackGuardAbort *>(E) != NULL)
  {
    throw ECallbackGuardAbort();
  }
  else if (dynamic_cast<EAbort *>(E) != NULL)
  {
    throw EAbort(E->GetMessage());
  }
  else
  {
    throw ExtException(E, L"");
  }
}
