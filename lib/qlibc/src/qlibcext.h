/******************************************************************************
 * qLibc - http://www.qdecoder.org
 *
 * Copyright (c) 2010-2012 Seungyoung Kim.
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
 * $Id: qlibc.h 55 2012-04-03 03:15:00Z seungyoung.kim $
 ******************************************************************************/

/**
 * qlibc extension header file
 *
 * @file qlibcext.h
 */

#ifndef _QLIBCEXT_H
#define _QLIBCEXT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "qlibc.h"

/******************************************************************************
 * Configuration Parser.
 * qconfig.c
 ******************************************************************************/

/* public functions */
extern qlisttbl_t *qconfig_parse_file(qlisttbl_t *tbl, const char *filepath,
                                      char sepchar, bool uniquekey);
extern qlisttbl_t *qconfig_parse_str(qlisttbl_t *tbl, const char *str,
                                     char sepchar, bool uniquekey);

/******************************************************************************
 * Rotating file logger.
 * qlog.c
 ******************************************************************************/

/* types */
typedef struct qlog_s qlog_t;

/* public functions */
extern qlog_t *qlog(const char *filepathfmt, mode_t mode, int rotateinterval,
                    bool flush);

/**
 * qlog structure
 */
struct qlog_s {
    /* capsulated member functions */
    bool (*write) (qlog_t *log, const char *str);
    bool (*writef) (qlog_t *log, const char *format, ...);
    bool (*duplicate) (qlog_t *log, FILE *outfp, bool flush);
    bool (*flush) (qlog_t *log);
    bool (*free) (qlog_t *log);

    /* private variables - do not access directly */
    qmutex_t  qmutex;  /*!< activated if compiled with --enable-threadsafe */

    char filepathfmt[PATH_MAX]; /*!< file file naming format like
                                     /somepath/daily-%Y%m%d.log */
    char filepath[PATH_MAX];    /*!< generated system path of log file */
    FILE *fp;           /*!< file pointer of logpath */
    mode_t mode;        /*!< file mode */
    int rotateinterval; /*!< log file will be rotate in this interval seconds */
    int nextrotate;  /*!< next rotate universal time, seconds */
    bool logflush;   /*!< flag for immediate flushing */

    FILE *outfp;    /*!< stream pointer for duplication */
    bool outflush;  /*!< flag for immediate flushing for duplicated stream */
};

/******************************************************************************
 * HTTP client implementation.
 * qhttpclient.c
 ******************************************************************************/

/* types */
typedef struct qhttpclient_s  qhttpclient_t;

/* public functions */
extern qhttpclient_t *qhttpclient(const char *hostname, int port);

/**
 * qhttpclient structure
 */

struct qhttpclient_s {
    /* capsulated member functions */
    bool (*setssl) (qhttpclient_t *client);
    void (*settimeout) (qhttpclient_t *client, int timeoutms);
    void (*setkeepalive) (qhttpclient_t *client, bool keepalive);
    void (*setuseragent) (qhttpclient_t *client, const char *useragent);

    bool (*open) (qhttpclient_t *client);

    bool (*head) (qhttpclient_t *client, const char *uri, int *rescode,
                  qlisttbl_t *reqheaders, qlisttbl_t *resheaders);
    bool (*get) (qhttpclient_t *client, const char *uri, int fd,
                 off_t *savesize, int *rescode,
                 qlisttbl_t *reqheaders, qlisttbl_t *resheaders,
                 bool (*callback) (void *userdata, off_t recvbytes),
                 void *userdata);
    bool (*put) (qhttpclient_t *client, const char *uri, int fd,
                 off_t length, int *retcode, qlisttbl_t *userheaders,
                 qlisttbl_t *resheaders,
                 bool (*callback) (void *userdata, off_t sentbytes),
                 void *userdata);
    void *(*cmd) (qhttpclient_t *client,
                  const char *method, const char *uri,
                  void *data, size_t size, int *rescode,
                  size_t *contentslength,
                  qlisttbl_t *reqheaders, qlisttbl_t *resheaders);

    bool (*sendrequest) (qhttpclient_t *client, const char *method,
                         const char *uri, qlisttbl_t *reqheaders);
    int (*readresponse) (qhttpclient_t *client, qlisttbl_t *resheaders,
                         off_t *contentlength);

    ssize_t (*gets) (qhttpclient_t *client, char *buf, size_t bufsize);
    ssize_t (*read) (qhttpclient_t *client, void *buf, size_t nbytes);
    ssize_t (*write) (qhttpclient_t *client, const void *buf,
                      size_t nbytes);
    off_t (*recvfile) (qhttpclient_t *client, int fd, off_t nbytes);
    off_t (*sendfile) (qhttpclient_t *client, int fd, off_t nbytes);

    bool (*close) (qhttpclient_t *client);
    void (*free) (qhttpclient_t *client);

    /* private variables - do not access directly */
    int socket;  /*!< socket descriptor */
    void *ssl;   /*!< will be used if SSL has been enabled at compile time */

    struct sockaddr_in addr;
    char *hostname;
    int port;

    int timeoutms;    /*< wait timeout miliseconds*/
    bool keepalive;   /*< keep-alive flag */
    char *useragent;  /*< user-agent name */

    bool connclose;   /*< response keep-alive flag for a last request */
};

/******************************************************************************
 * qdb_t - Database interface.
 * qdatabase.c
 ******************************************************************************/

/* database header files should be included before this header file. */
#ifdef _mysql_h
#define _Q_ENABLE_MYSQL  (1)
#endif /* _mysql_h */

/* types */
typedef struct qdb_s qdb_t;
typedef struct qdbresult_s qdbresult_t;

/* public functions */
extern qdb_t *qdb(const char *dbtype,
                  const char *addr, int port, const char *database,
                  const char *username, const char *password, bool autocommit);

/**
 * qdb structure
 */
struct qdb_s {
    /* capsulated member functions */
    bool (*open) (qdb_t *db);
    bool (*close) (qdb_t *db);

    int (*execute_update) (qdb_t *db, const char *query);
    int (*execute_updatef) (qdb_t *db, const char *format, ...);

    qdbresult_t *(*execute_query) (qdb_t *db, const char *query);
    qdbresult_t *(*execute_queryf) (qdb_t *db, const char *format, ...);

    bool (*begin_tran) (qdb_t *db);
    bool (*end_tran) (qdb_t *db, bool commit);
    bool (*commit) (qdb_t *db);
    bool (*rollback) (qdb_t *db);

    bool (*set_fetchtype) (qdb_t *db, bool fromdb);
    bool (*get_conn_status) (qdb_t *db);
    bool (*ping) (qdb_t *db);
    const char *(*get_error) (qdb_t *db, unsigned int *errorno);
    bool (*free) (qdb_t *db);

    /* private variables - do not access directly */
    qmutex_t  qmutex;  /*!< activated if compiled with --enable-threadsafe */

    bool connected;   /*!< if opened true, if closed false */

    struct {
        char *dbtype;
        char *addr;
        int port;
        char *username;
        char *password;
        char *database;
        bool autocommit;
        bool fetchtype;
    } info;   /*!< database connection infomation */

#ifdef _Q_ENABLE_MYSQL
    /* private variables for mysql database - do not access directly */
    MYSQL  *mysql;
#endif
};

/**
 * qdbresult structure
 */
struct qdbresult_s {
    /* capsulated member functions */
    const char *(*getstr) (qdbresult_t *result, const char *field);
    const char *(*get_str_at) (qdbresult_t *result, int idx);
    int (*getint) (qdbresult_t *result, const char *field);
    int (*get_int_at) (qdbresult_t *result, int idx);
    bool (*getnext) (qdbresult_t *result);

    int (*get_cols) (qdbresult_t *result);
    int (*get_rows) (qdbresult_t *result);
    int (*get_row) (qdbresult_t *result);

    bool (*free) (qdbresult_t *result);

#ifdef _Q_ENABLE_MYSQL
    /* private variables for mysql database - do not access directly */
    bool fetchtype;
    MYSQL_RES  *rs;
    MYSQL_FIELD  *fields;
    MYSQL_ROW  row;
    int cols;
    int cursor;
#endif
};

#ifdef __cplusplus
}
#endif

#endif /*_QLIBCEXT_H */
