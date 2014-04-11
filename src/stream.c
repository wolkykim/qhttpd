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
 * $Id: stream.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#include "qhttpd.h"

ssize_t streamRead(int nSockFd, void *pszBuffer, size_t nSize, int nTimeoutMs)
{
    ssize_t nReaded = qio_read(nSockFd, pszBuffer, nSize, nTimeoutMs);
#ifdef ENABLE_DEBUG
    if (nReaded > 0) DEBUG("[RX] (binary, readed %zd bytes)", nReaded);
#endif
    return nReaded;
}

ssize_t streamGets(int nSockFd, char *pszStr, size_t nSize, int nTimeoutMs)
{
    ssize_t nReaded = qio_gets(nSockFd, pszStr, nSize, nTimeoutMs);
#ifdef ENABLE_DEBUG
    if (nReaded > 0) DEBUG("[RX] %s", pszStr);
#endif
    return nReaded;
}

ssize_t streamGetb(int nSockFd, char *pszBuffer, size_t nSize, int nTimeoutMs)
{
    ssize_t nReaded = qio_read(nSockFd, pszBuffer, nSize, nTimeoutMs);
    DEBUG("[RX] (binary, readed/request=%zd/%zu bytes)", nReaded, nSize);
    return nReaded;
}

off_t streamSave(int nFd, int nSockFd, off_t nSize, int nTimeoutMs)
{
    off_t nSaved = qio_send(nFd, nSockFd, nSize, nTimeoutMs);
    DEBUG("[RX] (save %jd/%jd bytes)", nSaved, nSize);
    return nSaved;
}

ssize_t streamPrintf(int nSockFd, const char *format, ...)
{
    char *pszBuf;
    DYNAMIC_VSPRINTF(pszBuf, format);
    if (pszBuf == NULL) return -1;

    ssize_t nSent = qio_write(nSockFd, pszBuf, strlen(pszBuf), 0);

#ifdef ENABLE_DEBUG
    if (nSent > 0) DEBUG("[TX] %s", pszBuf);
    else DEBUG("[TX-ERR] %s", pszBuf);
#endif
    free(pszBuf);

    return nSent;
}

ssize_t streamPuts(int nSockFd, const char *pszStr)
{
    ssize_t nSent = qio_puts(nSockFd, pszStr, 0);

#ifdef ENABLE_DEBUG
    if (nSent > 0) DEBUG("[TX] %s", pszStr);
    else DEBUG("[TX-ERR] %s", pszStr);
#endif

    return nSent;
}

ssize_t streamStackOut(int nSockFd, qvector_t *vector, int nTimeoutMs)
{
    size_t nSize;
    char *pData = (char *)vector->toarray(vector, &nSize);

    ssize_t nWritten = 0;
    if (pData != NULL) {
        nWritten = qio_write(nSockFd, pData, nSize, nTimeoutMs);

#ifdef ENABLE_DEBUG
        if (g_debug) {
            pData[nSize - 1] = '\0';
            if (nWritten > 0) DEBUG("[TX] %s", pData);
            else DEBUG("[TX-ERR] %s", pData);
        }
#endif

        free(pData);
    }

    return nWritten;
}

ssize_t streamWrite(int nSockFd, const void *pszBuffer, size_t nSize, int nTimeoutMs)
{
    ssize_t nWritten = qio_write(nSockFd, pszBuffer, nSize, nTimeoutMs);
    DEBUG("[TX] (binary, written/request=%zd/%zu bytes)", nWritten, nSize);

    return nWritten;
}

ssize_t streamWritev(int nSockFd,  const struct iovec *pVector, int nCount, int nTimeoutMs)
{
    ssize_t nWritten = 0;
    int i;
    for (i = 0; i < nCount; i++) {
        ssize_t nSent = streamWrite(nSockFd, pVector[i].iov_base, pVector[i].iov_len, nTimeoutMs);
        if (nSent <= 0) break;
        nWritten += nSent;
    }

    /*
    ssize_t nWritten = writev(nSockFd, pVector, nCount);
    */

    DEBUG("[TX] (binary, written=%zd bytes, %d vectors)", nWritten, nCount);

    return nWritten;
}

off_t streamSend(int nSockFd, int nFd, off_t nSize, int nTimeoutMs)
{
    off_t nSent = qio_send(nSockFd, nFd, nSize, nTimeoutMs);
    DEBUG("[TX] (send %jd/%jd bytes)", nSent, nSize);

    return nSent;
}
