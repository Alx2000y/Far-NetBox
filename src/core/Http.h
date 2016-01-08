#pragma once

#include <memory>
#include <Classes.hpp>

struct ne_session_s;
struct ne_request_s;
struct ne_ssl_certificate_s;
struct ssl_st;

class THttp;
typedef void (__closure * THttpDownloadEvent)(THttp * Sender, int64_t Size, bool & Cancel);

class THttp : public TObject
{
public:
  THttp();
  ~THttp();

  void Get();
  void Post(const UnicodeString & Request);

  /*__property UnicodeString URL = { read = FURL, write = FURL };
  __property UnicodeString ProxyHost = { read = FProxyHost, write = FProxyHost };
  __property int ProxyPort = { read = FProxyPort, write = FProxyPort };
  __property UnicodeString Response = { read = GetResponse };
  __property RawByteString ResponseRaw = { read = FResponse };
  __property int64_t ResponseLength = { read = GetResponseLength };
  __property int64_t ResponseLimit = { read = FResponseLimit, write = FResponseLimit };
  __property THttpDownloadEvent OnDownload = { read = FOnDownload, write = FOnDownload};*/

private:
  UnicodeString FURL;
  UnicodeString FProxyHost;
  int FProxyPort;
  RawByteString FResponse;
  int64_t FResponseLimit;
  std::unique_ptr<Exception> FException;
  THttpDownloadEvent FOnDownload;
  UnicodeString FHostName;
  UnicodeString FCertificateError;

  static int NeonBodyReader(void * UserData, const char * Buf, size_t Len);
  int NeonBodyReaderImpl(const char * Buf, size_t Len);
  void SendRequest(const char * Method, const UnicodeString & Request);
  UnicodeString GetResponse();
  int64_t GetResponseLength();
  static void InitSslSession(ssl_st * Ssl, ne_session_s * Session);
  static int NeonServerSSLCallback(void * UserData, int Failures, const ne_ssl_certificate_s * Certificate);
  int NeonServerSSLCallbackImpl(int Failures, const ne_ssl_certificate_s * Certificate);
};

