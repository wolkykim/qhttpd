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
 * $Id: http_accesslog.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#include "qhttpd.h"

bool httpAccessLog(struct HttpRequest *pReq, struct HttpResponse *pRes)
{
    if (pReq->pszRequestMethod == NULL) return false;

    const char *pszHost = httpHeaderGetStr(pReq->pHeaders, "HOST");
    const char *pszReferer = httpHeaderGetStr(pReq->pHeaders, "REFERER");
    const char *pszAgent = httpHeaderGetStr(pReq->pHeaders, "USER-AGENT");

    g_acclog->writef(g_acclog, "%s - - [%s] \"%s http://%s%s %s\" %d %jd \"%s\" \"%s\"",
                     poolGetConnAddr(),  qtime_gmt_staticstr(poolGetConnReqTime()),
                     pReq->pszRequestMethod, pszHost, pReq->pszRequestUri, pReq->pszHttpVersion,
                     pRes->nResponseCode, pRes->nContentsLength,
                     (pszReferer != NULL) ? pszReferer : "-",
                     (pszAgent != NULL) ? pszAgent : "-"
                    );

    return true;
}
