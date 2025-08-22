#pragma once

#ifndef XBOXHTTP_H
#define XBOXHTTP_H

#ifdef __cplusplus
extern "C" {
#endif

// HTTP Method Types
typedef enum {
    XB_HTTP_GET,
    XB_HTTP_POST,
    XB_HTTP_PUT,
    XB_HTTP_DELETE,
    XB_HTTP_HEAD
} XB_HTTP_METHOD;

// HTTP Response Structure
typedef struct {
    int status_code;
    char* status_text;
    char* headers;
    char* body;
    int body_size;
} XB_HTTP_RESPONSE;

// HTTP Request Configuration
typedef struct {
    XB_HTTP_METHOD method;
    const char* url;
    const char* headers;
    const char* body;
    int body_size;
    int timeout_ms;
} XB_HTTP_REQUEST;

// Initialize the HTTP library
BOOL XBHTTP_Init();

// Cleanup the HTTP library
void XBHTTP_Cleanup();

// Perform an HTTP request
XB_HTTP_RESPONSE* XBHTTP_PerformRequest(const XB_HTTP_REQUEST* request);

// Free a response object
void XBHTTP_FreeResponse(XB_HTTP_RESPONSE* response);

// Helper function for simple GET request
XB_HTTP_RESPONSE* XBHTTP_Get(const char* url);

// Helper function for simple POST request
XB_HTTP_RESPONSE* XBHTTP_Post(const char* url, const char* data, int data_size);

// Helper function to save response body to file
BOOL XBHTTP_SaveToFile(const XB_HTTP_RESPONSE* response, const char* filename);

#ifdef __cplusplus
}
#endif

#endif // XBOXHTTP_H