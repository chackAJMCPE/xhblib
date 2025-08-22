#include "stdafx.h"
#include "XboxHTTP.h"
#include <xtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Internal state
typedef struct {
    BOOL initialized;
    WSADATA wsaData;
} XBHTTP_STATE;

static XBHTTP_STATE g_httpState = {0};

// Internal functions
static BOOL ParseUrl(const char* url, char* host, size_t hostSize, char* path, size_t pathSize, int* port);
static BOOL InitializeNetwork();
static void CleanupNetwork();
static BOOL ResolveHostname(const char* host, ULONG* ipAddress);
static SOCKET CreateAndConnectSocket(ULONG ipAddress, int port);
static BOOL SendHttpRequest(SOCKET sock, const XB_HTTP_REQUEST* request, const char* host, const char* path);
static XB_HTTP_RESPONSE* ReceiveHttpResponse(SOCKET sock);
static char* FindHeaderValue(const char* headers, const char* key);

// Initialize the HTTP library
BOOL XBHTTP_Init() {
    if (g_httpState.initialized) {
        return TRUE;
    }

    if (!InitializeNetwork()) {
        return FALSE;
    }

    g_httpState.initialized = TRUE;
    return TRUE;
}

// Cleanup the HTTP library
void XBHTTP_Cleanup() {
    if (g_httpState.initialized) {
        CleanupNetwork();
        g_httpState.initialized = FALSE;
    }
}

// Perform an HTTP request
XB_HTTP_RESPONSE* XBHTTP_PerformRequest(const XB_HTTP_REQUEST* request) {
    if (!g_httpState.initialized) {
        if (!XBHTTP_Init()) {
            return NULL;
        }
    }

    if (!request || !request->url) {
        return NULL;
    }

    char host[256] = {0};
    char path[512] = {0};
    int port = 80;

    if (!ParseUrl(request->url, host, sizeof(host), path, sizeof(path), &port)) {
        return NULL;
    }

    ULONG ipAddress;
    if (!ResolveHostname(host, &ipAddress)) {
        return NULL;
    }

    SOCKET sock = CreateAndConnectSocket(ipAddress, port);
    if (sock == INVALID_SOCKET) {
        return NULL;
    }

    if (!SendHttpRequest(sock, request, host, path)) {
        closesocket(sock);
        return NULL;
    }

    XB_HTTP_RESPONSE* response = ReceiveHttpResponse(sock);
    closesocket(sock);

    return response;
}

// Free a response object
void XBHTTP_FreeResponse(XB_HTTP_RESPONSE* response) {
    if (response) {
        if (response->status_text) free(response->status_text);
        if (response->headers) free(response->headers);
        if (response->body) free(response->body);
        free(response);
    }
}

// Helper function for simple GET request
XB_HTTP_RESPONSE* XBHTTP_Get(const char* url) {
    XB_HTTP_REQUEST request;
    memset(&request, 0, sizeof(request));
    request.method = XB_HTTP_GET;
    request.url = url;
    
    return XBHTTP_PerformRequest(&request);
}

// Helper function for simple POST request
XB_HTTP_RESPONSE* XBHTTP_Post(const char* url, const char* data, int data_size) {
    XB_HTTP_REQUEST request;
    memset(&request, 0, sizeof(request));
    request.method = XB_HTTP_POST;
    request.url = url;
    request.body = data;
    request.body_size = data_size;
    
    return XBHTTP_PerformRequest(&request);
}

// Helper function to save response body to file
BOOL XBHTTP_SaveToFile(const XB_HTTP_RESPONSE* response, const char* filename) {
    if (!response || !response->body || !filename) {
        return FALSE;
    }

    FILE* fp = NULL;
    if (fopen_s(&fp, filename, "wb") != 0 || !fp) {
        return FALSE;
    }

    size_t written = fwrite(response->body, 1, response->body_size, fp);
    fclose(fp);

    return (written == response->body_size);
}

// Internal function implementations

static BOOL ParseUrl(const char* url, char* host, size_t hostSize, char* path, size_t pathSize, int* port) {
    const char* url_start = strstr(url, "://");
    const char* host_start = url_start ? url_start + 3 : url;
    
    // Check if port is specified
    const char* port_start = strchr(host_start, ':');
    const char* path_start = strchr(host_start, '/');
    
    if (port_start && (!path_start || port_start < path_start)) {
        // Port is specified
        size_t host_len = port_start - host_start;
        if (host_len >= hostSize) return FALSE;
        
        strncpy(host, host_start, host_len);
        host[host_len] = '\0';
        
        *port = atoi(port_start + 1);
        
        if (path_start) {
            strncpy(path, path_start, pathSize - 1);
            path[pathSize - 1] = '\0';
        } else {
            strcpy(path, "/");
        }
    } else {
        // No port specified, use default
        *port = 80;
        
        if (path_start) {
            size_t host_len = path_start - host_start;
            if (host_len >= hostSize) return FALSE;
            
            strncpy(host, host_start, host_len);
            host[host_len] = '\0';
            
            strncpy(path, path_start, pathSize - 1);
            path[pathSize - 1] = '\0';
        } else {
            strncpy(host, host_start, hostSize - 1);
            host[hostSize - 1] = '\0';
            strcpy(path, "/");
        }
    }
    
    return TRUE;
}

static BOOL InitializeNetwork() {
    XNetStartupParams xnsp = {0};
    xnsp.cfgSizeOfStruct = sizeof(XNetStartupParams);
    xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
    
    if (XNetStartup(&xnsp) != 0) {
        return FALSE;
    }

    // Wait for network initialization
    DWORD dwStart = GetTickCount();
    while ((GetTickCount() - dwStart) < 6000) {}

    if (WSAStartup(MAKEWORD(2, 2), &g_httpState.wsaData) != 0) {
        XNetCleanup();
        return FALSE;
    }
    
    return TRUE;
}

static void CleanupNetwork() {
    WSACleanup();
    XNetCleanup();
}

static BOOL ResolveHostname(const char* host, ULONG* ipAddress) {
    XNDNS* dnsResult = NULL;
    INT err = XNetDnsLookup(host, NULL, &dnsResult);
    
    // Wait for DNS resolution
    while (dnsResult && dnsResult->iStatus == WSAEINPROGRESS) {
        Sleep(50);
    }

    if (!dnsResult || dnsResult->iStatus != 0 || dnsResult->cina == 0) {
        if (dnsResult) XNetDnsRelease(dnsResult);
        return FALSE;
    }

    *ipAddress = dnsResult->aina[0].S_un.S_addr;
    XNetDnsRelease(dnsResult);
    return TRUE;
}

static SOCKET CreateAndConnectSocket(ULONG ipAddress, int port) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }

    // Set security bypass option
    BOOL optTrue = TRUE;
    setsockopt(sock, SOL_SOCKET, 0x5801, (const char*)&optTrue, sizeof(BOOL));

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.S_un.S_addr = ipAddress;
    service.sin_port = htons(port);

    if (connect(sock, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

static const char* MethodToString(XB_HTTP_METHOD method) {
    switch (method) {
        case XB_HTTP_GET: return "GET";
        case XB_HTTP_POST: return "POST";
        case XB_HTTP_PUT: return "PUT";
        case XB_HTTP_DELETE: return "DELETE";
        case XB_HTTP_HEAD: return "HEAD";
        default: return "GET";
    }
}

static BOOL SendHttpRequest(SOCKET sock, const XB_HTTP_REQUEST* request, const char* host, const char* path) {
    char requestBuffer[4096];
    int pos = 0;
    
    // Build request line
    pos += sprintf_s(requestBuffer + pos, sizeof(requestBuffer) - pos, 
                    "%s %s HTTP/1.1\r\n", 
                    MethodToString(request->method), path);
    
    // Add Host header
    pos += sprintf_s(requestBuffer + pos, sizeof(requestBuffer) - pos, 
                    "Host: %s\r\n", host);
    
    // Add custom headers if provided
    if (request->headers) {
        pos += sprintf_s(requestBuffer + pos, sizeof(requestBuffer) - pos, 
                        "%s", request->headers);
        
        // Ensure headers end with CRLF
        if (strstr(request->headers, "\r\n") == NULL) {
            pos += sprintf_s(requestBuffer + pos, sizeof(requestBuffer) - pos, "\r\n");
        }
    }
    
    // Add Content-Length if there's a body
    if (request->body && request->body_size > 0) {
        pos += sprintf_s(requestBuffer + pos, sizeof(requestBuffer) - pos, 
                        "Content-Length: %d\r\n", request->body_size);
    }
    
    // End headers
    pos += sprintf_s(requestBuffer + pos, sizeof(requestBuffer) - pos, 
                    "Connection: close\r\n\r\n");
    
    // Send headers
    if (send(sock, requestBuffer, pos, 0) == SOCKET_ERROR) {
        return FALSE;
    }
    
    // Send body if present
    if (request->body && request->body_size > 0) {
        if (send(sock, request->body, request->body_size, 0) == SOCKET_ERROR) {
            return FALSE;
        }
    }
    
    return TRUE;
}

static XB_HTTP_RESPONSE* ReceiveHttpResponse(SOCKET sock) {
    XB_HTTP_RESPONSE* response = (XB_HTTP_RESPONSE*)malloc(sizeof(XB_HTTP_RESPONSE));
    if (!response) {
        return NULL;
    }
    memset(response, 0, sizeof(XB_HTTP_RESPONSE));
    
    // Receive data
    char buffer[4096];
    int bytesReceived;
    int headerState = 0;
    BOOL headersComplete = FALSE;
    int contentStart = 0;
    int contentLength = -1;
    
    // Headers buffer
    char* headersBuffer = NULL;
    int headersSize = 0;
    int headersCapacity = 1024;
    
    headersBuffer = (char*)malloc(headersCapacity);
    if (!headersBuffer) {
        free(response);
        return NULL;
    }
    
    // Body buffer
    char* bodyBuffer = NULL;
    int bodySize = 0;
    int bodyCapacity = 0;
    
    while ((bytesReceived = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        if (!headersComplete) {
            // Still processing headers
            for (int i = 0; i < bytesReceived; i++) {
                if (headerState == 0 && buffer[i] == '\r') headerState = 1;
                else if (headerState == 1 && buffer[i] == '\n') headerState = 2;
                else if (headerState == 2 && buffer[i] == '\r') headerState = 3;
                else if (headerState == 3 && buffer[i] == '\n') {
                    headersComplete = TRUE;
                    contentStart = i + 1;
                    break;
                }
                else {
                    headerState = 0;
                }
                
                // Add to headers buffer
                if (headersSize >= headersCapacity - 1) {
                    headersCapacity *= 2;
                    char* newBuffer = (char*)realloc(headersBuffer, headersCapacity);
                    if (!newBuffer) {
                        free(headersBuffer);
                        if (bodyBuffer) free(bodyBuffer);
                        free(response);
                        return NULL;
                    }
                    headersBuffer = newBuffer;
                }
                
                headersBuffer[headersSize++] = buffer[i];
            }
            
            headersBuffer[headersSize] = '\0';
            
            // Parse status line if we have it
            if (response->status_code == 0) {
                char* statusLineEnd = strstr(headersBuffer, "\r\n");
                if (statusLineEnd) {
                    *statusLineEnd = '\0';
                    
                    // Parse HTTP/1.x STATUS_CODE STATUS_TEXT
                    char* httpVersion = strchr(headersBuffer, ' ');
                    if (httpVersion) {
                        *httpVersion = '\0';
                        httpVersion++;
                        
                        char* statusCode = strchr(httpVersion, ' ');
                        if (statusCode) {
                            *statusCode = '\0';
                            statusCode++;
                            
                            response->status_code = atoi(httpVersion);
                            response->status_text = _strdup(statusCode);
                        }
                    }
                    
                    *statusLineEnd = '\r';
                }
            }
            
            // Parse Content-Length header if we have it
            if (contentLength == -1) {
                char* contentLengthHeader = FindHeaderValue(headersBuffer, "Content-Length");
                if (contentLengthHeader) {
                    contentLength = atoi(contentLengthHeader);
                    free(contentLengthHeader);
                }
            }
            
            if (headersComplete) {
                // Allocate body buffer if we know the content length
                if (contentLength > 0) {
                    bodyCapacity = contentLength;
                    bodyBuffer = (char*)malloc(bodyCapacity);
                    if (!bodyBuffer) {
                        free(headersBuffer);
                        free(response);
                        return NULL;
                    }
                } else {
                    // Unknown content length, start with a reasonable size
                    bodyCapacity = 4096;
                    bodyBuffer = (char*)malloc(bodyCapacity);
                    if (!bodyBuffer) {
                        free(headersBuffer);
                        free(response);
                        return NULL;
                    }
                }
                
                response->headers = headersBuffer;
                
                // Copy any body data from this packet
                if (contentStart < bytesReceived) {
                    int bodyBytes = bytesReceived - contentStart;
                    memcpy(bodyBuffer, buffer + contentStart, bodyBytes);
                    bodySize = bodyBytes;
                }
            }
        } else {
            // Headers complete, processing body
            int bytesToCopy = bytesReceived;
            
            // Check if we need to resize body buffer
            if (bodySize + bytesToCopy > bodyCapacity) {
                if (contentLength == -1) {
                    // Unknown content length, expand buffer
                    bodyCapacity = bodySize + bytesToCopy + 4096;
                    char* newBuffer = (char*)realloc(bodyBuffer, bodyCapacity);
                    if (!newBuffer) {
                        free(headersBuffer);
                        free(bodyBuffer);
                        free(response);
                        return NULL;
                    }
                    bodyBuffer = newBuffer;
                } else {
                    // Content length known, but we received more than expected
                    bytesToCopy = bodyCapacity - bodySize;
                }
            }
            
            memcpy(bodyBuffer + bodySize, buffer, bytesToCopy);
            bodySize += bytesToCopy;
        }
    }
    
    if (!headersComplete) {
        // Didn't receive complete headers
        free(headersBuffer);
        if (bodyBuffer) free(bodyBuffer);
        free(response);
        return NULL;
    }
    
    response->body = bodyBuffer;
    response->body_size = bodySize;
    
    return response;
}

static char* FindHeaderValue(const char* headers, const char* key) {
    char searchKey[256];
    sprintf_s(searchKey, sizeof(searchKey), "%s:", key);
    
    const char* headerStart = strstr(headers, searchKey);
    if (!headerStart) {
        return NULL;
    }
    
    headerStart += strlen(searchKey);
    
    // Skip whitespace
    while (*headerStart == ' ') {
        headerStart++;
    }
    
    const char* valueEnd = strstr(headerStart, "\r\n");
    if (!valueEnd) {
        return NULL;
    }
    
    size_t valueLen = valueEnd - headerStart;
    char* value = (char*)malloc(valueLen + 1);
    if (!value) {
        return NULL;
    }
    
    strncpy(value, headerStart, valueLen);
    value[valueLen] = '\0';
    
    return value;
}