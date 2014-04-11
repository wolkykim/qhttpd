/******************************************************************************
 * qHttpd - http://www.qdecoder.org
 *
 * Copyright (c) 2008-2012 Seungyoung Kim.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 * $Id: qhttpd.h 219 2012-05-16 21:57:53Z seungyoung.kim $
 ******************************************************************************/

#ifndef _QHTTPD_H
#define _QHTTPD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include <netdb.h>
#include <libgen.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "qlibc.h"
#include "qlibcext.h"

//
// PROGRAM SPECIFIC DEFINITIONS
//
#define PRG_INFO                "The qDecoder Project"
#define PRG_NAME                "qhttpd"
#define PRG_VERSION             "1.2.16"

#define DEF_CONFIG              "/usr/local/qhttpd/conf/qhttpd.conf"

//
// HARD-CODED INTERNAL LIMITATIONS
//

#define MAX_CHILDS      (512)
#define MAX_SEMAPHORES  (1+2)
#define MAX_SEMAPHORES_LOCK_SECS (10)   // the maximum secondes which
                                        // semaphores can be locked
#define MAX_HTTP_MEMORY_CONTENTS (1024*1024)  // if the contents size is less
                                              // than this, do not use temporary
                                              // file
#define MAX_USERCOUNTER (10)    // the amount of custom counter in shared memory
                                // for customizing purpose

#define MAX_LOGLEVEL    (4)     // the maximum log level

#define URI_MAX  (1024 * 4)     // the maximum request uri length
#define ETAG_MAX (8+1+8+1+8+1)  // the maximum etag string length including
// NULL termination

// TCP options
#define MAX_LISTEN_BACKLOG      (5)     // the maximum length the queue of
                                        // pending connections may grow up to.
#define SET_TCP_LINGER_TIMEOUT  (15)    // 0 for disable
#define SET_TCP_NODELAY         (1)     // 0 for disable
#define MAX_SHUTDOWN_WAIT       (5000)  // the maximum ms for waiting input
                                        // stream after socket shutdown

// default file creation mode
#define CRLF           "\r\n"   // CR+LF
#define DEF_DIR_MODE   (S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)
#define DEF_FILE_MODE  (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

// prefork management
#define MAX_PREFORK_AT_ONCE   (5)     // the maximum prefork servers at once
#define PERIODIC_JOB_INTERVAL (2)     // periodic job interval
#define KILL_IDLE_INTERVAL    (1000)  // the unit is ms, if idle servers are
                                      // more than max idle server, it will be
                                      // terminated by one in every interval.
                                      // This must be bigger than 1000.

//
// HTTP PROTOCOL CODES
//
#define HTTP_PROTOCOL_09    "HTTP/0.9"
#define HTTP_PROTOCOL_10    "HTTP/1.0"
#define HTTP_PROTOCOL_11    "HTTP/1.1"

//
// HTTP RESPONSE CODES
//
#define HTTP_NO_RESPONSE                (0)
#define HTTP_CODE_CONTINUE              (100)
#define HTTP_CODE_OK                    (200)
#define HTTP_CODE_CREATED               (201)
#define HTTP_CODE_NO_CONTENT            (204)
#define HTTP_CODE_PARTIAL_CONTENT       (206)
#define HTTP_CODE_MULTI_STATUS          (207)
#define HTTP_CODE_MOVED_TEMPORARILY     (302)
#define HTTP_CODE_NOT_MODIFIED          (304)
#define HTTP_CODE_BAD_REQUEST           (400)
#define HTTP_CODE_UNAUTHORIZED          (401)
#define HTTP_CODE_FORBIDDEN             (403)
#define HTTP_CODE_NOT_FOUND             (404)
#define HTTP_CODE_METHOD_NOT_ALLOWED    (405)
#define HTTP_CODE_REQUEST_TIME_OUT      (408)
#define HTTP_CODE_GONE                  (410)
#define HTTP_CODE_REQUEST_URI_TOO_LONG  (414)
#define HTTP_CODE_LOCKED                (423)
#define HTTP_CODE_INTERNAL_SERVER_ERROR (500)
#define HTTP_CODE_NOT_IMPLEMENTED       (501)
#define HTTP_CODE_SERVICE_UNAVAILABLE   (503)

//
// TYPE DEFINES
//
enum HttpAuthT {
    HTTP_AUTH_BASIC = 0,
    HTTP_AUTH_DIGEST
};

//
// CONFIGURATION STRUCTURES
//
struct ServerConfig {
    char szConfigFile[PATH_MAX];

    char szPidFile[PATH_MAX];
    char szMimeFile[PATH_MAX];

    int nPort;

    int nStartServers;
    int nMinSpareServers;
    int nMaxSpareServers;
    int nMaxIdleSeconds;
    int nMaxClients;
    int nMaxRequestsPerChild;

    bool    bEnableKeepAlive;
    int nMaxKeepAliveRequests;

    int nConnectionTimeout;
    bool    bIgnoreOverConnection;
    int nResponseExpires;

    char    szDocumentRoot[PATH_MAX];

    // allowed methods
    char    szAllowedMethods[PATH_MAX];
    struct {
        // HTTP methods
        bool bOptions;
        bool bHead;
        bool bGet;
        bool bPut;

        // HTTP extension - WebDAV methods
        bool bPropfind;
        bool bProppatch;
        bool bMkcol;
        bool bMove;
        bool bDelete;
        bool bLock;
        bool bUnlock;

        // Custom methods HERE
    } methods;

    char    szDirectoryIndex[NAME_MAX];

    bool    bEnableLua;
    char    szLuaScript[PATH_MAX];

    bool    bEnableStatus;
    char    szStatusUrl[URI_MAX];

    char    szErrorLog[PATH_MAX];
    char    szAccessLog[PATH_MAX];
    int nLogRotate;
    int nLogLevel;
};

//
// SHARED STRUCTURES
//

struct SharedData {
    // daemon info
    time_t nStartTime;
    int nTotalLaunched;         // total launched childs counter
    int nRunningChilds;         // number of running servers
    int nWorkingChilds;         // number of working servers

    int nTotalConnected;        // total connection counter
    int nTotalRequests;         // total processed requests counter

    // child info
    struct child {
        pid_t   nPid;           // pid, 0 means empty slot
        int     nTotalConnected; // total connection counter for this slot
        int     nTotalRequests; // total processed requests for this slot
        time_t  nStartTime;     // start time for this slot
        bool    bExit;          // flag for exit request after done

        struct {                // connected client information
            bool    bConnected; // flag for connection established
            time_t  nStartTime; // connection established time
            time_t  nEndTime;   // connection closed time
            int     nTotalRequests; // keep-alive requests counter

            int     nSockFd;       // socket descriptor
            char    szAddr[15+1];  // client IP address
            unsigned int nAddr;    // client IP address
            int     nPort;         // client port number

            bool    bRun;               // flag for working
            struct  timeval tvReqTime;  // request time
            struct  timeval tvResTime;  // response time
            char    szReqInfo[1024+1];  // additional request information
            int     nResponseCode;      // response code
        } conn;
    } child[MAX_CHILDS];

    // extra info
    int nUserCounter[MAX_USERCOUNTER];
};

//
// HTTP STRUCTURES
//
struct HttpRequest {
    // connection info
    int nSockFd;    // socket descriptor
    int nTimeout;   // timeout value for this request

    // request status
    int  nReqStatus;  // request status
    // 1:ok, 0:bad request, -1:timeout or connection closed
    char *pszDocumentRoot;   // document root for this request
    char *pszDirectoryIndex; // directory index file

    // request body
    char   *pszRequestBody; // whole request body
    size_t nRequestSize;    // size of request body

    // request line
    char *pszRequestMethod; // request method ex) GET
    char *pszRequestUri;    // url+query ex) /data%20path?query=the%20value
    char *pszHttpVersion;   // version ex) HTTP/1.1

    // parsed request information
    char *pszRequestHost;   // host ex) www.domain.com or www.domain.com:8080
    char *pszRequestDomain; // domain name ex) www.domain.com (no port number)
    char *pszRequestPath;   // decoded path ex) /data path
    char *pszQueryString;   // query string ex) query=the%20value

    // request header
    qlisttbl_t *pHeaders;    // request headers

    // contents
    off_t   nContentsLength; // contents length 0:no contents, n>0:has contents
    char   *pContents;       // contents data if parsed (if contents does not
    // parsed : nContentsLength>0 && pContents==NULL)
};

struct HttpResponse {
    bool bOut;                  // flag for response out already
    struct HttpRequest *pReq;   // request referer link. can be NULL.

    char *pszHttpVersion;       // response protocol
    int  nResponseCode;         // response code

    qlisttbl_t *pHeaders;        // response headers

    char  *pszContentType;      // contents mime type
    off_t nContentsLength;      // contents length
    char  *pContent;            // contents data
    bool  bChunked;             // flag for chunked data out
};

struct HttpUser {
    enum HttpAuthT nAuthType;   // HTTP_AUTH_BASIC or HTTP_AUTH_DIGEST
    char szUser[63+1];
    char szPassword[63+1];
};

//
// PROTO-TYPES
//

// main.c
extern int main(int argc, char *argv[]);

// version.c
extern void printUsages(void);
extern void printVersion(void);
extern char *getVersion(void);

// config.c
extern bool loadConfig(struct ServerConfig *pConf, char *pszFilePath);
extern bool checkConfig(struct ServerConfig *pConf);

// daemon.c
extern void daemonStart(bool nDaemonize);

// pool.c
extern bool poolInit(int nMaxChild);
extern bool poolFree(void);
extern struct SharedData *poolGetShm(void);
extern int poolSendSignal(int signo);

extern bool poolCheck(void);
extern int poolGetTotalLaunched(void);
extern int poolGetNumChilds(int *nWorking, int *nIdling);
extern int poolSetIdleExitReqeust(int nNum);
extern int poolSetExitReqeustAll(void);

extern bool poolChildReg(void);
extern bool poolChildDel(pid_t nPid);
extern int poolGetMySlotId(void);
extern bool poolGetExitRequest(void);
extern bool poolSetExitRequest(void);

extern int poolGetChildTotalRequests(void);
extern int poolGetChildKeepaliveRequests(void);

extern bool poolSetConnInfo(int nSockFd);
extern bool poolSetConnRequest(struct HttpRequest *pReq);
extern bool poolSetConnResponse(struct HttpResponse *pRes);
extern bool poolClearConnInfo(void);
extern char *poolGetConnAddr(void);
extern unsigned int poolGetConnNaddr(void);
extern int poolGetConnPort(void);
extern time_t poolGetConnReqTime(void);

// child.c
extern void childStart(int nSockFd);

// http_main.c
extern int httpMain(int nSockFd);
extern int httpRequestHandler(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern int httpSpecialRequestHandler(struct HttpRequest *pReq, struct HttpResponse *pRes);

// http_request.c
extern struct HttpRequest *httpRequestParse(int nSockFd, int nTimeout);
extern char *httpRequestGetSysPath(struct HttpRequest *pReq, char *pszBuf, size_t nBufSize, const char *pszPath);
extern bool httpRequestFree(struct HttpRequest *pReq);

// http_response.c
extern struct HttpResponse *httpResponseCreate(struct HttpRequest *pReq);
extern int httpResponseSetSimple(struct HttpResponse *pRes, int nResCode, bool nKeepAlive, const char *pszText);
extern bool httpResponseSetCode(struct HttpResponse *pRes, int nResCode, bool bKeepAlive);
extern bool httpResponseSetContent(struct HttpResponse *pRes, const char *pszContentType, const char *pContent, off_t nContentsLength);
extern bool httpResponseSetContentHtml(struct HttpResponse *pRes, const char *pszMsg);
extern bool httpResponseSetContentChunked(struct HttpResponse *pRes, bool bChunked);
extern bool httpResponseSetAuthRequired(struct HttpResponse *pRes, enum HttpAuthT nAuthType, const char *pszRealm);
extern bool httpResponseOut(struct HttpResponse *pRes);
extern bool httpResponseOutChunk(struct HttpResponse *pRes, const void *pData, size_t nSize);
extern bool httpResponseReset(struct HttpResponse *pRes);
extern void httpResponseFree(struct HttpResponse *pRes);
extern const char *httpResponseGetMsg(int nResCode);

#define response200(pRes)   httpResponseSetSimple(pRes, HTTP_CODE_OK, true, httpResponseGetMsg(HTTP_CODE_OK));
#define response201(pRes)   httpResponseSetSimple(pRes, HTTP_CODE_CREATED, true, httpResponseGetMsg(HTTP_CODE_CREATED));
#define response204(pRes)   httpResponseSetSimple(pRes, HTTP_CODE_NO_CONTENT, true, NULL);
#define response304(pRes)   httpResponseSetSimple(pRes, HTTP_CODE_NOT_MODIFIED, true, NULL);
#define response400(pRes)   httpResponseSetSimple(pRes, HTTP_CODE_BAD_REQUEST, false, httpResponseGetMsg(HTTP_CODE_BAD_REQUEST))
#define response403(pRes)   httpResponseSetSimple(pRes, HTTP_CODE_FORBIDDEN, true, httpResponseGetMsg(HTTP_CODE_FORBIDDEN))
#define response404(pRes)   httpResponseSetSimple(pRes, HTTP_CODE_NOT_FOUND, true, httpResponseGetMsg(HTTP_CODE_NOT_FOUND))
#define response404nc(pRes) httpResponseSetSimple(pRes, HTTP_CODE_NOT_FOUND, true, NULL)
#define response405(pRes)   httpResponseSetSimple(pRes, HTTP_CODE_METHOD_NOT_ALLOWED, false, httpResponseGetMsg(HTTP_CODE_METHOD_NOT_ALLOWED))
#define response414(pRes)   httpResponseSetSimple(pRes, HTTP_CODE_REQUEST_URI_TOO_LONG, false, httpResponseGetMsg(HTTP_CODE_REQUEST_URI_TOO_LONG))
#define response500(pRes)   httpResponseSetSimple(pRes, HTTP_CODE_INTERNAL_SERVER_ERROR, false, httpResponseGetMsg(HTTP_CODE_INTERNAL_SERVER_ERROR))
#define response501(pRes)   httpResponseSetSimple(pRes, HTTP_CODE_NOT_IMPLEMENTED, false, httpResponseGetMsg(HTTP_CODE_NOT_IMPLEMENTED))
#define response503(pRes)   httpResponseSetSimple(pRes, HTTP_CODE_SERVICE_UNAVAILABLE, false, httpResponseGetMsg(HTTP_CODE_SERVICE_UNAVAILABLE))

// http_header.c
extern const char *httpHeaderGetStr(qlisttbl_t *entries, const char *pszName);
extern int httpHeaderGetInt(qlisttbl_t *entries, const char *pszName);
extern bool httpHeaderSetStr(qlisttbl_t *entries, const char *pszName, const char *pszValue);
extern bool httpHeaderSetStrf(qlisttbl_t *entries, const char *pszName, const char *pszformat, ...);
extern bool httpHeaderRemove(qlisttbl_t *entries, const char *pszName);
extern bool httpHeaderHasCasestr(qlisttbl_t *entries, const char *pszName, const char *pszValue);
extern bool httpHeaderParseRange(const char *pszRangeHeader, off_t nFilesize, off_t *pnRangeOffset1, off_t *pnRangeOffset2, off_t *pnRangeSize);
extern bool httpHeaderSetExpire(qlisttbl_t *entries, int nExpire);

// http_auth.c
extern struct HttpUser *httpAuthParse(struct HttpRequest *pReq);

// http_method.c
extern int httpMethodOptions(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern int httpMethodHead(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern int httpMethodGet(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern int httpRealGet(struct HttpRequest *pReq, struct HttpResponse *pRes, int nFd, struct stat *pStat, const char *pszContentType);
extern int httpMethodPut(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern int httpRealPut(struct HttpRequest *pReq, struct HttpResponse *pRes, int nFd);
extern int httpMethodDelete(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern int httpMethodNotImplemented(struct HttpRequest *pReq, struct HttpResponse *pRes);

// http_method_dav.c
extern int httpMethodPropfind(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern int httpMethodProppatch(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern int httpMethodMkcol(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern int httpMethodMove(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern int httpMethodDelete(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern int httpMethodLock(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern int httpMethodUnlock(struct HttpRequest *pReq, struct HttpResponse *pRes);

// http_status.c
extern int httpStatusResponse(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern qvector_t *httpGetStatusHtml(void);

// http_accesslog.c
extern bool httpAccessLog(struct HttpRequest *pReq, struct HttpResponse *pRes);

// mime.c
extern bool mimeInit(const char *pszFilepath);
extern bool mimeFree(void);
extern const char *mimeDetect(const char *pszFilename);

// stream.c
extern int streamWaitReadable(int nSockFd, int nTimeoutMs);
extern ssize_t streamRead(int nSockFd, void *pszBuffer, size_t nSize, int nTimeoutMs);
extern ssize_t streamGets(int nSockFd, char *pszStr, size_t nSize, int nTimeoutMs);
extern ssize_t streamGetb(int nSockFd, char *pszBuffer, size_t nSize, int nTimeoutMs);
extern off_t streamSave(int nFd, int nSockFd, off_t nSize, int nTimeoutMs);
extern ssize_t streamPrintf(int nSockFd, const char *format, ...);
extern ssize_t streamPuts(int nSockFd, const char *pszStr);
extern ssize_t streamStackOut(int nSockFd, qvector_t *vector, int nTimeoutMs);
extern ssize_t streamWrite(int nSockFd, const void *pszBuffer, size_t nSize, int nTimeoutMs);
extern ssize_t streamWritev(int nSockFd,  const struct iovec *pVector, int nCount, int nTimeoutMs);
extern off_t streamSend(int nSockFd, int nFd, off_t nSize, int nTimeoutMs);

// util.c
extern int closeSocket(int nSockFd);
extern char *getEtag(char *pszBuf, size_t nBufSize, const char *pszPath, struct stat *pStat);
extern unsigned int getIp2Uint(const char *szIp);
extern float getDiffTimeval(struct timeval *t1, struct timeval *t0);
extern bool isValidPathname(const char *pszPath);
extern void correctPathname(char *pszPath);

// syscall.c
#include <dirent.h>
extern int sysOpen(const char *pszPath, int nFlags, mode_t nMode);
extern int sysClose(int nFd);
extern int sysStat(const char *pszPath, struct stat *pBuf);
extern int sysFstat(int nFd, struct stat *pBuf);
extern int sysUnlink(const char *pszPath);
extern int sysRename(const char *pszOldPath, const char *pszNewPath);
extern int sysMkdir(const char *pszPath, mode_t nMode);
extern int sysRmdir(const char *pszPath);
extern DIR *sysOpendir(const char *pszPath);
extern struct dirent *sysReaddir(DIR *pDir);
extern int sysClosedir(DIR *pDir);

// luascript.c
#ifdef ENABLE_LUA
extern bool luaInit(const char *pszScriptPath);
extern bool luaFree(void);
extern int luaRequestHandler(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern bool luaResponseHandler(void);
#endif

// hook.c
#ifdef ENABLE_HOOK
extern bool hookBeforeMainInit(void);
extern bool hookAfterConfigLoaded(struct ServerConfig *config, bool bConfigLoadSucceed);

extern bool hookAfterDaemonInit(void);
extern int hookWhileDaemonIdle(void);
extern bool hookBeforeDaemonEnd(void);
extern bool hookAfterDaemonSIGHUP(void);

extern bool hookAfterChildInit(void);
extern bool hookBeforeChildEnd(void);
extern bool hookAfterConnEstablished(int nSockFd);

extern int hookRequestHandler(struct HttpRequest *pReq, struct HttpResponse *pRes);
extern bool hookResponseHandler(struct HttpRequest *pReq, struct HttpResponse *pRes);
#endif

//
// GLOBAL VARIABLES
//
extern char *g_prginfo;
extern char *g_prgname;
extern char *g_prgversion;

extern bool g_debug;
extern sigset_t g_sigflags;

extern struct ServerConfig g_conf;
extern int g_semid;
extern qlog_t *g_errlog;
extern qlog_t *g_acclog;
extern int g_loglevel;

//
// DEFINITION FUNCTIONS
//
#define CONST_STRLEN(x)     (sizeof(x) - 1)
#define IS_EMPTY_STRING(x)  ( (x == NULL || x[0] == '\0') ? true : false )

#define DYNAMIC_VSPRINTF(s, f)                                          \
    do {                                                                \
        size_t _strsize;                                                \
        for(_strsize = 1024; ; _strsize *= 2) {                         \
            s = (char*)malloc(_strsize);                                \
            if(s == NULL) {                                             \
                DEBUG("DYNAMIC_VSPRINTF(): can't allocate memory.");    \
                break;                                                  \
            }                                                           \
            va_list _arglist;                                           \
            va_start(_arglist, f);                                      \
            int _n = vsnprintf(s, _strsize, f, _arglist);               \
            va_end(_arglist);                                           \
            if(_n >= 0 && _n < _strsize) break;                         \
            free(s);                                                    \
        }                                                               \
    } while(0)

#define _LOG(log, level, prestr, fmt, args...)  do {                    \
        if (g_loglevel >= level) {                                      \
            char _timestr[14+1];                                        \
            qtime_gmt_strf(_timestr, sizeof(_timestr), 0, "%Y%m%d%H%M%S"); \
            if(log != NULL)                                             \
                log->writef(log, "%s(%d):" prestr fmt                   \
                            , _timestr, getpid(), ##args);              \
            else                                                        \
                printf("%s(%d):" prestr fmt "\n"                        \
                       , _timestr, getpid(), ##args);                   \
        }                                                               \
    } while(0)

#define _LOG2(log, level, prestr, fmt, args...) do {                    \
        if (g_loglevel >= level) {                                      \
            char _timestr[14+1];                                        \
            qtime_gmt_strf(_timestr, sizeof(_timestr), 0, "%Y%m%d%H%M%S"); \
            if(log != NULL)                                             \
                log->writef(log, "%s(%d):" prestr fmt " (%s:%d)"        \
                            , _timestr, getpid(), ##args, __FILE__, __LINE__); \
            else                                                        \
                printf("%s(%d):" prestr fmt " (%s:%d)\n"                \
                       , _timestr, getpid(), ##args, __FILE__, __LINE__); \
        }                                                               \
    } while(0)

#define LOG_SYS(fmt, args...)   _LOG(g_errlog, 0, " ", fmt, ##args)
#define LOG_ERR(fmt, args...)   _LOG2(g_errlog, 1, " [ERROR] ", fmt, ##args)
#define LOG_WARN(fmt, args...)  _LOG2(g_errlog, 2, " [WARN] ", fmt, ##args)
#define LOG_INFO(fmt, args...)  _LOG(g_errlog, 3, " [INFO] ", fmt, ##args)

#define STOPWATCH_START()                                               \
    int _swno = 0;                                                      \
    struct timeval _tv1, _tv2;                                          \
    gettimeofday(&_tv1, NULL)

#define STOPWATCH_STOP(prefix)  {                                       \
        gettimeofday(&_tv2, NULL);                                      \
        _swno++;                                                        \
        struct timeval _diff;                                           \
        _diff.tv_sec = _tv2.tv_sec - _tv1.tv_sec;                       \
        if(_tv2.tv_usec >= _tv1.tv_usec) _diff.tv_usec = _tv2.tv_usec - _tv1.tv_usec; \
        else { _diff.tv_sec += 1; _diff.tv_usec = _tv1.tv_usec - _tv2.tv_usec; } \
        printf("STOPWATCH(%d,%s,%d): %zus %dus (%s:%d)\n",              \
               getpid(), prefix, _swno, _diff.tv_sec, (int)(_diff.tv_usec), __FILE__, __LINE__); \
        gettimeofday(&_tv1, NULL);                                      \
    }

#ifdef DEBUG
#undef DEBUG
#endif

#ifdef ENABLE_DEBUG

//
// DEBUG build
//
#define DEBUG(fmt, args...)                                             \
    do {                                                                \
        _LOG2(g_errlog, MAX_LOGLEVEL, " [DEBUG] ", fmt, ##args);        \
    } while (false)
#else

//
// RELEASE build
//
#define DEBUG(fms, args...)

#endif // ENABLE_DEBUG

#endif  // _QHTTPD_H
