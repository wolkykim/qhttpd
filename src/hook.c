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
 * $Id: hook.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#ifdef ENABLE_HOOK

#include "qhttpd.h"

/* EXAMPLE: hash table for doing virtual host
Q_HASHTBL* g_vhostsTbl = NULL;
*/

bool hookBeforeMainInit(void)
{

    /*
    // EXAMPLE: how to change program name to yours
    g_prginfo = "YOUR_PROGRAM_INFO"
    g_prgname = "YOUR_PROGRAM_NAME";
    g_prgversion = "YOUR_PROGRAM_VERSION";
    */

    return true;
}

bool hookAfterConfigLoaded(struct ServerConfig *config, bool bConfigLoadSucceed)
{

    /* EXAMPLE: how to override configurations
    config->nPort = 8080;
    */

    /* EXAMPLE: how to load virtual hosts into table
    Q_HASHTBL *vhostsTbl = qHashtbl(1000, true, 0);
    vhostsTbl->putstr(vhostsTbl, "www.qdecoder.org", "/data/qdecoder");
    vhostsTbl->putstr(vhostsTbl, "www.test.com", "/data/test");
    */

    return true;
}

bool hookAfterDaemonInit(void)
{
    return true;
}

// return : number of jobs did, in case of error return -1
int hookWhileDaemonIdle(void)
{
    return 0;
}

bool hookBeforeDaemonEnd(void)
{
    return true;
}

bool hookAfterDaemonSIGHUP(void)
{
    return true;
}

//
// for each connection
//

bool hookAfterChildInit(void)
{
    return true;
}

bool hookBeforeChildEnd(void)
{
    return true;
}

bool hookAfterConnEstablished(int nSockFd)
{
    return true;
}

//
// request & response hooking
//
// 1. parse request.
// 2.   call luaRequestHandler(), if LUA is enabled
// 3.   call hookRequestHandler(), if HOOK is enabled. (hook.c)
// 4.   call default request handler
// 5.   call hookResponseHandler(), if HOOK is enabled. (hook.c)
// 6.   call luaResponseHandler(), if LUA is enabled
// 7. response out

// return response code if you set response then step 4 will be skipped
// otherwise return 0 to bypass then step 4 will handle the request.
int hookRequestHandler(struct HttpRequest *pReq, struct HttpResponse *pRes)
{
    return 0;

    /* EXAMPLE: how to add or override methods
    int nResCode = 0;
    if(!strcmp(pReq->pszRequestMethod, "METHOD_NAME_TO_ADD_OR_REPLACE")) {
        nResCode = YOUR_METHOD_CALL(req, res);
    }

    return nResCode;
    */

    /* EXAMPLE: how to change document root dynamically
    if(pReq->pszDocRoot != NULL) free(pReq->pszDocRoot);
    pReq->pszDocRoot = strdup("/NEW_DOCUMENT_ROOT");
    return 0; // pass to default method handler
    */

    /* EXAMPLE: how to map virtual hosted document root
    int nResCode = 0;
    char *pVirtualDocRoot = g_vhostsTbl->getstr(g_vhostsTbl, pReq->pszRequestDomain, true);
    if(pVirtualDocRoot != NULL) {
        if(pReq->pszDocumentRoot != NULL) free(pReq->pszDocumentRoot);
        pReq->pszDocumentRoot = pVirtualDocRoot;
        DEBUG("Virtual Root: %s", pVirtualDocRoot);
    } else {
        // otherwise deny service
        nResCode = response404(pReq, pRes);
    }

    return nResCode;
    */

    /* EXAMPLE: HTTP basic authentication
    int nResCode = 0;
    if(!strcmp(pReq->pszRequestMethod, "PUT")) {
        // parse authorization header
        struct HttpUser *pHttpUser = httpAuthParse(pReq);

        // validate authorization header
        bool bAuth = false;
        if(pHttpUser != NULL) {
            DEBUG("User %s, Password %s", pHttpUser->szUser, pHttpUser->szPassword);
            if(YOUR_PASSWORD_CHECK(pHttpUser) == true) {
                bAuth = true;
            }
        }

        // if authentication failed, send 401
        if(bAuth == false) {
            // add Basic or Digest Authentication HTTP header
            httpResponseSetAuthRequired(pRes, HTTP_AUTH_BASIC, "YOUR_REALM");
            //httpResponseSetAuthRequired(pRes, HTTP_AUTH_DIGEST, pReq->pszRequestDomain);

            // set response
            nResCode = httpResponseSetSimple(pReq, pRes, HTTP_CODE_UNAUTHORIZED, true, httpResponseGetMsg(HTTP_CODE_UNAUTHORIZED));
        }
    }
    return nResCode;
    */
}

bool hookResponseHandler(struct HttpRequest *pReq, struct HttpResponse *pRes)
{
    if (pRes->bOut == true) return true;

    // returns false if you want to log out failure during this call.
    return true;
}

#endif
