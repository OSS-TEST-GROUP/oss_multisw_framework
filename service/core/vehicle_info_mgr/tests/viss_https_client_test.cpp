#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

struct client_buffer {
    char p[2048];
    int len;
};

static int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                         void *user, void *in, size_t len) {
    struct client_buffer *cb = (struct client_buffer *)user;

    switch (reason) {
        case LWS_CALLBACK_CLIENT_HTTP_BIND_PROTOCOL:
            cb->len = 0;
            break;

        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
            if (cb->len + (int)len < (int)sizeof(cb->p)) {
                memcpy(cb->p + cb->len, in, len);
                cb->len += (int)len;
            }
            return 0;

        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP: {
            char buffer[1024];
            char *px = buffer;
            int l = sizeof(buffer);
            if (lws_http_client_read(wsi, &px, &l) < 0)
                return -1;
            break;
        }

        case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
            cb->p[cb->len] = '\0';
            std::cout << "\n--- HTTPS Response Received ---\n" << cb->p << "\n-------------------------------\n" << std::endl;
            // Quit the loop
            lws_cancel_service(lws_get_context(wsi));
            break;

        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            std::cerr << "--- HTTPS Connection Error: " << (in ? (char *)in : "unknown") << " ---" << std::endl;
            lws_cancel_service(lws_get_context(wsi));
            break;

        default:
            break;
    }

    return lws_callback_http_dummy(wsi, reason, user, in, len);
}

static const struct lws_protocols protocols[] = {
    { "http", callback_http, sizeof(struct client_buffer), 0 },
    { NULL, NULL, 0, 0 }
};

int main(int argc, char **argv) {
    struct lws_context_creation_info info;
    struct lws_client_connect_info i;
    struct lws_context *context;
    const char *url = "localhost";
    int port = 7443;

    memset(&info, 0, sizeof(info));
    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.protocols = protocols;
    info.port = CONTEXT_PORT_NO_LISTEN;

    context = lws_create_context(&info);
    if (!context) {
        std::cerr << "Failed to create LWS context" << std::endl;
        return 1;
    }

    memset(&i, 0, sizeof(i));
    i.context = context;
    i.address = url;
    i.port = port;
    i.path = "/Vehicle/Speed"; // Dummy path for testing
    i.host = i.address;
    i.origin = i.address;
    i.method = "GET";
    i.protocol = protocols[0].name;
    
    // Skip SSL verification for self-signed certificates
    i.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;

    std::cout << "Connecting to HTTPS " << url << ":" << port << "..." << std::endl;
    if (!lws_client_connect_via_info(&i)) {
        std::cerr << "Failed to initiate HTTPS connection" << std::endl;
        lws_context_destroy(context);
        return 1;
    }

    while (lws_service(context, 100) >= 0);

    lws_context_destroy(context);
    std::cout << "Test completed." << std::endl;

    return 0;
}
