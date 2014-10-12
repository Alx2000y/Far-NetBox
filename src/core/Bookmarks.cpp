
#include <vcl.h>
#pragma hdrstop
#include <Common.h>
#include "NamedObjs.h"
#include "Bookmarks.h"
#include "Configuration.h"
#include "HierarchicalStorage.h"
#include "TextsCore.h"

TBookmarks::TBookmarks() : TObject()
{
  FSharedKey = TNamedObjectList::HiddenPrefix + L"shared";
  FBookmarkLists = new TStringList();
  FBookmarkLists->SetSorted(true);
  FBookmarkLists->SetCaseSensitive(false);
  FBookmarkLists->SetDuplicates(dupError);
}

TBookmarks::~TBookmarks()
{
  Clear();
  SAFE_DESTROY(FBookmarkLists);
}

void TBookmarks::Clear()
{
  for (intptr_t Index = 0; Index < FBookmarkLists->GetCount(); ++Index)
  {
    TObject * Object = FBookmarkLists->GetObject(Index);
    SAFE_DESTROY(Object);
  }
  FBookmarkLists->Clear();
}

UnicodeString TBookmarks::Keys[] = { L"Local", L"Remote", L"ShortCuts", L"Options" };

void TBookmarks::Load(THierarchicalStorage * Storage)
{
  for (intptr_t Index = 0; Index <= 3; ++Index)
  {
    if (Storage->OpenSubKey(Keys[Index], false))
    {
      std::unique_ptr<TStrings> BookmarkKeys(new TStringList());
      Storage->GetSubKeyNames(BookmarkKeys.get());
      for (intptr_t Index = 0; Index < BookmarkKeys->GetCount(); ++Index)
      {
        UnicodeString Key = BookmarkKeys->GetString(Index);
        if (Storage->OpenSubKey(Key, false))
        {
          TBookmarkList * BookmarkList = GetBookmarks(Key);
          if (BookmarkList == nullptr)
          {
            BookmarkList = new TBookmarkList();
            FBookmarkLists->AddObject(Key, BookmarkList);
          }
          if (Index < 3)
          {
            LoadLevel(Storage, L"", Index, BookmarkList);
          }
          else
          {
            BookmarkList->LoadOptions(Storage);
          }
          Storage->CloseSubKey();
        }
      }
      Storage->CloseSubKey();
    }
  }

  ModifyAll(false);
}

void TBookmarks::LoadLevel(THierarchicalStorage * Storage, const UnicodeString & Key,
  intptr_t AIndex, TBookmarkList * BookmarkList)
{
  std::unique_ptr<TStrings> Names(new TStringList());
  Storage->GetValueNames(Names.get());
  UnicodeString Name;
  UnicodeString Directory;
  TShortCut ShortCut(0);
  for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
  {
    Name = Names->GetString(Index);
    bool IsDirectory = (AIndex == 0) || (AIndex == 1);
    if (IsDirectory)
    {
      Directory = Storage->ReadString(Name, L"");
    }
    else
    {
      Directory = L""; // use only in case of malformed config
      ShortCut = static_cast<TShortCut>(Storage->ReadInteger(Name, 0));
    }
    if (Name.ToInt() > 0)
    {
      assert(IsDirectory); // unless malformed
      Name = Directory;
    }
    if (!Name.IsEmpty())
    {
      TBookmark * Bookmark = BookmarkList->FindByName(Key, Name);
      bool New = (Bookmark == nullptr);
      if (New)
      {
        Bookmark = new TBookmark();
        Bookmark->SetNode(Key);
        Bookmark->SetName(Name);
      }
      switch (AIndex)
      {
        case 0:
          Bookmark->SetLocal(Directory);
          break;

        case 1:
          Bookmark->SetRemote(Directory);
          break;

        case 2:
          Bookmark->SetShortCut(ShortCut);
          break;
      }
      if (New)
      {
        BookmarkList->Add(Bookmark);
      }
    }
  }

  Storage->GetSubKeyNames(Names.get());
  for (intptr_t Index = 0; Index < Names->GetCount(); ++Index)
  {
    Name = Names->GetString(Index);
    if (Storage->OpenSubKey(Name, false))
    {
      LoadLevel(Storage, Key + (Key.IsEmpty() ? L"" : L"/") + Name, AIndex, BookmarkList);
      Storage->CloseSubKey();
    }
  }
}

void TBookmarks::Save(THierarchicalStorage * Storage, bool All)
{
  for (intptr_t Idx = 0; Idx <= 3; Idx++)
  {
    if (Storage->OpenSubKey(Keys[Idx], true))
    {
      for (intptr_t Index = 0; Index < FBookmarkLists->GetCount(); ++Index)
      {
        TBookmarkList * BookmarkList = NB_STATIC_DOWNCAST(TBookmarkList, FBookmarkLists->GetObject(Index));
        if (All || BookmarkList->GetModified())
        {
          UnicodeString Key;
          Key = FBookmarkLists->GetString(Index);
          Storage->RecursiveDeleteSubKey(Key);
          if (Storage->OpenSubKey(Key, true))
          {
            if (Idx < 3)
            {
              for (intptr_t IndexB = 0; IndexB < BookmarkList->GetCount(); IndexB++)
              {
                TBookmark * Bookmark = BookmarkList->GetBookmarks(IndexB);
                // avoid creating empty subfolder if there's no shortcut
                if ((Idx == 0) || (Idx == 1) ||
                    ((Idx == 2) && (Bookmark->GetShortCut() != 0)))
                {
                  bool HasNode = !Bookmark->GetNode().IsEmpty();
                  if (!HasNode || Storage->OpenSubKey(Bookmark->GetNode(), true))
                  {
                    switch (Idx)
                    {
                      case 0:
                        Storage->WriteString(Bookmark->GetName(), Bookmark->GetLocal());
                        break;

                      case 1:
                        Storage->WriteString(Bookmark->GetName(), Bookmark->GetRemote());
                        break;

                      case 2:
                        assert(Bookmark->GetShortCut() != 0);
                        Storage->WriteInteger(Bookmark->GetName(), Bookmark->GetShortCut());
                        break;
                    }

                    if (HasNode)
                    {
                      Storage->CloseSubKey();
                    }
                  }
                }
              }
            }
            else
            {
              BookmarkList->SaveOptions(Storage);
            }
            Storage->CloseSubKey();
          }
        }
      }
      Storage->CloseSubKey();
    }
  }

  if (!All)
  {
    ModifyAll(false);
  }
}

void TBookmarks::ModifyAll(bool Modify)
{
  for (intptr_t Index = 0; Index < FBookmarkLists->GetCount(); ++Index)
  {
    TBookmarkList * BookmarkList = NB_STATIC_DOWNCAST(TBookmarkList, FBookmarkLists->GetObject(Index));
    assert(BookmarkList);
    BookmarkList->SetModified(Modify);
  }
}

TBookmarkList * TBookmarks::GetBookmarks(const UnicodeString & AIndex)
{
  intptr_t Index = FBookmarkLists->IndexOf(AIndex.c_str());
  if (Index >= 0)
  {
    return NB_STATIC_DOWNCAST(TBookmarkList, FBookmarkLists->GetObject(Index));
  }
  else
  {
    return nullptr;
  }
}

void TBookmarks::SetBookmarks(const UnicodeString & AIndex, TBookmarkList * Value)
{
  intptr_t Index = FBookmarkLists->IndexOf(AIndex.c_str());
  if (Index >= 0)
  {
    TBookmarkList * BookmarkList;
    BookmarkList = NB_STATIC_DOWNCAST(TBookmarkList, FBookmarkLists->GetObject(Index));
    BookmarkList->Assign(Value);
  }
  else
  {
    TBookmarkList * BookmarkList = new TBookmarkList();
    BookmarkList->Assign(Value);
    FBookmarkLists->AddObject(AIndex, BookmarkList);
  }
}

TBookmarkList * TBookmarks::GetSharedBookmarks()
{
  return GetBookmarks(FSharedKey);
}

void TBookmarks::SetSharedBookmarks(TBookmarkList * Value)
{
  SetBookmarks(FSharedKey, Value);
}

TBookmarkList::TBookmarkList(): TPersistent()
{
  FModified = false;
  FBookmarks = new TStringList();
  FBookmarks->SetCaseSensitive(false);
  FOpenedNodes = new TStringList();
  FOpenedNodes->SetCaseSensitive(false);
  FOpenedNodes->SetSorted(true);
}

TBookmarkList::~TBookmarkList()
{
  Clear();
  SAFE_DESTROY(FBookmarks);
  SAFE_DESTROY(FOpenedNodes);
}

void TBookmarkList::Clear()
{
  for (intptr_t Index = 0; Index < FBookmarks->GetCount(); ++Index)
  {
    TObject * Object = FBookmarks->GetObject(Index);
    SAFE_DESTROY(Object);
  }
  FBookmarks->Clear();
  FOpenedNodes->Clear();
}

void TBookmarkList::Assign(const TPersistent * Source)
{
  const TBookmarkList * SourceList = NB_STATIC_DOWNCAST_CONST(TBookmarkList, Source);
  if (SourceList)
  {
    Clear();
    for (intptr_t Index = 0; Index < SourceList->FBookmarks->GetCount(); ++Index)
    {
      TBookmark * Bookmark = new TBookmark();
      Bookmark->Assign(NB_STATIC_DOWNCAST(TBookmark, SourceList->FBookmarks->GetObject(Index)));
      Add(Bookmark);
    }
    FOpenedNodes->Assign(SourceList->FOpenedNodes);
    SetModified(SourceList->GetModified());
  }
  else
  {
    TPersistent::Assign(Source);
  }
}

void TBookmarkList::LoadOptions(THierarchicalStorage * Storage)
{
  FOpenedNodes->SetCommaText(Storage->ReadString(L"OpenedNodes", L""));
}

void TBookmarkList::SaveOptions(THierarchicalStorage * Storage)
{
  Storage->WriteString(L"OpenedNodes", FOpenedNodes->GetCommaText());
}

void TBookmarkList::Add(TBookmark * Bookmark)
{
  Insert(GetCount(), Bookmark);
}

void TBookmarkList::InsertBefore(TBookmark * BeforeBookmark, TBookmark * Bookmark)
{
  assert(BeforeBookmark);
  intptr_t Index = FBookmarks->IndexOf(BeforeBookmark->GetKey().c_str());
  assert(Index >= 0);
  Insert(Index, Bookmark);
}

void TBookmarkList::MoveTo(TBookmark * ToBookmark,
  TBookmark * Bookmark, bool Before)
{
  assert(ToBookmark != nullptr);
  intptr_t NewIndex = FBookmarks->IndexOf(ToBookmark->GetKey().c_str());
  assert(Bookmark != nullptr);
  intptr_t OldIndex = FBookmarks->IndexOf(Bookmark->GetKey().c_str());
  if (Before && (NewIndex > OldIndex))
  {
    // otherwise item is moved after the item in the target index
    NewIndex--;
  }
  else if (!Before && (NewIndex < OldIndex))
  {
    NewIndex++;
  }
  FModified = true;
  FBookmarks->Move(OldIndex, NewIndex);
}

void TBookmarkList::Insert(intptr_t Index, TBookmark * Bookmark)
{
  assert(Bookmark);
  assert(!Bookmark->FOwner);
  assert(!Bookmark->GetName().IsEmpty());

  FModified = true;
  Bookmark->FOwner = this;
  if (FBookmarks->IndexOf(Bookmark->GetKey().c_str()) >= 0)
  {
    throw Exception(FMTLOAD(DUPLICATE_BOOKMARK, Bookmark->GetName().c_str()));
  }
  FBookmarks->InsertObject(Index, Bookmark->GetKey(), Bookmark);
}

void TBookmarkList::Delete(TBookmark *& Bookmark)
{
  assert(Bookmark);
  assert(Bookmark->FOwner == this);
  intptr_t Index = IndexOf(Bookmark);
  assert(Index >= 0);
  FModified = true;
  Bookmark->FOwner = nullptr;
  FBookmarks->Delete(Index);
  SAFE_DESTROY(Bookmark);
}

intptr_t TBookmarkList::IndexOf(TBookmark * Bookmark)
{
  return FBookmarks->IndexOf(Bookmark->GetKey().c_str());
}

void TBookmarkList::KeyChanged(intptr_t Index)
{
  assert(Index < GetCount());
  TBookmark * Bookmark = NB_STATIC_DOWNCAST(TBookmark, FBookmarks->GetObject(Index));
  assert(FBookmarks->GetString(Index) != Bookmark->GetKey());
  if (FBookmarks->IndexOf(Bookmark->GetKey().c_str()) >= 0)
  {
    throw Exception(FMTLOAD(DUPLICATE_BOOKMARK, Bookmark->GetName().c_str()));
  }
  FBookmarks->SetString(Index, Bookmark->GetKey());
}

TBookmark * TBookmarkList::FindByName(const UnicodeString & Node, const UnicodeString & Name)
{
  intptr_t Index = FBookmarks->IndexOf(TBookmark::BookmarkKey(Node, Name).c_str());
  TBookmark * Bookmark = ((Index >= 0) ? NB_STATIC_DOWNCAST(TBookmark, FBookmarks->GetObject(Index)) : nullptr);
  assert(!Bookmark || (Bookmark->GetNode() == Node && Bookmark->GetName() == Name));
  return Bookmark;
}

TBookmark * TBookmarkList::FindByShortCut(TShortCut ShortCut)
{
  for (intptr_t Index = 0; Index < FBookmarks->GetCount(); ++Index)
  {
    if (GetBookmarks(Index)->GetShortCut() == ShortCut)
    {
      return GetBookmarks(Index);
    }
  }
  return nullptr;
}

intptr_t TBookmarkList::GetCount()
{
  return FBookmarks->GetCount();
}

TBookmark * TBookmarkList::GetBookmarks(intptr_t Index)
{
  TBookmark * Bookmark = NB_STATIC_DOWNCAST(TBookmark, FBookmarks->GetObject(Index));
  assert(Bookmark);
  return Bookmark;
}

bool TBookmarkList::GetNodeOpened(const UnicodeString & Index)
{
  return (FOpenedNodes->IndexOf(Index.c_str()) >= 0);
}

void TBookmarkList::SetNodeOpened(const UnicodeString & AIndex, bool Value)
{
  intptr_t Index = FOpenedNodes->IndexOf(AIndex.c_str());
  if ((Index >= 0) != Value)
  {
    if (Value)
    {
      FOpenedNodes->Add(AIndex);
    }
    else
    {
      FOpenedNodes->Delete(Index);
    }
    FModified = true;
  }
}

void TBookmarkList::ShortCuts(TShortCuts & ShortCuts)
{
  for (intptr_t Index = 0; Index < GetCount(); ++Index)
  {
    TBookmark * Bookmark = GetBookmarks(Index);
    if (Bookmark->GetShortCut() != 0)
    {
      ShortCuts.Add(Bookmark->GetShortCut());
    }
  }
}

TBookmark::TBookmark()
{
  FOwner = nullptr;
}

void TBookmark::Assign(const TPersistent * Source)
{
  const TBookmark * SourceBookmark = NB_STATIC_DOWNCAST_CONST(TBookmark, Source);
  if (SourceBookmark)
  {
    SetName(SourceBookmark->GetName());
    SetLocal(SourceBookmark->GetLocal());
    SetRemote(SourceBookmark->GetRemote());
    SetNode(SourceBookmark->GetNode());
    SetShortCut(SourceBookmark->GetShortCut());
  }
  else
  {
    TPersistent::Assign(Source);
  }
}

void TBookmark::SetName(const UnicodeString & Value)
{
  if (GetName() != Value)
  {
    intptr_t OldIndex = FOwner ? FOwner->IndexOf(this) : -1;
    UnicodeString OldName = FName;
    FName = Value;
    try
    {
      Modify(OldIndex);
    }
    catch (...)
    {
      FName = OldName;
      throw;
    }
  }
}

void TBookmark::SetLocal(const UnicodeString & Value)
{
  if (GetLocal() != Value)
  {
    FLocal = Value;
    Modify(-1);
  }
}

void TBookmark::SetRemote(const UnicodeString & Value)
{
  if (GetRemote() != Value)
  {
    FRemote = Value;
    Modify(-1);
  }
}

void TBookmark::SetNode(const UnicodeString & Value)
{
  if (GetNode() != Value)
  {
    intptr_t OldIndex = FOwner ? FOwner->IndexOf(this) : -1;
    FNode = Value;
    Modify(OldIndex);
  }
}

void TBookmark::SetShortCut(TShortCut Value)
{
  if (GetShortCut() != Value)
  {
    FShortCut = Value;
    Modify(-1);
  }
}

void TBookmark::Modify(intptr_t OldIndex)
{
  if (FOwner)
  {
    FOwner->SetModified(true);
    if (OldIndex >= 0)
    {
      FOwner->KeyChanged(OldIndex);
    }
  }
}

UnicodeString TBookmark::BookmarkKey(const UnicodeString & Node, const UnicodeString & Name)
{
  return FORMAT(L"%s\1%s", Node.c_str(), Name.c_str());
}

UnicodeString TBookmark::GetKey()
{
  return BookmarkKey(GetNode(), GetName());
}

NB_IMPLEMENT_CLASS(TBookmark, NB_GET_CLASS_INFO(TPersistent), nullptr);
NB_IMPLEMENT_CLASS(TBookmarkList, NB_GET_CLASS_INFO(TPersistent), nullptr);
