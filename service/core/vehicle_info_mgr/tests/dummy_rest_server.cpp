#include <libwebsockets.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "Logger.h"
#include "Utils.h"

struct PerSessionData {
    std::string body;
    std::string uri;
    bool is_post;
};

static int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                         void *user, void *in, size_t len) {
    PerSessionData *psd = (PerSessionData *)user;

    switch (reason) {
        case LWS_CALLBACK_HTTP: {
            char uri[256] = {0};
            psd->is_post = false;

            if (lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI) > 0) {
                lws_hdr_copy(wsi, uri, sizeof(uri), WSI_TOKEN_POST_URI);
                psd->is_post = true;
            } else if (lws_hdr_total_length(wsi, WSI_TOKEN_GET_URI) > 0) {
                lws_hdr_copy(wsi, uri, sizeof(uri), WSI_TOKEN_GET_URI);
            }

            psd->uri = uri;
            LOG_INFO << "[DummyServer] Received Request: " << (psd->is_post ? "POST" : "GET") << " " << psd->uri;

            if (psd->is_post && psd->uri == "/upload") {
                LOG_INFO << "[DummyServer] Waiting for POST body data on /upload...";
                return 0; // Wait for body
            }

            // Otherwise, 404 or simple 200
            LOG_WARN << "[DummyServer] Unhandled path, returning 200 OK for " << psd->uri;
            lws_return_http_status(wsi, HTTP_STATUS_OK, "Dummy Server OK");
            return -1;
        }

        case LWS_CALLBACK_HTTP_BODY:
            if (psd->is_post && psd->uri == "/upload") {
                psd->body.append((const char *)in, len);
            }
            break;

        case LWS_CALLBACK_HTTP_BODY_COMPLETION:
            if (psd->is_post && psd->uri == "/upload") {
                LOG_INFO << "[DummyServer] Received upload data completely. Size: " << psd->body.size() << " bytes";
                
                std::string filename = "received_signals.csv.gz";
                std::ofstream ofs(filename, std::ios::binary);
                if (ofs) {
                    ofs.write(psd->body.data(), psd->body.size());
                    ofs.close();
                    LOG_INFO << "[DummyServer] Successfully saved data to " << filename;
                } else {
                    LOG_ERROR << "[DummyServer] Failed to open " << filename << " for writing.";
                }

                unsigned char buffer[LWS_PRE + 128];
                unsigned char *p = &buffer[LWS_PRE], *end = &buffer[sizeof(buffer) - 1];
                if (lws_add_http_common_headers(wsi, HTTP_STATUS_OK, "text/plain", 2, &p, end)) return 1;
                if (lws_finalize_http_header(wsi, &p, end)) return 1;
                lws_write(wsi, &buffer[LWS_PRE], p - &buffer[LWS_PRE], LWS_WRITE_HTTP_HEADERS);
                lws_write(wsi, (unsigned char*)"OK", 2, LWS_WRITE_HTTP);
                
                return lws_http_transaction_completed(wsi);
            }
            break;

        default:
            break;
    }
    return 0;
}

static const struct lws_protocols protocols[] = {
    { "http", callback_http, sizeof(PerSessionData), 0 },
    { NULL, NULL, 0, 0 }
};

int main(int argc, char **argv) {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = 8080;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        LOG_ERROR << "lws init failed";
        return 1;
    }

    LOG_INFO << "Dummy REST Server started on port 8080. Path: /upload";

    while (true) {
        lws_service(context, 100);
    }

    lws_context_destroy(context);
    return 0;
}
