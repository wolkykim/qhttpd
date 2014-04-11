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
 * $Id: util.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#include "qhttpd.h"

int closeSocket(int nSockFd)
{
    // close connection
    if (shutdown(nSockFd, SHUT_WR) == 0) {
        char szDummyBuf[1024];
        while (true) {
            ssize_t nDummyRead = streamRead(nSockFd, szDummyBuf, sizeof(szDummyBuf), MAX_SHUTDOWN_WAIT);
            if (nDummyRead <= 0) break;
            DEBUG("Throw %zu bytes from dummy input stream.", nDummyRead);
        }
    }
    return close(nSockFd);
}

char *getEtag(char *pszBuf, size_t nBufSize, const char *pszPath, struct stat *pStat)
{
    unsigned int nFilepathHash = qhashfnv1_32((const void *)pszPath, strlen(pszPath));
    snprintf(pszBuf, nBufSize, "%08x-%08x-%08x", nFilepathHash, (unsigned int)pStat->st_size, (unsigned int)pStat->st_mtime);
    pszBuf[nBufSize - 1] = '\0';
    return pszBuf;
}

unsigned int getIp2Uint(const char *szIp)
{
    char szBuf[15+1];
    char *pszToken;
    int nTokenCnt = 0;
    unsigned int nAddr = 0;

    // check length
    if (strlen(szIp) > 15) return 0;

    // copy to buffer
    strcpy(szBuf, szIp);
    for (pszToken = strtok(szBuf, "."); pszToken != NULL; pszToken = strtok(NULL, ".")) {
        nTokenCnt++;

        if (nTokenCnt == 1) nAddr += (unsigned int)(atoi(pszToken)) * 0x1000000;
        else if (nTokenCnt == 2) nAddr += (unsigned int)(atoi(pszToken)) * 0x10000;
        else if (nTokenCnt == 3) nAddr += (unsigned int)(atoi(pszToken)) * 0x100;
        else if (nTokenCnt == 4) nAddr += (unsigned int)(atoi(pszToken));
        else return 0;
    }
    if (nTokenCnt != 4) return 0;

    return nAddr;
}

float getDiffTimeval(struct timeval *t1, struct timeval *t0)
{
    struct timeval nowt;
    float nDiff;

    if (t1 == NULL) {
        gettimeofday(&nowt, NULL);
        t1 = &nowt;
    }

    nDiff = t1->tv_sec - t0->tv_sec;
    if (t1->tv_usec - t0->tv_usec != 0) nDiff += (float)(t1->tv_usec - t0->tv_usec) / 1000000;

    //DEBUG("%ld.%ld %ld.%ld", t1->tv_sec, t1->tv_usec, t0->tv_sec, t0->tv_usec);

    return nDiff;
}

/**
 * validate file path
 */
bool isValidPathname(const char *pszPath)
{
    if (pszPath == NULL) return false;

    int nLen = strlen(pszPath);
    if (nLen == 0 || nLen >= PATH_MAX) return false;
    else if (pszPath[0] != '/') return false;
    else if (strpbrk(pszPath, "\\:*?\"<>|") != NULL) return false;

    // check folder name length
    char *t;
    int n;
    for (n = 0, t = (char *)pszPath; *t != '\0'; t++) {
        if (*t == '/') {
            n = 0;
            continue;
        }

        if (n >= FILENAME_MAX) {
            DEBUG("Filename too long.");
            return false;
        }

        n++;
    }

    return true;
}

/**
 * Correting path
 *
 * @note
 *    remove :  heading & tailing white spaces, double slashes, tailing slash
 */
void correctPathname(char *pszPath)
{
    // take off heading & tailing white spaces
    qstrtrim(pszPath);

    // take off double slashes
    while (strstr(pszPath, "//") != NULL) qstrreplace("sr", pszPath, "//", "/");

    // take off tailing slash
    int nLen = strlen(pszPath);
    if (nLen <= 1) return;
    if (pszPath[nLen - 1] == '/') pszPath[nLen - 1] = '\0';
}
