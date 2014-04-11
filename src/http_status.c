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
 * $Id: http_status.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#include "qhttpd.h"

int httpStatusResponse(struct HttpRequest *pReq, struct HttpResponse *pRes)
{
    if (pReq == NULL || pRes == NULL) return 0;

    qvector_t *obHtml = httpGetStatusHtml();
    if (obHtml == NULL) return response500(pRes);

    // get size
    size_t nHtmlSize = obHtml->list->datasize(obHtml->list);

    // set response
    httpResponseSetCode(pRes, HTTP_CODE_OK, true);
    httpResponseSetContent(pRes, "text/html; charset=\"utf-8\"", NULL, nHtmlSize);

    // print out header
    httpResponseOut(pRes);

    // print out contents
    streamStackOut(pReq->nSockFd, obHtml, pReq->nTimeout * 1000);

    // free
    obHtml->free(obHtml);

    return HTTP_CODE_OK;
}

qvector_t *httpGetStatusHtml(void)
{
    struct SharedData *pShm = NULL;
    pShm = poolGetShm();
    if (pShm == NULL) return NULL;

    char *pszVersionStr = getVersion();

    qvector_t *obHtml = qvector();
    obHtml->addstr(obHtml, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">" CRLF);
    obHtml->addstr(obHtml, "<html>" CRLF);
    obHtml->addstr(obHtml, "<head>" CRLF);
    obHtml->addstrf(obHtml,"  <title>%s/%s Status</title>" CRLF, g_prgname, g_prgversion);
    obHtml->addstr(obHtml, "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />" CRLF);
    obHtml->addstr(obHtml, "  <style type=\"text/css\">" CRLF);
    obHtml->addstr(obHtml, "    body,td,th { font-size:14px; }" CRLF);
    obHtml->addstr(obHtml, "  </style>" CRLF);
    obHtml->addstr(obHtml, "</head>" CRLF);
    obHtml->addstr(obHtml, "<body>" CRLF);

    obHtml->addstrf(obHtml,"<h1>%s/%s Status</h1>" CRLF, g_prgname, g_prgversion);
    obHtml->addstr(obHtml, "<dl>" CRLF);
    obHtml->addstrf(obHtml,"  <dt>Server Version: %s</dt>" CRLF, pszVersionStr);
    obHtml->addstrf(obHtml,"  <dt>Current Time: %s" CRLF, qtime_gmt_staticstr(0));
    obHtml->addstrf(obHtml,"  , Start Time: %s</dt>" CRLF, qtime_gmt_staticstr(pShm->nStartTime));
    obHtml->addstrf(obHtml,"  <dt>Total Connections : %d" CRLF, pShm->nTotalConnected);
    obHtml->addstrf(obHtml,"  , Total Requests : %d</dt>" CRLF, pShm->nTotalRequests);
    obHtml->addstrf(obHtml,"  , Total Launched: %d" CRLF, pShm->nTotalLaunched);
    obHtml->addstrf(obHtml,"  , Running Servers: %d</dt>" CRLF, pShm->nRunningChilds);
    obHtml->addstrf(obHtml,"  , Working Servers: %d</dt>" CRLF, pShm->nWorkingChilds);
    obHtml->addstrf(obHtml,"  <dt>Start Servers: %d" CRLF, g_conf.nStartServers);
    obHtml->addstrf(obHtml,"  , Min Spare Servers: %d" CRLF, g_conf.nMinSpareServers);
    obHtml->addstrf(obHtml,"  , Max Spare Servers: %d" CRLF, g_conf.nMaxSpareServers);
    obHtml->addstrf(obHtml,"  , Max Clients: %d</dt>" CRLF, g_conf.nMaxClients);
    obHtml->addstr(obHtml, "</dl>" CRLF);

    free(pszVersionStr);

    obHtml->addstr(obHtml, "<table width='100%%' border=1 cellpadding=1 cellspacing=0>" CRLF);
    obHtml->addstr(obHtml, "  <tr>" CRLF);
    obHtml->addstr(obHtml, "    <th colspan=5>Server Information</th>" CRLF);
    obHtml->addstr(obHtml, "    <th colspan=5>Current Connection</th>" CRLF);
    obHtml->addstr(obHtml, "    <th colspan=4>Request Information</th>" CRLF);
    obHtml->addstr(obHtml, "  </tr>" CRLF);

    obHtml->addstr(obHtml, "  <tr>" CRLF);
    obHtml->addstr(obHtml, "    <th>SNO</th>" CRLF);
    obHtml->addstr(obHtml, "    <th>PID</th>" CRLF);
    obHtml->addstr(obHtml, "    <th>Started</th>" CRLF);
    obHtml->addstr(obHtml, "    <th>Conns</th>" CRLF);
    obHtml->addstr(obHtml, "    <th>Reqs</th>" CRLF);

    obHtml->addstr(obHtml, "    <th>Status</th>" CRLF);
    obHtml->addstr(obHtml, "    <th>Client IP</th>" CRLF);
    obHtml->addstr(obHtml, "    <th>Conn Time</th>" CRLF);
    obHtml->addstr(obHtml, "    <th>Runs</th>" CRLF);
    obHtml->addstr(obHtml, "    <th>Reqs</th>" CRLF);

    obHtml->addstr(obHtml, "    <th>Request Information</th>" CRLF);
    obHtml->addstr(obHtml, "    <th>Res</th>" CRLF);
    obHtml->addstr(obHtml, "    <th>Req Time</th>" CRLF);
    obHtml->addstr(obHtml, "    <th>Runs</th>" CRLF);
    obHtml->addstr(obHtml, "  </tr>" CRLF);

    int i, j;
    for (i = j = 0; i <  g_conf.nMaxClients; i++) {
        if (pShm->child[i].nPid == 0) continue;
        j++;

        char *pszStatus = "-";
        int nConnRuns = 0;
        float nReqRuns = 0;

        if (pShm->child[i].conn.bConnected == true) {
            if (pShm->child[i].conn.bRun == true) pszStatus = "W";
            else pszStatus = "K";
        }

        if (pShm->child[i].conn.nEndTime >= pShm->child[i].conn.nStartTime) nConnRuns = difftime(pShm->child[i].conn.nEndTime, pShm->child[i].conn.nStartTime);
        else if (pShm->child[i].conn.nStartTime > 0) nConnRuns = difftime(time(NULL), pShm->child[i].conn.nStartTime);

        if (pShm->child[i].conn.bRun == true) nReqRuns = getDiffTimeval(NULL, &pShm->child[i].conn.tvReqTime);
        else nReqRuns = getDiffTimeval(&pShm->child[i].conn.tvResTime, &pShm->child[i].conn.tvReqTime);

        char szTimeStr[sizeof(char) * (CONST_STRLEN("YYYYMMDDhhmmss")+1)];
        obHtml->addstr(obHtml, "  <tr align=center>" CRLF);
        obHtml->addstrf(obHtml,"    <td>%d</td>" CRLF, j);
        obHtml->addstrf(obHtml,"    <td>%u</td>" CRLF, (unsigned int)pShm->child[i].nPid);
        obHtml->addstrf(obHtml,"    <td>%s</td>" CRLF, qtime_gmt_strf(szTimeStr, sizeof(szTimeStr), pShm->child[i].nStartTime, "%Y%m%d%H%M%S"));
        obHtml->addstrf(obHtml,"    <td align=right>%d</td>" CRLF, pShm->child[i].nTotalConnected);
        obHtml->addstrf(obHtml,"    <td align=right>%d</td>" CRLF, pShm->child[i].nTotalRequests);

        obHtml->addstrf(obHtml,"    <td>%s</td>" CRLF, pszStatus);
        obHtml->addstrf(obHtml,"    <td align=left>%s:%d</td>" CRLF, pShm->child[i].conn.szAddr, pShm->child[i].conn.nPort);
        obHtml->addstrf(obHtml,"    <td>%s</td>" CRLF, (pShm->child[i].conn.nStartTime > 0) ? qtime_gmt_strf(szTimeStr, sizeof(szTimeStr), pShm->child[i].conn.nStartTime, "%Y%m%d%H%M%S") : "&nbsp;");
        obHtml->addstrf(obHtml,"    <td align=right>%ds</td>" CRLF, nConnRuns);
        obHtml->addstrf(obHtml,"    <td align=right>%d</td>" CRLF, pShm->child[i].conn.nTotalRequests);

        obHtml->addstrf(obHtml,"    <td align=left>%s&nbsp;</td>" CRLF, pShm->child[i].conn.szReqInfo);
        if (pShm->child[i].conn.nResponseCode == 0) obHtml->addstr(obHtml,"    <td>&nbsp;</td>" CRLF);
        else obHtml->addstrf(obHtml,"    <td>%d</td>" CRLF, pShm->child[i].conn.nResponseCode);
        obHtml->addstrf(obHtml,"    <td>%s</td>" CRLF, (pShm->child[i].conn.tvReqTime.tv_sec > 0) ? qtime_gmt_strf(szTimeStr, sizeof(szTimeStr), pShm->child[i].conn.tvReqTime.tv_sec, "%Y%m%d%H%M%S") : "&nbsp;");
        obHtml->addstrf(obHtml,"    <td align=right>%.1fms</td>" CRLF, (nReqRuns * 1000));
        obHtml->addstr(obHtml, "  </tr>" CRLF);
    }
    obHtml->addstr(obHtml, "</table>" CRLF);

    obHtml->addstr(obHtml, "<hr>" CRLF);
    obHtml->addstrf(obHtml,"%s v%s, %s" CRLF, g_prgname, g_prgversion, g_prginfo);
    obHtml->addstr(obHtml, "</body>" CRLF);
    obHtml->addstr(obHtml, "</html>" CRLF);

    return obHtml;
}
