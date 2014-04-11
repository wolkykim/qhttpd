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
 * $Id: luascript.c 214 2012-05-05 00:33:31Z seungyoung.kim $
 ******************************************************************************/

#ifdef ENABLE_LUA

#include "qhttpd.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

/////////////////////////////////////////////////////////////////////////
// LUA BINDING FUNCTIONS
/////////////////////////////////////////////////////////////////////////
static int lualib_http_getRemoteAddr(lua_State *lua);
static int lualib_http_getRemotePort(lua_State *lua);
static int lualib_http_terminate(lua_State *lua);
static const struct luaL_Reg lualib_http [] = {
    {"getRemoteAddr", lualib_http_getRemoteAddr},
    {"getRemotePort", lualib_http_getRemotePort},
    {"terminate", lualib_http_terminate},
    {NULL, NULL}
};

static int lualib_request_getRequest(lua_State *lua);
static int lualib_request_setRequest(lua_State *lua);
static int lualib_request_getHeader(lua_State *lua);
static int lualib_request_setHeader(lua_State *lua);
static int lualib_request_delHeader(lua_State *lua);
static const struct luaL_Reg lualib_request [] = {
    {"getRequest", lualib_request_getRequest},
    {"setRequest", lualib_request_setRequest},
    {"getHeader", lualib_request_getHeader},
    {"setHeader", lualib_request_setHeader},
    {"delHeader", lualib_request_delHeader},
    {NULL, NULL}
};

static int lualib_response_getCode(lua_State *lua);
static int lualib_response_setCode(lua_State *lua);
static int lualib_response_setContent(lua_State *lua);
static int lualib_response_getHeader(lua_State *lua);
static int lualib_response_setHeader(lua_State *lua);
static int lualib_response_delHeader(lua_State *lua);
static const struct luaL_Reg lualib_response [] = {
    {"getCode", lualib_response_getCode},
    {"setCode", lualib_response_setCode},
    {"setContent", lualib_response_setContent},
    {"getHeader", lualib_response_getHeader},
    {"setHeader", lualib_response_setHeader},
    {"delHeader", lualib_response_delHeader},
    {NULL, NULL}
};

/////////////////////////////////////////////////////////////////////////
// PRIVATE VARIABLES
/////////////////////////////////////////////////////////////////////////
static lua_State *m_lua = NULL;
struct HttpRequest *m_pReq = NULL;
struct HttpResponse *m_pRes = NULL;

/////////////////////////////////////////////////////////////////////////
// HOOKING FUNCTIONS
/////////////////////////////////////////////////////////////////////////

bool luaInit(const char *pszScriptPath)
{
    if (m_lua != NULL) return false;

    m_lua = luaL_newstate();
    luaL_openlibs(m_lua);

    // register binding functions
    luaL_register(m_lua, "http", lualib_http);
    luaL_register(m_lua, "request", lualib_request);
    luaL_register(m_lua, "response", lualib_response);

    // load script source
    if (luaL_loadfile(m_lua, pszScriptPath) != 0) {
        LOG_WARN("LUA ERR : %s", lua_tostring(m_lua, -1));
        luaFree();
        return false;
    }

    // run script
    if (lua_pcall(m_lua, 0, 0, 0) != 0) {
        LOG_WARN("LUA ERR : %s", lua_tostring(m_lua, -1));
    }

    // clear stack
    lua_settop(m_lua, 0);

    return true;
}

bool luaFree(void)
{
    if (m_lua == NULL) return false;

    lua_close(m_lua);
    m_lua = NULL;
    m_pReq = NULL;
    m_pRes = NULL;
    return true;
}

int luaRequestHandler(struct HttpRequest *pReq, struct HttpResponse *pRes)
{
    if (m_lua == NULL) return 0;

    // get session data
    m_pReq = pReq;
    m_pRes = pRes;

    // clear stack
    lua_settop(m_lua, 0);

    // call function
    lua_getglobal(m_lua, "requestHandler");
    if (lua_pcall(m_lua, 0, 1, 0) != 0) {
        LOG_WARN("LUA ERR : %s", lua_tostring(m_lua, -1));
    }

    // get return value
    int nResCode = 0;
    if (lua_isnumber (m_lua, -1) == 1) {
        nResCode = lua_tonumber(m_lua, -1);
        lua_pop(m_lua, 1);
        httpResponseSetCode(pRes, nResCode, true);
    }

    DEBUG("luaRequestHandler: %d", nResCode);

    return nResCode;
}

// in case of bad request, hook will not be called.
bool luaResponseHandler(void)
{
    if (m_lua == NULL || m_pReq == NULL || m_pRes == NULL) return false;

    lua_getglobal(m_lua, "responseHandler");
    if (lua_pcall(m_lua, 0, 1, 0) != 0) {
        LOG_WARN("LUA ERR : %s", lua_tostring(m_lua, -1));
    }

    // remove session temporary information
    m_pReq = NULL;
    m_pRes = NULL;
    return true;
}

//
// lua binding functions
//

static int lualib_http_getRemoteAddr(lua_State *lua)
{
    lua_pushstring(lua, poolGetConnAddr());
    return 1;
}

static int lualib_http_getRemotePort(lua_State *lua)
{
    lua_pushinteger(lua, poolGetConnPort());
    return 1;
}

static int lualib_http_terminate(lua_State *lua)
{
    poolSetExitRequest();
    return 0;
}

static const char *LUA_GETTBLSTR(lua_State *L, const char *N)
{
    lua_pushstring(L, N);
    lua_gettable(L, -2);
    const char *V = lua_tostring(L, -1);
    lua_pop(L, 1); // remove value
    return V;
}

#define LUA_SETTBLSTR(L, N, V)  { lua_pushstring(L, V); lua_setfield(L, -2, N); }
#define LUA_SETTBLINT(L, N, V)  { lua_pushinteger(L, V); lua_setfield(L, -2, N); }
static int lualib_request_getRequest(lua_State *lua)
{
    if (m_pReq == NULL) {
        lua_pushnil(lua);
        return 1;
    }

    lua_newtable(lua);
    LUA_SETTBLSTR(lua, "requestMethod", m_pReq->pszRequestMethod);
    LUA_SETTBLSTR(lua, "requestHost", m_pReq->pszRequestHost);
    LUA_SETTBLSTR(lua, "requestPath", m_pReq->pszRequestPath);
    LUA_SETTBLSTR(lua, "queryString", m_pReq->pszQueryString);

    return 1;
}

static int lualib_request_setRequest(lua_State *lua)
{
    if (lua_istable (lua, -1) != 1 || m_pReq == NULL) {
        lua_pushboolean(lua, false);
        return 1;
    }

    free(m_pReq->pszRequestMethod);
    m_pReq->pszRequestMethod = strdup(LUA_GETTBLSTR(lua, "requestMethod"));

    free(m_pReq->pszRequestHost);
    m_pReq->pszRequestHost = strdup(LUA_GETTBLSTR(lua, "requestHost"));

    free(m_pReq->pszRequestPath);
    m_pReq->pszRequestPath = strdup(LUA_GETTBLSTR(lua, "requestPath"));

    free(m_pReq->pszQueryString);
    m_pReq->pszQueryString = strdup(LUA_GETTBLSTR(lua, "queryString"));

    // generate new request uri
    char *pszNewUri = (char *)malloc((strlen(m_pReq->pszRequestPath)*3) + 1 + strlen(m_pReq->pszQueryString) + 1);
    if (pszNewUri != NULL) {
        strcpy(pszNewUri, m_pReq->pszRequestPath);
        qurl_encode(pszNewUri, strlen(pszNewUri));
        if (IS_EMPTY_STRING(m_pReq->pszQueryString) == false) {
            strcat(pszNewUri, "?");
            strcat(pszNewUri, m_pReq->pszQueryString);
        }

        free(m_pReq->pszRequestUri);
        m_pReq->pszRequestUri =  pszNewUri;
    }

    DEBUG("lualib_http_setRequest: %s", m_pReq->pszRequestUri);

    // return true
    lua_pushboolean(lua, true);
    return 1;
}

static int lualib_request_getHeader(lua_State *lua)
{
    if (lua_isstring (lua, -1) != 1 || m_pReq == NULL) {
        lua_pushnil(lua);
        return 1;
    }

    const char *pszName = lua_tostring(lua, -1);
    lua_pop(lua, 1);
    const char *pszValue = httpHeaderGetStr(m_pReq->pHeaders, pszName);

    if (pszValue != NULL) lua_pushstring(lua, pszValue);
    else lua_pushnil(lua);

    return 1;
}

static int lualib_request_setHeader(lua_State *lua)
{
    if (lua_isstring (lua, -2) != 1 || lua_isstring (lua, -1) != 1 || m_pReq == NULL) {
        lua_pushboolean(lua, false);
        return 1;
    }

    const char *pszValue = lua_tostring(lua, -1);
    lua_pop(lua, 1);
    const char *pszName = lua_tostring(lua, -1);
    lua_pop(lua, 1);

    if (httpHeaderSetStr(m_pReq->pHeaders, pszName, pszValue) == true) lua_pushboolean(lua, true);
    else lua_pushboolean(lua, false);
    return 1;
}

static int lualib_request_delHeader(lua_State *lua)
{
    if (lua_isstring (lua, -1) != 1 || m_pReq == NULL) {
        lua_pushboolean(lua, false);
        return 1;
    }

    const char *pszName = lua_tostring(lua, -1);
    lua_pop(lua, 1);

    if (httpHeaderRemove(m_pReq->pHeaders, pszName) == true) lua_pushboolean(lua, true);
    else lua_pushboolean(lua, false);
    return 1;
}

static int lualib_response_getCode(lua_State *lua)
{
    if (m_pRes == NULL) {
        lua_pushinteger(lua, 0);
        return 1;
    }

    lua_pushinteger(lua, m_pRes->nResponseCode);
    return 1;
}

static int lualib_response_setCode(lua_State *lua)
{
    if (m_pRes == NULL) {
        lua_pushboolean(lua, false);
        return 1;
    }

    int nResCode = lua_tonumber(m_lua, -1);
    lua_pop(m_lua, 1);
    m_pRes->nResponseCode = nResCode;

    // return true
    lua_pushboolean(lua, true);
    return 1;
}

static int lualib_response_setContent(lua_State *lua)
{
    if (lua_isstring (lua, -2) != 1 || lua_isstring (lua, -1) != 1 || m_pRes == NULL) {
        lua_pushboolean(lua, false);
        return 1;
    }

    const char *pszContent = lua_tostring(lua, -1);
    lua_pop(lua, 1);
    const char *pszContentType = lua_tostring(lua, -1);
    lua_pop(lua, 1);
    httpResponseSetContent(m_pRes, pszContentType, pszContent, strlen(pszContent));

    DEBUG("lualib_http_setContent: %s %s", pszContentType, pszContent);

    // return true
    lua_pushboolean(lua, true);
    return 1;
}

static int lualib_response_getHeader(lua_State *lua)
{
    if (lua_isstring (lua, -1) != 1 || m_pRes == NULL) {
        lua_pushnil(lua);
        return 1;
    }

    const char *pszName = lua_tostring(lua, -1);
    lua_pop(lua, 1);
    const char *pszValue = httpHeaderGetStr(m_pRes->pHeaders, pszName);

    if (pszValue != NULL) lua_pushstring(lua, pszValue);
    else lua_pushnil(lua);

    return 1;
}

static int lualib_response_setHeader(lua_State *lua)
{
    if (lua_isstring (lua, -2) != 1 || lua_isstring (lua, -1) != 1 || m_pRes == NULL) {
        lua_pushboolean(lua, false);
        return 1;
    }

    const char *pszValue = lua_tostring(lua, -1);
    lua_pop(lua, 1);
    const char *pszName = lua_tostring(lua, -1);
    lua_pop(lua, 1);

    if (httpHeaderSetStr(m_pRes->pHeaders, pszName, pszValue) == true) lua_pushboolean(lua, true);
    else lua_pushboolean(lua, false);
    return 1;
}

static int lualib_response_delHeader(lua_State *lua)
{
    if (lua_isstring (lua, -1) != 1 || m_pRes == NULL) {
        lua_pushboolean(lua, false);
        return 1;
    }

    const char *pszName = lua_tostring(lua, -1);
    lua_pop(lua, 1);

    if (httpHeaderRemove(m_pRes->pHeaders, pszName) == true) lua_pushboolean(lua, true);
    else lua_pushboolean(lua, false);
    return 1;
}

#endif
