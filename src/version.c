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
 * $Id: version.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#include "qhttpd.h"

void printUsages(void)
{
    printVersion();
    fprintf(stderr, "Usage: %s [-dDVh] [-c configfile]\n", g_prgname);
    fprintf(stderr, "  -c filepath	Set configuration file.\n");
    fprintf(stderr, "  -d		Run as debugging mode.\n");
    fprintf(stderr, "  -D		Do not daemonize.\n");
    fprintf(stderr, "  -V		Version info.\n");
    fprintf(stderr, "  -h		Display this help message and exit.\n");
}

void printVersion(void)
{
    char *verstr = getVersion();
    fprintf(stderr, "%s\n", verstr);
    free(verstr);
}

char *getVersion(void)
{
    qvector_t *ver = qvector();

    ver->addstrf(ver, "%s v%s", g_prgname, g_prgversion);

#ifdef ENABLE_DEBUG
    ver->addstr(ver, " (DEBUG;");
#else
    ver->addstr(ver, " (RELEASE;");
#endif

    ver->addstrf(ver, " %s %s;", __DATE__, __TIME__);

#ifdef ENABLE_LFS
    ver->addstr(ver, " --enable-lfs");
#endif

#ifdef ENABLE_LUA
    ver->addstr(ver, " --enable-lua");
#endif

#ifdef ENABLE_HOOK
    ver->addstr(ver, " --enable-hook");
#endif

    ver->addstr(ver, ")");

    char *final = ver->tostring(ver);
    ver->free(ver);

    return final;
}
