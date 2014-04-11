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
 * $Id: http_auth.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#include "qhttpd.h"

struct HttpUser *httpAuthParse(struct HttpRequest *pReq) {
    const char *pszAuthHeader = httpHeaderGetStr(pReq->pHeaders, "AUTHORIZATION");
    if (IS_EMPTY_STRING(pszAuthHeader) == true) {
        // no auth header
        return NULL;
    }

    if (!strncasecmp(pszAuthHeader, "Basic ", CONST_STRLEN("Basic "))) {
        struct HttpUser *pHttpUser = (struct HttpUser *)malloc(sizeof(struct HttpUser));
        memset((void *)pHttpUser, 0, sizeof(struct HttpUser));
        pHttpUser->nAuthType = HTTP_AUTH_BASIC;

        // get data
        char *pszDecBase64 = strdup(pszAuthHeader + CONST_STRLEN("Basic "));
        if (IS_EMPTY_STRING(pszDecBase64) == true) {
            // empty auth string
            if (pszDecBase64 != NULL) free(pszDecBase64);
            return pHttpUser;
        }

        // parse data
        qbase64_decode(pszDecBase64);
        char *pszColon = strstr(pszDecBase64, ":");
        if (pszColon == NULL) {
            qstrcpy(pHttpUser->szUser, sizeof(pHttpUser->szUser), pszDecBase64);
        } else {
            *pszColon = '\0';
            qstrcpy(pHttpUser->szUser, sizeof(pHttpUser->szUser), pszDecBase64);
            qstrcpy(pHttpUser->szPassword, sizeof(pHttpUser->szPassword), pszColon + 1);
        }

        DEBUG("Auth BASIC : %s/%s", pHttpUser->szUser, pHttpUser->szPassword);

        free(pszDecBase64);
        return pHttpUser;
    } else if (!strncasecmp(pszAuthHeader, "Digest ", CONST_STRLEN("Digest "))) {
        // not supported yet
        return NULL;
    }

    // not supported auth type
    return NULL;
}
