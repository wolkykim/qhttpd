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
 * $Id: daemon.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#include "qhttpd.h"

/////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
/////////////////////////////////////////////////////////////////////////
static int m_nBindSockFd = -1;

static void daemonEnd(int nStatus);
static void daemonSignalInit(void *func);
static void daemonSignal(int signo);
static void daemonSignalHandler(void);
static bool ignoreConnection(int nSockFd, long int nTimeoutMs);

/////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
/////////////////////////////////////////////////////////////////////////

void daemonStart(bool nDaemonize)
{
    // init signal
    daemonSignalInit(daemonSignal);

    // set mask
    umask(0);

    // open logs
    g_errlog = qlog(g_conf.szErrorLog, 0644, g_conf.nLogRotate, true);
    g_acclog = qlog(g_conf.szAccessLog, 0644, g_conf.nLogRotate, true);
    if (g_errlog == NULL || g_acclog == NULL) {
        fprintf(stderr, "Can't open log file.\n");
        daemonEnd(EXIT_FAILURE);
    }

    // entering daemon mode
    if (nDaemonize) {
        FILE *dup_stderr = fdopen(dup(fileno(stderr)), "w");
        daemon(false, false); // after this line, parent's pid will be changed.
        g_errlog->duplicate(g_errlog, dup_stderr, true);
    } else {
        g_errlog->duplicate(g_errlog, stdout, true);
        g_acclog->duplicate(g_acclog, stdout, false);
    }

    // save pid
    if (qcount_save(g_conf.szPidFile, getpid()) == false) {
        LOG_ERR("Can't create pid file.");
        daemonEnd(EXIT_FAILURE);
    }

    // init semaphore
    if ((g_semid = qsem_init(g_conf.szPidFile, 's', MAX_SEMAPHORES, true)) < 0) {
        LOG_ERR("Can't initialize semaphore.");
        daemonEnd(EXIT_FAILURE);
    }
    LOG_INFO("Semaphore created.");

    // init shared memory
    if (poolInit(g_conf.nMaxClients) == false) {
        LOG_ERR("Can't initialize child management pool.");
        daemonEnd(EXIT_FAILURE);
    }
    LOG_INFO("Child management pool created.");

    // load mime
    if (IS_EMPTY_STRING(g_conf.szMimeFile) == false) {
        if (mimeInit(g_conf.szMimeFile) == true) {
            LOG_INFO("Mimetypes loaded.");
        } else {
            LOG_WARN("Can't load mimetypes from %s", g_conf.szMimeFile);
        }
    } else {
        LOG_INFO("No mimetype configuration file set.");
    }

    // init socket
    int nSockFd;
    if ((nSockFd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        LOG_ERR("Can't create socket.");
        daemonEnd(EXIT_FAILURE);
    } else {
        int so_reuseaddr = 1;
        int so_keepalive = 0;
        int so_tcpnodelay = 0;
        int so_sndbufsize = 0; //32 * 1024;
        int so_rcvbufsize = 0; //32 * 1024;

        if (so_reuseaddr > 0) setsockopt(nSockFd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr));
        if (so_keepalive > 0) setsockopt(nSockFd, SOL_SOCKET, SO_KEEPALIVE, &so_keepalive, sizeof(so_keepalive));
        if (so_tcpnodelay > 0) setsockopt(nSockFd, IPPROTO_TCP, TCP_NODELAY, &so_tcpnodelay, sizeof(so_tcpnodelay));
        if (so_sndbufsize > 0) setsockopt(nSockFd, SOL_SOCKET, SO_SNDBUF, &so_sndbufsize, sizeof(so_sndbufsize));
        if (so_rcvbufsize > 0) setsockopt(nSockFd, SOL_SOCKET, SO_RCVBUF, &so_rcvbufsize, sizeof(so_rcvbufsize));
    }

    // save it
    m_nBindSockFd = nSockFd; // store bind sock id

    // set to non-block socket
    int nSockFlags = fcntl(nSockFd, F_GETFL, 0);
    fcntl(nSockFd, F_SETFL, nSockFlags | O_NONBLOCK);

    // bind
    struct sockaddr_in svrAddr;     // server address information
    svrAddr.sin_family = AF_INET;       // host byte order
    svrAddr.sin_port = htons(g_conf.nPort); // short, network byte order
    svrAddr.sin_addr.s_addr = INADDR_ANY;   // auto-fill with my IP
    memset((void *)&(svrAddr.sin_zero), 0, sizeof(svrAddr.sin_zero)); // zero the rest of the struct

    if (bind(nSockFd, (struct sockaddr *)&svrAddr, sizeof(struct sockaddr)) == -1) {
        LOG_ERR("Can't bind port %d (errno: %d)", g_conf.nPort, errno);
        daemonEnd(EXIT_FAILURE);
    }
    LOG_INFO("Binding port %d succeed.", g_conf.nPort);

    // listen
    if (listen(nSockFd, MAX_LISTEN_BACKLOG) == -1) {
        LOG_ERR("Can't listen port %d.", g_conf.nPort);
        daemonEnd(EXIT_FAILURE);
    }

#ifdef ENABLE_HOOK
    // after init hook
    if (hookAfterDaemonInit() == false) {
        LOG_ERR("Hook failed.");
        daemonEnd(EXIT_FAILURE);
    }
#endif

    // succeed initialization, turn off error out
    if (nDaemonize) {
        g_errlog->duplicate(g_errlog, NULL, false);
    }

    // starting.
    LOG_SYS("%s %s is ready on the port %d.", g_prgname, g_prgversion, g_conf.nPort);

    // prefork management
    int nIgnoredConn = 0;
    int nChildFlag = 0; // n : number of spares required, -n : number of times reqched over max idle, 0 : no action
    while (true) {
        // signal handling
        daemonSignalHandler();

        //
        // SECTION: calculate prefork status
        //

        // get child count
        int nTotalLaunched = poolGetTotalLaunched();
        int nRunningChilds, nWorkingChilds, nIdleChilds;
        nRunningChilds = poolGetNumChilds(&nWorkingChilds, &nIdleChilds);

        // increase or decrease childs
        if (nRunningChilds < g_conf.nStartServers) { // should be launched at least start servers
            // increase spare servers
            nChildFlag = g_conf.nStartServers - nRunningChilds;
        } else {
            // running childs are equal or more than start servers
            if (nIdleChilds < g_conf.nMinSpareServers) { // not enough idle childs
                if (nRunningChilds < g_conf.nMaxClients) {
                    // increase spare servers
                    nChildFlag = g_conf.nMinSpareServers - nIdleChilds;
                    if (nChildFlag + nRunningChilds > g_conf.nMaxClients) {
                        nChildFlag = g_conf.nMaxClients - nRunningChilds;
                    }
                } else if (nIdleChilds <= 0) {
                    // max connection reached
                    nChildFlag = 0;

                    // ignore connectin
                    if (g_conf.bIgnoreOverConnection == true) {
                        while (ignoreConnection(nSockFd, 0) == true) {
                            nIgnoredConn++;
                            LOG_WARN("Maximum connection reached. Connection ignored. (%d)", nIgnoredConn);
                        }
                    }
                }
            } else if (nIdleChilds > g_conf.nMaxSpareServers) {
                // too much idle childs
                if (nRunningChilds > g_conf.nStartServers) {
                    nChildFlag--;
                } else {
                    nChildFlag = 0;
                }
            } else {
                // between min and max
                nChildFlag = 0;
            }
        }

        //
        // SECTION: prefork control
        //
        //DEBUG("%d %d %d %d", nRunningChilds, nWorkingChilds, nIdleChilds, nChildFlag);

        // launching spare server
        if (nChildFlag > 0) {
            DEBUG("Launching %d/%d spare servers. (working:%d, running:%d)", nChildFlag, MAX_PREFORK_AT_ONCE, nWorkingChilds, nRunningChilds);

            int i;
            for (i = 0; i < nChildFlag && i < MAX_PREFORK_AT_ONCE; i++) {
                int nCpid = fork();
                if (nCpid < 0) { // error
                    LOG_ERR("Can't create child.");
                    usleep(100 * 1000);
                } else if (nCpid == 0) { // this is child
                    DEBUG("Child %d launched", getpid());

                    // main job
                    childStart(nSockFd);

                    // safety code, never reached.
                    daemonEnd(EXIT_FAILURE);
                } else { // this is parent
                    // wait for the child register itself to shared pool.
                    int nWait;
                    for (nWait = 0; nWait < 10000; nWait++) {
                        //DEBUG("%d %d", nTotalLaunched, poolGetTotalLaunched());
                        if (nTotalLaunched != poolGetTotalLaunched()) break;
                        DEBUG("Waiting child registered at pool. [%d]", nWait+1);
                        usleep(1*100);
                    }
                    if (nWait == 10000) {
                        LOG_WARN("Delayed child launching.");
                    }

                    // reset flag
                    nChildFlag = 0;
                }
            }

            continue;
        }

        // decrease spare server
        if (nChildFlag < 0) {
            // removing 1 child per sec
            if (nChildFlag <= (-1 * KILL_IDLE_INTERVAL)) {
                //DEBUG("Removing 1(%d) spare server. (working:%d, running:%d)", nChildFlag, nWorkingChilds, nRunningChilds);

                // reset flag
                nChildFlag = 0;

                if (poolSetIdleExitReqeust(1) <= 0) {
                    LOG_WARN("Can't set exit flag.");
                }
            }
        }

        //
        // SECTION: periodic job
        //
        static time_t nLastSec = 0;
        if (time(NULL) - nLastSec < PERIODIC_JOB_INTERVAL) {
            // get some sleep
            usleep(1 * 1000);
            continue;
        } else {
            //DEBUG("%d %d %d %d", nRunningChilds, nWorkingChilds, nIdleChilds, nChildFlag);

            // safety code : check semaphore dead-lock bug
            static int nSemLockCnt[MAX_SEMAPHORES];
            int i;
            for (i = 0; i < MAX_SEMAPHORES; i++) {
                if (qsem_check(g_semid, i) == true) {
                    nSemLockCnt[i]++;
                    if (nSemLockCnt[i] > MAX_SEMAPHORES_LOCK_SECS) {
                        LOG_ERR("Force to unlock semaphore no %d", i);
                        qsem_leave(g_semid, i);  // force to unlock
                        nSemLockCnt[i] = 0;
                    }
                } else {
                    nSemLockCnt[i] = 0;
                }
            }

            // check pool
            if (poolCheck() == true) {
                LOG_WARN("Child count mismatch. fixed.");
            }

#ifdef ENABLE_HOOK
            if (hookWhileDaemonIdle() < 0) {
                LOG_ERR("Hook failed.");
            }
#endif

            // update running time
            nLastSec = time(NULL);
        }
    }

    daemonEnd(EXIT_SUCCESS);
}

static void daemonEnd(int nStatus)
{
    static bool bAlready = false;

    if (bAlready == true) return;
    bAlready = true;

    int nRunningChilds, nWait;
    for (nWait = 15; nWait >= 0 && (nRunningChilds = poolGetNumChilds(NULL, NULL)) > 0; nWait--) {
        if (nWait > 5) {
            LOG_INFO("Soft shutting down [%d]. Waiting %d childs.", nWait-5, nRunningChilds);
            poolSetIdleExitReqeust(nRunningChilds);
        } else if (nWait > 0) {
            LOG_INFO("Hard shutting down [%d]. Waiting %d childs.", nWait, nRunningChilds);
            kill(0, SIGTERM);
        } else {
            LOG_WARN("%d childs are still alive. Give up!", nRunningChilds);
        }

        // sleep
        while (sleep(1) != 0);

        // waitpid
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }

#ifdef ENABLE_HOOK
    if (hookBeforeDaemonEnd() == false) {
        LOG_ERR("Hook failed.");
    }
#endif

    // close bind sock
    if (m_nBindSockFd >= 0) {
        close(m_nBindSockFd);
        m_nBindSockFd = -1;
    }

    // destroy mime
    if (mimeFree() == false) {
        LOG_WARN("Can't destroy mime types.");
    }

    // destroy shared memory
    if (poolFree() == false) {
        LOG_WARN("Can't destroy child management pool .");
    } else {
        LOG_INFO("Child management pool destroied.");
    }

    // destroy semaphore
    int i;
    for (i = 0; i < MAX_SEMAPHORES; i++) qsem_leave(g_semid, i); // force to unlock every semaphores
    if (qsem_free(g_semid) == false) {
        LOG_WARN("Can't destroy semaphore.");
    } else {
        LOG_INFO("Semaphore destroied.");
    }

    // remove pid file
    if (qfile_exist(g_conf.szPidFile) == true && unlink(g_conf.szPidFile) != 0) {
        LOG_WARN("Can't remove pid file %s", g_conf.szPidFile);
    }

    // final
    LOG_SYS("%s Terminated.", g_prgname);

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

static void daemonSignalInit(void *func)
{
    // init sigaction
    struct sigaction sa;
    sa.sa_handler = func;
    sa.sa_flags = 0;
    sigemptyset (&sa.sa_mask);

    // to handle
    sigaction(SIGCHLD, &sa, NULL);
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

static void daemonSignal(int signo)
{
    sigaddset(&g_sigflags, signo);
}

static void daemonSignalHandler(void)
{
    if (sigismember(&g_sigflags, SIGCHLD)) {
        sigdelset(&g_sigflags, SIGCHLD);
        DEBUG("Caughted SIGCHLD");

        pid_t nChildPid;
        int nChildStatus = 0;
        while ((nChildPid = waitpid(-1, &nChildStatus, WNOHANG)) > 0) {
            DEBUG("Detecting child(%d) terminated. Status : %d", nChildPid, nChildStatus);

            // if child is killed unexpectly such like SIGKILL, we remove child info here
            if (poolChildDel(nChildPid) == true) {
                LOG_WARN("Child %d killed unexpectly.", nChildPid);
            }
        }
    } else if (sigismember(&g_sigflags, SIGHUP)) {
        sigdelset(&g_sigflags, SIGHUP);
        LOG_INFO("Caughted SIGHUP");

        struct ServerConfig newconf;
        memset((void *)&newconf, 0, sizeof(newconf));
        bool bConfigLoadStatus = loadConfig(&newconf, g_conf.szConfigFile);

#ifdef ENABLE_HOOK
        bConfigLoadStatus = hookAfterConfigLoaded(&newconf, bConfigLoadStatus);
        if (bConfigLoadStatus == true) {
            if (checkConfig(&newconf) == false) {
                LOG_ERR("Failed to verify configuration.");
                bConfigLoadStatus = false;
            }
        }
#endif

        if (bConfigLoadStatus == true) {
            // copy new config
            g_conf = newconf;

            // reload mime
            mimeFree();
            if (mimeInit(g_conf.szMimeFile) == false) {
                LOG_WARN("Failed to load mimetypes from %s", g_conf.szMimeFile);
            }

#ifdef ENABLE_HOOK
            // hup hook
            if (hookAfterDaemonSIGHUP() == false) {
                LOG_ERR("Hook failed.");
            }
#endif
            // re-launch childs
            poolSetExitReqeustAll();

            LOG_INFO("Configuration re-loaded.");
        } else {
            LOG_ERR("Can't reload configuration.");
        }
    } else if (sigismember(&g_sigflags, SIGTERM)) {
        sigdelset(&g_sigflags, SIGTERM);
        LOG_INFO("Caughted SIGTERM");

        daemonEnd(EXIT_SUCCESS);
    } else if (sigismember(&g_sigflags, SIGINT)) {
        sigdelset(&g_sigflags, SIGINT);
        LOG_INFO("Caughted SIGINT");

        daemonEnd(EXIT_SUCCESS);
    } else if (sigismember(&g_sigflags, SIGUSR1)) {
        sigdelset(&g_sigflags, SIGUSR1);
        LOG_INFO("Caughted SIGUSR1");

        if (g_loglevel < MAX_LOGLEVEL) g_loglevel++;
        poolSendSignal(SIGUSR1);
        LOG_SYS("Increasing log-level to %d.", g_loglevel);

    } else if (sigismember(&g_sigflags, SIGUSR2)) {
        sigdelset(&g_sigflags, SIGUSR2);
        LOG_INFO("Caughted SIGUSR2");

        if (g_loglevel > 0) g_loglevel--;
        poolSendSignal(SIGUSR2);
        LOG_SYS("Decreasing log-level to %d.", g_loglevel);
    }
}

static bool ignoreConnection(int nSockFd, long int nTimeoutMs)
{
    struct sockaddr_in connAddr;
    socklen_t nConnLen = sizeof(connAddr);
    int nNewSockFd;

    // wait connection
    if (qio_wait_readable(nSockFd, nTimeoutMs) <= 0) return false;

    // accept connection
    if ((nNewSockFd = accept(nSockFd, (struct sockaddr *)&connAddr, &nConnLen)) == -1) return false;

    //
    // caughted connection
    //

    // parse request
    struct HttpRequest *pReq = httpRequestParse(nSockFd, g_conf.nConnectionTimeout);
    if (pReq == NULL) {
        LOG_ERR("Can't parse request.");
        closeSocket(nNewSockFd);
        return false;
    }

    // create response
    struct HttpResponse *pRes = httpResponseCreate(pReq);
    if (pRes == NULL) {
        LOG_ERR("Can't create response.");
        httpRequestFree(pReq);
        closeSocket(nNewSockFd);
        return false;
    }

    // set response
    pRes->pszHttpVersion = strdup(HTTP_PROTOCOL_11);
    pRes->nResponseCode = HTTP_CODE_SERVICE_UNAVAILABLE;
    httpHeaderSetStr(pRes->pHeaders, "Connection", "close");

    // serialize & stream out
    httpResponseOut(pRes);

    // logging
    httpAccessLog(pReq, pRes);

    // close connection immediately
    closeSocket(nNewSockFd);

    // free resources
    if (pRes != NULL) httpResponseFree(pRes);
    if (pReq != NULL) httpRequestFree(pReq);

    return true;
}
