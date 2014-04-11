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
 * $Id: config.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#include "qhttpd.h"

#define fetch2Str(e, d, n)  do {                                        \
        const char *t = e->getstr(e, n, false);                         \
        if(t == NULL) {                                                 \
            DEBUG("No such key : %s", n);                               \
            e->free(e);                                                 \
            return false;                                               \
        }                                                               \
        qstrcpy(d, sizeof(d), t);                                       \
    } while (false)


#define fetch2Int(e, d, n)                                              \
    do {                                                                \
        const char *t = e->getstr(e, n, false);                         \
        if(t == NULL) {                                                 \
            DEBUG("No such key : %s", n);                               \
            e->free(e);                                                 \
            return false;                                               \
        }                                                               \
        d = atoi(t);                                                    \
    } while (false)

#define fetch2Bool(e, d, n)                                             \
    do {                                                                \
        const char *t = e->getstr(e, n, false);                         \
        if(t == NULL) {                                                 \
            DEBUG("No such key : %s", n);                               \
            e->free(e);                                                 \
            return false;                                               \
        }                                                               \
        d = (!strcasecmp(t, "YES") || !strcasecmp(t, "TRUE") ||         \
             !strcasecmp(t, "ON")) ? true : false;                      \
    } while (false)

/**
 * configuration parser
 *
 * @param pConf     ServerConfig structure
 * @param pszFilePath   file path of egis.conf
 * @return true if successful otherwise returns false
 */
bool loadConfig(struct ServerConfig *pConf, char *pszFilePath)
{
    if (pszFilePath == NULL || !strcmp(pszFilePath, "")) {
        return false;
    }

    // parse configuration file
    qlisttbl_t *conflist = qconfig_parse_file(NULL, pszFilePath, '=', true);
    if (conflist == NULL) {
        DEBUG("Can't open file %s", pszFilePath);
        return false;
    }

    // copy to structure
    qstrcpy(pConf->szConfigFile, sizeof(pConf->szConfigFile), pszFilePath);

    fetch2Str(conflist, pConf->szPidFile, "PidFile");
    fetch2Str(conflist, pConf->szMimeFile, "MimeFile");

    fetch2Int(conflist, pConf->nPort, "Port");

    fetch2Int(conflist, pConf->nStartServers, "StartServers");
    fetch2Int(conflist, pConf->nMinSpareServers, "MinSpareServers");
    fetch2Int(conflist, pConf->nMaxSpareServers, "MaxSpareServers");
    fetch2Int(conflist, pConf->nMaxIdleSeconds, "MaxIdleSeconds");
    fetch2Int(conflist, pConf->nMaxClients, "MaxClients");
    fetch2Int(conflist, pConf->nMaxRequestsPerChild, "MaxRequestsPerChild");

    fetch2Bool(conflist, pConf->bEnableKeepAlive, "EnableKeepAlive");
    fetch2Int(conflist, pConf->nMaxKeepAliveRequests, "MaxKeepAliveRequests");

    fetch2Int(conflist, pConf->nConnectionTimeout, "ConnectionTimeout");
    fetch2Bool(conflist, pConf->bIgnoreOverConnection, "IgnoreOverConnection");
    fetch2Int(conflist, pConf->nResponseExpires, "ResponseExpires");

    fetch2Str(conflist, pConf->szDocumentRoot, "DocumentRoot");

    fetch2Str(conflist, pConf->szAllowedMethods, "AllowedMethods");

    fetch2Str(conflist, pConf->szDirectoryIndex, "DirectoryIndex");

    fetch2Bool(conflist, pConf->bEnableLua, "EnableLua");
    fetch2Str(conflist, pConf->szLuaScript, "LuaScript");

    fetch2Bool(conflist, pConf->bEnableStatus, "EnableStatus");
    fetch2Str(conflist, pConf->szStatusUrl, "StatusUrl");

    fetch2Str(conflist, pConf->szErrorLog, "ErrorLog");
    fetch2Str(conflist, pConf->szAccessLog, "AccessLog");
    fetch2Int(conflist, pConf->nLogRotate, "LogRotate");
    fetch2Int(conflist, pConf->nLogLevel, "LogLevel");

    // check config
    checkConfig(pConf);

    //
    // free resources
    //
    conflist->free(conflist);

    return true;
}

bool checkConfig(struct ServerConfig *pConf)
{
    // allowed methods parsing
    qstrupper(pConf->szAllowedMethods);
    if (!strcmp(pConf->szAllowedMethods, "ALL")) {
        qstrcpy(pConf->szAllowedMethods, sizeof(pConf->szAllowedMethods),
                "OPTIONS,HEAD,GET,PUT"
                ","
                "PROPFIND,PROPPATCH,MKCOL,MOVE,DELETE,LOCK,UNLOCK");
    }

    if (strstr(pConf->szAllowedMethods, "OPTIONS") != NULL) pConf->methods.bOptions = true;
    if (strstr(pConf->szAllowedMethods, "HEAD") != NULL) pConf->methods.bHead = true;
    if (strstr(pConf->szAllowedMethods, "GET") != NULL) pConf->methods.bGet = true;
    if (strstr(pConf->szAllowedMethods, "PUT") != NULL) pConf->methods.bPut = true;

    if (strstr(pConf->szAllowedMethods, "PROPFIND") != NULL) pConf->methods.bPropfind = true;
    if (strstr(pConf->szAllowedMethods, "PROPPATCH") != NULL) pConf->methods.bProppatch = true;
    if (strstr(pConf->szAllowedMethods, "MKCOL") != NULL) pConf->methods.bMkcol = true;
    if (strstr(pConf->szAllowedMethods, "MOVE") != NULL) pConf->methods.bMove = true;
    if (strstr(pConf->szAllowedMethods, "DELETE") != NULL) pConf->methods.bDelete = true;
    if (strstr(pConf->szAllowedMethods, "LOCK") != NULL) pConf->methods.bLock = true;
    if (strstr(pConf->szAllowedMethods, "UNLOCK") != NULL) pConf->methods.bUnlock = true;

    return true;
}
