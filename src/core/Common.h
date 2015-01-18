
#pragma once

#include <CoreDefs.hpp>
#include <headers.hpp>

#include "Exceptions.h"

inline void ThrowExtException() { throw ExtException(static_cast<Exception *>(nullptr), UnicodeString(L"")); }
//#define EXCEPTION throw ExtException(nullptr, L"")
#define THROWOSIFFALSE(C) { if (!(C)) ::RaiseLastOSError(); }
#define SAFE_DESTROY_EX(CLASS, OBJ) { CLASS * PObj = OBJ; OBJ = nullptr; delete PObj; }
#define SAFE_DESTROY(OBJ) SAFE_DESTROY_EX(TObject, OBJ)
//#define NULL_TERMINATE(S) S[_countof(S) - 1] = L'\0'
//#define ASCOPY(dest, source) \
//  { \
//    AnsiString CopyBuf = ::W2MB(source).c_str(); \
//    strncpy(dest, CopyBuf.c_str(), _countof(dest)); \
//    dest[_countof(dest)-1] = '\0'; \
//  }
#define FORMAT(S, ...) ::Format(S, ##__VA_ARGS__)
#define FMTLOAD(Id, ...) ::FmtLoadStr(Id, ##__VA_ARGS__)
//#define LENOF(x) ( (sizeof((x))) / (sizeof(*(x))))
#define FLAGSET(SET, FLAG) (((SET) & (FLAG)) == (FLAG))
#define FLAGCLEAR(SET, FLAG) (((SET) & (FLAG)) == 0)
#define FLAGMASK(ENABLE, FLAG) ((ENABLE) ? (FLAG) : 0)
#define SWAP(TYPE, FIRST, SECOND) \
  { TYPE __Backup = FIRST; FIRST = SECOND; SECOND = __Backup; }

extern const wchar_t EngShortMonthNames[12][4];
#define CONST_BOM "\xEF\xBB\xBF"
extern const wchar_t TokenPrefix;
extern const wchar_t NoReplacement;
extern const wchar_t TokenReplacement;
#define LOCAL_INVALID_CHARS "/\\:*?\"<>|"
#define PASSWORD_MASK "***"

UnicodeString ReplaceChar(const UnicodeString & Str, wchar_t A, wchar_t B);
UnicodeString DeleteChar(const UnicodeString & Str, wchar_t C);
void PackStr(UnicodeString & Str);
void PackStr(RawByteString & Str);
void Shred(UnicodeString & Str);
UnicodeString MakeValidFileName(const UnicodeString & AFileName);
UnicodeString RootKeyToStr(HKEY RootKey);
UnicodeString BooleanToStr(bool B);
UnicodeString BooleanToEngStr(bool B);
UnicodeString DefaultStr(const UnicodeString & Str, const UnicodeString & Default);
UnicodeString CutToChar(UnicodeString & Str, wchar_t Ch, bool Trim);
UnicodeString CopyToChars(const UnicodeString & Str, intptr_t & From,
  const UnicodeString & Chs, bool Trim,
  wchar_t * Delimiter = nullptr, bool DoubleDelimiterEscapes = false);
UnicodeString CopyToChar(const UnicodeString & Str, wchar_t Ch, bool Trim);
UnicodeString DelimitStr(const UnicodeString & Str, const UnicodeString & Chars);
UnicodeString ShellDelimitStr(const UnicodeString & Str, wchar_t Quote);
UnicodeString ExceptionLogString(Exception *E);
UnicodeString MainInstructions(const UnicodeString & S);
UnicodeString MainInstructionsFirstParagraph(const UnicodeString & S);
bool ExtractMainInstructions(UnicodeString & S, UnicodeString & MainInstructions);
UnicodeString RemoveMainInstructionsTag(const UnicodeString & S);
UnicodeString UnformatMessage(const UnicodeString & S);
UnicodeString RemoveInteractiveMsgTag(const UnicodeString & S);
bool IsNumber(const UnicodeString & Str);
UnicodeString SystemTemporaryDirectory();
UnicodeString GetShellFolderPath(int CSIdl);
UnicodeString StripPathQuotes(const UnicodeString & APath);
UnicodeString AddPathQuotes(const UnicodeString & APath);
void SplitCommand(const UnicodeString & Command, UnicodeString & Program,
  UnicodeString & Params, UnicodeString & Dir);
UnicodeString ValidLocalFileName(const UnicodeString & AFileName);
UnicodeString ValidLocalFileName(
  const UnicodeString & AFileName, wchar_t InvalidCharsReplacement,
  const UnicodeString & TokenizibleChars, const UnicodeString & LocalInvalidChars);
UnicodeString ExtractProgram(const UnicodeString & Command);
UnicodeString ExtractProgramName(const UnicodeString & Command);
UnicodeString FormatCommand(const UnicodeString & Program, const UnicodeString & Params);
UnicodeString ExpandFileNameCommand(const UnicodeString & Command,
  const UnicodeString & AFileName);
void ReformatFileNameCommand(UnicodeString & Command);
UnicodeString EscapePuttyCommandParam(const UnicodeString & Param);
UnicodeString ExpandEnvironmentVariables(const UnicodeString & Str);
bool ComparePaths(const UnicodeString & APath1, const UnicodeString & APath2);
bool CompareFileName(const UnicodeString & APath1, const UnicodeString & APath2);
int CompareLogicalText(const UnicodeString & S1, const UnicodeString & S2);
bool IsReservedName(const UnicodeString & AFileName);
UnicodeString ApiPath(const UnicodeString & APath);
UnicodeString DisplayableStr(const RawByteString & Str);
UnicodeString ByteToHex(uint8_t B, bool UpperCase = true);
UnicodeString BytesToHex(const uint8_t * B, uintptr_t Length, bool UpperCase = true, wchar_t Separator = L'\0');
UnicodeString BytesToHex(const RawByteString & Str, bool UpperCase = true, wchar_t Separator = L'\0');
UnicodeString CharToHex(wchar_t Ch, bool UpperCase = true);
RawByteString HexToBytes(const UnicodeString & Hex);
uint8_t HexToByte(const UnicodeString & Hex);
bool IsLowerCaseLetter(wchar_t Ch);
bool IsUpperCaseLetter(wchar_t Ch);
bool IsLetter(wchar_t Ch);
bool IsDigit(wchar_t Ch);
bool IsHex(wchar_t Ch);
UnicodeString DecodeUrlChars(const UnicodeString & S);
UnicodeString EncodeUrlChars(const UnicodeString & S);
UnicodeString EncodeUrlString(const UnicodeString & S);
UnicodeString EncodeUrlPath(const UnicodeString & S);
UnicodeString AppendUrlParams(const UnicodeString & URL, const UnicodeString & Params);
bool RecursiveDeleteFile(const UnicodeString & AFileName, bool ToRecycleBin);
void DeleteFileChecked(const UnicodeString & AFileName);
uintptr_t CancelAnswer(uintptr_t Answers);
uintptr_t AbortAnswer(uintptr_t Answers);
uintptr_t ContinueAnswer(uintptr_t Answers);
UnicodeString LoadStr(intptr_t Ident, intptr_t MaxLength = 0);
UnicodeString LoadStrPart(intptr_t Ident, intptr_t Part);
UnicodeString EscapeHotkey(const UnicodeString & Caption);
bool CutToken(UnicodeString & Str, UnicodeString & Token,
  UnicodeString * RawToken = nullptr);
void AddToList(UnicodeString & List, const UnicodeString & Value, const UnicodeString & Delimiter);
bool IsWinVista();
bool IsWin7();
bool IsWine();
int64_t Round(double Number);
bool TryRelativeStrToDateTime(const UnicodeString & S, TDateTime & DateTime);
LCID GetDefaultLCID();
UnicodeString DefaultEncodingName();
UnicodeString WindowsProductName();
bool GetWindowsProductType(DWORD & Type);
bool IsDirectoryWriteable(const UnicodeString & APath);
UnicodeString FormatNumber(int64_t Size);
UnicodeString FormatSize(int64_t Size);
UnicodeString ExtractFileBaseName(const UnicodeString & APath);
TStringList * TextToStringList(const UnicodeString & Text);
UnicodeString TrimVersion(const UnicodeString & Version);
UnicodeString FormatVersion(int MajorVersion, int MinorVersion, int SubminorVersion);
TFormatSettings GetEngFormatSettings();
//int ParseShortEngMonthName(const UnicodeString & MonthStr);
// The defaults are equal to defaults of TStringList class (except for Sorted)
TStringList * CreateSortedStringList(bool CaseSensitive = false, TDuplicatesEnum Duplicates = dupIgnore);
UnicodeString FindIdent(const UnicodeString & Ident, TStrings * Idents);

DEFINE_CALLBACK_TYPE3(TProcessLocalFileEvent, void,
  const UnicodeString & /*FileName*/, const TSearchRec & /*Rec*/, void * /*Param*/);
bool FileSearchRec(const UnicodeString & AFileName, TSearchRec & Rec);
struct TSearchRecChecked : public TSearchRec
{
  UnicodeString Path;
};
DWORD FindCheck(DWORD Result, const UnicodeString & APath);
DWORD FindFirstChecked(const UnicodeString & APath, DWORD LocalFileAttrs, TSearchRecChecked & F);
DWORD FindNextChecked(TSearchRecChecked & F);
void ProcessLocalDirectory(const UnicodeString & ADirName,
  TProcessLocalFileEvent CallBackFunc, void * Param = nullptr, DWORD FindAttrs = INVALID_FILE_ATTRIBUTES);

enum TDSTMode
{
  dstmWin  = 0, //
  dstmUnix = 1, // adjust UTC time to Windows "bug"
  dstmKeep = 2
};
bool UsesDaylightHack();
TDateTime EncodeDateVerbose(Word Year, Word Month, Word Day);
TDateTime EncodeTimeVerbose(Word Hour, Word Min, Word Sec, Word MSec);
TDateTime SystemTimeToDateTimeVerbose(const SYSTEMTIME & SystemTime);
TDateTime UnixToDateTime(int64_t TimeStamp, TDSTMode DSTMode);
TDateTime ConvertTimestampToUTC(const TDateTime & DateTime);
TDateTime ConvertTimestampFromUTC(const TDateTime & DateTime);
FILETIME DateTimeToFileTime(const TDateTime & DateTime, TDSTMode DSTMode);
TDateTime AdjustDateTimeFromUnix(const TDateTime & DateTime, TDSTMode DSTMode);
void UnifyDateTimePrecision(TDateTime & DateTime1, TDateTime & DateTime2);
TDateTime FileTimeToDateTime(const FILETIME & FileTime);
int64_t ConvertTimestampToUnix(const FILETIME & FileTime,
  TDSTMode DSTMode);
int64_t ConvertTimestampToUnixSafe(const FILETIME & FileTime,
  TDSTMode DSTMode);
UnicodeString FixedLenDateTimeFormat(const UnicodeString & Format);
UnicodeString StandardTimestamp(const TDateTime & DateTime);
UnicodeString StandardTimestamp();
UnicodeString StandardDatestamp();
UnicodeString FormatTimeZone(intptr_t Sec);
UnicodeString GetTimeZoneLogString();
bool AdjustClockForDSTEnabled();
intptr_t CompareFileTime(const TDateTime & T1, const TDateTime & T2);
intptr_t TimeToMSec(const TDateTime & T);
intptr_t TimeToSeconds(const TDateTime & T);
intptr_t TimeToMinutes(const TDateTime & T);

class TGuard : public TObject
{
NB_DISABLE_COPY(TGuard)
public:
  explicit TGuard(const TCriticalSection & ACriticalSection);
  ~TGuard();

private:
  const TCriticalSection & FCriticalSection;
};

class TUnguard : public TObject
{
NB_DISABLE_COPY(TUnguard)
public:
  explicit TUnguard(TCriticalSection & ACriticalSection);
  ~TUnguard();

private:
  TCriticalSection & FCriticalSection;
};

#define MB_TEXT(x) const_cast<wchar_t *>(::MB2W(x).c_str())
//#define CALLSTACK
//#define CCALLSTACK(TRACING)
//#define TRACING
//#undef TRACE
//#define TRACE(MESSAGE)
//#define TRACEFMT(MESSAGE, ...)
//#define CTRACE(TRACING, MESSAGE)
//#define CTRACEFMT(TRACING, MESSAGE, ...)

#include <assert.h>
//#define ACCESS_VIOLATION_TEST { (*((int*)nullptr)) = 0; }
#ifndef _DEBUG
#undef assert
#define assert(p)   ((void)0)
#define CHECK(p) p
#define FAIL
//#define TRACE_EXCEPT_BEGIN
//#define TRACE_EXCEPT_END
//#define TRACE_CATCH_ALL catch (...)
//#define CLEAN_INLINE
//#define TRACEE_(E)
//#define TRACEE
//#define TRACE_EXCEPT
#define ALWAYS_TRUE(p) p
#define ALWAYS_FALSE(p) p
#define NOT_NULL(P) P
#else
#define CHECK(p) { bool __CHECK_RESULT__ = (p); assert(__CHECK_RESULT__); }
#define FAIL assert(false)
#define ALWAYS_TRUE(p) (p)
#define ALWAYS_FALSE(p) (p)
#define NOT_NULL(P) P
#define CLEAN_INLINE
#endif
#ifndef USEDPARAM
#define USEDPARAM(p) void(p);
#endif

template<class T>
class TValueRestorer : public TObject
{
public:
  inline explicit TValueRestorer(T & Target, const T & Value) :
    FTarget(Target),
    FValue(Value),
    FArmed(true)
  {
  }

  inline explicit TValueRestorer(T & Target) :
    FTarget(Target),
    FValue(Target),
    FArmed(true)
  {
  }

  void Release()
  {
    if (FArmed)
    {
      FTarget = FValue;
      FArmed = false;
    }
  }

  inline ~TValueRestorer()
  {
    Release();
  }

protected:
  T & FTarget;
  T FValue;
  bool FArmed;
};

class TAutoNestingCounter : TValueRestorer<int>
{
public:
  inline explicit TAutoNestingCounter(int & Target) :
    TValueRestorer<int>(Target)
  {
    assert(Target >= 0);
    ++Target;
  }

  inline ~TAutoNestingCounter()
  {
    assert(!FArmed || (FTarget == (FValue + 1)));
  }
};

class TAutoFlag : public TValueRestorer<bool>
{
public:
  TAutoFlag(bool & Target) :
    TValueRestorer<bool>(Target)
  {
    assert(!Target);
    Target = true;
  }

  ~TAutoFlag()
  {
    assert(!FArmed || FTarget);
  }
};

UnicodeString FormatBytes(int64_t Bytes, bool UseOrders = true);

