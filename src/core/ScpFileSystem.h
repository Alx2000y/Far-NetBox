
#pragma once

#include <FileSystems.h>
#include <CopyParam.h>

class TCommandSet;
class TSecureShell;

class TSCPFileSystem : public TCustomFileSystem
{
NB_DISABLE_COPY(TSCPFileSystem)
public:
  explicit TSCPFileSystem(TTerminal * ATerminal);
  virtual ~TSCPFileSystem();

  virtual void Init(void *); // TSecureShell *
  virtual void FileTransferProgress(int64_t /* TransferSize */, int64_t /* Bytes */) {}

  virtual void Open();
  virtual void Close();
  virtual bool GetActive() const;
  virtual void CollectUsage();
  virtual void Idle();
  virtual UnicodeString AbsolutePath(const UnicodeString & APath, bool Local);
  virtual UnicodeString AbsolutePath(const UnicodeString & APath, bool Local) const;
  virtual void AnyCommand(const UnicodeString & Command,
    TCaptureOutputEvent OutputEvent);
  virtual void ChangeDirectory(const UnicodeString & Directory);
  virtual void CachedChangeDirectory(const UnicodeString & Directory);
  virtual void AnnounceFileListOperation();
  virtual void ChangeFileProperties(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const TRemoteProperties * Properties,
    TChmodSessionAction & Action);
  virtual bool LoadFilesProperties(TStrings * AFileList);
  virtual void CalculateFilesChecksum(const UnicodeString & Alg,
    TStrings * AFileList, TStrings * Checksums,
    TCalculatedChecksumEvent OnCalculatedChecksum);
  virtual void CopyToLocal(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void CopyToRemote(const TStrings * AFilesToCopy,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress,
    TOnceDoneOperation & OnceDoneOperation);
  virtual void RemoteCreateDirectory(const UnicodeString & ADirName);
  virtual void CreateLink(const UnicodeString & AFileName, const UnicodeString & PointTo, bool Symbolic);
  virtual void RemoteDeleteFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, intptr_t Params, TRmSessionAction & Action);
  virtual void CustomCommandOnFile(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & Command, intptr_t Params, TCaptureOutputEvent OutputEvent);
  virtual void DoStartup();
  virtual void HomeDirectory();
  virtual bool IsCapable(intptr_t Capability) const;
  virtual void LookupUsersGroups();
  virtual void ReadCurrentDirectory();
  virtual void ReadDirectory(TRemoteFileList * FileList);
  virtual void ReadFile(const UnicodeString & AFileName,
    TRemoteFile *& File);
  virtual void ReadSymlink(TRemoteFile * SymlinkFile,
    TRemoteFile *& File);
  virtual void RemoteRenameFile(const UnicodeString & AFileName,
    const UnicodeString & NewName);
  virtual void RemoteCopyFile(const UnicodeString & AFileName,
    const UnicodeString & NewName);
  virtual TStrings * GetFixedPaths();
  virtual void SpaceAvailable(const UnicodeString & APath,
    TSpaceAvailable & ASpaceAvailable);
  virtual const TSessionInfo & GetSessionInfo() const;
  virtual const TFileSystemInfo & GetFileSystemInfo(bool Retrieve);
  virtual bool TemporaryTransferFile(const UnicodeString & AFileName);
  virtual bool GetStoredCredentialsTried() const;
  virtual UnicodeString FSGetUserName() const;

protected:
  TStrings * GetOutput() const { return FOutput; }
  intptr_t GetReturnCode() const { return FReturnCode; }

  virtual UnicodeString GetCurrDirectory() const;

private:
  TSecureShell * FSecureShell;
  TCommandSet * FCommandSet;
  TFileSystemInfo FFileSystemInfo;
  UnicodeString FCurrentDirectory;
  TStrings * FOutput;
  intptr_t FReturnCode;
  UnicodeString FCachedDirectoryChange;
  bool FProcessingCommand;
  int FLsFullTime;
  TCaptureOutputEvent FOnCaptureOutput;

  void ClearAliases();
  void ClearAlias(const UnicodeString & Alias);
  void CustomReadFile(const UnicodeString & AFileName,
    TRemoteFile *& File, TRemoteFile * ALinkedByFile);
  static UnicodeString DelimitStr(const UnicodeString & Str);
  void DetectReturnVar();
  bool IsLastLine(UnicodeString & Line);
  static bool IsTotalListingLine(const UnicodeString & Line);
  void EnsureLocation();
  void ExecCommand(const UnicodeString & Cmd, intptr_t Params,
    const UnicodeString & CmdString);
  void ExecCommand2(TFSCommand Cmd, intptr_t Params, ...);
  void ReadCommandOutput(intptr_t Params, const UnicodeString * Cmd = nullptr);
  void SCPResponse(bool * GotLastLine = nullptr);
  void SCPDirectorySource(const UnicodeString & DirectoryName,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, intptr_t Level);
  inline void SCPError(const UnicodeString & Message, bool Fatal);
  void SCPSendError(const UnicodeString & Message, bool Fatal);
  void SCPSink(const UnicodeString & AFileName,
    const TRemoteFile * AFile, const UnicodeString & TargetDir,
    const UnicodeString & SourceDir,
    const TCopyParamType * CopyParam, bool & Success,
    TFileOperationProgressType * OperationProgress, intptr_t Params, intptr_t Level);
  void SCPSource(const UnicodeString & AFileName,
    const TRemoteFile * AFile,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, intptr_t Params,
    TFileOperationProgressType * OperationProgress, intptr_t Level);
  void SendCommand(const UnicodeString & Cmd);
  void SkipFirstLine();
  void SkipStartupMessage();
  void UnsetNationalVars();
  TRemoteFile * CreateRemoteFile(const UnicodeString & ListingStr,
    TRemoteFile * LinkedByFile = nullptr);
  void CaptureOutput(const UnicodeString & AddedLine, bool StdError);
  void ChangeFileToken(const UnicodeString & DelimitedName,
    const TRemoteToken & Token, TFSCommand Cmd, const UnicodeString & RecursiveStr);
  uintptr_t ConfirmOverwrite(
    const UnicodeString & AFullFileName, const UnicodeString & AFileName, TOperationSide Side,
    const TOverwriteFileParams * FileParams, const TCopyParamType * CopyParam,
    intptr_t Params, TFileOperationProgressType * OperationProgress);

  static bool RemoveLastLine(UnicodeString & Line,
    intptr_t & ReturnCode, const UnicodeString & ALastLine);
};

