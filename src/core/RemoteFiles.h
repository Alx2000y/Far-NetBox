#pragma once

#include <map.h>
#include <Sysutils.hpp>

enum TModificationFmt
{
  mfNone,
  mfMDHM,
  mfMDY,
  mfFull
};

#define SYMLINKSTR L" -> "
#define PARENTDIRECTORY L".."
#define THISDIRECTORY L"."
#define ROOTDIRECTORY L"/"
#define FILETYPE_DEFAULT L'-'
#define FILETYPE_SYMLINK L'L'
#define FILETYPE_DIRECTORY L'D'
#define PARTIAL_EXT L".filepart"

class TTerminal;
class TRights;
class TRemoteFileList;
class THierarchicalStorage;

class TRemoteToken : public TObject
{
public:
  TRemoteToken();
  explicit TRemoteToken(const UnicodeString & Name);
  explicit TRemoteToken(const TRemoteToken & rht);

  void Clear();

  bool operator ==(const TRemoteToken & rht) const;
  bool operator !=(const TRemoteToken & rht) const;
  TRemoteToken & operator =(const TRemoteToken & rht);

  intptr_t Compare(const TRemoteToken & rht) const;

  UnicodeString GetName() const { return FName; }
  void SetName(const UnicodeString & Value) { FName = Value; }
  bool GetNameValid() const;
  intptr_t GetID() const { return FID; }
  void SetID(intptr_t Value);
  bool GetIDValid() const { return FIDValid; }
  bool GetIsSet() const;
  UnicodeString GetLogText() const;
  UnicodeString GetDisplayText() const;

private:
  UnicodeString FName;
  intptr_t FID;
  bool FIDValid;
};

class TRemoteTokenList : public TObject
{
public:
  TRemoteTokenList * Duplicate() const;
  void Clear();
  void Add(const TRemoteToken & Token);
  void AddUnique(const TRemoteToken & Token);
  bool Exists(const UnicodeString & Name) const;
  const TRemoteToken * Find(uintptr_t ID) const;
  const TRemoteToken * Find(const UnicodeString & Name) const;
  void Log(TTerminal * Terminal, const wchar_t * Title);

  intptr_t GetCount() const;
  const TRemoteToken * Token(intptr_t Index) const;

private:
  typedef rde::vector<TRemoteToken> TTokens;
  typedef rde::map<UnicodeString, size_t> TNameMap;
  typedef rde::map<intptr_t, size_t> TIDMap;
  TTokens FTokens;
  mutable TNameMap FNameMap;
  mutable TIDMap FIDMap;
};

class TRemoteFile : public TPersistent
{
NB_DISABLE_COPY(TRemoteFile)
NB_DECLARE_CLASS(TRemoteFile)
public:
  explicit TRemoteFile(TRemoteFile * ALinkedByFile = nullptr);
  virtual ~TRemoteFile();
  TRemoteFile * Duplicate(bool Standalone = true) const;

  void ShiftTime(const TDateTime & Difference);
  void Complete();

  TRemoteFileList * GetDirectory() const { return FDirectory; }
  void SetDirectory(TRemoteFileList * Value) { FDirectory = Value; }
  int64_t GetSize() const;
  void SetSize(int64_t Value) { FSize = Value; }
  const TRemoteToken & GetFileOwner() const;
  TRemoteToken & GetFileOwner();
  void SetFileOwner(const TRemoteToken & Value);
  const TRemoteToken & GetFileGroup() const;
  TRemoteToken & GetFileGroup();
  void SetFileGroup(const TRemoteToken & Value);
  UnicodeString GetFileName() const { return FFileName; }
  void SetFileName(const UnicodeString & Value);
  TDateTime GetModification() const { return FModification; }
  TModificationFmt GetModificationFmt() const { return FModificationFmt; }
  void SetModificationFmt(TModificationFmt Value) { FModificationFmt = Value; }
  TDateTime GetLastAccess() const { return FLastAccess; }
  void SetLastAccess(const TDateTime & Value) { FLastAccess = Value; }
  bool GetIsSymLink() const { return FIsSymLink; }
  UnicodeString GetLinkTo() const;
  void SetLinkTo(const UnicodeString & Value);
  TRights * GetRights() const { return FRights; }
  UnicodeString GetHumanRights() const { return FHumanRights; }
  void SetHumanRights(const UnicodeString & Value) { FHumanRights = Value; }
  TTerminal * GetTerminal() const { return FTerminal; }
  bool GetSelected() const { return FSelected; }
  void SetSelected(bool Value) { FSelected = Value; }
  void SetFullFileName(const UnicodeString & Value);

public:
  intptr_t GetAttr() const;
  bool GetBrokenLink() const;
  bool GetIsDirectory() const;
  TRemoteFile * GetLinkedFile() const;
  void SetLinkedFile(TRemoteFile * Value);
  UnicodeString GetModificationStr() const;
  void SetModification(const TDateTime & Value);
  void SetListingStr(const UnicodeString & Value);
  UnicodeString GetListingStr() const;
  UnicodeString GetRightsStr() const;
  wchar_t GetType() const;
  void SetType(wchar_t AType);
  void SetTerminal(TTerminal * Value);
  void SetRights(TRights * Value);
  UnicodeString GetFullFileName() const;
  bool GetHaveFullFileName() const;
  intptr_t GetIconIndex() const;
  UnicodeString GetTypeName() const;
  bool GetIsHidden() const;
  void SetIsHidden(bool Value);
  bool GetIsParentDirectory() const;
  bool GetIsThisDirectory() const;
  bool GetIsInaccesibleDirectory() const;
  UnicodeString GetExtension() const;
  UnicodeString GetUserModificationStr() const;

protected:
  void FindLinkedFile();

private:
  TRemoteFileList * FDirectory;
  TRemoteToken FOwner;
  TModificationFmt FModificationFmt;
  int64_t FSize;
  UnicodeString FFileName;
  int64_t FINodeBlocks;
  TDateTime FModification;
  TDateTime FLastAccess;
  TRemoteToken FGroup;
  intptr_t FIconIndex;
  Boolean FIsSymLink;
  TRemoteFile * FLinkedFile;
  TRemoteFile * FLinkedByFile;
  UnicodeString FLinkTo;
  TRights * FRights;
  UnicodeString FHumanRights;
  TTerminal * FTerminal;
  wchar_t FType;
  bool FSelected;
  bool FCyclicLink;
  UnicodeString FFullFileName;
  int FIsHidden;
  UnicodeString FTypeName;
/*
  int __fastcall GetAttr();
  bool __fastcall GetBrokenLink();
  bool __fastcall GetIsDirectory() const;
  TRemoteFile * __fastcall GetLinkedFile();
  void __fastcall SetLinkedFile(TRemoteFile * value);
  UnicodeString __fastcall GetModificationStr();
  void __fastcall SetModification(const TDateTime & value);
  void __fastcall SetListingStr(UnicodeString value);
  UnicodeString __fastcall GetListingStr();
  UnicodeString __fastcall GetRightsStr();
  wchar_t __fastcall GetType() const;
  void __fastcall SetType(wchar_t AType);
  void __fastcall SetTerminal(TTerminal * value);
  void __fastcall SetRights(TRights * value);
  UnicodeString __fastcall GetFullFileName() const;
  bool __fastcall GetHaveFullFileName() const;
  int __fastcall GetIconIndex() const;
  UnicodeString __fastcall GetTypeName();
  bool __fastcall GetIsHidden();
  void __fastcall SetIsHidden(bool value);
  bool __fastcall GetIsParentDirectory() const;
  bool __fastcall GetIsThisDirectory() const;
  bool __fastcall GetIsInaccesibleDirectory() const;
  UnicodeString __fastcall GetExtension();
  UnicodeString __fastcall GetUserModificationStr();
*/

private:
  void LoadTypeInfo() const;

/*
  __property int Attr = { read = GetAttr };
  __property bool BrokenLink = { read = GetBrokenLink };
  __property TRemoteFileList * Directory = { read = FDirectory, write = FDirectory };
  __property UnicodeString RightsStr = { read = GetRightsStr };
  __property __int64 Size = { read = GetSize, write = FSize };
  __property TRemoteToken Owner = { read = FOwner, write = FOwner };
  __property TRemoteToken Group = { read = FGroup, write = FGroup };
  __property UnicodeString FileName = { read = FFileName, write = FFileName };
  __property int INodeBlocks = { read = FINodeBlocks };
  __property TDateTime Modification = { read = FModification, write = SetModification };
  __property UnicodeString ModificationStr = { read = GetModificationStr };
  __property UnicodeString UserModificationStr = { read = GetUserModificationStr };
  __property TModificationFmt ModificationFmt = { read = FModificationFmt, write = FModificationFmt };
  __property TDateTime LastAccess = { read = FLastAccess, write = FLastAccess };
  __property bool IsSymLink = { read = FIsSymLink };
  __property bool IsDirectory = { read = GetIsDirectory };
  __property TRemoteFile * LinkedFile = { read = GetLinkedFile, write = SetLinkedFile };
  __property UnicodeString LinkTo = { read = FLinkTo, write = FLinkTo };
  __property UnicodeString ListingStr = { read = GetListingStr, write = SetListingStr };
  __property TRights * Rights = { read = FRights, write = SetRights };
  __property UnicodeString HumanRights = { read = FHumanRights, write = FHumanRights };
  __property TTerminal * Terminal = { read = FTerminal, write = SetTerminal };
  __property wchar_t Type = { read = GetType, write = SetType };
  __property bool Selected  = { read=FSelected, write=FSelected };
  __property UnicodeString FullFileName  = { read = GetFullFileName, write = FFullFileName };
  __property bool HaveFullFileName  = { read = GetHaveFullFileName };
  __property int IconIndex = { read = GetIconIndex };
  __property UnicodeString TypeName = { read = GetTypeName };
  __property bool IsHidden = { read = GetIsHidden, write = SetIsHidden };
  __property bool IsParentDirectory = { read = GetIsParentDirectory };
  __property bool IsThisDirectory = { read = GetIsThisDirectory };
  __property bool IsInaccesibleDirectory  = { read=GetIsInaccesibleDirectory };
  __property UnicodeString Extension  = { read=GetExtension };
*/
};

class TRemoteDirectoryFile : public TRemoteFile
{
public:
  TRemoteDirectoryFile();
  virtual ~TRemoteDirectoryFile() {}
};

class TRemoteParentDirectory : public TRemoteDirectoryFile
{
public:
  explicit TRemoteParentDirectory(TTerminal * Terminal);
  virtual ~TRemoteParentDirectory() {}
};

class TRemoteFileList : public TObjectList
{
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
friend class TFTPFileSystem;
friend class TWebDAVFileSystem;
NB_DECLARE_CLASS(TRemoteFileList)
protected:
  UnicodeString FDirectory;
  TDateTime FTimestamp;
  TRemoteFile * GetParentDirectory();
public:
  TRemoteFileList();
  virtual ~TRemoteFileList() { Reset(); }
  virtual void Reset();
  TRemoteFile * FindFile(const UnicodeString & AFileName) const;
  virtual void DuplicateTo(TRemoteFileList * Copy) const;
  virtual void AddFile(TRemoteFile * AFile);
  virtual void AddFiles(const TRemoteFileList * AFileList);
  UnicodeString GetDirectory() const { return FDirectory; }
  virtual void SetDirectory(const UnicodeString & Value);
  TRemoteFile * GetFile(Integer Index) const;
  UnicodeString GetFullDirectory() const;
  Boolean GetIsRoot() const;
  UnicodeString GetParentPath() const;
  int64_t GetTotalSize() const;
  TDateTime GetTimestamp() const { return FTimestamp; }
};

class TRemoteDirectory : public TRemoteFileList
{
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
friend class TWebDAVFileSystem;
NB_DISABLE_COPY(TRemoteDirectory)
public:
  explicit TRemoteDirectory(TTerminal * aTerminal, TRemoteDirectory * Template = nullptr);
  virtual ~TRemoteDirectory();
  virtual void AddFile(TRemoteFile * AFile);
  virtual void DuplicateTo(TRemoteFileList * Copy) const;
  virtual void Reset();
  TTerminal * GetTerminal() const { return FTerminal; }
  void SetTerminal(TTerminal * Value) { FTerminal = Value; }
  TStrings * GetSelectedFiles() const;
  Boolean GetIncludeParentDirectory() const { return FIncludeParentDirectory; }
  void SetIncludeParentDirectory(Boolean Value);
  Boolean GetIncludeThisDirectory() const { return FIncludeThisDirectory; }
  void SetIncludeThisDirectory(Boolean Value);
  void ReleaseRelativeDirectories();
  Boolean GetLoaded() const;
  TRemoteFile * GetParentDirectory() const { return FParentDirectory; }
  TRemoteFile * GetThisDirectory() const { return FThisDirectory; }
  virtual void SetDirectory(const UnicodeString & Value);

private:
  Boolean FIncludeParentDirectory;
  Boolean FIncludeThisDirectory;
  TTerminal * FTerminal;
  mutable TStrings * FSelectedFiles;
  TRemoteFile * FParentDirectory;
  TRemoteFile * FThisDirectory;
};

class TRemoteDirectoryCache : private TStringList
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TRemoteDirectoryCache)
public:
  TRemoteDirectoryCache();
  virtual ~TRemoteDirectoryCache();
  bool HasFileList(const UnicodeString & Directory);
  bool HasNewerFileList(const UnicodeString & Directory, const TDateTime & Timestamp);
  bool GetFileList(const UnicodeString & Directory,
    TRemoteFileList * FileList);
  void AddFileList(TRemoteFileList * FileList);
  void ClearFileList(const UnicodeString & Directory, bool SubDirs);
  void Clear();
  bool GetIsEmpty() const;

protected:
  virtual void Delete(intptr_t Index);

private:
  TCriticalSection FSection;
  void DoClearFileList(const UnicodeString & Directory, bool SubDirs);
};

class TRemoteDirectoryChangesCache : private TStringList
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  explicit TRemoteDirectoryChangesCache(intptr_t MaxSize);
  virtual ~TRemoteDirectoryChangesCache(){}

  void AddDirectoryChange(const UnicodeString & SourceDir,
    const UnicodeString & Change, const UnicodeString & TargetDir);
  void ClearDirectoryChange(const UnicodeString & SourceDir);
  void ClearDirectoryChangeTarget(const UnicodeString & TargetDir);
  bool GetDirectoryChange(const UnicodeString & SourceDir,
    const UnicodeString & Change, UnicodeString & TargetDir) const;
  void Clear();

  void Serialize(UnicodeString & Data) const;
  void Deserialize(const UnicodeString & Data);

  bool GetIsEmpty() const;

private:
  static bool DirectoryChangeKey(const UnicodeString & SourceDir,
    const UnicodeString & Change, UnicodeString & Key);
  void SetValue(const UnicodeString & Name, const UnicodeString & Value);
  UnicodeString GetValue(const UnicodeString & Name) const;
  UnicodeString GetValue(const UnicodeString & Name);

  intptr_t FMaxSize;
};

class TRights : public TObject
{
public:
  static const intptr_t TextLen = 9;
  static const wchar_t UndefSymbol = L'$';
  static const wchar_t UnsetSymbol = L'-';
  static const wchar_t BasicSymbols[];
  static const wchar_t CombinedSymbols[];
  static const wchar_t ExtendedSymbols[];
  static const wchar_t ModeGroups[];
  enum TRight
  {
    rrUserIDExec, rrGroupIDExec, rrStickyBit,
    rrUserRead, rrUserWrite, rrUserExec,
    rrGroupRead, rrGroupWrite, rrGroupExec,
    rrOtherRead, rrOtherWrite, rrOtherExec,
    rrFirst = rrUserIDExec, rrLast = rrOtherExec
  };
  enum TFlag
  {
    rfSetUID =    04000, rfSetGID =      02000, rfStickyBit = 01000,
    rfUserRead =  00400, rfUserWrite =   00200, rfUserExec =  00100,
    rfGroupRead = 00040, rfGroupWrite =  00020, rfGroupExec = 00010,
    rfOtherRead = 00004, rfOtherWrite =  00002, rfOtherExec = 00001,
    rfRead =      00444, rfWrite =       00222, rfExec =      00111,
    rfNo =        00000, rfDefault =     00644, rfAll =       00777,
    rfSpecials =  07000, rfAllSpecials = 07777
  };
  enum TUnsupportedFlag
  {
    rfDirectory = 040000
  };
  enum TState
  {
    rsNo,
    rsYes,
    rsUndef
  };

public:
  static TFlag RightToFlag(TRight Right);

  TRights();
  TRights(const TRights & Source);
  explicit TRights(uint16_t Number);
  void Assign(const TRights * Source);
  void AddExecute();
  void AllUndef();

  bool operator ==(const TRights & rhr) const;
  bool operator ==(uint16_t rhr) const;
  bool operator !=(const TRights & rhr) const;
  TRights & operator =(const TRights & rhr);
  TRights & operator =(uint16_t rhr);
  TRights operator ~() const;
  TRights operator &(uint16_t rhr) const;
  TRights operator &(const TRights & rhr) const;
  TRights & operator &=(uint16_t rhr);
  TRights & operator &=(const TRights & rhr);
  TRights operator |(uint16_t rhr) const;
  TRights operator |(const TRights & rhr) const;
  TRights & operator |=(uint16_t rhr);
  TRights & operator |=(const TRights & rhr);
  operator uint16_t() const;
  operator uint32_t() const;

  bool GetIsUndef() const;
  UnicodeString GetModeStr() const;
  UnicodeString GetSimplestStr() const;
  void SetNumber(uint16_t Value);
  UnicodeString GetText() const;
  void SetText(const UnicodeString & Value);
  void SetOctal(const UnicodeString & AValue);
  uint16_t GetNumber() const;
  uint16_t GetNumberSet() const { return FSet; }
  uint16_t GetNumberUnset() const { return FUnset; }
  uint32_t GetNumberDecadic() const;
  UnicodeString GetOctal() const;
  bool GetReadOnly() const;
  bool GetRight(TRight Right) const;
  TState GetRightUndef(TRight Right) const;
  void SetAllowUndef(bool Value);
  void SetReadOnly(bool Value);
  void SetRight(TRight Right, bool Value);
  void SetRightUndef(TRight Right, TState Value);
  bool GetAllowUndef() const { return FAllowUndef; }
  bool GetUnknown() const { return FUnknown; }

private:
  bool FAllowUndef;
  uint16_t FSet;
  uint16_t FUnset;
  UnicodeString FText;
  bool FUnknown;
};

enum TValidProperty
{
  vpRights = 0x1,
  vpGroup = 0x2,
  vpOwner = 0x4,
  vpModification = 0x8,
  vpLastAccess = 0x10,
};
// FIXME
// typedef Set<TValidProperty, vpRights, vpLastAccess> TValidProperties;
class TValidProperties : public TObject
{
public:
  TValidProperties() :
    FValue(0)
  {
  }
  void Clear()
  {
    FValue = 0;
  }
  bool Contains(TValidProperty Value) const
  {
    return (FValue & Value) != 0;
  }
  bool operator == (const TValidProperties & rhs) const
  {
    return FValue == rhs.FValue;
  }
  bool operator != (const TValidProperties & rhs) const
  {
    return !(operator == (rhs));
  }
  TValidProperties & operator << (const TValidProperty Value)
  {
    FValue |= Value;
    return *this;
  }
  TValidProperties & operator >> (const TValidProperty Value)
  {
    FValue &= ~(static_cast<int64_t>(Value));
    return *this;
  }
  bool Empty() const
  {
    return FValue == 0;
  }

private:
  int64_t FValue;
};

class TRemoteProperties : public TObject
{
NB_DECLARE_CLASS(TRemoteProperties)
public:
  TValidProperties Valid;
  bool Recursive;
  TRights Rights;
  bool AddXToDirectories;
  TRemoteToken Group;
  TRemoteToken Owner;
  int64_t Modification; // unix time
  int64_t LastAccess; // unix time

  TRemoteProperties();
  TRemoteProperties(const TRemoteProperties & rhp);
  bool operator ==(const TRemoteProperties & rhp) const;
  bool operator !=(const TRemoteProperties & rhp) const;
  void Default();
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;

  static TRemoteProperties CommonProperties(TStrings * AFileList);
  static TRemoteProperties ChangedProperties(const TRemoteProperties & OriginalProperties, TRemoteProperties & NewProperties);

public:
  TRemoteProperties & operator=(const TRemoteProperties & other);
};

namespace core {

bool IsUnixStyleWindowsPath(const UnicodeString & APath);
bool UnixIsAbsolutePath(const UnicodeString & APath);
UnicodeString UnixIncludeTrailingBackslash(const UnicodeString & APath);
UnicodeString UnixExcludeTrailingBackslash(const UnicodeString & APath, bool Simple = false);
UnicodeString SimpleUnixExcludeTrailingBackslash(const UnicodeString & APath);
UnicodeString UnixExtractFileDir(const UnicodeString & APath);
UnicodeString UnixExtractFilePath(const UnicodeString & APath);
UnicodeString UnixExtractFileName(const UnicodeString & APath);
UnicodeString UnixExtractFileExt(const UnicodeString & APath);
Boolean UnixSamePath(const UnicodeString & APath1, const UnicodeString & APath2);
bool UnixIsChildPath(const UnicodeString & Parent, const UnicodeString & Child);
bool ExtractCommonPath(const TStrings * AFiles, OUT UnicodeString & APath);
bool UnixExtractCommonPath(const TStrings * AFiles, OUT UnicodeString & APath);
UnicodeString ExtractFileName(const UnicodeString & APath, bool Unix);
bool IsUnixRootPath(const UnicodeString & APath);
bool IsUnixHiddenFile(const UnicodeString & APath);
UnicodeString AbsolutePath(const UnicodeString & Base, const UnicodeString & APath);
UnicodeString FromUnixPath(const UnicodeString & APath);
UnicodeString ToUnixPath(const UnicodeString & APath);
UnicodeString MinimizeName(const UnicodeString & AFileName, intptr_t MaxLen, bool Unix);
UnicodeString MakeFileList(const TStrings * AFileList);
TDateTime ReduceDateTimePrecision(const TDateTime & DateTime,
  TModificationFmt Precision);
TModificationFmt LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2);
UnicodeString UserModificationStr(const TDateTime & DateTime,
  TModificationFmt Precision);
UnicodeString ModificationStr(const TDateTime & DateTime,
  TModificationFmt Precision);
int FakeFileImageIndex(const UnicodeString & AFileName, uint32_t Attrs = INVALID_FILE_ATTRIBUTES,
  UnicodeString * TypeName = nullptr);
bool SameUserName(const UnicodeString & UserName1, const UnicodeString & UserName2);

} // namespace core

