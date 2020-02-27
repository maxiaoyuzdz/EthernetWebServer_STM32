/****************************************************************************************************************************
 * EthernetWebServer_STM32.h - Dead simple web-server.
 * For STM32 with built-in Ethernet (Nucleo-144, DISCOVERY, etc)
 *
 * EthernetWebServer_STM32 is a library for the STM32 run built-in Ethernet WebServer
 *
 * Forked and modified from ESP8266 https://github.com/esp8266/Arduino/releases
 * Built by Khoi Hoang https://github.com/khoih-prog/ESP8266_AT_WebServer
 * Licensed under MIT license
 * Version: 1.0.0
 *
 * Original author:
 * @file       Esp8266WebServer.h
 * @author     Ivan Grokhotkov
 *
 * Version Modified By   Date      Comments
 * ------- -----------  ---------- -----------
 *  1.0.0   K Hoang      13/02/2020 Initial coding for STM32 with built-in Ethernet (Nucleo-144, DISCOVERY, etc)
 *****************************************************************************************************************************/

#ifndef EthernetWebServer_STM32_h
#define EthernetWebServer_STM32_h

#include <functional-vlpp.h>

#ifndef USE_BUILTIN_ETHERNET
  // Use true  for built-in Ethernet (Nucleo-144, DISCOVERY, etc) using STM32Ethernet library (https://github.com/khoih-prog/STM32Ethernet)
  // Use false for for ENC28J60 and UIPEthernet library (https://github.com/UIPEthernet/UIPEthernet)
  #define USE_BUILTIN_ETHERNET   false
#endif

#if USE_BUILTIN_ETHERNET
  #include <LwIP.h>
  #include <STM32Ethernet.h>
#else
  #include <UIPEthernet.h>
  #include <utility/logging.h>
#endif

enum HTTPMethod { HTTP_ANY, HTTP_GET, HTTP_POST, HTTP_PUT, HTTP_PATCH, HTTP_DELETE, HTTP_OPTIONS };
enum HTTPUploadStatus { UPLOAD_FILE_START, UPLOAD_FILE_WRITE, UPLOAD_FILE_END,
                        UPLOAD_FILE_ABORTED };
enum HTTPClientStatus { HC_NONE, HC_WAIT_READ, HC_WAIT_CLOSE };

#define HTTP_DOWNLOAD_UNIT_SIZE 1460

#define HTTP_UPLOAD_BUFLEN 2048

#define HTTP_MAX_DATA_WAIT 1000 //ms to wait for the client to send the request
#define HTTP_MAX_POST_WAIT 1000 //ms to wait for POST data to arrive
#define HTTP_MAX_SEND_WAIT 5000 //ms to wait for data chunk to be ACKed
#define HTTP_MAX_CLOSE_WAIT 2000 //ms to wait for the client to close the connection

#define CONTENT_LENGTH_UNKNOWN ((size_t) -1)
#define CONTENT_LENGTH_NOT_SET ((size_t) -2)

class EthernetWebServer;

typedef struct {
  HTTPUploadStatus status;
  String  filename;
  String  name;
  String  type;
  size_t  totalSize;    // file size
  size_t  currentSize;  // size of data currently in buf
  uint8_t buf[HTTP_UPLOAD_BUFLEN];
} HTTPUpload;

#include "detail/RequestHandler_STM32.h"

class EthernetWebServer
{
public:
  EthernetWebServer(int port = 80);
  ~EthernetWebServer();

  void begin();
  void handleClient();

  void close();
  void stop();

  bool authenticate(const char * username, const char * password);
  void requestAuthentication();

  typedef vl::Func<void(void)> THandlerFunction;
  //typedef std::function<void(void)> THandlerFunction;
  //typedef void (*THandlerFunction)(void);
  
  void on(const String &uri, THandlerFunction handler);
  void on(const String &uri, HTTPMethod method, THandlerFunction fn);
  void on(const String &uri, HTTPMethod method, THandlerFunction fn, THandlerFunction ufn);
  void addHandler(RequestHandler* handler);
  void onNotFound(THandlerFunction fn);  //called when handler is not assigned
  void onFileUpload(THandlerFunction fn); //handle file uploads

  String uri() { return _currentUri; }
  HTTPMethod method() { return _currentMethod; }
  EthernetClient client() { return _currentClient; }
  HTTPUpload& upload() { return _currentUpload; }

  String arg(String name);        // get request argument value by name
  String arg(int i);              // get request argument value by number
  String argName(int i);          // get request argument name by number
  int args();                     // get arguments count
  bool hasArg(String name);       // check if argument exists
  void collectHeaders(const char* headerKeys[], const size_t headerKeysCount); // set the request headers to collect
  String header(String name);      // get request header value by name
  String header(int i);              // get request header value by number
  String headerName(int i);          // get request header name by number
  int headers();                     // get header count
  bool hasHeader(String name);       // check if header exists

  String hostHeader();            // get request host header if available or empty String if not

  // send response to the client
  // code - HTTP response code, can be 200 or 404
  // content_type - HTTP content type, like "text/plain" or "image/png"
  // content - actual content body
  void send(int code, const char* content_type = NULL, const String& content = String(""));
  void send(int code, char* content_type, const String& content);
  void send(int code, const String& content_type, const String& content);
  //KH
  void send(int code, char*  content_type, const String& content, size_t contentLength);
  
  void setContentLength(size_t contentLength);
  void sendHeader(const String& name, const String& value, bool first = false);
  void sendContent(const String& content);
  void sendContent(const String& content, size_t size);
  void sendContent_P(PGM_P content);
  void sendContent_P(PGM_P content, size_t size);
  
  static String urlDecode(const String& text);

template<typename T> size_t streamFile(T &file, const String& contentType){
  setContentLength(file.size());
  if (String(file.name()).endsWith(".gz") &&
      contentType != "application/x-gzip" &&
      contentType != "application/octet-stream"){
    sendHeader("Content-Encoding", "gzip");
  }
  send(200, contentType, "");
  return _currentClient.write(file);
}

protected:
  void _addRequestHandler(RequestHandler* handler);
  void _handleRequest();
  bool _parseRequest(EthernetClient& client);
  void _parseArguments(String data);
  static String _responseCodeToString(int code);
  bool _parseForm(EthernetClient& client, String boundary, uint32_t len);
  bool _parseFormUploadAborted();
  void _uploadWriteByte(uint8_t b);
  uint8_t _uploadReadByte(EthernetClient& client);
  void _prepareHeader(String& response, int code, const char* content_type, size_t contentLength);
  bool _collectHeader(const char* headerName, const char* headerValue);

  struct RequestArgument {
    String key;
    String value;
  };

  EthernetServer  _server;

  EthernetClient  _currentClient;
  HTTPMethod  _currentMethod;
  String      _currentUri;
  uint8_t     _currentVersion;
  HTTPClientStatus _currentStatus;
  unsigned long _statusChange;

  RequestHandler*  _currentHandler;
  RequestHandler*  _firstHandler;
  RequestHandler*  _lastHandler;
  THandlerFunction _notFoundHandler;
  THandlerFunction _fileUploadHandler;

  int              _currentArgCount;
  RequestArgument* _currentArgs;
  HTTPUpload       _currentUpload;

  int              _headerKeysCount;
  RequestArgument* _currentHeaders;
  size_t           _contentLength;
  String           _responseHeaders;

  String           _hostHeader;
  bool             _chunked;

};

#include "EthernetWebServer_STM32-impl.h"
#include "Parsing_STM32-impl.h"


#endif //EthernetWebServer_STM32_h
