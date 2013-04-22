//---------------------------------------------------------------------------
#include "stdafx.h"
#include <Options.h>
#include <FileZillaIntern.h>
#include <FileZillaIntf.h>
#include <Crypt.h>
//---------------------------------------------------------------------------
CString COptions::GetInstanceOption(CApiLog * Instance, int OptionID)
{
  ASSERT(Instance);
  ASSERT(dynamic_cast<TFileZillaIntern *>(Instance) != NULL);

  TFileZillaIntern * Intern = static_cast<TFileZillaIntern *>(Instance);

  const TFileZillaIntf * Intf = Intern->GetOwner();
  ASSERT(Intf != NULL);
  CString Result = Intf->Option(OptionID);
  switch (OptionID)
  {
    case OPTION_PROXYPASS:
    case OPTION_FWPASS:
      Result = CCrypt::encrypt(Result);
      break;
  }
  return Result;
}
//---------------------------------------------------------------------------
int COptions::GetInstanceOptionVal(CApiLog * Instance, int OptionID)
{
  ASSERT(Instance);
  ASSERT(dynamic_cast<TFileZillaIntern *>(Instance) != NULL);

  TFileZillaIntern * Intern = static_cast<TFileZillaIntern *>(Instance);

  const TFileZillaIntf * Intf = Intern->GetOwner();
  ASSERT(Intf != NULL);
  return (int)Intf->OptionVal(OptionID);
}
