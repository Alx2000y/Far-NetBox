//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "RemoteFiles.h"
#include "Common.h"

#include <SysUtils.hpp>
#include <StrUtils.hpp>

#include "Exceptions.h"
#include "Interface.h"
#include "Terminal.h"
#include "TextsCore.h"
#include "HelpCore.h"
/* TODO 1 : Path class instead of UnicodeString (handle relativity...) */
//---------------------------------------------------------------------------
bool IsUnixStyleWindowsPath(const UnicodeString & Path)
{
  return (Path.Length() >= 3) && IsLetter(Path[1]) && (Path[2] == L':') && (Path[3] == L'/');
}
//---------------------------------------------------------------------------
bool UnixIsAbsolutePath(const UnicodeString & Path)
{
  return
    ((Path.Length() >= 1) && (Path[1] == L'/')) ||
    // we need this for FTP only, but this is unfortunately used in a static context
    ::IsUnixStyleWindowsPath(Path);
}
//---------------------------------------------------------------------------
UnicodeString UnixIncludeTrailingBackslash(const UnicodeString & Path)
{
  // it used to return "/" when input path was empty
  if (!Path.IsEmpty() && !Path.IsDelimiter(L"/", Path.Length()))
  {
    return Path + L"/";
  }
  else
  {
    return Path;
  }
}
//---------------------------------------------------------------------------
// Keeps "/" for root path
UnicodeString UnixExcludeTrailingBackslash(const UnicodeString & Path, bool Simple)
{
  if (Path.IsEmpty() ||
      (Path == L"/") ||
      !Path.IsDelimiter(L"/", Path.Length()) ||
      (!Simple && ((Path.Length() == 3) && ::IsUnixStyleWindowsPath(Path))))
  {
    return Path;
  }
  else
  {
    return Path.SubString(1, Path.Length() - 1);
  }
}
//---------------------------------------------------------------------------
UnicodeString SimpleUnixExcludeTrailingBackslash(const UnicodeString & Path)
{
  return ::UnixExcludeTrailingBackslash(Path, true);
}
//---------------------------------------------------------------------------
Boolean UnixSamePath(const UnicodeString & Path1, const UnicodeString & Path2)
{
  return (::UnixIncludeTrailingBackslash(Path1) == ::UnixIncludeTrailingBackslash(Path2));
}
//---------------------------------------------------------------------------
bool UnixIsChildPath(const UnicodeString & Parent, const UnicodeString & Child)
{
  UnicodeString Parent2 = ::UnixIncludeTrailingBackslash(Parent);
  UnicodeString Child2 = ::UnixIncludeTrailingBackslash(Child);
  return (Child2.SubString(1, Parent2.Length()) == Parent2);
}
//---------------------------------------------------------------------------
UnicodeString UnixExtractFileDir(const UnicodeString & Path)
{
  intptr_t Pos = Path.LastDelimiter(L'/');
  // it used to return Path when no slash was found
  if (Pos > 1)
  {
    return Path.SubString(1, Pos - 1);
  }
  else
  {
    return (Pos == 1) ? UnicodeString(L"/") : UnicodeString();
  }
}
//---------------------------------------------------------------------------
// must return trailing backslash
UnicodeString UnixExtractFilePath(const UnicodeString & Path)
{
  intptr_t Pos = Path.LastDelimiter(L'/');
  // it used to return Path when no slash was found
  return (Pos > 0) ? Path.SubString(1, Pos) : UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString UnixExtractFileName(const UnicodeString & Path)
{
  intptr_t Pos = Path.LastDelimiter(L'/');
  UnicodeString Result;
  if (Pos > 0)
  {
    Result = Path.SubString(Pos + 1, Path.Length() - Pos);
  }
  else
  {
    Result = Path;
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString UnixExtractFileExt(const UnicodeString & Path)
{
  UnicodeString FileName = ::UnixExtractFileName(Path);
  intptr_t Pos = FileName.LastDelimiter(L".");
  return (Pos > 0) ? Path.SubString(Pos, Path.Length() - Pos + 1) : UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString ExtractFileName(const UnicodeString & Path, bool Unix)
{
  if (Unix)
  {
    return ::UnixExtractFileName(Path);
  }
  else
  {
    return ::ExtractFilename(Path, L'\\');
  }
}
//---------------------------------------------------------------------------
bool ExtractCommonPath(const TStrings * AFiles, OUT UnicodeString & Path)
{
  assert(AFiles->GetCount() > 0);

  Path = ::ExtractFilePath(AFiles->GetString(0));
  bool Result = !Path.IsEmpty();
  if (Result)
  {
    for (intptr_t Index = 1; Index < AFiles->GetCount(); ++Index)
    {
      while (!Path.IsEmpty() &&
        (AFiles->GetString(Index).SubString(1, Path.Length()) != Path))
      {
        intptr_t PrevLen = Path.Length();
        Path = ::ExtractFilePath(::ExcludeTrailingBackslash(Path));
        if (Path.Length() == PrevLen)
        {
          Path = L"";
          Result = false;
        }
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool UnixExtractCommonPath(const TStrings * const AFiles, OUT UnicodeString & Path)
{
  assert(AFiles->GetCount() > 0);

  Path = ::UnixExtractFilePath(AFiles->GetString(0));
  bool Result = !Path.IsEmpty();
  if (Result)
  {
    for (intptr_t Index = 1; Index < AFiles->GetCount(); ++Index)
    {
      while (!Path.IsEmpty() &&
        (AFiles->GetString(Index).SubString(1, Path.Length()) != Path))
      {
        intptr_t PrevLen = Path.Length();
        Path = ::UnixExtractFilePath(::UnixExcludeTrailingBackslash(Path));
        if (Path.Length() == PrevLen)
        {
          Path = L"";
          Result = false;
        }
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool IsUnixRootPath(const UnicodeString & Path)
{
  return Path.IsEmpty() || (Path == ROOTDIRECTORY);
}
//---------------------------------------------------------------------------
bool IsUnixHiddenFile(const UnicodeString & AFileName)
{
  return (AFileName != ROOTDIRECTORY) && (AFileName != PARENTDIRECTORY) &&
    !AFileName.IsEmpty() && (AFileName[1] == L'.');
}
//---------------------------------------------------------------------------
UnicodeString AbsolutePath(const UnicodeString & Base, const UnicodeString & Path)
{
  // There's a duplicate implementation in TTerminal::ExpandFileName()
  UnicodeString Result;
  if (Path.IsEmpty())
  {
    Result = Base;
  }
  else if (Path[1] == L'/')
  {
    Result = ::UnixExcludeTrailingBackslash(Path);
  }
  else
  {
    Result = ::UnixIncludeTrailingBackslash(
      ::UnixIncludeTrailingBackslash(Base) + Path);
    intptr_t P;
    while ((P = Result.Pos(L"/../")) > 0)
    {
      // special case, "/../" => "/"
      if (P == 1)
      {
        Result = L"/";
      }
      else
      {
        intptr_t P2 = Result.SubString(1, P-1).LastDelimiter(L"/");
        assert(P2 > 0);
        Result.Delete(P2, P - P2 + 3);
      }
    }
    while ((P = Result.Pos(L"/./")) > 0)
    {
      Result.Delete(P, 2);
    }
    Result = ::UnixExcludeTrailingBackslash(Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString FromUnixPath(const UnicodeString & Path)
{
  return ReplaceStr(Path, L"/", L"\\");
}
//---------------------------------------------------------------------------
UnicodeString ToUnixPath(const UnicodeString & Path)
{
  return ReplaceStr(Path, L"\\", L"/");
}
//---------------------------------------------------------------------------
static void CutFirstDirectory(UnicodeString & S, bool Unix)
{
  UnicodeString Sep = Unix ? L"/" : L"\\";
  if (S == Sep)
  {
    S = L"";
  }
  else
  {
    bool Root = false;
    intptr_t P = 0;
    if (S[1] == Sep[1])
    {
      Root = true;
      S.Delete(1, 1);
    }
    else
    {
      Root = false;
    }
    if (S[1] == L'.')
    {
      S.Delete(1, 4);
    }
    P = S.Pos(Sep[1]);
    if (P)
    {
      S.Delete(1, P);
      S = L"..." + Sep + S;
    }
    else
    {
      S = L"";
    }
    if (Root)
    {
      S = Sep + S;
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString MinimizeName(const UnicodeString & AFileName, intptr_t MaxLen, bool Unix)
{
  UnicodeString Drive, Dir, Name, Result;
  UnicodeString Sep = Unix ? L"/" : L"\\";

  Result = AFileName;
  if (Unix)
  {
    intptr_t P = Result.LastDelimiter(L"/");
    if (P)
    {
      Dir = Result.SubString(1, P);
      Name = Result.SubString(P + 1, Result.Length() - P);
    }
    else
    {
      Dir = L"";
      Name = Result;
    }
  }
  else
  {
    Dir = ::ExtractFilePath(Result);
    Name = ::ExtractFileName(Result, false);

    if (Dir.Length() >= 2 && Dir[2] == L':')
    {
      Drive = Dir.SubString(1, 2);
      Dir.Delete(1, 2);
    }
  }

  while ((!Dir.IsEmpty() || !Drive.IsEmpty()) && (Result.Length() > MaxLen))
  {
    if (Dir == Sep + L"..." + Sep)
    {
      Dir = L"..." + Sep;
    }
    else if (Dir == L"")
    {
      Drive = L"";
    }
    else
    {
      CutFirstDirectory(Dir, Unix);
    }
    Result = Drive + Dir + Name;
  }

  if (Result.Length() > MaxLen)
  {
    Result = Result.SubString(1, MaxLen);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString MakeFileList(const TStrings * AFileList)
{
  UnicodeString Result;
  for (intptr_t Index = 0; Index < AFileList->GetCount(); ++Index)
  {
    if (!Result.IsEmpty())
    {
      Result += L" ";
    }

    UnicodeString FileName = AFileList->GetString(Index);
    // currently this is used for local file only, so no delimiting is done
    if (FileName.Pos(L" ") > 0)
    {
      Result += L"\"" + FileName + L"\"";
    }
    else
    {
      Result += FileName;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
// copy from BaseUtils.pas
TDateTime ReduceDateTimePrecision(const TDateTime & DateTime,
  TModificationFmt Precision)
{
  TDateTime Result = DateTime;
  if (Precision == mfNone)
  {
    Result = double(0.0);
  }
  else if (Precision != mfFull)
  {
    uint16_t Y, M, D, H, N, S, MS;

    DecodeDate(Result, Y, M, D);
    DecodeTime(Result, H, N, S, MS);
    switch (Precision)
    {
      case mfMDHM:
        S = 0;
        MS = 0;
        break;

      case mfMDY:
        H = 0;
        N = 0;
        S = 0;
        MS = 0;
        break;

      default:
        FAIL;
    }

    Result = EncodeDateVerbose(Y, M, D) + EncodeTimeVerbose(H, N, S, MS);
  }
  return Result;
}
//---------------------------------------------------------------------------
TModificationFmt LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2)
{
  return (Precision1 < Precision2) ? Precision1 : Precision2;
}
//---------------------------------------------------------------------------
UnicodeString UserModificationStr(const TDateTime & DateTime,
  TModificationFmt Precision)
{
  Word Year, Month, Day, Hour, Min, Sec, MSec;
  DateTime.DecodeDate(Year, Month, Day);
  DateTime.DecodeTime(Hour, Min, Sec, MSec);
  switch (Precision)
  {
    case mfNone:
      return L"";
    case mfMDY:
      return FORMAT(L"%3s %2d %2d", EngShortMonthNames[Month-1], Day, Year);
    case mfMDHM:
      return FORMAT(L"%3s %2d %2d:%2.2d",
        EngShortMonthNames[Month-1], Day, Hour, Min);
    case mfFull:
      return FORMAT(L"%3s %2d %2d:%2.2d:%2.2d %4d",
        EngShortMonthNames[Month-1], Day, Hour, Min, Sec, Year);
    default:
      assert(false);
  }
  return UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString ModificationStr(const TDateTime & DateTime,
  TModificationFmt Precision)
{
  uint16_t Year, Month, Day, Hour, Min, Sec, MSec;
  DateTime.DecodeDate(Year, Month, Day);
  DateTime.DecodeTime(Hour, Min, Sec, MSec);
  switch (Precision)
  {
    case mfNone:
      return L"";

    case mfMDY:
      return FORMAT(L"%3s %2d %2d", EngShortMonthNames[Month-1], Day, Year);

    case mfMDHM:
      return FORMAT(L"%3s %2d %2d:%2.2d",
        EngShortMonthNames[Month-1], Day, Hour, Min);

    default:
      FAIL;
      // fall thru

    case mfFull:
      return FORMAT(L"%3s %2d %2d:%2.2d:%2.2d %4d",
        EngShortMonthNames[Month-1], Day, Hour, Min, Sec, Year);
  }
}
//---------------------------------------------------------------------------
int FakeFileImageIndex(const UnicodeString & /* AFileName */, uint32_t /* Attrs */,
  UnicodeString * /* TypeName */)
{
  /*Attrs |= FILE_ATTRIBUTE_NORMAL;

  TSHFileInfoW SHFileInfo = {0};
  // On Win2k we get icon of "ZIP drive" for ".." (parent directory)
  if ((FileName == L"..") ||
      ((FileName.Length() == 2) && (FileName[2] == L':') && IsLetter(FileName[1])) ||
      IsReservedName(FileName))
  {
    FileName = L"dumb";
  }
  // this should be somewhere else, probably in TUnixDirView,
  // as the "partial" overlay is added there too
  if (AnsiSameText(UnixExtractFileExt(FileName), PARTIAL_EXT))
  {
    static const size_t PartialExtLen = LENOF(PARTIAL_EXT) - 1;
    FileName.SetLength(FileName.Length() - PartialExtLen);
  }

  int Icon;
  if (SHGetFileInfo(UnicodeString(FileName).c_str(),
        Attrs, &SHFileInfo, sizeof(SHFileInfo),
        SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME) != 0)
  {

    if (TypeName != nullptr)
    {
      *TypeName = SHFileInfo.szTypeName;
    }
    Icon = SHFileInfo.iIcon;
  }
  else
  {
    if (TypeName != nullptr)
    {
      *TypeName = L"";
    }
    Icon = -1;
  }


  return Icon;*/
  return -1;
}
//---------------------------------------------------------------------------
TRemoteToken::TRemoteToken() :
  FID(0),
  FIDValid(false)
{
}
//---------------------------------------------------------------------------
TRemoteToken::TRemoteToken(const UnicodeString & Name) :
  FName(Name),
  FID(0),
  FIDValid(false)
{
}
//---------------------------------------------------------------------------
TRemoteToken::TRemoteToken(const TRemoteToken & rhp) :
  FName(rhp.FName),
  FID(rhp.FID),
  FIDValid(rhp.FIDValid)
{
}
//---------------------------------------------------------------------------
void TRemoteToken::Clear()
{
  FID = 0;
  FIDValid = false;
}
//---------------------------------------------------------------------------
bool TRemoteToken::operator ==(const TRemoteToken & rht) const
{
  return
    (FName == rht.FName) &&
    (FIDValid == rht.FIDValid) &&
    (!FIDValid || (FID == rht.FID));
}
//---------------------------------------------------------------------------
bool TRemoteToken::operator !=(const TRemoteToken & rht) const
{
  return !(*this == rht);
}
//---------------------------------------------------------------------------
TRemoteToken & TRemoteToken::operator =(const TRemoteToken & rht)
{
  if (this != &rht)
  {
    FName = rht.FName;
    FIDValid = rht.FIDValid;
    FID = rht.FID;
  }
  return *this;
}
//---------------------------------------------------------------------------
intptr_t TRemoteToken::Compare(const TRemoteToken & rht) const
{
  intptr_t Result;
  if (!FName.IsEmpty())
  {
    if (!rht.FName.IsEmpty())
    {
      Result = AnsiCompareText(FName, rht.FName);
    }
    else
    {
      Result = -1;
    }
  }
  else
  {
    if (!rht.FName.IsEmpty())
    {
      Result = 1;
    }
    else
    {
      if (FIDValid)
      {
        if (rht.FIDValid)
        {
          Result = (FID < rht.FID) ? -1 : ((FID > rht.FID) ? 1 : 0);
        }
        else
        {
          Result = -1;
        }
      }
      else
      {
        if (rht.FIDValid)
        {
          Result = 1;
        }
        else
        {
          Result = 0;
        }
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteToken::SetID(intptr_t Value)
{
  FID = Value;
  FIDValid = Value != 0;
}
//---------------------------------------------------------------------------
bool TRemoteToken::GetNameValid() const
{
  return !FName.IsEmpty();
}
//---------------------------------------------------------------------------
bool TRemoteToken::GetIsSet() const
{
  return !FName.IsEmpty() || FIDValid;
}
//---------------------------------------------------------------------------
UnicodeString TRemoteToken::GetDisplayText() const
{
  if (!FName.IsEmpty())
  {
    return FName;
  }
  else if (FIDValid)
  {
    return IntToStr(FID);
  }
  else
  {
    return UnicodeString();
  }
}
//---------------------------------------------------------------------------
UnicodeString TRemoteToken::GetLogText() const
{
  return FORMAT(L"\"%s\" [%d]", FName.c_str(), static_cast<int>(FID));
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteTokenList * TRemoteTokenList::Duplicate() const
{
  std::unique_ptr<TRemoteTokenList> Result(new TRemoteTokenList());
  TTokens::const_iterator it = FTokens.begin();
  while (it != FTokens.end())
  {
    Result->Add(*it);
    ++it;
  }
  return Result.release();
}
//---------------------------------------------------------------------------
void TRemoteTokenList::Clear()
{
  FTokens.clear();
  FNameMap.clear();
  FIDMap.clear();
}
//---------------------------------------------------------------------------
void TRemoteTokenList::Add(const TRemoteToken & Token)
{
  FTokens.push_back(Token);
  if (Token.GetIDValid())
  {
    // std::pair<TIDMap::iterator, bool> Position =
      FIDMap.insert(TIDMap::value_type(Token.GetID(), FTokens.size() - 1));
  }
  if (Token.GetNameValid())
  {
    // std::pair<TNameMap::iterator, bool> Position =
      FNameMap.insert(TNameMap::value_type(Token.GetName(), FTokens.size() - 1));
  }
}
//---------------------------------------------------------------------------
void TRemoteTokenList::AddUnique(const TRemoteToken & Token)
{
  if (Token.GetIDValid())
  {
    TIDMap::const_iterator it = FIDMap.find(Token.GetID());
    if (it != FIDMap.end())
    {
      // is present already.
      // may have different name (should not),
      // but what can we do about it anyway?
    }
    else
    {
      Add(Token);
    }
  }
  else if (Token.GetNameValid())
  {
    TNameMap::const_iterator it = FNameMap.find(Token.GetName());
    if (it != FNameMap.end())
    {
      // is present already.
    }
    else
    {
      Add(Token);
    }
  }
  else
  {
    // can happen, e.g. with winsshd/SFTP
  }
}
//---------------------------------------------------------------------------
bool TRemoteTokenList::Exists(const UnicodeString & Name) const
{
  return (FNameMap.find(Name) != FNameMap.end());
}
//---------------------------------------------------------------------------
const TRemoteToken * TRemoteTokenList::Find(uintptr_t ID) const
{
  TIDMap::const_iterator it = FIDMap.find(ID);
  const TRemoteToken * Result;
  if (it != FIDMap.end())
  {
    Result = &FTokens[(*it).second];
  }
  else
  {
    Result = nullptr;
  }
  return Result;
}
//---------------------------------------------------------------------------
const TRemoteToken * TRemoteTokenList::Find(const UnicodeString & Name) const
{
  TNameMap::const_iterator it = FNameMap.find(Name);
  const TRemoteToken * Result;
  if (it != FNameMap.end())
  {
    Result = &FTokens[(*it).second];
  }
  else
  {
    Result = nullptr;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteTokenList::Log(TTerminal * Terminal, const wchar_t * Title)
{
  if (!FTokens.empty())
  {
    Terminal->LogEvent(FORMAT(L"Following %s found:", Title));
    for (intptr_t Index = 0; Index < static_cast<intptr_t>(FTokens.size()); ++Index)
    {
      Terminal->LogEvent(UnicodeString(L"  ") + FTokens[Index].GetLogText());
    }
  }
  else
  {
    Terminal->LogEvent(FORMAT(L"No %s found.", Title));
  }
}
//---------------------------------------------------------------------------
intptr_t TRemoteTokenList::GetCount() const
{
  return static_cast<intptr_t>(FTokens.size());
}
//---------------------------------------------------------------------------
const TRemoteToken * TRemoteTokenList::Token(intptr_t Index) const
{
  return &FTokens[Index];
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteFile::TRemoteFile(TRemoteFile * ALinkedByFile):
  TPersistent(),
  FDirectory(nullptr),
  FSize(0),
  FINodeBlocks(0),
  FIconIndex(0),
  FIsSymLink(false),
  FLinkedFile(nullptr),
  FLinkedByFile(nullptr),
  FRights(nullptr),
  FTerminal(nullptr),
  FType(0),
  FSelected(false),
  FCyclicLink(false),
  FIsHidden(0)
{
  FLinkedFile = nullptr;
  FRights = new TRights();
  FIconIndex = -1;
  FCyclicLink = false;
  FModificationFmt = mfFull;
  FLinkedByFile = ALinkedByFile;
  FTerminal = nullptr;
  FDirectory = nullptr;
  FIsHidden = -1;
}
//---------------------------------------------------------------------------
TRemoteFile::~TRemoteFile()
{
  SAFE_DESTROY(FRights);
  SAFE_DESTROY(FLinkedFile);
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteFile::Duplicate(bool Standalone) const
{
  std::unique_ptr<TRemoteFile> Result(new TRemoteFile());
  if (FLinkedFile)
  {
    Result->FLinkedFile = FLinkedFile->Duplicate(true);
    Result->FLinkedFile->FLinkedByFile = Result.get();
  }
  Result->SetRights(FRights);
#define COPY_FP(PROP) Result->F ## PROP = F ## PROP;
  COPY_FP(Terminal);
  COPY_FP(Owner);
  COPY_FP(ModificationFmt);
  COPY_FP(Size);
  COPY_FP(FileName);
  COPY_FP(INodeBlocks);
  COPY_FP(Modification);
  COPY_FP(LastAccess);
  COPY_FP(Group);
  COPY_FP(IconIndex);
  COPY_FP(TypeName);
  COPY_FP(IsSymLink);
  COPY_FP(LinkTo);
  COPY_FP(Type);
  COPY_FP(Selected);
  COPY_FP(CyclicLink);
#undef COPY_FP
  if (Standalone && (!FFullFileName.IsEmpty() || (GetDirectory() != nullptr)))
  {
    Result->FFullFileName = GetFullFileName();
  }
  return Result.release();
}
//---------------------------------------------------------------------------
void TRemoteFile::LoadTypeInfo() const
{
  /* TODO : If file is link: Should be attributes taken from linked file? */
  /* uint32_t Attrs = INVALID_FILE_ATTRIBUTES;
  if (GetIsDirectory())
  {
    Attrs |= FILE_ATTRIBUTE_DIRECTORY;
  }
  if (GetIsHidden())
  {
    Attrs |= FILE_ATTRIBUTE_HIDDEN;
  }

  UnicodeString DumbFileName = (GetIsSymLink() && !GetLinkTo().IsEmpty() ? GetLinkTo() : GetFileName());

  FIconIndex = FakeFileImageIndex(DumbFileName, Attrs, &FTypeName); */
}
//---------------------------------------------------------------------------
intptr_t TRemoteFile::GetIconIndex() const
{
  if (FIconIndex == -1)
  {
    const_cast<TRemoteFile *>(this)->LoadTypeInfo();
  }
  return FIconIndex;
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetTypeName() const
{
  // check availability of type info by icon index, because type name can be empty
  if (FIconIndex < 0)
  {
    LoadTypeInfo();
  }
  return FTypeName;
}
//---------------------------------------------------------------------------
Boolean TRemoteFile::GetIsHidden() const
{
  bool Result;
  switch (FIsHidden)
  {
    case 0:
      Result = false;
      break;

    case 1:
      Result = true;
      break;

    default:
      Result = ::IsUnixHiddenFile(GetFileName());
      break;
  }

  return Result;
}

//---------------------------------------------------------------------------
void TRemoteFile::SetIsHidden(bool Value)
{
  FIsHidden = Value ? 1 : 0;
}
//---------------------------------------------------------------------------
Boolean TRemoteFile::GetIsDirectory() const
{
  return (UpCase(GetType()) == FILETYPE_DIRECTORY);
}
//---------------------------------------------------------------------------
Boolean TRemoteFile::GetIsParentDirectory() const
{
  return wcscmp(FFileName.c_str(), PARENTDIRECTORY) == 0;
}
//---------------------------------------------------------------------------
Boolean TRemoteFile::GetIsThisDirectory() const
{
  return wcscmp(FFileName.c_str(), THISDIRECTORY) == 0;
}
//---------------------------------------------------------------------------
Boolean TRemoteFile::GetIsInaccesibleDirectory() const
{
  Boolean Result;
  if (GetIsDirectory())
  {
    assert(GetTerminal());
    Result = !
       (SameText(GetTerminal()->GetUserName(), L"root")) ||
       (((GetRights()->GetRightUndef(TRights::rrOtherExec) != TRights::rsNo)) ||
        ((GetRights()->GetRight(TRights::rrGroupExec) != TRights::rsNo) &&
         GetTerminal()->GetMembership()->Exists(GetFileGroup().GetName())) ||
        ((GetRights()->GetRight(TRights::rrUserExec) != TRights::rsNo) &&
         (SameText(GetTerminal()->GetUserName(), GetFileOwner().GetName()))));
  }
  else
  {
    Result = False;
  }
  return Result;
}
//---------------------------------------------------------------------------
wchar_t TRemoteFile::GetType() const
{
  if (GetIsSymLink() && FLinkedFile)
  {
    return FLinkedFile->GetType();
  }
  else
  {
    return FType;
  }
}
//---------------------------------------------------------------------------
void TRemoteFile::SetType(wchar_t AType)
{
  FType = AType;
  FIsSymLink = (static_cast<wchar_t>(towupper(FType)) == FILETYPE_SYMLINK);
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteFile::GetLinkedFile() const
{
  // do not call FindLinkedFile as it would be called repeatedly for broken symlinks
  return FLinkedFile;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetLinkedFile(TRemoteFile * Value)
{
  if (FLinkedFile != Value)
  {
    if (FLinkedFile)
    {
      SAFE_DESTROY(FLinkedFile);
    }
    FLinkedFile = Value;
  }
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetBrokenLink() const
{
  assert(GetTerminal());
  // If file is symlink but we couldn't find linked file we assume broken link
  return (GetIsSymLink() && (FCyclicLink || !FLinkedFile) &&
    GetTerminal()->GetResolvingSymlinks());
  // "!FLinkTo.IsEmpty()" removed because it does not work with SFTP
}
//---------------------------------------------------------------------------
void TRemoteFile::ShiftTime(const TDateTime & Difference)
{
  double D = Difference.GetValue();
  if (!IsZero(D) && (FModificationFmt != mfMDY))
  {
    assert(static_cast<int>(FModification) != 0);
    FModification = FModification.GetValue() + D;
    assert(static_cast<int>(FLastAccess) != 0);
    FLastAccess = FLastAccess.GetValue() + D;
  }
}
//---------------------------------------------------------------------------
void TRemoteFile::SetModification(const TDateTime & Value)
{
  if (FModification != Value)
  {
    FModificationFmt = mfFull;
    FModification = Value;
  }
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetUserModificationStr()
{
  return ::UserModificationStr(GetModification(), FModificationFmt);
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetModificationStr() const
{
  return ::ModificationStr(GetModification(), FModificationFmt);
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetExtension()
{
  return ::UnixExtractFileExt(FFileName);
}
//---------------------------------------------------------------------------
void TRemoteFile::SetRights(TRights * Value)
{
  FRights->Assign(Value);
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetRightsStr() const
{
  return FRights->GetUnknown() ? UnicodeString() : FRights->GetText();
}
//---------------------------------------------------------------------------
void TRemoteFile::SetListingStr(const UnicodeString & Value)
{
  // Value stored in 'Value' can be used for error message
  UnicodeString ListingStr = Value;
  FIconIndex = -1;
  try
  {
    UnicodeString Col;

    // Do we need to do this (is ever TAB is LS output)?
    ListingStr = ReplaceChar(ListingStr, L'\t', L' ');

    SetType(ListingStr[1]);
    ListingStr.Delete(1, 1);

    auto GetNCol = [&]()
    {
      if (ListingStr.IsEmpty())
        throw Exception(L"");
      intptr_t P = ListingStr.Pos(L' ');
      if (P)
      {
        Col = ListingStr;
        Col.SetLength(P - 1);
        ListingStr.Delete(1, P);
      }
      else
      {
        Col = ListingStr;
        ListingStr.Clear();
      }
    };
    auto GetCol = [&]()
    {
      GetNCol();
      ListingStr = TrimLeft(ListingStr);
    };

    // Rights string may contain special permission attributes (S,t, ...)
    // (TODO: maybe no longer necessary, once we can handle the special permissions)
    GetRights()->SetAllowUndef(True);
    // On some system there is no space between permissions and node blocks count columns
    // so we get only first 9 characters and trim all following spaces (if any)
    GetRights()->SetText(ListingStr.SubString(1, 9));
    ListingStr.Delete(1, 9);
    // Rights column maybe followed by '+', '@' or '.' signs, we ignore them
    // (On MacOS, there may be a space in between)
    if (!ListingStr.IsEmpty() && ((ListingStr[1] == L'+') || (ListingStr[1] == L'@') || (ListingStr[1] == L'.')))
    {
      ListingStr.Delete(1, 1);
    }
    else if ((ListingStr.Length() >= 2) && (ListingStr[1] == L' ') &&
             ((ListingStr[2] == L'+') || (ListingStr[2] == L'@') || (ListingStr[2] == L'.')))
    {
      ListingStr.Delete(1, 2);
    }
    ListingStr = ListingStr.TrimLeft();

    GetCol();
    if (!TryStrToInt(Col, FINodeBlocks))
    {
      // if the column is not an integer, suppose it's owner
      // (Android BusyBox)
      FINodeBlocks = 0;
    }
    else
    {
      GetCol();
    }

    FOwner.SetName(Col);

    // #60 17.10.01: group name can contain space
    FGroup.SetName(L"");
    GetCol();
    int64_t ASize;
    do
    {
      FGroup.SetName(FGroup.GetName() + Col);
      GetCol();
      assert(!Col.IsEmpty());
      // for devices etc.. there is additional column ending by comma, we ignore it
      if (Col[Col.Length()] == L',')
        GetCol();
      ASize = StrToInt64Def(Col, -1);
      // if it's not a number (file size) we take it as part of group name
      // (at least on CygWin, there can be group with space in its name)
      if (ASize < 0)
        Col = L" " + Col;
    }
    while (ASize < 0);

    // do not read modification time and filename if it is already set
    if (IsZero(FModification.GetValue()) && GetFileName().IsEmpty())
    {
      FSize = ASize;

      bool DayMonthFormat = false;
      Word Year = 0, Month = 0, Day = 0, Hour = 0, Min = 0, Sec = 0;
      Word CurrYear = 0, CurrMonth = 0, CurrDay = 0;
      DecodeDate(Date(), CurrYear, CurrMonth, CurrDay);

      GetCol();
      // format dd mmm or mmm dd ?
      Day = ToWord(StrToIntDef(Col, 0));
      if (Day > 0)
      {
        DayMonthFormat = true;
        GetCol();
      }
      Month = 0;
      auto Col2Month = [&]()
      {
        for (Word IMonth = 0; IMonth < 12; IMonth++)
          if (!Col.CompareIC(EngShortMonthNames[IMonth]))
          {
            Month = IMonth;
            Month++;
            break;
          }
      };

      Col2Month();
      // if the column is not known month name, it may have been "yyyy-mm-dd"
      // for --full-time format
      if ((Month == 0) && (Col.Length() == 10) && (Col[5] == L'-') && (Col[8] == L'-'))
      {
        Year = ToWord(Col.SubString(1, 4).ToInt());
        Month = ToWord(Col.SubString(6, 2).ToInt());
        Day = ToWord(Col.SubString(9, 2).ToInt());
        GetCol();
        Hour = ToWord(Col.SubString(1, 2).ToInt());
        Min = ToWord(Col.SubString(4, 2).ToInt());
        if (Col.Length() >= 8)
        {
          Sec = ToWord(Sysutils::StrToInt64(Col.SubString(7, 2)));
        }
        else
        {
          Sec = 0;
        }
        FModificationFmt = mfFull;
        // skip TZ (TODO)
        // do not trim leading space of filename
        GetNCol();
      }
      else if ((Month == 0) && (Col.Length() == 3))
      {
        // drwxr-xr-x   4 root  wheel   512  2 mmm 13:00 .'.
        Month = CurrMonth;
        GetCol();
        Hour = ToWord(Col.SubString(1, 2).ToInt());
        Min = ToWord(Col.SubString(4, 2).ToInt());
        if (Col.Length() >= 8)
        {
          Sec = ToWord(Sysutils::StrToInt64(Col.SubString(7, 2)));
        }
        else
        {
          Sec = 0;
        }
      }
      else
      {
        bool FullTime = false;
        // or it may have been day name for another format of --full-time
        if (Month == 0)
        {
          GetCol();
          Col2Month();
          // neither standard, not --full-time format
          if (Month == 0)
          {
            Abort();
          }
          else
          {
            FullTime = true;
          }
        }

        if (Day == 0)
        {
          GetNCol();
          Day = ToWord(Sysutils::StrToInt64(Col));
        }
        if ((Day < 1) || (Day > 31))
        {
          Abort();
        }

        // second full-time format
        // ddd mmm dd hh:nn:ss yyyy
        if (FullTime)
        {
          GetCol();
          if (Col.Length() != 8)
          {
            Abort();
          }
          Hour = ToWord(Sysutils::StrToInt64(Col.SubString(1, 2)));
          Min = ToWord(Sysutils::StrToInt64(Col.SubString(4, 2)));
          Sec = ToWord(Sysutils::StrToInt64(Col.SubString(7, 2)));
          FModificationFmt = mfFull;
          // do not trim leading space of filename
          GetNCol();
          Year = ToWord(Sysutils::StrToInt64(Col));
        }
        else
        {
          // for format dd mmm the below description seems not to be true,
          // the year is not aligned to 5 characters
          if (DayMonthFormat)
          {
            GetCol();
          }
          else
          {
            // Time/Year indicator is always 5 characters long (???), on most
            // systems year is aligned to right (_YYYY), but on some to left (YYYY_),
            // we must ensure that trailing space is also deleted, so real
            // separator space is not treated as part of file name
            Col = ListingStr.SubString(1, 6).Trim();
            ListingStr.Delete(1, 6);
          }
          // GetNCol(); // We don't want to trim input strings (name with space at beginning???)
          // Check if we got time (contains :) or year
          intptr_t P;
          if ((P = ToWord(Col.Pos(L':'))) > 0)
          {
            Hour = ToWord(Sysutils::StrToInt64(Col.SubString(1, P - 1)));
            Min = ToWord(Sysutils::StrToInt64(Col.SubString(P + 1, Col.Length() - P)));
            if ((Hour > 23) || (Min > 59))
              Abort();
            // When we don't got year, we assume current year
            // with exception that the date would be in future
            // in this case we assume last year.
            DecodeDate(Date(), Year, CurrMonth, CurrDay);
            if ((Month > CurrMonth) ||
                (Month == CurrMonth && Day > CurrDay))
            {
              Year--;
            }
            Sec = 0;
            FModificationFmt = mfMDHM;
          }
          else
          {
            Year = ToWord(Sysutils::StrToInt64(Col));
            if (Year > 10000)
              Abort();
            // When we don't got time we assume midnight
            Hour = 0;
            Min = 0;
            Sec = 0;
            FModificationFmt = mfMDY;
          }
        }
      }

      if (Year == 0)
        Year = CurrYear;
      if (Month == 0)
        Month = CurrMonth;
      if (Day == 0)
        Day = CurrDay;
      FModification = EncodeDateVerbose(Year, Month, Day) + EncodeTimeVerbose(Hour, Min, Sec, 0);
      // adjust only when time is known,
      // adjusting default "midnight" time makes no sense
      if ((FModificationFmt == mfMDHM) || (FModificationFmt == mfFull))
      {
        assert(GetTerminal() != nullptr);
        FModification = ::AdjustDateTimeFromUnix(FModification,
          GetTerminal()->GetSessionData()->GetDSTMode());
      }

      if (IsZero(FLastAccess.GetValue()))
      {
        FLastAccess = FModification;
      }

      // separating space is already deleted, other spaces are treated as part of name

      {
        FLinkTo = L"";
        if (GetIsSymLink())
        {
          intptr_t P = ListingStr.Pos(SYMLINKSTR);
          if (P)
          {
            FLinkTo = ListingStr.SubString(
              P + wcslen(SYMLINKSTR), ListingStr.Length() - P + wcslen(SYMLINKSTR) + 1);
            ListingStr.SetLength(P - 1);
          }
          else
          {
            Abort();
          }
        }
        FFileName = ::UnixExtractFileName(::Trim(ListingStr));
      }
    }
  }
  catch (Exception & E)
  {
    throw ETerminal(&E, FmtLoadStr(LIST_LINE_ERROR, Value.c_str()), HELP_LIST_LINE_ERROR);
  }
}
//---------------------------------------------------------------------------
void TRemoteFile::Complete()
{
  assert(GetTerminal() != nullptr);
  if (GetIsSymLink() && GetTerminal()->GetResolvingSymlinks())
  {
    FindLinkedFile();
  }
}
//---------------------------------------------------------------------------
void TRemoteFile::FindLinkedFile()
{
  assert(GetTerminal() && GetIsSymLink());

  if (FLinkedFile)
  {
    SAFE_DESTROY(FLinkedFile);
  }
  FLinkedFile = nullptr;

  FCyclicLink = false;
  if (!GetLinkTo().IsEmpty())
  {
    // check for cyclic link
    TRemoteFile * LinkedBy = FLinkedByFile;
    while (LinkedBy)
    {
      if (LinkedBy->GetLinkTo() == GetLinkTo())
      {
        // this is currently redundant information, because it is used only to
        // detect broken symlink, which would be otherwise detected
        // by FLinkedFile == nullptr
        FCyclicLink = true;
        break;
      }
      LinkedBy = LinkedBy->FLinkedByFile;
    }
  }

  if (FCyclicLink)
  {
    TRemoteFile * LinkedBy = FLinkedByFile;
    while (LinkedBy)
    {
      LinkedBy->FCyclicLink = true;
      LinkedBy = LinkedBy->FLinkedByFile;
    }
  }
  else
  {
    assert(GetTerminal()->GetResolvingSymlinks());
    GetTerminal()->SetExceptionOnFail(true);
    try
    {
      SCOPE_EXIT
      {
        GetTerminal()->SetExceptionOnFail(false);
      };
      GetTerminal()->ReadSymlink(this, FLinkedFile);
    }
    catch (Exception & E)
    {
      if (NB_STATIC_DOWNCAST(EFatal, &E) != nullptr)
      {
        throw;
      }
      else
      {
        GetTerminal()->GetLog()->AddException(&E);
      }
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetListingStr() const
{
  // note that ModificationStr is longer than 12 for mfFull
  UnicodeString LinkPart;
  // expanded from ?: to avoid memory leaks
  if (GetIsSymLink())
  {
    LinkPart = UnicodeString(SYMLINKSTR) + GetLinkTo();
  }
  return Format(L"%s%s %3s %-8s %-8s %9s %-12s %s%s",
    GetType(), GetRights()->GetText().c_str(), Int64ToStr(FINodeBlocks).c_str(), GetFileOwner().GetName().c_str(),
    GetFileGroup().GetName().c_str(), Int64ToStr(GetSize()).c_str(), GetModificationStr().c_str(), GetFileName().c_str(),
    LinkPart.c_str());
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetFullFileName() const
{
  if (FFullFileName.IsEmpty())
  {
    assert(GetTerminal());
    assert(GetDirectory() != nullptr);
    UnicodeString Path;
    if (GetIsParentDirectory())
    {
      Path = GetDirectory()->GetParentPath();
    }
    else if (GetIsDirectory())
    {
      Path = ::UnixIncludeTrailingBackslash(GetDirectory()->GetFullDirectory() + GetFileName());
    }
    else
    {
      Path = GetDirectory()->GetFullDirectory() + GetFileName();
    }
    return GetTerminal()->TranslateLockedPath(Path, true);
  }
  else
  {
    return FFullFileName;
  }
}
//---------------------------------------------------------------------------
bool TRemoteFile::GetHaveFullFileName() const
{
  return !FFullFileName.IsEmpty() || (GetDirectory() != nullptr);
}
//---------------------------------------------------------------------------
intptr_t TRemoteFile::GetAttr() const
{
  intptr_t Result = 0;
  if (GetRights()->GetReadOnly())
  {
    Result |= faReadOnly;
  }
  if (GetIsHidden())
  {
    Result |= faHidden;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetTerminal(TTerminal * Value)
{
  FTerminal = Value;
  if (FLinkedFile)
  {
    FLinkedFile->SetTerminal(Value);
  }
}
//---------------------------------------------------------------------------
const TRemoteToken & TRemoteFile::GetFileOwner() const
{
  return FOwner;
}
//---------------------------------------------------------------------------
TRemoteToken & TRemoteFile::GetFileOwner()
{
  return FOwner;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetFileOwner(const TRemoteToken & Value)
{
  FOwner = Value;
}
//---------------------------------------------------------------------------
const TRemoteToken & TRemoteFile::GetFileGroup() const
{
  return FGroup;
}
//---------------------------------------------------------------------------
TRemoteToken & TRemoteFile::GetFileGroup()
{
  return FGroup;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetFileGroup(const TRemoteToken & Value)
{
  FGroup = Value;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetFileName(const UnicodeString & Value)
{
  FFileName = Value;
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFile::GetLinkTo() const
{
  return FLinkTo;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetLinkTo(const UnicodeString & Value)
{
  FLinkTo = Value;
}
//---------------------------------------------------------------------------
void TRemoteFile::SetFullFileName(const UnicodeString & Value)
{
  FFullFileName = Value;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteDirectoryFile::TRemoteDirectoryFile() : TRemoteFile()
{
  SetModification(TDateTime(0.0));
  SetModificationFmt(mfNone);
  SetLastAccess(GetModification());
  SetType(L'D');
  SetSize(0);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteParentDirectory::TRemoteParentDirectory(TTerminal * ATerminal)
  : TRemoteDirectoryFile()
{
  SetFileName(PARENTDIRECTORY);
  SetTerminal(ATerminal);
}
//=== TRemoteFileList ------------------------------------------------------
TRemoteFileList::TRemoteFileList() :
  TObjectList()
{
  FTimestamp = Now();
  SetOwnsObjects(true);
}
//---------------------------------------------------------------------------
void TRemoteFileList::AddFile(TRemoteFile * File)
{
  Add(File);
  File->SetDirectory(this);
}
//---------------------------------------------------------------------------
void TRemoteFileList::DuplicateTo(TRemoteFileList * Copy) const
{
  Copy->Reset();
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    TRemoteFile * File = GetFile(Index);
    Copy->AddFile(File->Duplicate(false));
  }
  Copy->FDirectory = GetDirectory();
  Copy->FTimestamp = FTimestamp;
}
//---------------------------------------------------------------------------
void TRemoteFileList::Reset()
{
  FTimestamp = Now();
  TObjectList::Clear();
}
//---------------------------------------------------------------------------
void TRemoteFileList::SetDirectory(const UnicodeString & Value)
{
  FDirectory = ::UnixExcludeTrailingBackslash(Value);
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFileList::GetFullDirectory()
{
  return ::UnixIncludeTrailingBackslash(GetDirectory());
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteFileList::GetFile(Integer Index) const
{
  return NB_STATIC_DOWNCAST(TRemoteFile, GetItem(Index));
}
//---------------------------------------------------------------------------
Boolean TRemoteFileList::GetIsRoot()
{
  return (GetDirectory() == ROOTDIRECTORY);
}
//---------------------------------------------------------------------------
UnicodeString TRemoteFileList::GetParentPath()
{
  return ::UnixExtractFilePath(GetDirectory());
}
//---------------------------------------------------------------------------
int64_t TRemoteFileList::GetTotalSize()
{
  int64_t Result = 0;
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    if (!GetFile(Index)->GetIsDirectory())
    {
      Result += GetFile(Index)->GetSize();
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteFileList::FindFile(const UnicodeString & AFileName) const
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    if (GetFile(Index)->GetFileName() == AFileName)
    {
      return GetFile(Index);
    }
  }
  return nullptr;
}
//=== TRemoteDirectory ------------------------------------------------------
TRemoteDirectory::TRemoteDirectory(TTerminal * aTerminal, TRemoteDirectory * Template) :
  TRemoteFileList(), FTerminal(aTerminal)
{
  FSelectedFiles = nullptr;
  FThisDirectory = nullptr;
  FParentDirectory = nullptr;
  if (Template == nullptr)
  {
    FIncludeThisDirectory = false;
    FIncludeParentDirectory = true;
  }
  else
  {
    FIncludeThisDirectory = Template->FIncludeThisDirectory;
    FIncludeParentDirectory = Template->FIncludeParentDirectory;
  }
}
//---------------------------------------------------------------------------
TRemoteDirectory::~TRemoteDirectory()
{
  ReleaseRelativeDirectories();
}
//---------------------------------------------------------------------------
void TRemoteDirectory::ReleaseRelativeDirectories()
{
  if ((GetThisDirectory() != nullptr) && !GetIncludeThisDirectory())
  {
    SAFE_DESTROY(FThisDirectory);
  }
  if ((GetParentDirectory() != nullptr) && !GetIncludeParentDirectory())
  {
    SAFE_DESTROY(FParentDirectory);
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectory::Reset()
{
  ReleaseRelativeDirectories();
  TRemoteFileList::Reset();
}
//---------------------------------------------------------------------------
void TRemoteDirectory::SetDirectory(const UnicodeString & Value)
{
  TRemoteFileList::SetDirectory(Value);
}
//---------------------------------------------------------------------------
void TRemoteDirectory::AddFile(TRemoteFile * File)
{
  if (File->GetIsThisDirectory())
  {
    FThisDirectory = File;
  }
  if (File->GetIsParentDirectory())
  {
    FParentDirectory = File;
  }

  if ((!File->GetIsThisDirectory() || GetIncludeThisDirectory()) &&
      (!File->GetIsParentDirectory() || GetIncludeParentDirectory()))
  {
    TRemoteFileList::AddFile(File);
  }
  File->SetTerminal(GetTerminal());
}
//---------------------------------------------------------------------------
void TRemoteDirectory::DuplicateTo(TRemoteFileList * Copy) const
{
  TRemoteFileList::DuplicateTo(Copy);
  if (GetThisDirectory() && !GetIncludeThisDirectory())
  {
    Copy->AddFile(GetThisDirectory()->Duplicate(false));
  }
  if (GetParentDirectory() && !GetIncludeParentDirectory())
  {
    Copy->AddFile(GetParentDirectory()->Duplicate(false));
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirectory::GetLoaded() const
{
  return ((GetTerminal() != nullptr) && GetTerminal()->GetActive() && !GetDirectory().IsEmpty());
}
//---------------------------------------------------------------------------
TStrings * TRemoteDirectory::GetSelectedFiles() const
{
  if (!FSelectedFiles)
  {
    FSelectedFiles = new TStringList();
  }
  else
  {
    FSelectedFiles->Clear();
  }

  for (intptr_t Index = 0; Index < GetCount(); Index ++)
  {
    if (GetFile(Index)->GetSelected())
    {
      FSelectedFiles->Add(GetFile(Index)->GetFullFileName());
    }
  }

  return FSelectedFiles;
}
//---------------------------------------------------------------------------
void TRemoteDirectory::SetIncludeParentDirectory(Boolean Value)
{
  if (GetIncludeParentDirectory() != Value)
  {
    FIncludeParentDirectory = Value;
    if (Value && GetParentDirectory())
    {
      assert(IndexOf(GetParentDirectory()) < 0);
      Add(GetParentDirectory());
    }
    else if (!Value && GetParentDirectory())
    {
      assert(IndexOf(GetParentDirectory()) >= 0);
      Extract(GetParentDirectory());
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectory::SetIncludeThisDirectory(Boolean Value)
{
  if (GetIncludeThisDirectory() != Value)
  {
    FIncludeThisDirectory = Value;
    if (Value && GetThisDirectory())
    {
      assert(IndexOf(GetThisDirectory()) < 0);
      Add(GetThisDirectory());
    }
    else if (!Value && GetThisDirectory())
    {
      assert(IndexOf(GetThisDirectory()) >= 0);
      Extract(GetThisDirectory());
    }
  }
}
//===========================================================================
TRemoteDirectoryCache::TRemoteDirectoryCache(): TStringList()
{
  SetSorted(true);
  SetDuplicates(dupError);
  SetCaseSensitive(true);
}
//---------------------------------------------------------------------------
TRemoteDirectoryCache::~TRemoteDirectoryCache()
{
  Clear();
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::Clear()
{
  TGuard Guard(FSection);

  SCOPE_EXIT
  {
    TStringList::Clear();
  };
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    TRemoteFileList * List = NB_STATIC_DOWNCAST(TRemoteFileList, GetObject(Index));
    SAFE_DESTROY(List);
    SetObject(Index, nullptr);
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::GetIsEmpty() const
{
  TGuard Guard(FSection);

  return (const_cast<TRemoteDirectoryCache *>(this)->GetCount() == 0);
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::HasFileList(const UnicodeString & Directory)
{
  TGuard Guard(FSection);

  intptr_t Index = IndexOf(::UnixExcludeTrailingBackslash(Directory));
  return (Index >= 0);
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::HasNewerFileList(const UnicodeString & Directory,
  const TDateTime& Timestamp)
{
  TGuard Guard(FSection);

  intptr_t Index = IndexOf(::UnixExcludeTrailingBackslash(Directory));
  if (Index >= 0)
  {
    TRemoteFileList * FileList = NB_STATIC_DOWNCAST(TRemoteFileList, GetObject(Index));
    if (FileList->GetTimestamp() <= Timestamp)
    {
      Index = -1;
    }
  }
  return (Index >= 0);
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryCache::GetFileList(const UnicodeString & Directory,
  TRemoteFileList * FileList)
{
  TGuard Guard(FSection);

  intptr_t Index = IndexOf(::UnixExcludeTrailingBackslash(Directory));
  bool Result = (Index >= 0);
  if (Result)
  {
    assert(GetObject(Index) != nullptr);
    NB_STATIC_DOWNCAST(TRemoteFileList, GetObject(Index))->DuplicateTo(FileList);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::AddFileList(TRemoteFileList * FileList)
{
  assert(FileList);
  if (FileList)
  {
    TRemoteFileList * Copy = new TRemoteFileList();

    FileList->DuplicateTo(Copy);

    TGuard Guard(FSection);

    // file list cannot be cached already with only one thread, but it can be
    // when directory is loaded by secondary terminal
    DoClearFileList(FileList->GetDirectory(), false);
    AddObject(Copy->GetDirectory(), Copy);
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::ClearFileList(const UnicodeString & Directory, bool SubDirs)
{
  TGuard Guard(FSection);
  DoClearFileList(Directory, SubDirs);
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::DoClearFileList(const UnicodeString & Directory, bool SubDirs)
{
  UnicodeString Directory2 = ::UnixExcludeTrailingBackslash(Directory);
  intptr_t Index = IndexOf(Directory2);
  if (Index >= 0)
  {
    Delete(Index);
  }
  if (SubDirs)
  {
    Directory2 = ::UnixIncludeTrailingBackslash(Directory2);
    Index = GetCount() - 1;
    while (Index >= 0)
    {
      if (GetString(Index).SubString(1, Directory2.Length()) == Directory2)
      {
        Delete(Index);
      }
      Index--;
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryCache::Delete(intptr_t Index)
{
  TRemoteFileList * List = NB_STATIC_DOWNCAST(TRemoteFileList, GetObject(Index));
  SAFE_DESTROY(List);
  TStringList::Delete(Index);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TRemoteDirectoryChangesCache::TRemoteDirectoryChangesCache(intptr_t MaxSize) :
  TStringList(),
  FMaxSize(MaxSize)
{
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::Clear()
{
  TStringList::Clear();
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryChangesCache::GetIsEmpty() const
{
  return (const_cast<TRemoteDirectoryChangesCache *>(this)->GetCount() == 0);
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::SetValue(const UnicodeString & Name,
  const UnicodeString & Value)
{
  intptr_t Index = IndexOfName(Name);
  if (Index >= 0)
  {
    Delete(Index);
  }
  TStringList::SetValue(Name, Value);
}
//---------------------------------------------------------------------------
UnicodeString TRemoteDirectoryChangesCache::GetValue(const UnicodeString & Name)
{
  UnicodeString Value = TStringList::GetValue(Name);
  TStringList::SetValue(Name, Value);
  return Value;
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::AddDirectoryChange(
  const UnicodeString & SourceDir, const UnicodeString & Change,
  const UnicodeString & TargetDir)
{
  assert(!TargetDir.IsEmpty());
  SetValue(TargetDir, L"//");
  if (TTerminal::ExpandFileName(Change, SourceDir) != TargetDir)
  {
    UnicodeString Key;
    if (DirectoryChangeKey(SourceDir, Change, Key))
    {
      SetValue(Key, TargetDir);
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::ClearDirectoryChange(
  const UnicodeString & SourceDir)
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    if (GetName(Index).SubString(1, SourceDir.Length()) == SourceDir)
    {
      Delete(Index);
      Index--;
    }
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::ClearDirectoryChangeTarget(
  const UnicodeString & TargetDir)
{
  UnicodeString Key;
  // hack to clear at least local sym-link change in case symlink is deleted
  DirectoryChangeKey(::UnixExcludeTrailingBackslash(::UnixExtractFilePath(TargetDir)),
    ::UnixExtractFileName(TargetDir), Key);

  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    UnicodeString Name = GetName(Index);
    if ((Name.SubString(1, TargetDir.Length()) == TargetDir) ||
        (GetValue(Name).SubString(1, TargetDir.Length()) == TargetDir) ||
        (!Key.IsEmpty() && (Name == Key)))
    {
      Delete(Index);
      Index--;
    }
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryChangesCache::GetDirectoryChange(
  const UnicodeString & SourceDir, const UnicodeString & Change, UnicodeString & TargetDir)
{
  UnicodeString Key;
  Key = TTerminal::ExpandFileName(Change, SourceDir);
  if (Key.IsEmpty())
  {
    Key = L"/";
  }
  bool Result = (IndexOfName(Key.c_str()) >= 0);
  if (Result)
  {
    TargetDir = GetValue(Key);
    // TargetDir is not "//" here only when Change is full path to symbolic link
    if (TargetDir == L"//")
    {
      TargetDir = Key;
    }
  }
  else
  {
    Result = DirectoryChangeKey(SourceDir, Change, Key);
    if (Result)
    {
      UnicodeString Directory = GetValue(Key);
      Result = !Directory.IsEmpty();
      if (Result)
      {
        TargetDir = Directory;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::Serialize(UnicodeString & Data)
{
  Data = L"A";
  intptr_t ACount = GetCount();
  if (ACount > FMaxSize)
  {
    std::unique_ptr<TStrings> Limited(new TStringList());
    intptr_t Index = ACount - FMaxSize;
    while (Index < ACount)
    {
      Limited->Add(GetString(Index));
      ++Index;
    }
    Data += Limited->GetText();
  }
  else
  {
    Data += GetText();
  }
}
//---------------------------------------------------------------------------
void TRemoteDirectoryChangesCache::Deserialize(const UnicodeString & Data)
{
  if (Data.IsEmpty())
  {
    SetText(L"");
  }
  else
  {
    SetText(Data.c_str() + 1);
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirectoryChangesCache::DirectoryChangeKey(
  const UnicodeString & SourceDir, const UnicodeString & Change, UnicodeString & Key)
{
  bool Result = !Change.IsEmpty();
  if (Result)
  {
    bool Absolute = ::UnixIsAbsolutePath(Change);
    Result = !SourceDir.IsEmpty() || Absolute;
    if (Result)
    {
      // expanded from ?: to avoid memory leaks
      if (Absolute)
      {
        Key = Change;
      }
      else
      {
        Key = SourceDir + L"," + Change;
      }
    }
  }
  return Result;
}
//=== TRights ---------------------------------------------------------------
const wchar_t TRights::BasicSymbols[] = L"rwxrwxrwx";
const wchar_t TRights::CombinedSymbols[] = L"--s--s--t";
const wchar_t TRights::ExtendedSymbols[] = L"--S--S--T";
const wchar_t TRights::ModeGroups[] = L"ugo";
//---------------------------------------------------------------------------
TRights::TRights()
{
  FAllowUndef = false;
  FSet = 0;
  FUnset = 0;
  SetNumber(0);
  FUnknown = true;
}
//---------------------------------------------------------------------------
TRights::TRights(uint16_t ANumber)
{
  FAllowUndef = false;
  FSet = 0;
  FUnset = 0;
  SetNumber(ANumber);
}
//---------------------------------------------------------------------------
TRights::TRights(const TRights & Source)
{
  Assign(&Source);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TRights::Assign(const TRights * Source)
{
  FAllowUndef = Source->GetAllowUndef();
  FSet = Source->FSet;
  FUnset = Source->FUnset;
  FText = Source->FText;
  FUnknown = Source->FUnknown;
}
//---------------------------------------------------------------------------
TRights::TFlag TRights::RightToFlag(TRights::TRight Right)
{
  return static_cast<TFlag>(1 << (rrLast - Right));
}
//---------------------------------------------------------------------------
bool TRights::operator ==(const TRights & rhr) const
{
  if (GetAllowUndef() || rhr.GetAllowUndef())
  {
    for (int Right = rrFirst; Right <= rrLast; Right++)
    {
      if (GetRightUndef(static_cast<TRight>(Right)) !=
          rhr.GetRightUndef(static_cast<TRight>(Right)))
      {
        return false;
      }
    }
    return true;
  }
  else
  {
    return (GetNumber() == rhr.GetNumber());
  }
}
//---------------------------------------------------------------------------
bool TRights::operator ==(uint16_t rhr) const
{
  return (GetNumber() == rhr);
}
//---------------------------------------------------------------------------
bool TRights::operator !=(const TRights & rhr) const
{
  return !(*this == rhr);
}
//---------------------------------------------------------------------------
TRights & TRights::operator =(uint16_t rhr)
{
  SetNumber(rhr);
  return *this;
}
//---------------------------------------------------------------------------
TRights & TRights::operator =(const TRights & rhr)
{
  Assign(&rhr);
  return *this;
}
//---------------------------------------------------------------------------
TRights TRights::operator ~() const
{
  TRights Result(static_cast<uint16_t>(~GetNumber()));
  return Result;
}
//---------------------------------------------------------------------------
TRights TRights::operator &(const TRights & rhr) const
{
  TRights Result(*this);
  Result &= rhr;
  return Result;
}
//---------------------------------------------------------------------------
TRights TRights::operator &(uint16_t rhr) const
{
  TRights Result(*this);
  Result &= rhr;
  return Result;
}
//---------------------------------------------------------------------------
TRights & TRights::operator &=(const TRights & rhr)
{
  if (GetAllowUndef() || rhr.GetAllowUndef())
  {
    for (int Right = rrFirst; Right <= rrLast; Right++)
    {
      if (GetRightUndef(static_cast<TRight>(Right)) !=
            rhr.GetRightUndef(static_cast<TRight>(Right)))
      {
        SetRightUndef(static_cast<TRight>(Right), rsUndef);
      }
    }
  }
  else
  {
    SetNumber(GetNumber() & rhr.GetNumber());
  }
  return *this;
}
//---------------------------------------------------------------------------
TRights & TRights::operator &=(uint16_t rhr)
{
  SetNumber(GetNumber() & rhr);
  return *this;
}
//---------------------------------------------------------------------------
TRights TRights::operator |(const TRights & rhr) const
{
  TRights Result(*this);
  Result |= rhr;
  return Result;
}
//---------------------------------------------------------------------------
TRights TRights::operator |(uint16_t rhr) const
{
  TRights Result(*this);
  Result |= rhr;
  return Result;
}
//---------------------------------------------------------------------------
TRights & TRights::operator |=(const TRights & rhr)
{
  SetNumber(GetNumber() | rhr.GetNumber());
  return *this;
}
//---------------------------------------------------------------------------
TRights & TRights::operator |=(uint16_t rhr)
{
  SetNumber(GetNumber() | rhr);
  return *this;
}
//---------------------------------------------------------------------------
void TRights::SetAllowUndef(bool Value)
{
  if (FAllowUndef != Value)
  {
    assert(!Value || ((FSet | FUnset) == rfAllSpecials));
    FAllowUndef = Value;
  }
}
//---------------------------------------------------------------------------
void TRights::SetText(const UnicodeString & Value)
{
  if (Value != GetText())
  {
    if ((Value.Length() != TextLen) ||
        (!GetAllowUndef() && (Value.Pos(UndefSymbol) > 0)) ||
        (Value.Pos(L" ") > 0))
    {
      throw Exception(FMTLOAD(RIGHTS_ERROR, Value.c_str()));
    }

    FSet = 0;
    FUnset = 0;
    intptr_t Flag = 00001;
    int ExtendedFlag = 01000; //-V536
    bool KeepText = false;
    for (intptr_t Index = TextLen; Index >= 1; Index--)
    {
      if (Value[Index] == UnsetSymbol)
      {
        FUnset |= static_cast<uint16_t>(Flag | ExtendedFlag);
      }
      else if (Value[Index] == UndefSymbol)
      {
        // do nothing
      }
      else if (Value[Index] == CombinedSymbols[Index - 1])
      {
        FSet |= static_cast<uint16_t>(Flag | ExtendedFlag);
      }
      else if (Value[Index] == ExtendedSymbols[Index - 1])
      {
        FSet |= static_cast<uint16_t>(ExtendedFlag);
        FUnset |= static_cast<uint16_t>(Flag);
      }
      else
      {
        if (Value[Index] != BasicSymbols[Index - 1])
        {
          KeepText = true;
        }
        FSet |= static_cast<uint16_t>(Flag);
        if (Index % 3 == 0)
        {
          FUnset |= static_cast<uint16_t>(ExtendedFlag);
        }
      }

      Flag <<= 1;
      if (Index % 3 == 1)
      {
        ExtendedFlag <<= 1;
      }
    }

    FText = KeepText ? Value : UnicodeString();
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
UnicodeString TRights::GetText() const
{
  if (!FText.IsEmpty())
  {
    return FText;
  }
  else
  {
    UnicodeString Result(TextLen, 0);

    intptr_t Flag = 00001;
    int ExtendedFlag = 01000; //-V536
    bool ExtendedPos = true;
    wchar_t Symbol;
    intptr_t Index = TextLen;
    while (Index >= 1)
    {
      if (ExtendedPos &&
          ((FSet & (Flag | ExtendedFlag)) == (Flag | ExtendedFlag)))
      {
        Symbol = CombinedSymbols[Index - 1];
      }
      else if ((FSet & Flag) != 0)
      {
        Symbol = BasicSymbols[Index - 1];
      }
      else if (ExtendedPos && ((FSet & ExtendedFlag) != 0))
      {
        Symbol = ExtendedSymbols[Index - 1];
      }
      else if ((!ExtendedPos && ((FUnset & Flag) == Flag)) ||
        (ExtendedPos && ((FUnset & (Flag | ExtendedFlag)) == (Flag | ExtendedFlag))))
      {
        Symbol = UnsetSymbol;
      }
      else
      {
        Symbol = UndefSymbol;
      }

      Result[Index] = Symbol;

      Flag <<= 1;
      Index--;
      ExtendedPos = ((Index % 3) == 0);
      if (ExtendedPos)
      {
        ExtendedFlag <<= 1;
      }
    }
    return Result;
  }
}
//---------------------------------------------------------------------------
void TRights::SetOctal(const UnicodeString & AValue)
{
  UnicodeString Value(AValue);
  if (Value.Length() == 3)
  {
    Value = L"0" + Value;
  }

  if (GetOctal() != Value.c_str())
  {
    bool Correct = (Value.Length() == 4);
    if (Correct)
    {
      for (intptr_t Index = 1; (Index <= Value.Length()) && Correct; Index++)
      {
        Correct = (Value[Index] >= L'0') && (Value[Index] <= L'7');
      }
    }

    if (!Correct)
    {
      throw Exception(FMTLOAD(INVALID_OCTAL_PERMISSIONS, AValue.c_str()));
    }

    SetNumber(static_cast<uint16_t>(
      ((Value[1] - L'0') << 9) +
      ((Value[2] - L'0') << 6) +
      ((Value[3] - L'0') << 3) +
      ((Value[4] - L'0') << 0)));
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
uint32_t TRights::GetNumberDecadic() const
{
  uint32_t N = GetNumberSet(); // used to be "Number"
  uint32_t Result =
      ((N & 07000) / 01000 * 1000) +
      ((N & 00700) /  0100 *  100) +
      ((N & 00070) /   010 *   10) +
      ((N & 00007) /    01 *    1);

  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TRights::GetOctal() const
{
  UnicodeString Result;
  uint16_t N = GetNumberSet(); // used to be "Number"
  Result.SetLength(4);
  Result[1] = static_cast<wchar_t>(L'0' + ((N & 07000) >> 9));
  Result[2] = static_cast<wchar_t>(L'0' + ((N & 00700) >> 6));
  Result[3] = static_cast<wchar_t>(L'0' + ((N & 00070) >> 3));
  Result[4] = static_cast<wchar_t>(L'0' + ((N & 00007) >> 0));

  return Result;
}
//---------------------------------------------------------------------------
void TRights::SetNumber(uint16_t Value)
{
  if ((FSet != Value) || ((FSet | FUnset) != rfAllSpecials))
  {
    FSet = Value;
    FUnset = static_cast<uint16_t>(rfAllSpecials & ~FSet);
    FText = L"";
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
uint16_t TRights::GetNumber() const
{
  assert(!GetIsUndef());
  return FSet;
}
//---------------------------------------------------------------------------
void TRights::SetRight(TRight Right, bool Value)
{
  SetRightUndef(Right, (Value ? rsYes : rsNo));
}
//---------------------------------------------------------------------------
bool TRights::GetRight(TRight Right) const
{
  TState State = GetRightUndef(Right);
  assert(State != rsUndef);
  return (State == rsYes);
}
//---------------------------------------------------------------------------
void TRights::SetRightUndef(TRight Right, TState Value)
{
  if (Value != GetRightUndef(Right))
  {
    assert((Value != rsUndef) || GetAllowUndef());

    TFlag Flag = RightToFlag(Right);

    switch (Value)
    {
      case rsYes:
        FSet |= static_cast<uint16_t>(Flag);
        FUnset &= static_cast<uint16_t>(~Flag);
        break;

      case rsNo:
        FSet &= static_cast<uint16_t>(~Flag);
        FUnset |= static_cast<uint16_t>(Flag);
        break;

      case rsUndef:
      default:
        FSet &= static_cast<uint16_t>(~Flag);
        FUnset &= static_cast<uint16_t>(~Flag);
        break;
    }

    FText = L"";
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
TRights::TState TRights::GetRightUndef(TRight Right) const
{
  TFlag Flag = RightToFlag(Right);
  TState Result;

  if ((FSet & Flag) != 0)
  {
    Result = rsYes;
  }
  else if ((FUnset & Flag) != 0)
  {
    Result = rsNo;
  }
  else
  {
    Result = rsUndef;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRights::SetReadOnly(bool Value)
{
  SetRight(rrUserWrite, !Value);
  SetRight(rrGroupWrite, !Value);
  SetRight(rrOtherWrite, !Value);
}
//---------------------------------------------------------------------------
bool TRights::GetReadOnly() const
{
  return GetRight(rrUserWrite) && GetRight(rrGroupWrite) && GetRight(rrOtherWrite);
}
//---------------------------------------------------------------------------
UnicodeString TRights::GetSimplestStr() const
{
  if (GetIsUndef())
  {
    return GetModeStr();
  }
  else
  {
    return GetOctal();
  }
}
//---------------------------------------------------------------------------
UnicodeString TRights::GetModeStr() const
{
  UnicodeString Result;
  UnicodeString SetModeStr, UnsetModeStr;
  TRight Right;
  int Index;

  for (int Group = 0; Group < 3; Group++)
  {
    SetModeStr = L"";
    UnsetModeStr = L"";
    for (int Mode = 0; Mode < 3; Mode++)
    {
      Index = (Group * 3) + Mode;
      Right = static_cast<TRight>(rrUserRead + Index);
      switch (GetRightUndef(Right))
      {
        case rsYes:
          SetModeStr += BasicSymbols[Index];
          break;

        case rsNo:
          UnsetModeStr += BasicSymbols[Index];
          break;
      }
    }

    Right = static_cast<TRight>(rrUserIDExec + Group);
    Index = (Group * 3) + 2;
    switch (GetRightUndef(Right))
    {
      case rsYes:
        SetModeStr += CombinedSymbols[Index];
        break;

      case rsNo:
        UnsetModeStr += CombinedSymbols[Index];
        break;
    }

    if (!SetModeStr.IsEmpty() || !UnsetModeStr.IsEmpty())
    {
      if (!Result.IsEmpty())
      {
        Result += L',';
      }
      Result += ModeGroups[Group];
      if (!SetModeStr.IsEmpty())
      {
        Result += L"+" + SetModeStr;
      }
      if (!UnsetModeStr.IsEmpty())
      {
        Result += L"-" + UnsetModeStr;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TRights::AddExecute()
{
  for (int Group = 0; Group < 3; Group++)
  {
    if ((GetRightUndef(static_cast<TRight>(rrUserRead + (Group * 3))) == rsYes) ||
        (GetRightUndef(static_cast<TRight>(rrUserWrite + (Group * 3))) == rsYes))
    {
      SetRight(static_cast<TRight>(rrUserExec + (Group * 3)), true);
    }
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
void TRights::AllUndef()
{
  if ((FSet != 0) || (FUnset != 0))
  {
    FSet = 0;
    FUnset = 0;
    FText = L"";
  }
  FUnknown = false;
}
//---------------------------------------------------------------------------
bool TRights::GetIsUndef() const
{
  return ((FSet | FUnset) != rfAllSpecials);
}
//---------------------------------------------------------------------------
TRights::operator uint16_t() const
{
  return GetNumber();
}
//---------------------------------------------------------------------------
TRights::operator uint32_t() const
{
  return GetNumber();
}
//=== TRemoteProperties -------------------------------------------------------
TRemoteProperties::TRemoteProperties()
{
  Default();
}
//---------------------------------------------------------------------------
TRemoteProperties::TRemoteProperties(const TRemoteProperties & rhp) :
  Valid(rhp.Valid),
  Recursive(rhp.Recursive),
  Rights(rhp.Rights),
  AddXToDirectories(rhp.AddXToDirectories),
  Group(rhp.Group),
  Owner(rhp.Owner),
  Modification(rhp.Modification),
  LastAccess(rhp.Modification)
{
}
//---------------------------------------------------------------------------
void TRemoteProperties::Default()
{
  Valid.Clear();
  AddXToDirectories = false;
  Recursive = false;
  Rights.SetAllowUndef(false);
  Rights.SetNumber(0);
  Group.Clear();
  Owner.Clear();
  Modification = 0;
  LastAccess = 0;
}
//---------------------------------------------------------------------------
bool TRemoteProperties::operator ==(const TRemoteProperties & rhp) const
{
  bool Result = (Valid == rhp.Valid && Recursive == rhp.Recursive);

  if (Result)
  {
    if ((Valid.Contains(vpRights) &&
          (Rights != rhp.Rights || AddXToDirectories != rhp.AddXToDirectories)) ||
        (Valid.Contains(vpOwner) && (Owner != rhp.Owner)) ||
        (Valid.Contains(vpGroup) && (Group != rhp.Group)) ||
        (Valid.Contains(vpModification) && (Modification != rhp.Modification)) ||
        (Valid.Contains(vpLastAccess) && (LastAccess != rhp.LastAccess)))
    {
      Result = false;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TRemoteProperties::operator !=(const TRemoteProperties & rhp) const
{
  return !(*this == rhp);
}
//---------------------------------------------------------------------------
TRemoteProperties TRemoteProperties::CommonProperties(TStrings * FileList)
{
  // TODO: Modification and LastAccess
  TRemoteProperties CommonProperties;
  for (intptr_t Index = 0; Index < FileList->GetCount(); ++Index)
  {
    TRemoteFile * File = NB_STATIC_DOWNCAST(TRemoteFile, FileList->GetObject(Index));
    assert(File);
    if (!Index)
    {
      CommonProperties.Rights.Assign(File->GetRights());
      // previously we allowed undef implicitly for directories,
      // now we do it explicitly in properties dialog and only in combination
      // with "recursive" option
      CommonProperties.Rights.SetAllowUndef(File->GetRights()->GetIsUndef());
      CommonProperties.Valid << vpRights;
      if (File->GetFileOwner().GetIsSet())
      {
        CommonProperties.Owner = File->GetFileOwner();
        CommonProperties.Valid << vpOwner;
      }
      if (File->GetFileGroup().GetIsSet())
      {
        CommonProperties.Group = File->GetFileGroup();
        CommonProperties.Valid << vpGroup;
      }
    }
    else
    {
      CommonProperties.Rights.SetAllowUndef(True);
      CommonProperties.Rights &= *File->GetRights();
      if (CommonProperties.Owner != File->GetFileOwner())
      {
        CommonProperties.Owner.Clear();
        CommonProperties.Valid >> vpOwner;
      }
      if (CommonProperties.Group != File->GetFileGroup())
      {
        CommonProperties.Group.Clear();
        CommonProperties.Valid >> vpGroup;
      }
    }
  }
  return CommonProperties;
}
//---------------------------------------------------------------------------
TRemoteProperties TRemoteProperties::ChangedProperties(
  const TRemoteProperties & OriginalProperties, TRemoteProperties & NewProperties)
{
  // TODO: Modification and LastAccess
  if (!NewProperties.Recursive)
  {
    if (NewProperties.Rights == OriginalProperties.Rights &&
        !NewProperties.AddXToDirectories)
    {
      NewProperties.Valid >> vpRights;
    }

    if (NewProperties.Group == OriginalProperties.Group)
    {
      NewProperties.Valid >> vpGroup;
    }

    if (NewProperties.Owner == OriginalProperties.Owner)
    {
      NewProperties.Valid >> vpOwner;
    }

    NewProperties.Group.SetID(OriginalProperties.Group.GetID());
    NewProperties.Owner.SetID(OriginalProperties.Owner.GetID());
  }
  return NewProperties;
}

TRemoteProperties & TRemoteProperties::operator=(const TRemoteProperties & other)
{
  Valid = other.Valid;
  Recursive = other.Recursive;
  Rights = other.Rights;
  AddXToDirectories = other.AddXToDirectories;
  Group = other.Group;
  Owner = other.Owner;
  Modification = other.Modification;
  LastAccess = other.Modification;
  return *this;
}
//---------------------------------------------------------------------------
void TRemoteProperties::Load(THierarchicalStorage * Storage)
{
  uint8_t Buf[sizeof(Valid)];
  if (static_cast<size_t>(Storage->ReadBinaryData(L"Valid", &Buf, sizeof(Buf))) == sizeof(Buf))
  {
    memmove(&Valid, Buf, sizeof(Valid));
  }

  if (Valid.Contains(vpRights))
  {
    Rights.SetText(Storage->ReadString(L"Rights", Rights.GetText()));
  }

  // TODO
}
//---------------------------------------------------------------------------
void TRemoteProperties::Save(THierarchicalStorage * Storage) const
{
  Storage->WriteBinaryData(UnicodeString(L"Valid"),
    static_cast<const void *>(&Valid), sizeof(Valid));

  if (Valid.Contains(vpRights))
  {
    Storage->WriteString(L"Rights", Rights.GetText());
  }

  // TODO
}
//------------------------------------------------------------------------------
NB_IMPLEMENT_CLASS(TRemoteFile, NB_GET_CLASS_INFO(TPersistent), nullptr);
NB_IMPLEMENT_CLASS(TRemoteFileList, NB_GET_CLASS_INFO(TObjectList), nullptr);
NB_IMPLEMENT_CLASS(TRemoteProperties, NB_GET_CLASS_INFO(TObject), nullptr)
//------------------------------------------------------------------------------
