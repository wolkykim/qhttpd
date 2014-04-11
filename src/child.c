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
 * $Id: child.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#include "qhttpd.h"

/////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
/////////////////////////////////////////////////////////////////////////
static void childEnd(int nStatus);
static void childSignalInit(void *func);
static void childSignal(int signo);
static void childSignalHandler(void);
static void setClientSocketOption(int nSockFd);

/////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
/////////////////////////////////////////////////////////////////////////

void childStart(int nSockFd)
{
    // init signal
    childSignalInit(childSignal);

    // register at pool.
    if (poolChildReg() == false) {
        LOG_ERR("Can't register myself at the pool.");
        childEnd(EXIT_FAILURE);
    }

#ifdef ENABLE_HOOK
    if (hookAfterChildInit() == false) {
        LOG_ERR("Hook failed.");
        childEnd(EXIT_FAILURE);
    }
#endif

#ifdef ENABLE_LUA
    if (g_conf.bEnableLua == true) {
        if (luaInit(g_conf.szLuaScript) == false) {
            LOG_WARN("Can't initialize lua engine.");
        }
    }
#endif

    // init random
    srand((unsigned)(time(NULL) + getpid() + nSockFd));

    int nIdleCnt = 0;
    while (true) {
        //
        // SECTION: connection waiting
        //

        // signal handling
        childSignalHandler();

        // check exit request
        if (poolGetExitRequest() == true) {
            DEBUG("Caughted exit request.");
            break;
        }

        // check maximum requests
        if (g_conf.nMaxRequestsPerChild > 0
            && poolGetChildTotalRequests() >= g_conf.nMaxRequestsPerChild) {
            DEBUG("Maximum requests per child are reached. (%d/%d)", poolGetChildTotalRequests(), g_conf.nMaxRequestsPerChild);
            break;
        }

        // wait connection
        int nStatus = qio_wait_readable(nSockFd, 1000); // wait 1 sec
        if (nStatus < 0) break;
        else if (nStatus == 0) {
            // periodic(1 sec) job here

            // idle time check
            nIdleCnt++;
            if (g_conf.nMaxIdleSeconds > 0 && nIdleCnt > g_conf.nMaxIdleSeconds) {
                int nRunningChilds, nIdleChilds;
                nRunningChilds = poolGetNumChilds(NULL, &nIdleChilds);
                if (nRunningChilds > g_conf.nStartServers && nIdleChilds > g_conf.nMinSpareServers) {
                    DEBUG("Maximum idle seconds(%d) are reached.", g_conf.nMaxIdleSeconds);
                    break;
                }
            }

            continue;
        }

        // new connection arrived
        nIdleCnt = 0;

        struct sockaddr_in connAddr;     // client address information
        socklen_t nConnLen = sizeof(connAddr);
        int nNewSockFd;
        if ((nNewSockFd = accept(nSockFd, (struct sockaddr *)&connAddr, &nConnLen)) == -1) {
            // caught by another process
            //DEBUG("I'm late...");
            continue;
        }

        //
        // SECTION: connection established
        //

        // connection accepted
        DEBUG("Connection established.");

        // set socket option
        setClientSocketOption(nNewSockFd);

#ifdef ENABLE_HOOK
        // connection hook
        if (hookAfterConnEstablished(nNewSockFd) == true) {
#endif
            // register client information
            if (poolSetConnInfo(nNewSockFd) == true) {
                // launch main logic
                httpMain(nNewSockFd);
            }
#ifdef ENABLE_HOOK
        } else {
            LOG_ERR("Hook failed.");
        }
#endif
        // close connection
        closeSocket(nNewSockFd);

        // clear client information
        poolClearConnInfo();

        DEBUG("Closing connection.");
    }

    // ending connection
    childEnd(EXIT_SUCCESS);
}

static void childEnd(int nStatus)
{
    static bool bAlready = false;

    if (bAlready == true) return;
    bAlready = true;

#ifdef ENABLE_LUA
    // release lua
    if (g_conf.bEnableLua == true) luaFree();
#endif

#ifdef ENABLE_HOOK
    // hook
    if (hookBeforeChildEnd() == false) {
        LOG_ERR("Hook failed.");
    }
#endif

    // remove child info
    if (poolChildDel(0) == false) {
        LOG_WARN("Can't find pid %d from connection list", getpid());
    }

    // quit
    LOG_INFO("Child terminated.");

    // close log
    if (g_acclog != NULL) {
        g_acclog->free(g_acclog);
        g_acclog = NULL;
    }
    if (g_errlog != NULL) {
        g_errlog->free(g_errlog);
        g_errlog = NULL;
    }

    exit(nStatus);
}

static void childSignalInit(void *func)
{
    // init sigaction
    struct sigaction sa;
    sa.sa_handler = func;
    sa.sa_flags = 0;
    sigemptyset (&sa.sa_mask);

    // to handle
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    // to ignore
    signal(SIGPIPE, SIG_IGN);

    // reset signal flags;
    sigemptyset(&g_sigflags);
}

static void childSignal(int signo)
{
    sigaddset(&g_sigflags, signo);
    if (signo == SIGTERM || signo == SIGINT) childSignalHandler();
}

static void childSignalHandler(void)
{
    if (sigismember(&g_sigflags, SIGHUP)) {
        sigdelset(&g_sigflags, SIGHUP);
        LOG_INFO("Child : Caughted SIGHUP ");

        if (poolSetExitRequest() == false) childEnd(EXIT_FAILURE);
    } else if (sigismember(&g_sigflags, SIGTERM)) {
        sigdelset(&g_sigflags, SIGTERM);
        LOG_INFO("Child : Caughted SIGTERM");

        childEnd(EXIT_SUCCESS);
    } else if (sigismember(&g_sigflags, SIGINT)) {
        sigdelset(&g_sigflags, SIGINT);
        LOG_INFO("Child : Caughted SIGINT");

        childEnd(EXIT_SUCCESS);
    } else if (sigismember(&g_sigflags, SIGUSR1)) {
        sigdelset(&g_sigflags, SIGUSR1);
        LOG_INFO("Child : Caughted SIGUSR1");

        if (g_loglevel < MAX_LOGLEVEL) g_loglevel++;
        LOG_INFO("Increasing log-level to %d.", g_loglevel);

    } else if (sigismember(&g_sigflags, SIGUSR2)) {
        sigdelset(&g_sigflags, SIGUSR2);
        LOG_INFO("Child : Caughted SIGUSR2");

        if (g_loglevel > 0) g_loglevel--;
        LOG_INFO("Decreasing log-level to %d.", g_loglevel);
    }
}

static void setClientSocketOption(int nSockFd)
{
    // linger option
    if (SET_TCP_LINGER_TIMEOUT > 0) {
        struct linger li;
        li.l_onoff = 1;
        li.l_linger = SET_TCP_LINGER_TIMEOUT;
        if (setsockopt(nSockFd, SOL_SOCKET, SO_LINGER, &li, sizeof(struct linger)) < 0) {
            LOG_WARN("Socket option(SO_LINGER) set failed.");
        }
    }

    // nodelay option
    if (SET_TCP_NODELAY > 0) {
        int so_tcpnodelay = 1;
        if (setsockopt(nSockFd, IPPROTO_TCP, TCP_NODELAY, &so_tcpnodelay, sizeof(so_tcpnodelay)) < 0) {
            LOG_WARN("Socket option(TCP_NODELAY) set failed.");
        }
    }

    // nonblock socket
    /*
    int nSockFlags = fcntl(nSockFd, F_GETFL, 0);
    fcntl(nSockFd, F_SETFL, nSockFlags | O_NONBLOCK);
    */
}
