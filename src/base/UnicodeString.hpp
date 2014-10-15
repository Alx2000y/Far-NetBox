#pragma once

#include <stdint.h>
#include <string>

#include <nbglobals.h>
#include "local.hpp"

class RawByteString;
class UnicodeString;
class AnsiString;

class UTF8String
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  UTF8String() {}
  explicit UTF8String(const UTF8String & rht);
  explicit UTF8String(const UnicodeString & Str);
  UTF8String(const wchar_t * Str);
  UTF8String(const wchar_t * Str, intptr_t Size);
  UTF8String(const char * Str, intptr_t Size);

  ~UTF8String() {}

  operator const wchar_t * () const { return c_str(); }
  const wchar_t * c_str() const { return Data.c_str(); }
  intptr_t Length() const { return Data.size(); }
  intptr_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  void SetLength(intptr_t nLength) { Data.resize(nLength); }
  UTF8String & Delete(intptr_t Index, intptr_t Count);
  UTF8String & Insert(const wchar_t * Str, intptr_t Pos);
  UTF8String SubString(intptr_t Pos, intptr_t Len = -1) const;

  intptr_t Pos(wchar_t Ch) const;

public:
  UTF8String & operator=(const UnicodeString & StrCopy);
  UTF8String & operator=(const UTF8String & StrCopy);
  UTF8String & operator=(const RawByteString & StrCopy);
  UTF8String & operator=(const char * lpszData);
  UTF8String & operator=(const wchar_t * lpwszData);
  UTF8String & operator=(wchar_t chData);

  UTF8String operator +(const UTF8String & rhs) const;
  UTF8String operator +(const RawByteString & rhs) const;
  UTF8String & operator +=(const UTF8String & rhs);
  UTF8String & operator +=(const RawByteString & rhs);
  UTF8String & operator +=(const char rhs);
  UTF8String & operator +=(const char * rhs);

  friend bool operator ==(const UTF8String & lhs, const UTF8String & rhs);
  friend bool operator !=(const UTF8String & lhs, const UTF8String & rhs);

private:
  void Init(const wchar_t * Str, intptr_t Length);
  void Init(const char * Str, intptr_t Length);

  typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, custom_nballocator_t<wchar_t> > wstring_t;
  wstring_t Data;
};

class UnicodeString
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  UnicodeString() {}
  UnicodeString(const wchar_t * Str) { Init(Str, ::StrLength(Str)); }
  UnicodeString(const wchar_t * Str, intptr_t Size) { Init(Str, Size); }
  UnicodeString(const wchar_t Src) { Init(&Src, 1); }
  UnicodeString(const char * Str, intptr_t Size) { Init(Str, Size); }
  UnicodeString(const char * Str) { Init(Str, Str ? strlen(Str) : 0); }
  UnicodeString(intptr_t Size, wchar_t Ch) : Data(Size, Ch) {}

  UnicodeString(const UnicodeString & Str) { Init(Str.c_str(), Str.GetLength()); }
  explicit UnicodeString(const UTF8String & Str) { Init(Str.c_str(), Str.GetLength()); }
  explicit UnicodeString(const AnsiString & Str);

  ~UnicodeString() {}

  const wchar_t * c_str() const { return Data.c_str(); }
  const wchar_t * data() const { return Data.c_str(); }
  intptr_t Length() const { return Data.size(); }
  intptr_t GetLength() const { return Length(); }
  intptr_t GetBytesCount() const { return (Length() + 1) * sizeof(wchar_t); }
  bool IsEmpty() const { return Length() == 0; }
  void SetLength(intptr_t nLength) { Data.resize(nLength); }
  UnicodeString & Delete(intptr_t Index, intptr_t Count) { Data.erase(Index - 1, Count); return *this; }
  UnicodeString & Clear() { Data.clear(); return *this; }

  UnicodeString & Lower(intptr_t nStartPos = 1, intptr_t nLength = -1);
  UnicodeString & Upper(intptr_t nStartPos = 1, intptr_t nLength = -1);

  UnicodeString & LowerCase() { return Lower(); }
  UnicodeString & UpperCase() { return Upper(); }

  intptr_t Compare(const UnicodeString & Str) const;
  intptr_t CompareIC(const UnicodeString & Str) const;
  intptr_t ToInt() const;
  intptr_t FindFirstNotOf(const wchar_t * Str) const { return Data.find_first_not_of(Str); }

  UnicodeString & Replace(intptr_t Pos, intptr_t Len, const wchar_t * Str, intptr_t DataLen);
  UnicodeString & Replace(intptr_t Pos, intptr_t Len, const UnicodeString & Str) { return Replace(Pos, Len, Str.c_str(), Str.GetLength()); }
  UnicodeString & Replace(intptr_t Pos, intptr_t Len, const wchar_t * Str) { return Replace(Pos, Len, Str, ::StrLength(NullToEmpty(Str))); }
  UnicodeString & Replace(intptr_t Pos, intptr_t Len, wchar_t Ch) { return Replace(Pos, Len, &Ch, 1); }
  UnicodeString & Replace(intptr_t Pos, wchar_t Ch) { return Replace(Pos, 1, &Ch, 1); }

  UnicodeString & Append(const wchar_t * Str, intptr_t StrLen) { return Replace(GetLength(), 0, Str, StrLen); }
  UnicodeString & Append(const UnicodeString & Str) { return Append(Str.c_str(), Str.GetLength()); }
  UnicodeString & Append(const wchar_t * Str) { return Append(Str, ::StrLength(NullToEmpty(Str))); }
  UnicodeString & Append(const wchar_t Ch) { return Append(&Ch, 1); }
  UnicodeString & Append(const char * lpszAdd, UINT CodePage=CP_OEMCP);

  UnicodeString & Insert(intptr_t Pos, const wchar_t * Str, intptr_t StrLen);
  UnicodeString & Insert(intptr_t Pos, const UnicodeString & Str) { return Insert(Pos, Str.c_str(), Str.Length()); }
  UnicodeString & Insert(const wchar_t * Str, intptr_t Pos) { return Insert(Pos, Str, wcslen(NullToEmpty(Str))); }
  UnicodeString & Insert(const wchar_t Ch, intptr_t Pos) { return Insert(Pos, &Ch, 1); }
  UnicodeString & Insert(const UnicodeString & Str, intptr_t Pos) { return Insert(Pos, Str); }

  intptr_t Pos(wchar_t Ch) const;
  intptr_t Pos(const UnicodeString & Str) const;

  intptr_t RPos(wchar_t Ch) const { return Data.find_last_of(Ch) + 1; }
  bool RPos(intptr_t & nPos, wchar_t Ch, intptr_t nStartPos = 0) const;

  UnicodeString SubStr(intptr_t Pos, intptr_t Len = -1) const;
  UnicodeString SubString(intptr_t Pos, intptr_t Len = -1) const;

  bool IsDelimiter(const UnicodeString & Chars, intptr_t Pos) const;
  intptr_t LastDelimiter(const UnicodeString & Delimiters) const;

  UnicodeString Trim() const;
  UnicodeString TrimLeft() const;
  UnicodeString TrimRight() const;

  void sprintf(const wchar_t * fmt, ...);

public:
  UnicodeString & operator=(const UnicodeString & StrCopy);
  UnicodeString & operator=(const RawByteString & StrCopy);
  UnicodeString & operator=(const AnsiString & StrCopy);
  UnicodeString & operator=(const UTF8String & StrCopy);
  UnicodeString & operator=(const wchar_t * lpwszData);
  UnicodeString & operator=(const char * lpszData);
  UnicodeString & operator=(const wchar_t Ch);

  UnicodeString operator +(const UnicodeString & rhs) const;
  UnicodeString operator +(const RawByteString & rhs) const;
  UnicodeString operator +(const AnsiString & rhs) const;
  UnicodeString operator +(const UTF8String & rhs) const;

  friend UnicodeString operator +(const wchar_t lhs, const UnicodeString & rhs);
  friend UnicodeString operator +(const UnicodeString & lhs, wchar_t rhs);
  friend UnicodeString operator +(const wchar_t * lhs, const UnicodeString & rhs);
  friend UnicodeString operator +(const UnicodeString & lhs, const wchar_t * rhs);
  friend UnicodeString operator +(const UnicodeString & lhs, const char * rhs);

  UnicodeString & operator +=(const UnicodeString & rhs);
  UnicodeString & operator +=(const wchar_t * rhs);
  UnicodeString & operator +=(const UTF8String & rhs);
  UnicodeString & operator +=(const RawByteString & rhs);
  UnicodeString & operator +=(const char rhs);
  UnicodeString & operator +=(const char * rhs);
  UnicodeString & operator +=(const wchar_t rhs);

  bool operator ==(const UnicodeString & Str) const { return Data == Str.Data; }
  bool operator !=(const UnicodeString & Str) const { return Data != Str.Data; }

  friend bool operator ==(const UnicodeString & lhs, const wchar_t * rhs);
  friend bool operator ==(const wchar_t * lhs, const UnicodeString & rhs);
  friend bool operator !=(const UnicodeString & lhs, const wchar_t * rhs);
  friend bool operator !=(const wchar_t * lhs, const UnicodeString & rhs);

  wchar_t operator [](intptr_t Idx) const;
  wchar_t & operator [](intptr_t Idx);

private:
  void Init(const wchar_t * Str, intptr_t Length);
  void Init(const char * Str, intptr_t Length);
  void ThrowIfOutOfRange(intptr_t Idx) const;

  typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, custom_nballocator_t<wchar_t> > wstring_t;
  wstring_t Data;
};

class RawByteString;

class AnsiString
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  AnsiString() {}
  AnsiString(const AnsiString & rht);
  AnsiString(intptr_t Size, char Ch) : Data(Size, Ch) {}
  AnsiString(const wchar_t * Str) { Init(Str, ::StrLength(Str)); }
  AnsiString(const wchar_t * Str, intptr_t Size) { Init(Str, Size); }
  AnsiString(const char * Str) { Init(Str, Str ? strlen(Str) : 0); }
  AnsiString(const char * Str, intptr_t Size) { Init(Str, Size); }
  AnsiString(const uint8_t * Str) { Init(Str, Str ? strlen(reinterpret_cast<const char *>(Str)) : 0); }
  AnsiString(const uint8_t * Str, intptr_t Size) { Init(Str, Size); }
  explicit AnsiString(const UnicodeString & Str) { Init(Str.c_str(), Str.GetLength()); }
  explicit AnsiString(const UTF8String & Str) { Init(Str.c_str(), Str.GetLength()); }
  explicit AnsiString(const RawByteString & Str);
  ~AnsiString() {}

  const char * c_str() const { return Data.c_str(); }
  intptr_t Length() const { return Data.size(); }
  intptr_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  void SetLength(intptr_t nLength) { Data.resize(nLength); }
  AnsiString & Delete(intptr_t Index, intptr_t Count) { Data.erase(Index - 1, Count); return *this; }
  AnsiString & Clear() { Data.clear(); return *this; }
  AnsiString & Insert(const char * Str, intptr_t Pos);
  AnsiString SubString(intptr_t Pos, intptr_t Len = -1) const;

  intptr_t Pos(wchar_t Ch) const;
  intptr_t Pos(const wchar_t * Str) const;

  char operator [](intptr_t Idx) const;
  char & operator [](intptr_t Idx);

  AnsiString & Append(const char * Str, intptr_t StrLen) { Data.append(Str, StrLen); return *this; }
  AnsiString & Append(const AnsiString & Str) { return Append(Str.c_str(), Str.GetLength()); }
  AnsiString & Append(const char * Str) { return Append(Str, strlen(Str ? Str : "")); }
  AnsiString & Append(const char Ch) { return Append(&Ch, 1); }

public:
  AnsiString & operator=(const UnicodeString & strCopy);
  AnsiString & operator=(const RawByteString & strCopy);
  AnsiString & operator=(const AnsiString & strCopy);
  AnsiString & operator=(const UTF8String & strCopy);
  AnsiString & operator=(const char * lpszData);
  AnsiString & operator=(const wchar_t * lpwszData);
  AnsiString & operator=(wchar_t chData);

  AnsiString operator +(const UnicodeString & rhs) const;
  AnsiString operator +(const AnsiString & rhs) const;

  AnsiString & operator +=(const AnsiString & rhs);
  AnsiString & operator +=(const char Ch);
  AnsiString & operator +=(const char * rhs);

  // bool operator ==(const AnsiString & Str) const { return Data == Str.Data; }
  // bool operator !=(const AnsiString & Str) const { return Data != Str.Data; }

  friend bool operator ==(const AnsiString & lhs, const AnsiString & rhs)
  { return strcmp(lhs.Data.c_str(), rhs.Data.c_str()) == 0; }
  friend bool operator !=(const AnsiString & lhs, const AnsiString & rhs)
  { return strcmp(lhs.Data.c_str(), rhs.Data.c_str()) != 0; }

  friend bool operator ==(const AnsiString & lhs, const char * rhs)
  { return strcmp(lhs.Data.c_str(), rhs ? rhs : "") == 0; }
  friend bool operator ==(const char * lhs, const AnsiString & rhs)
  { return strcmp(lhs ? lhs : "", rhs.Data.c_str()) == 0; }
  friend bool operator !=(const AnsiString & lhs, const char * rhs)
  { return strcmp(lhs.Data.c_str(), rhs ? rhs : "") != 0; }
  friend bool operator !=(const char * lhs, const AnsiString & rhs)
  { return strcmp(lhs ? lhs : "", rhs.Data.c_str()) != 0; }

private:
  void Init(const wchar_t * Str, intptr_t Length);
  void Init(const char * Str, intptr_t Length);
  void Init(const uint8_t * Str, intptr_t Length);
  void ThrowIfOutOfRange(intptr_t Idx) const;

  typedef std::basic_string<char, std::char_traits<char>, custom_nballocator_t<char> > string_t;
  string_t Data;
};

class RawByteString
{
  CUSTOM_MEM_ALLOCATION_IMPL
public:
  RawByteString() {}
  RawByteString(const wchar_t * Str) { Init(Str, ::StrLength(Str)); }
  RawByteString(const wchar_t * Str, intptr_t Size) { Init(Str, Size); }
  RawByteString(const char * Str) { Init(Str, Str ? strlen(Str) : 0); }
  RawByteString(const char * Str, intptr_t Size) { Init(Str, Size); }
  RawByteString(const uint8_t * Str) { Init(Str, Str ? strlen(reinterpret_cast<const char *>(Str)) : 0); }
  RawByteString(const uint8_t * Str, intptr_t Size) { Init(Str, Size); }
  RawByteString(const UnicodeString & Str) { Init(Str.c_str(), Str.GetLength()); }
  RawByteString(const RawByteString & Str) { Init(Str.c_str(), Str.GetLength()); }
  RawByteString(const AnsiString & Str) { Init(Str.c_str(), Str.GetLength()); }
  RawByteString(const UTF8String & Str) { Init(Str.c_str(), Str.GetLength()); }
  ~RawByteString() {}

  operator const char * () const { return reinterpret_cast<const char *>(Data.c_str()); }
  operator UnicodeString() const;
  const char * c_str() const { return reinterpret_cast<const char *>(Data.c_str()); }
  intptr_t Length() const { return Data.size(); }
  intptr_t GetLength() const { return Length(); }
  bool IsEmpty() const { return Length() == 0; }
  void SetLength(intptr_t nLength) { Data.resize(nLength); }
  RawByteString & Clear() { SetLength(0); return *this; }
  RawByteString & Delete(intptr_t Index, intptr_t Count) { Data.erase(Index - 1, Count); return *this; }
  RawByteString & Insert(const char * Str, intptr_t Pos);
  RawByteString SubString(intptr_t Pos, intptr_t Len = -1) const;

  intptr_t Pos(wchar_t Ch) const;
  intptr_t Pos(const wchar_t * Str) const;
  intptr_t Pos(const char Ch) const;
  intptr_t Pos(const char * Ch) const;

public:
  RawByteString & operator=(const UnicodeString & strCopy);
  RawByteString & operator=(const RawByteString & strCopy);
  RawByteString & operator=(const AnsiString & strCopy);
  RawByteString & operator=(const UTF8String & strCopy);
  RawByteString & operator=(const char * lpszData);
  RawByteString & operator=(const wchar_t * lpwszData);
  RawByteString & operator=(wchar_t chData);

  RawByteString operator +(const RawByteString & rhs) const;

  RawByteString & operator +=(const RawByteString & rhs);
  RawByteString & operator +=(const char Ch);

  bool operator ==(const char * rhs) const
  { return strcmp(reinterpret_cast<const char *>(Data.c_str()), rhs) == 0; }
  friend bool operator ==(RawByteString & lhs, RawByteString & rhs)
  { return lhs.Data == rhs.Data; }
  friend bool operator !=(RawByteString & lhs, RawByteString & rhs)
  { return lhs.Data != rhs.Data; }

private:
  void Init(const wchar_t * Str, intptr_t Length);
  void Init(const char * Str, intptr_t Length);
  void Init(const uint8_t * Str, intptr_t Length);

  typedef std::basic_string<uint8_t, std::char_traits<uint8_t>, custom_nballocator_t<uint8_t> > rawstring_t;
  rawstring_t Data;
};

namespace rde {

template<typename S>
bool operator==(const S & lhs, const S & rhs)
{
  return lhs.Compare(rhs) == 0;
}

template<typename S>
bool operator!=(const S & lhs, const S & rhs)
{
  return !(lhs == rhs);
}

template<typename S>
bool operator<(const S & lhs, const S & rhs)
{
  return lhs.Compare(rhs) < 0;
}

template<typename S>
bool operator>(const S & lhs, const S & rhs)
{
  return lhs.Compare(rhs) > 0;
}

}  // namespace rde

