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
 * $Id: pool.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#include "qhttpd.h"

/////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
/////////////////////////////////////////////////////////////////////////
#define POOL_SEM_ID     (1)
#define POOL_SEM_MAXWAIT    (5000)

static struct SharedData *m_pShm = NULL;
static int m_nShmId = -1;

static int m_nMaxChild = 0;
static int m_nMySlotId = -1; // for this process

/////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTION PROTOTYPES
/////////////////////////////////////////////////////////////////////////

static bool poolInitData(void);
static void poolInitSlot(int nSlotId);
static int poolFindSlot(int nPid);

/////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
/////////////////////////////////////////////////////////////////////////

// returns true, success
// returns false, fail
bool poolInit(int nMaxChild)
{
    struct SharedData *pShm;
    int nShmId;

    if (nMaxChild > MAX_CHILDS) {
        LOG_ERR("The number of maximum childs(%d) is limited to system maximum %d.", nMaxChild, MAX_CHILDS);
        nMaxChild = MAX_CHILDS;
    }

    nShmId = qshm_init(g_conf.szPidFile, '0', sizeof(struct SharedData), true);
    if (nShmId < 0) return false;

    pShm = (struct SharedData *)qshm_get(nShmId);
    if (pShm == NULL) return false;

    m_nShmId = nShmId;
    m_pShm = pShm;
    m_nMaxChild = nMaxChild;
    poolInitData();
    return true;
}

bool poolFree(void)
{
    if (m_nShmId >= 0) {
        qshm_free(m_nShmId);
        m_nShmId = -1;
        m_pShm = NULL;
    }
    return true;
}

struct SharedData *poolGetShm(void) {
    return m_pShm;
}

int poolSendSignal(int signo)
{
    int i, n;

    n = 0;
    for (i = 0; i < m_nMaxChild; i++) {
        if (m_pShm->child[i].nPid == 0) continue;
        if (kill(m_pShm->child[i].nPid, signo) == 0) n++;
    }

    return n;
}

/*
 * Check pool counter consistency.
 */
bool poolCheck(void)
{
    if (m_pShm == NULL) return false;

    if (qsem_enter_nowait(g_semid, POOL_SEM_ID) == false) return false;

    int i, nTotal, nWorking;
    for (i = nTotal = nWorking = 0; i < m_nMaxChild; i++) {
        if (m_pShm->child[i].nPid > 0) {
            nTotal++;
            if (m_pShm->child[i].conn.bConnected == true) {
                nWorking++;
            }
        }
    }

    // check current child
    bool bFixed = false;
    if (m_pShm->nRunningChilds != nTotal) {
        LOG_INFO("Child counter adjusted from %d to %d.", m_pShm->nRunningChilds, nTotal);
        m_pShm->nRunningChilds = nTotal;
        bFixed = true;
    }

    if (m_pShm->nWorkingChilds != nWorking) {
        LOG_INFO("Working counter adjusted from %d to %d.", m_pShm->nWorkingChilds, nWorking);
        m_pShm->nWorkingChilds = nWorking;
        bFixed = true;
    }

    qsem_leave(g_semid, POOL_SEM_ID);

    return bFixed;
}

/*
 * Get number of total launched childs
 *
 * @return umber of total launched childs
 */
int poolGetTotalLaunched(void)
{
    if (m_pShm == NULL) return 0;
    return m_pShm->nTotalLaunched;
}

/*
 * Get number of childs.
 *
 * @return number of running childs
 */
int poolGetNumChilds(int *nWorking, int *nIdling)
{
    if (m_pShm == NULL) return false;

    //qsem_enter_force(g_semid, POOL_SEM_ID, POOL_SEM_MAXWAIT, NULL);

    if (m_pShm->nRunningChilds < 0 || m_pShm->nWorkingChilds < 0
        || m_pShm->nRunningChilds < m_pShm->nWorkingChilds) {
        poolCheck();
    }

    if (nWorking != NULL) *nWorking = m_pShm->nWorkingChilds;
    if (nIdling != NULL) *nIdling = (m_pShm->nRunningChilds - m_pShm->nWorkingChilds);

    //qsem_leave(g_semid, POOL_SEM_ID);

    return m_pShm->nRunningChilds;
}

/*
 * Send exit to number of idle childs.
 *
 * @return number of processes set
 */
int poolSetIdleExitReqeust(int nNum)
{
    int i, nCnt = 0;

    qsem_enter_force(g_semid, POOL_SEM_ID, POOL_SEM_MAXWAIT, NULL);

    // scan first
    for (i = 0; i < m_nMaxChild; i++) {
        if (m_pShm->child[i].nPid > 0 && m_pShm->child[i].conn.bConnected == false) {
            if (m_pShm->child[i].bExit == true) {
                nCnt++;
            }
        }
    }

    // set exit request
    for (i = 0; nCnt < nNum && i < m_nMaxChild; i++) {
        if (m_pShm->child[i].nPid > 0 && m_pShm->child[i].conn.bConnected == false) {
            if (m_pShm->child[i].bExit == false) {
                m_pShm->child[i].bExit = true;
            }
            nCnt++;
        }
    }

    qsem_leave(g_semid, POOL_SEM_ID);

    return nCnt;;
}

/*
 * Send exit to all childs.
 *
 * @return number of processes set
 */
int poolSetExitReqeustAll(void)
{
    qsem_enter_force(g_semid, POOL_SEM_ID, POOL_SEM_MAXWAIT, NULL);

    int i, nCnt = 0;
    for (i = 0; i < m_nMaxChild; i++) {
        if (m_pShm->child[i].nPid > 0) {
            m_pShm->child[i].bExit = true;
            nCnt++;
        }
    }

    qsem_leave(g_semid, POOL_SEM_ID);

    return nCnt;;
}

/////////////////////////////////////////////////////////////////////////
// MEMBER FUNCTIONS - child management
/////////////////////////////////////////////////////////////////////////

/*
 * called by child
 */
bool poolChildReg(void)
{
    if (m_nMySlotId >= 0) {
        LOG_WARN("already registered.");
        return false;
    }

    qsem_enter_force(g_semid, POOL_SEM_ID, POOL_SEM_MAXWAIT, NULL);
    // find empty slot
    int nSlot = poolFindSlot(0);
    if (nSlot < 0) {
        LOG_WARN("Shared Pool FULL. Maximum connection reached.");

        // set global info
        m_pShm->nTotalLaunched++;

        qsem_leave(g_semid, POOL_SEM_ID);
        return false;
    }

    // clear slot
    poolInitSlot(nSlot);
    m_pShm->child[nSlot].nPid = getpid();
    m_pShm->child[nSlot].nStartTime = time(NULL);
    m_pShm->child[nSlot].conn.bConnected = false;

    // set global info
    m_pShm->nTotalLaunched++;
    m_pShm->nRunningChilds++;

    // set member variable
    m_nMySlotId = nSlot;

    qsem_leave(g_semid, POOL_SEM_ID);
    return true;
}

// called by child and daemon
// if nPid is 0, use m_nMySlotId
bool poolChildDel(pid_t nPid)
{
    qsem_enter_force(g_semid, POOL_SEM_ID, POOL_SEM_MAXWAIT, NULL);

    int nSlot;
    if (nPid == 0) {
        nSlot = m_nMySlotId;
    } else {
        nSlot = poolFindSlot(nPid);
    }

    if (nSlot < 0) {
        qsem_leave(g_semid, POOL_SEM_ID);
        return false;
    }

    // clear rest of all
    //poolInitSlot(nSlot);
    m_pShm->child[nSlot].nPid = 0; // data cleaning will be executed at the time when it is reused.
    m_pShm->nRunningChilds--;

    qsem_leave(g_semid, POOL_SEM_ID);
    return true;
}

int poolGetMySlotId(void)
{
    return m_nMySlotId;
}

bool poolGetExitRequest(void)
{
    if (m_nMySlotId < 0) return true;

    return m_pShm->child[m_nMySlotId].bExit;
}

bool poolSetExitRequest(void)
{
    if (m_nMySlotId < 0) return false;

    m_pShm->child[m_nMySlotId].bExit = true;
    return true;
}

/////////////////////////////////////////////////////////////////////////
// MEMBER FUNCTIONS - child
/////////////////////////////////////////////////////////////////////////

int poolGetChildTotalRequests(void)
{
    if (m_nMySlotId < 0) return -1;

    return m_pShm->child[m_nMySlotId].nTotalRequests;
}

int poolGetChildKeepaliveRequests(void)
{
    if (m_nMySlotId < 0) return -1;

    return m_pShm->child[m_nMySlotId].conn.nTotalRequests;
}

/////////////////////////////////////////////////////////////////////////
// MEMBER FUNCTIONS - connection
/////////////////////////////////////////////////////////////////////////

bool poolSetConnInfo(int nSockFd)
{
    if (m_nMySlotId < 0) return NULL;

    struct sockaddr_in sockAddr;
    socklen_t sockSize = sizeof(sockAddr);

    // get client info
    if (getpeername(nSockFd, (struct sockaddr *)&sockAddr, &sockSize) != 0) {
        LOG_WARN("getpeername() failed. (errno:%d)", errno);
        return false;
    }


    qsem_enter_force(g_semid, POOL_SEM_ID, POOL_SEM_MAXWAIT, NULL);

    // set slot data
    m_pShm->child[m_nMySlotId].conn.bConnected = true;
    m_pShm->child[m_nMySlotId].conn.nStartTime = time(NULL);
    m_pShm->child[m_nMySlotId].conn.nTotalRequests = 0;

    m_pShm->child[m_nMySlotId].conn.nSockFd = nSockFd;
    qstrcpy(m_pShm->child[m_nMySlotId].conn.szAddr, sizeof(m_pShm->child[m_nMySlotId].conn.szAddr), inet_ntoa(sockAddr.sin_addr));
    m_pShm->child[m_nMySlotId].conn.nAddr = getIp2Uint(m_pShm->child[m_nMySlotId].conn.szAddr);
    m_pShm->child[m_nMySlotId].conn.nPort = (int)sockAddr.sin_port; // int is more convenience to use

    // set child info
    m_pShm->child[m_nMySlotId].nTotalConnected++;

    // set global info
    m_pShm->nTotalConnected++;
    m_pShm->nWorkingChilds++;

    qsem_leave(g_semid, POOL_SEM_ID);

    return true;
}

bool poolSetConnRequest(struct HttpRequest *pReq)
{
    int nReqSize = sizeof(m_pShm->child[m_nMySlotId].conn.szReqInfo);
    char *pszReqMethod = pReq->pszRequestMethod;
    char *pszReqUri = pReq->pszRequestUri;

    if (pszReqMethod == NULL) pszReqMethod = "";
    if (pszReqUri == NULL) pszReqUri = "";

    snprintf(m_pShm->child[m_nMySlotId].conn.szReqInfo, nReqSize - 1, "%s %s", pszReqMethod, pszReqUri);
    m_pShm->child[m_nMySlotId].conn.szReqInfo[nReqSize - 1] = '\0';

    m_pShm->child[m_nMySlotId].conn.bRun = true;
    m_pShm->child[m_nMySlotId].conn.nResponseCode = 0;
    gettimeofday(&m_pShm->child[m_nMySlotId].conn.tvReqTime, NULL);

    m_pShm->child[m_nMySlotId].conn.nTotalRequests++;
    m_pShm->child[m_nMySlotId].nTotalRequests++;
    m_pShm->nTotalRequests++;

    return true;
}

bool poolSetConnResponse(struct HttpResponse *pRes)
{
    qsem_enter_force(g_semid, POOL_SEM_ID, POOL_SEM_MAXWAIT, NULL);

    m_pShm->child[m_nMySlotId].conn.nResponseCode = pRes->nResponseCode;
    gettimeofday(&m_pShm->child[m_nMySlotId].conn.tvResTime, NULL);
    m_pShm->child[m_nMySlotId].conn.bRun = false;

    qsem_leave(g_semid, POOL_SEM_ID);

    return true;
}

bool poolClearConnInfo(void)
{
    if (m_nMySlotId < 0) return NULL;

    qsem_enter_force(g_semid, POOL_SEM_ID, POOL_SEM_MAXWAIT, NULL);

    m_pShm->child[m_nMySlotId].conn.bConnected = false;
    m_pShm->child[m_nMySlotId].conn.nEndTime = time(NULL); // set endtime

    m_pShm->nWorkingChilds--;

    qsem_leave(g_semid, POOL_SEM_ID);

    return true;
}

char *poolGetConnAddr(void)
{
    if (m_nMySlotId < 0) return NULL;

    return m_pShm->child[m_nMySlotId].conn.szAddr;
}

unsigned int poolGetConnNaddr(void)
{
    if (m_nMySlotId < 0) return -1;

    return m_pShm->child[m_nMySlotId].conn.nAddr;
}

int poolGetConnPort(void)
{
    if (m_nMySlotId < 0) return -1;

    return m_pShm->child[m_nMySlotId].conn.nPort;
}

time_t poolGetConnReqTime(void)
{
    if (m_nMySlotId < 0) return -1;

    return m_pShm->child[m_nMySlotId].conn.tvReqTime.tv_sec;
}

/////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS - child
/////////////////////////////////////////////////////////////////////////

// clear shared memory
static bool poolInitData(void)
{
    if (m_pShm == NULL) return false;

    // clear everything at the first
    memset((void *)m_pShm, 0, sizeof(struct SharedData));

    // set start time
    m_pShm->nStartTime = time(NULL);
    m_pShm->nTotalLaunched = 0;
    m_pShm->nRunningChilds = 0;
    m_pShm->nWorkingChilds = 0;

    // clear child. we clear all available slot even we do not use slot over m_nMaxChild
    int i;
    for (i = 0; i < MAX_CHILDS; i++) poolInitSlot(i);

    return true;
}

static void poolInitSlot(int nSlotId)
{
    if (m_pShm == NULL) return;

    memset((void *)&m_pShm->child[nSlotId], 0, sizeof(struct child));
    m_pShm->child[nSlotId].nPid = 0; // does not need, but to make sure
}

static int poolFindSlot(int nPid)
{
    if (m_pShm == NULL) return -1;

    int i;
    for (i = 0; i < m_nMaxChild; i++) {
        if (m_pShm->child[i].nPid == nPid) return i;
    }

    return -1;
}
