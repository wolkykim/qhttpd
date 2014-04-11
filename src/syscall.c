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
 * $Id: syscall.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#include "qhttpd.h"

int sysOpen(const char *pszPath, int nFlags, mode_t nMode)
{
    return open(pszPath, nFlags, nMode);
}

int sysClose(int nFd)
{
    return close(nFd);
}

int sysStat(const char *pszPath, struct stat *pBuf)
{
    return stat(pszPath, pBuf);
}

int sysFstat(int nFd, struct stat *pBuf)
{
    return fstat(nFd, pBuf);
}

int sysUnlink(const char *pszPath)
{
    return unlink(pszPath);
}

int sysRename(const char *pszOldPath, const char *pszNewPath)
{
    return rename(pszOldPath, pszNewPath);
}

int sysMkdir(const char *pszPath, mode_t nMode)
{
    return mkdir(pszPath, nMode);
}

int sysRmdir(const char *pszPath)
{
    return rmdir(pszPath);
}

DIR *sysOpendir(const char *pszPath)
{
    return opendir(pszPath);
}

struct dirent *sysReaddir(DIR *pDir) {
    return readdir(pDir);
}

int sysClosedir(DIR *pDir)
{
    return closedir(pDir);
}
