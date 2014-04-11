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
 * $Id: mime.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#include "qhttpd.h"


#define DEF_MIME_ENTRY  "_DEF_"
#define DEF_MIME_TYPE   "application/octet-stream"

qlisttbl_t *m_mimelist = NULL;

bool mimeInit(const char *pszFilepath)
{
    if (m_mimelist != NULL) return false;

    m_mimelist = qconfig_parse_file(NULL, pszFilepath, '=', true);

    if (m_mimelist == NULL) return false;
    return true;
}

bool mimeFree(void)
{
    if (m_mimelist == NULL) return false;
    m_mimelist->free(m_mimelist);
    m_mimelist = NULL;
    return true;
}

const char *mimeDetect(const char *pszFilename)
{
    if (pszFilename == NULL || m_mimelist == NULL) return DEF_MIME_TYPE;

    char *pszMimetype = NULL;
    char *pszExt = qfile_get_ext(pszFilename);
    if (pszExt != NULL) {
        qstrupper(pszExt);
        pszMimetype = (char *)m_mimelist->getstr(m_mimelist, pszExt, false);
        free(pszExt);
    }

    if (pszMimetype == NULL) {
        pszMimetype = (char *)m_mimelist->getstr(m_mimelist, DEF_MIME_ENTRY, false);
        if (pszMimetype == NULL) pszMimetype = DEF_MIME_TYPE;
    }

    return pszMimetype;
}
