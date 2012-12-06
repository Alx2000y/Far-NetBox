//---------------------------------------------------------------------------
#include <vector>

#include <Classes.hpp>
#include <Common.h>
#include "XmlStorage.h"
#include "TextsCore.h"
#include "FarUtil.h"
//---------------------------------------------------------------------------
static const char * CONST_XML_VERSION21 = "2.1";
static const char * CONST_ROOT_NODE = "NetBox";
static const char * CONST_SESSION_NODE = "Session";
static const char * CONST_VERSION_ATTR = "version";
static const char * CONST_NAME_ATTR = "name";
//---------------------------------------------------------------------------
TXmlStorage::TXmlStorage(const UnicodeString AStorage,
                         const UnicodeString StoredSessionsSubKey) :
  THierarchicalStorage(ExcludeTrailingBackslash(AStorage)),
  FXmlDoc(NULL),
  FCurrentElement(NULL),
  FStoredSessionsSubKey(StoredSessionsSubKey),
  FFailed(0),
  FStoredSessionsOpened(false)
{
}
//---------------------------------------------------------------------------
void TXmlStorage::Init()
{
  THierarchicalStorage::Init();
  FXmlDoc = new TiXmlDocument();
}
//---------------------------------------------------------------------------
TXmlStorage::~TXmlStorage()
{
  if (GetAccessMode() == smReadWrite)
  {
    WriteXml();
  }
  delete FXmlDoc;
}
//---------------------------------------------------------------------------
bool TXmlStorage::LoadXml()
{
  CNBFile xmlFile;
  if (!xmlFile.OpenRead(GetStorage().c_str()))
  {
    return false;
  }
  size_t buffSize = static_cast<size_t>(xmlFile.GetFileSize() + 1);
  if (buffSize > 1000000)
  {
    return false;
  }
  std::string buff(buffSize, 0);
  if (!xmlFile.Read(&buff[0], buffSize))
  {
    return false;
  }

  FXmlDoc->Parse(buff.c_str());
  if (FXmlDoc->Error())
  {
    return false;
  }

  // Get and check root node
  TiXmlElement * xmlRoot = FXmlDoc->RootElement();
  if (!xmlRoot) return false;
  const char * Value = xmlRoot->Value();
  if (!Value) return false;
  if (strcmp(Value, CONST_ROOT_NODE) != 0) return false;
  const char * attr = xmlRoot->Attribute(CONST_VERSION_ATTR);
  if (!attr) return false;
  uintptr_t Version = StrToVersionNumber(UnicodeString(attr));
  if (Version < MAKEVERSIONNUMBER(2,0,0)) return false;
  TiXmlElement * Element = xmlRoot->FirstChildElement(ToStdString(FStoredSessionsSubKey).c_str());
  if (Element != NULL)
  {
    FCurrentElement = FXmlDoc->RootElement();
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------
bool TXmlStorage::WriteXml()
{
  TiXmlPrinter xmlPrinter;
  xmlPrinter.SetIndent("  ");
  xmlPrinter.SetLineBreak("\r\n");
  FXmlDoc->Accept(&xmlPrinter);
  const char * xmlContent = xmlPrinter.CStr();
  if (!xmlContent || !*xmlContent)
  {
    return false;
  }

  return (CNBFile::SaveFile(GetStorage().c_str(), xmlContent) == ERROR_SUCCESS);
}
//---------------------------------------------------------------------------
bool TXmlStorage::Copy(TXmlStorage * Storage)
{
  Classes::Error(SNotImplemented, 3020);
  bool Result = false;
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TXmlStorage::GetSource()
{
  return GetStorage();
}
//---------------------------------------------------------------------------
void TXmlStorage::SetAccessMode(TStorageAccessMode Value)
{
  THierarchicalStorage::SetAccessMode(Value);
  switch (GetAccessMode())
  {
    case smRead:
      LoadXml();
      break;

    case smReadWrite:
    default:
      FXmlDoc->LinkEndChild(new TiXmlDeclaration("1.0", "UTF-8", ""));
      assert(FCurrentElement == NULL);
      FCurrentElement = new TiXmlElement(CONST_ROOT_NODE);
      FCurrentElement->SetAttribute(CONST_VERSION_ATTR, CONST_XML_VERSION21);
      FXmlDoc->LinkEndChild(FCurrentElement);
      break;
  }
}
//---------------------------------------------------------------------------
bool __fastcall TXmlStorage::DoKeyExists(const UnicodeString SubKey, bool ForceAnsi)
{
  Classes::Error(SNotImplemented, 3024);
  UnicodeString K = PuttyMungeStr(SubKey);
  bool Result = false; // FRegistry->KeyExists(K);
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TXmlStorage::DoOpenSubKey(const UnicodeString MungedSubKey, bool CanCreate)
{
  TiXmlElement * OldCurrentElement = FCurrentElement;
  TiXmlElement * Element = NULL;
  std::string subKey = ToStdString(MungedSubKey);
  if (CanCreate)
  {
    if (FStoredSessionsOpened)
    {
      Element = new TiXmlElement(CONST_SESSION_NODE);
      Element->SetAttribute(CONST_NAME_ATTR, subKey);
    }
    else
    {
      Element = new TiXmlElement(subKey);
    }
    FCurrentElement->LinkEndChild(Element);
  }
  else
  {
    Element = FindChildElement(subKey);
  }
  bool Result = Element != NULL;
  if (Result)
  {
    FSubElements.push_back(OldCurrentElement);
    FCurrentElement = Element;
    FStoredSessionsOpened = (MungedSubKey == FStoredSessionsSubKey);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TXmlStorage::CloseSubKey()
{
  THierarchicalStorage::CloseSubKey();
  if (FKeyHistory->Count && FSubElements.size())
  {
    FCurrentElement = FSubElements.back();
    FSubElements.pop_back();
  }
  else
  {
    FCurrentElement = NULL;
  }
}
//---------------------------------------------------------------------------
bool TXmlStorage::DeleteSubKey(const UnicodeString SubKey)
{
  bool Result = false;
  TiXmlElement * Element = FindElement(SubKey);
  if (Element != NULL)
  {
    FCurrentElement->RemoveChild(Element);
    Result = true;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TXmlStorage::GetSubKeyNames(TStrings * Strings)
{
  for (TiXmlElement * Element = FCurrentElement->FirstChildElement();
       Element != NULL; Element = Element->NextSiblingElement())
  {
    UnicodeString val = GetValue(Element);
    Strings->Add(PuttyUnMungeStr(val));
  }
}
//---------------------------------------------------------------------------
void TXmlStorage::GetValueNames(TStrings * Strings)
{
  Classes::Error(SNotImplemented, 3022);
  // FRegistry->GetValueNames(Strings);
}
//---------------------------------------------------------------------------
bool TXmlStorage::DeleteValue(const UnicodeString Name)
{
  bool Result = false;
  TiXmlElement * Element = FindElement(Name);
  if (Element != NULL)
  {
    FCurrentElement->RemoveChild(Element);
    Result = true;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TXmlStorage::RemoveIfExists(const UnicodeString Name)
{
  TiXmlElement * Element = FindElement(Name);
  if (Element != NULL)
  {
    FCurrentElement->RemoveChild(Element);
  }
}
//---------------------------------------------------------------------------
void TXmlStorage::AddNewElement(const UnicodeString Name, const UnicodeString Value)
{
  std::string name = ToStdString(Name);
  std::string StrValue = ToStdString(Value);
  TiXmlElement * Element = new TiXmlElement(name);
  Element->LinkEndChild(new TiXmlText(StrValue.c_str()));
  FCurrentElement->LinkEndChild(Element);
}
//---------------------------------------------------------------------------
UnicodeString TXmlStorage::GetSubKeyText(const UnicodeString Name)
{
  TiXmlElement * Element = FindElement(Name);
  if (!Element)
  {
    return UnicodeString();
  }
  if (ToUnicodeString(CONST_SESSION_NODE) == Name)
  {
    return ToUnicodeString(std::string(Element->Attribute(CONST_NAME_ATTR)));
  }
  else
  {
    return ToUnicodeString(Element->GetText() ? std::string(Element->GetText()) : std::string());
  }
}
//---------------------------------------------------------------------------
TiXmlElement * TXmlStorage::FindElement(const UnicodeString Name)
{
  for (const TiXmlElement * Element = FCurrentElement->FirstChildElement();
       Element != NULL; Element = Element->NextSiblingElement())
  {
    UnicodeString name = ToUnicodeString(Element->ValueStr());
    if (name == Name)
    {
      return const_cast<TiXmlElement *>(Element);
    }
  }
  return NULL;
}
//---------------------------------------------------------------------------
TiXmlElement * TXmlStorage::FindChildElement(const std::string & subKey)
{
  TiXmlElement * Result = NULL;
  // assert(FCurrentElement);
  if (FStoredSessionsOpened)
  {
    TiXmlElement * Element = FCurrentElement->FirstChildElement(CONST_SESSION_NODE);
    if (Element && !strcmp(Element->Attribute(CONST_NAME_ATTR), subKey.c_str()))
    {
      Result = Element;
    }
  }
  else if (FCurrentElement)
  {
    Result = FCurrentElement->FirstChildElement(subKey.c_str());
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TXmlStorage::GetValue(TiXmlElement * Element)
{
  assert(Element);
  UnicodeString Result;
  if (FStoredSessionsOpened && Element->Attribute(CONST_NAME_ATTR))
  {
    Result = ToUnicodeString(Element->Attribute(CONST_NAME_ATTR));
  }
  else
  {
    Result = ToUnicodeString(Element->ValueStr());
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TXmlStorage::ValueExists(const UnicodeString Value)
{
  bool Result = false;
  TiXmlElement * Element = FindElement(Value);
  if (Element != NULL)
  {
    Result = true;
  }
  return Result;
}
//---------------------------------------------------------------------------
size_t TXmlStorage::BinaryDataSize(const UnicodeString Name)
{
  Classes::Error(SNotImplemented, 3026);
  size_t Result = 0; // FRegistry->GetDataSize(Name);
  return Result;
}
//---------------------------------------------------------------------------
bool TXmlStorage::ReadBool(const UnicodeString Name, bool Default)
{
  UnicodeString res = ReadString(Name, L"");
  if (res.IsEmpty())
  {
    return Default;
  }
  else
  {
    return AnsiCompareIC(res, ::BooleanToEngStr(true)) == 0;
  }
}
//---------------------------------------------------------------------------
TDateTime TXmlStorage::ReadDateTime(const UnicodeString Name, TDateTime Default)
{
  double res = ReadFloat(Name, Default.operator double());
  return TDateTime(res);
}
//---------------------------------------------------------------------------
double TXmlStorage::ReadFloat(const UnicodeString Name, double Default)
{
  return StrToFloatDef(GetSubKeyText(Name), Default);
}
//---------------------------------------------------------------------------
int TXmlStorage::ReadInteger(const UnicodeString Name, int Default)
{
  return StrToIntDef(GetSubKeyText(Name), Default);
}
//---------------------------------------------------------------------------
__int64 TXmlStorage::ReadInt64(const UnicodeString Name, __int64 Default)
{
  return StrToInt64Def(GetSubKeyText(Name), Default);
}
//---------------------------------------------------------------------------
UnicodeString TXmlStorage::ReadStringRaw(const UnicodeString Name, const UnicodeString Default)
{
  UnicodeString Result = GetSubKeyText(Name);
  return Result.IsEmpty() ? Default : Result;
}
//---------------------------------------------------------------------------
size_t TXmlStorage::ReadBinaryData(const UnicodeString Name,
  void * Buffer, size_t Size)
{
  Classes::Error(SNotImplemented, 3028);
  size_t Result = 0;
  return Result;
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteBool(const UnicodeString Name, bool Value)
{
  WriteString(Name, ::BooleanToEngStr(Value));
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteDateTime(const UnicodeString Name, TDateTime Value)
{
  WriteFloat(Name, Value);
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteFloat(const UnicodeString Name, double Value)
{
  RemoveIfExists(Name);
  AddNewElement(Name, FORMAT(L"%.5f", Value));
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteStringRaw(const UnicodeString Name, const UnicodeString Value)
{
  RemoveIfExists(Name);
  AddNewElement(Name, Value);
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteInteger(const UnicodeString Name, int Value)
{
  RemoveIfExists(Name);
  AddNewElement(Name, ::IntToStr(Value));
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteInt64(const UnicodeString Name, __int64 Value)
{
  RemoveIfExists(Name);
  AddNewElement(Name, ::Int64ToStr(Value));
}
//---------------------------------------------------------------------------
void TXmlStorage::WriteBinaryData(const UnicodeString Name,
  const void * Buffer, size_t Size)
{
  RemoveIfExists(Name);
  AddNewElement(Name, ::StrToHex(UnicodeString(reinterpret_cast<const wchar_t *>(Buffer), Size), true));
}
//---------------------------------------------------------------------------
int TXmlStorage::GetFailed()
{
  int Result = FFailed;
  FFailed = 0;
  return Result;
}
