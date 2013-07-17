#include "common.h"
#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static void _connection_close_handler(iostream_t *stream);
static void _on_http_header_data(iostream_t *stream, void *data, size_t len);

connection_t* connection_accept(server_t *server, int listen_fd) {
    connection_t  *conn;
    iostream_t   *stream;
    socklen_t    addr_len;
    int          conn_fd;
    request_t    req;
    response_t   resp;
    struct sockaddr_in remote_addr;

        // -------- Accepting connection ----------------------------
    printf("Accepting new connection...\n");
    addr_len = sizeof(struct sockaddr_in);
    conn_fd = accept(listen_fd, (struct sockaddr*) &remote_addr, &addr_len);
    printf("Connection fd: %d...\n", conn_fd);
    if (conn_fd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            perror("Error accepting new connection");
        return NULL;
    }

    conn = (connection_t*) calloc(1, sizeof(connection_t));
    if (conn == NULL) {
        fprintf(stderr, "Error allocating memory for connection\n");
        goto error;
    }
    bzero(conn, sizeof(connection_t));

    if (set_nonblocking(conn_fd) < 0) {
        perror("Error configuring Non-blocking");
        goto error;
    }
    
    stream = iostream_create(server->ioloop, conn_fd, 10240, 40960, conn);
    if (stream == NULL) {
        fprintf(stderr, "Error creating iostream");
        goto error;
    }
    
    iostream_set_close_handler(stream, _connection_close_handler);

    conn->server = server;
    conn->stream = stream;
    inet_ntop(AF_INET, &remote_addr.sin_addr, conn->remote_ip, 20);
    conn->remote_port = remote_addr.sin_port;
    conn->state = CONN_ACTIVE;
    conn->request  = request_create(conn);
    conn->response = response_create(conn);
    
    return conn;

    error:
    close(conn_fd);
    if (conn != NULL)
        free(conn);
    return NULL;
}

int connection_close(connection_t *conn) {
    return iostream_close(conn->stream);
}

int connection_destroy(connection_t *conn) {
    free(conn);
    return 0;
}

int connection_run(connection_t *conn) {
    iostream_read_until(conn->stream, "\r\n\r\n", _on_http_header_data);
    return 0;
}

static void _on_http_header_data(iostream_t *stream, void *data, size_t len) {
    connection_t   *conn;
    request_t      *req;
    response_t     *resp;
    handler_ctx_t  *ctx;
    size_t         consumed;

    ctx = NULL;
    conn = (connection_t*) stream->user_data;
    req = conn->request;
    resp = conn->response;

    if (req == NULL || resp == NULL) {
        fprintf(stderr, "Error creating request/response\n");
        connection_close(conn);
        return;
    }

    if (request_parse_headers(req, (char*)data, len, &consumed)
        != STATUS_COMPLETE) {
        fprintf(stderr, "Error parsing request headers\n");
        connection_close(conn);
        return;
    }

    // TODO Handle Unknown HTTP version
    resp->version = req->version;

    conn->server->handler(req, resp, ctx);
}

static void _connection_close_handler(iostream_t *stream) {
    //TODO handle connection close
}