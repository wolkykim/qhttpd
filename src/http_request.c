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
 * $Id: http_request.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#include "qhttpd.h"

static char *_readRequest(int nSockFd, size_t *nRequestSize, int nTimeout);
static char *_getCorrectedHostname(const char *pszHostname);
static char *_getCorrectedDomainname(const char *pszDomainname);

/*
 * @return  HttpRequest pointer
 *      NULL : system error
 */
struct HttpRequest *httpRequestParse(int nSockFd, int nTimeout) {
    struct HttpRequest *pReq;
    char szLineBuf[URI_MAX + 32];

    //
    // initialize request structure
    //
    pReq = (struct HttpRequest *)malloc(sizeof(struct HttpRequest));
    if (pReq == NULL) return NULL;

    // initialize request structure
    memset((void *)pReq, 0, sizeof(struct HttpRequest));

    // set initial values
    pReq->nSockFd = nSockFd;
    pReq->nTimeout = nTimeout;

    pReq->nReqStatus = 0;
    pReq->nContentsLength = -1;

    pReq->pHeaders = qlisttbl();
    if (pReq->pHeaders == NULL) {
        free(pReq);
        return NULL;
    }
    pReq->pHeaders->setcase(pReq->pHeaders, true);  // case insensitive lookup

    //
    // Read whole request at once to reduce a amount of read() system call.
    //
    pReq->pszRequestBody = _readRequest(nSockFd, &pReq->nRequestSize, pReq->nTimeout * 1000);
    if (pReq->pszRequestBody == NULL) {
        DEBUG("Connection is closed by peer.");
        pReq->nReqStatus = -1;
        return pReq;
    }

    // set offset for qstrgets()
    char *pszBodyOffset = pReq->pszRequestBody;

    //
    // Parse HTTP header
    //

    // Parse request line : "method uri protocol"
    {
        char *pszReqMethod, *pszReqUri, *pszHttpVer, *pszTmp;

        // read line
        if (qstrgets(szLineBuf, sizeof(szLineBuf), &pszBodyOffset) == NULL) return pReq;

        // parse line
        pszReqMethod = strtok(szLineBuf, " ");
        pszReqUri = strtok(NULL, " ");
        pszHttpVer = strtok(NULL, " ");
        pszTmp = strtok(NULL, " ");

        if (pszReqMethod == NULL || pszReqUri == NULL || pszHttpVer == NULL || pszTmp != NULL) {
            DEBUG("Invalid request line.");
            return pReq;
        }

        //DEBUG("pszReqMethod %s", pszReqMethod);
        //DEBUG("pszReqUri %s", pszReqUri);
        //DEBUG("pszHttpVer %s", pszHttpVer);

        //
        // request method
        //
        qstrupper(pszReqMethod);
        pReq->pszRequestMethod = strdup(pszReqMethod);

        //
        // http version
        //
        qstrupper(pszHttpVer);
        if (strcmp(pszHttpVer, HTTP_PROTOCOL_09)
            && strcmp(pszHttpVer, HTTP_PROTOCOL_10)
            && strcmp(pszHttpVer, HTTP_PROTOCOL_11)
           ) {
            DEBUG("Unknown protocol: %s", pszHttpVer);
            return pReq;
        }
        pReq->pszHttpVersion = strdup(pszHttpVer);

        //
        // request uri
        //

        // if request has only path
        if (pszReqUri[0] == '/') {
            pReq->pszRequestUri = strdup(pszReqUri);
            // if request has full uri format
        } else if (!strncasecmp(pszReqUri, "HTTP://", CONST_STRLEN("HTTP://"))) {
            // divide uri into host and path
            pszTmp = strstr(pszReqUri + CONST_STRLEN("HTTP://"), "/");
            if (pszTmp == NULL) {   // No path, ex) http://a.b.c:80
                httpHeaderSetStr(pReq->pHeaders, "Host", pszReqUri + CONST_STRLEN("HTTP://"));
                pReq->pszRequestUri = strdup("/");
            } else {        // Has path, ex) http://a.b.c:80/100
                *pszTmp = '\0';
                httpHeaderSetStr(pReq->pHeaders, "Host", pszReqUri  + CONST_STRLEN("HTTP://"));
                *pszTmp = '/';
                pReq->pszRequestUri = strdup(pszTmp);
            }
        }
        // invalid format
        else {
            DEBUG("Unable to parse uri: %s", pszReqUri);
            return pReq;
        }

        // request path
        pReq->pszRequestPath = strdup(pReq->pszRequestUri);

        // remove query string from request path
        pszTmp = strstr(pReq->pszRequestPath, "?");
        if (pszTmp != NULL) {
            *pszTmp ='\0';
            pReq->pszQueryString = strdup(pszTmp + 1);
        } else {
            pReq->pszQueryString = strdup("");
        }

        // decode path
        qurl_decode(pReq->pszRequestPath);

        // check path
        if (isValidPathname(pReq->pszRequestPath) == false) {
            DEBUG("Invalid URI format : %s", pReq->pszRequestUri);
            return pReq;
        }
        correctPathname(pReq->pszRequestPath);
    }

    // Parse parameter headers : "key: value"
    while (true) {
        // read line
        if (qstrgets(szLineBuf, sizeof(szLineBuf), &pszBodyOffset) == NULL) return pReq;
        if (IS_EMPTY_STRING(szLineBuf) == true) break; // detect line-feed

        // separate :
        char *tmp = strstr(szLineBuf, ":");
        if (tmp == NULL) {
            DEBUG("Request header field is missing ':' separator.");
            return pReq;
        }

        *tmp = '\0';
        char *name = qstrupper(qstrtrim(szLineBuf)); // to capital letters
        char *value = qstrtrim(tmp + 1);

        // put
        httpHeaderSetStr(pReq->pHeaders, name, value);
    }

    // parse host
    pReq->pszRequestHost = _getCorrectedHostname(httpHeaderGetStr(pReq->pHeaders, "HOST"));
    if (IS_EMPTY_STRING(pReq->pszRequestHost) == true) {
        DEBUG("Can't find host information.");
        return pReq;
    }
    httpHeaderSetStr(pReq->pHeaders, "Host", pReq->pszRequestHost);

    // set domain
    pReq->pszRequestDomain = _getCorrectedDomainname(pReq->pszRequestHost);

    // Parse Contents
    if (httpHeaderGetStr(pReq->pHeaders, "CONTENT-LENGTH") != NULL) {
        pReq->nContentsLength = (off_t)atoll(httpHeaderGetStr(pReq->pHeaders, "CONTENT-LENGTH"));

        // do not load into memory in case of PUT and POST method
        if (strcmp(pReq->pszRequestMethod, "PUT")
            && strcmp(pReq->pszRequestMethod, "POST")
            && pReq->nContentsLength <= MAX_HTTP_MEMORY_CONTENTS) {
            if (pReq->nContentsLength == 0) {
                pReq->pContents = strdup("");
            } else {
                // allocate memory
                pReq->pContents = (char *)malloc(pReq->nContentsLength + 1);
                if (pReq->pContents == NULL) {
                    LOG_WARN("Memory allocation failed.");
                    return pReq;
                }

                // save into memory
                int nReaded = streamGetb(nSockFd, pReq->pContents, pReq->nContentsLength, pReq->nTimeout * 1000);
                if (nReaded >= 0) pReq->pContents[nReaded] = '\0';
                DEBUG("%s", pReq->pContents);

                if (pReq->nContentsLength != nReaded) {
                    DEBUG("Connection is closed before request completion.");
                    free(pReq->pContents);
                    pReq->pContents = NULL;
                    pReq->nContentsLength = -1;
                    return pReq;
                }
            }
        }
    }

    // set document root
    pReq->pszDocumentRoot = strdup(g_conf.szDocumentRoot);
    if (IS_EMPTY_STRING(g_conf.szDirectoryIndex) == false) {
        pReq->pszDirectoryIndex = strdup(g_conf.szDirectoryIndex);
    }

    // change flag to normal state
    pReq->nReqStatus = 1;

    return pReq;
}

char *httpRequestGetSysPath(struct HttpRequest *pReq, char *pszBuf, size_t nBufSize, const char *pszPath)
{
    if (pReq == NULL || pReq->nReqStatus != 1) return NULL;

    // generate abs path
    snprintf(pszBuf, nBufSize, "%s%s", pReq->pszDocumentRoot, pszPath);
    pszBuf[nBufSize - 1] = '\0';

    return pszBuf;
}

bool httpRequestFree(struct HttpRequest *pReq)
{
    if (pReq == NULL) return false;

    if (pReq->pszDocumentRoot != NULL) free(pReq->pszDocumentRoot);
    if (pReq->pszDirectoryIndex != NULL) free(pReq->pszDirectoryIndex);

    if (pReq->pszRequestBody != NULL) free(pReq->pszRequestBody);

    if (pReq->pszRequestMethod != NULL) free(pReq->pszRequestMethod);
    if (pReq->pszRequestUri != NULL) free(pReq->pszRequestUri);
    if (pReq->pszHttpVersion != NULL) free(pReq->pszHttpVersion);

    if (pReq->pszRequestHost != NULL) free(pReq->pszRequestHost);
    if (pReq->pszRequestDomain != NULL) free(pReq->pszRequestDomain);
    if (pReq->pszRequestPath != NULL) free(pReq->pszRequestPath);
    if (pReq->pszQueryString != NULL) free(pReq->pszQueryString);

    if (pReq->pHeaders != NULL) pReq->pHeaders->free(pReq->pHeaders);
    if (pReq->pContents != NULL) free(pReq->pContents);

    free(pReq);

    return true;
}

#define REQ_READ_BLOCK_SIZE     (1024 * 4)
static char *_readRequest(int nSockFd, size_t *nRequestSize, int nTimeout)
{
    // read headers
    int nBlockRead = 0; // 0: read 1 byte, 1: enable block read, -1 block read disabled
    bool bEndOfHeader = false;

    size_t nBufSize = 0;
    char *pszReqBuf = NULL;
    size_t nTotal = 0;
    do {
        int nMaxRead = nBufSize - nTotal;
        if (nMaxRead == 0) {
            // alloc or realloc
            nBufSize += REQ_READ_BLOCK_SIZE;
            char *pszNewBuf = realloc(pszReqBuf, nBufSize + 1); // for debugging purpose, allocate 1 byte more for storing termination character
            if (pszNewBuf == NULL) break;;

            pszReqBuf = pszNewBuf;
            nMaxRead = nBufSize - nTotal;
            //DEBUG("malloc %d %d", nBufSize, pszReqBuf);
        }

        if (qio_wait_readable(nSockFd, nTimeout) <= 0) break;
        ssize_t nRead = read(nSockFd, pszReqBuf + nTotal, (nBlockRead > 0) ? nMaxRead : 1);
        //ssize_t nRead = streamRead(pszReqBuf + nTotal, nSockFd, (nBlockRead > 0) ? nMaxRead : 1, nTimeout);

        if (nRead <= 0) break;
        nTotal += nRead;

        // turn on block read only for the requests which wait server's response after sending request.
        if (nBlockRead == 0 && pszReqBuf[nTotal - 1] == ' ') {
            if ((nTotal == CONST_STRLEN("GET ") && !strncmp(pszReqBuf, "GET ", CONST_STRLEN("GET ")))
                || (nTotal == CONST_STRLEN("HEAD ") && !strncmp(pszReqBuf, "HEAD ", CONST_STRLEN("HEAD ")))
                || (nTotal == CONST_STRLEN("OPTIONS ") && !strncmp(pszReqBuf, "OPTIONS ", CONST_STRLEN("OPTIONS ")))
               ) {
                nBlockRead = 1; // enable block read
            } else {
                nBlockRead = -1; // disable block read
            }
        }
        // check end of headers
        else if (nTotal >= CONST_STRLEN(CRLF CRLF) && pszReqBuf[nTotal - 1] == '\n') {
            if (!strncmp(pszReqBuf + nTotal - CONST_STRLEN(CRLF CRLF), CRLF CRLF, CONST_STRLEN(CRLF CRLF))) {
                bEndOfHeader = true;
            }
        }
    } while (bEndOfHeader == false);

#ifdef ENABLE_DEBUG
    if (pszReqBuf != NULL) {
        pszReqBuf[nTotal] = '\0';
        if (bEndOfHeader == true) DEBUG("[RX] %s", pszReqBuf);
        else if (nTotal > 0)DEBUG("[RX-ERR] %s", pszReqBuf);
    }
#endif

    if (bEndOfHeader == true) {
        if (nRequestSize != NULL) *nRequestSize = nTotal;
        return pszReqBuf;
    }

    if (pszReqBuf != NULL) free(pszReqBuf);
    return NULL;
}

static char *_getCorrectedHostname(const char *pszHostname)
{
    char *pszHost = NULL;
    if (pszHostname != NULL) {
        pszHost = strdup(pszHostname);
        qstrlower(pszHost);

        // if port number is 80, take it off
        char *pszTmp = strstr(pszHost, ":");
        if (pszTmp != NULL && !strcmp(pszTmp, ":80")) *pszTmp = '\0';
    }

    return pszHost;
}

static char *_getCorrectedDomainname(const char *pszDomainname)
{
    char *pszDomain = NULL;
    if (pszDomainname != NULL) {
        pszDomain = strdup(pszDomainname);
        char *pszColon = strstr(pszDomain, ":");
        if (pszColon != NULL) {
            *pszColon = '\0';
        }
    }

    return pszDomain;
}
