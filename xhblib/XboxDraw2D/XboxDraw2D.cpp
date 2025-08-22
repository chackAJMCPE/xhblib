#include "stdafx.h"
#include "XboxDraw2D.h"
#include <d3dx9.h>
#include <xnamath.h>

// Global Direct3D objects
LPDIRECT3D9       g_pD3D = nullptr;
LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr;

// 2D rendering resources
static bool s_bInitialized = false;
static bool s_bTexInitialized = false;

// Rectangle rendering
static LPDIRECT3DVERTEXBUFFER9 s_pRectVB = nullptr;
static LPDIRECT3DVERTEXDECLARATION9 s_pRectVertexDecl = nullptr;
static LPDIRECT3DVERTEXSHADER9 s_pRectVertexShader = nullptr;
static LPDIRECT3DPIXELSHADER9 s_pRectPixelShader = nullptr;
static XMMATRIX s_matOrtho;

// Texture rendering
static LPDIRECT3DVERTEXBUFFER9 s_pTexVB = nullptr;
static LPDIRECT3DVERTEXDECLARATION9 s_pTexVertexDecl = nullptr;
static LPDIRECT3DVERTEXSHADER9 s_pTexVertexShader = nullptr;
static LPDIRECT3DPIXELSHADER9 s_pTexPixelShader = nullptr;

struct XD2D_Texture {
    LPDIRECT3DTEXTURE9 texture;
    int original_width;
    int original_height;
    int pot_width;
    int pot_height;
};


// Vertex structure for colored rectangles
struct COLORVERTEX {
    float Position[3];
    DWORD Color;
};

// Vertex structure for textured rectangles
struct TEXTUREVERTEX {
    float Position[3];
    float TexCoord[2];
};

// Rectangle vertex shader
static const char* s_strRectVertexShaderProgram = 
" float4x4 matWVP : register(c0);              \n"  
"                                              \n"  
" struct VS_IN                                 \n"  
" {                                            \n" 
"     float4 ObjPos   : POSITION;              \n"  
"     float4 Color    : COLOR;                 \n"  
" };                                           \n" 
"                                              \n" 
" struct VS_OUT                                \n" 
" {                                            \n" 
"     float4 ProjPos  : POSITION;              \n"  
"     float4 Color    : COLOR;                 \n"  
" };                                           \n"  
"                                              \n"  
" VS_OUT main( VS_IN In )                      \n"  
" {                                            \n"  
"     VS_OUT Out;                              \n"  
"     Out.ProjPos = mul( matWVP, In.ObjPos );  \n"  
"     Out.Color = In.Color;                    \n"  
"     return Out;                              \n"  
" }                                            \n";

// Rectangle pixel shader
static const char* s_strRectPixelShaderProgram = 
" struct PS_IN                                 \n"
" {                                            \n"
"     float4 Color : COLOR;                    \n" 
" };                                           \n" 
"                                              \n"  
" float4 main( PS_IN In ) : COLOR              \n"  
" {                                            \n"  
"     return In.Color;                         \n"  
" }                                            \n"; 

// Texture vertex shader
static const char* s_strTexVertexShaderProgram = 
" float4x4 matWVP : register(c0);              \n"  
" float2 uvScale : register(c4);               \n"  // UV scaling for POT padding
"                                              \n"  
" struct VS_IN                                 \n"  
" {                                            \n" 
"     float4 ObjPos   : POSITION;              \n"  
"     float2 TexCoord : TEXCOORD0;             \n"  
" };                                           \n" 
"                                              \n" 
" struct VS_OUT                                \n" 
" {                                            \n" 
"     float4 ProjPos  : POSITION;              \n"  
"     float2 TexCoord : TEXCOORD0;             \n"  
" };                                           \n"  
"                                              \n"  
" VS_OUT main( VS_IN In )                      \n"  
" {                                            \n"  
"     VS_OUT Out;                              \n"  
"     Out.ProjPos = mul( matWVP, In.ObjPos );  \n"  
"     Out.TexCoord = In.TexCoord * uvScale;    \n"  // Apply UV scaling
"     return Out;                              \n"  
" }                                            \n";

// Texture pixel shader
static const char* s_strTexPixelShaderProgram = 
" sampler2D Tex0 : register(s0);               \n"
"                                              \n"
" struct PS_IN                                 \n"
" {                                            \n"
"     float2 TexCoord : TEXCOORD0;             \n" 
" };                                           \n" 
"                                              \n"  
" float4 main( PS_IN In ) : COLOR              \n"  
" {                                            \n"  
"     return tex2D( Tex0, In.TexCoord );       \n"  
" }                                            \n"; 

// Initialize Direct3D and start the first scene
HRESULT XD2D_Init() {
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return E_FAIL;

    D3DPRESENT_PARAMETERS d3dpp = {0};
    d3dpp.BackBufferWidth        = SCREEN_WIDTH;
    d3dpp.BackBufferHeight       = SCREEN_HEIGHT;
    d3dpp.BackBufferFormat       = D3DFMT_A8R8G8B8;
    d3dpp.BackBufferCount        = 1;
    d3dpp.MultiSampleType        = D3DMULTISAMPLE_NONE;
    d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;

    HRESULT hr = g_pD3D->CreateDevice(
        0,
        D3DDEVTYPE_HAL,
        nullptr,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &d3dpp,
        &g_pd3dDevice
    );

    if (FAILED(hr) || !g_pd3dDevice)
        return E_FAIL;

    // Create orthographic projection matrix (pixel coordinates)
    s_matOrtho = XMMatrixOrthographicOffCenterLH(
        0.0f, 
        static_cast<float>(SCREEN_WIDTH), 
        static_cast<float>(SCREEN_HEIGHT), 
        0.0f, 
        0.0f, 
        1.0f
    );

    // Disable backface culling
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    // Start the first scene
    hr = g_pd3dDevice->BeginScene();
    if (FAILED(hr) && hr != D3DERR_INVALIDCALL)
        return E_FAIL;

    return S_OK;
}

// Render one frame (clear, draw, present) - updated version
HRESULT XD2D_Render() {
    if (!g_pd3dDevice)
        return E_FAIL;

    // End current scene safely
    HRESULT hr = g_pd3dDevice->EndScene();
    if (FAILED(hr) && hr != D3DERR_INVALIDCALL)
        return E_FAIL;

    // Present the back buffer
    hr = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
    if (FAILED(hr))
        return E_FAIL;

    // Begin new scene for next frame
    hr = g_pd3dDevice->BeginScene();
    if (FAILED(hr) && hr != D3DERR_INVALIDCALL)
        return E_FAIL;

    // Clear screen to black
    hr = g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
    if (FAILED(hr))
        return E_FAIL;

    return S_OK;
}

// Shut down Direct3D safely
HRESULT XD2D_End() {
    if (g_pd3dDevice) {
        // End the scene if still open
        HRESULT hr = g_pd3dDevice->EndScene();
        if (FAILED(hr) && hr != D3DERR_INVALIDCALL)
            return E_FAIL;

        // Release rectangle resources
        if (s_pRectVB) {
            s_pRectVB->Release();
            s_pRectVB = nullptr;
        }
        if (s_pRectVertexDecl) {
            s_pRectVertexDecl->Release();
            s_pRectVertexDecl = nullptr;
        }
        if (s_pRectVertexShader) {
            s_pRectVertexShader->Release();
            s_pRectVertexShader = nullptr;
        }
        if (s_pRectPixelShader) {
            s_pRectPixelShader->Release();
            s_pRectPixelShader = nullptr;
        }

        // Release texture resources
        if (s_pTexVB) {
            s_pTexVB->Release();
            s_pTexVB = nullptr;
        }
        if (s_pTexVertexDecl) {
            s_pTexVertexDecl->Release();
            s_pTexVertexDecl = nullptr;
        }
        if (s_pTexVertexShader) {
            s_pTexVertexShader->Release();
            s_pTexVertexShader = nullptr;
        }
        if (s_pTexPixelShader) {
            s_pTexPixelShader->Release();
            s_pTexPixelShader = nullptr;
        }

        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }

    if (g_pD3D) {
        g_pD3D->Release();
        g_pD3D = nullptr;
    }

    s_bInitialized = false;
    s_bTexInitialized = false;
    return S_OK;
}

// Initialize rectangle rendering resources
static HRESULT InitRectResources() {
    if (s_bInitialized) 
        return S_OK;

    HRESULT hr;

    // Compile and create vertex shader
    ID3DXBuffer* pVertexShaderCode = nullptr;
    ID3DXBuffer* pVertexErrorMsg = nullptr;
    hr = D3DXCompileShader(s_strRectVertexShaderProgram, 
                           (UINT)strlen(s_strRectVertexShaderProgram),
                           nullptr, 
                           nullptr, 
                           "main", 
                           "vs_2_0", 
                           0, 
                           &pVertexShaderCode, 
                           &pVertexErrorMsg, 
                           nullptr);
    if (FAILED(hr)) {
        if (pVertexErrorMsg) 
            OutputDebugStringA((char*)pVertexErrorMsg->GetBufferPointer());
        if (pVertexErrorMsg) pVertexErrorMsg->Release();
        if (pVertexShaderCode) pVertexShaderCode->Release();
        return E_FAIL;
    }

    hr = g_pd3dDevice->CreateVertexShader(
        (DWORD*)pVertexShaderCode->GetBufferPointer(), 
        &s_pRectVertexShader
    );
    pVertexShaderCode->Release();
    if (FAILED(hr)) return E_FAIL;

    // Compile and create pixel shader
    ID3DXBuffer* pPixelShaderCode = nullptr;
    ID3DXBuffer* pPixelErrorMsg = nullptr;
    hr = D3DXCompileShader(s_strRectPixelShaderProgram, 
                           (UINT)strlen(s_strRectPixelShaderProgram),
                           nullptr, 
                           nullptr, 
                           "main", 
                           "ps_2_0", 
                           0, 
                           &pPixelShaderCode, 
                           &pPixelErrorMsg,
                           nullptr);
    if (FAILED(hr)) {
        if (pPixelErrorMsg) 
            OutputDebugStringA((char*)pPixelErrorMsg->GetBufferPointer());
        if (pPixelErrorMsg) pPixelErrorMsg->Release();
        if (pPixelShaderCode) pPixelShaderCode->Release();
        return E_FAIL;
    }

    hr = g_pd3dDevice->CreatePixelShader(
        (DWORD*)pPixelShaderCode->GetBufferPointer(), 
        &s_pRectPixelShader
    );
    pPixelShaderCode->Release();
    if (FAILED(hr)) return E_FAIL;

    // Create vertex declaration
    D3DVERTEXELEMENT9 RectVertexElements[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
        D3DDECL_END()
    };
    hr = g_pd3dDevice->CreateVertexDeclaration(RectVertexElements, &s_pRectVertexDecl);
    if (FAILED(hr)) return E_FAIL;

    // Create unit quad vertex buffer
    hr = g_pd3dDevice->CreateVertexBuffer(
        6 * sizeof(COLORVERTEX),
        D3DUSAGE_WRITEONLY,
        0,
        D3DPOOL_MANAGED,
        &s_pRectVB,
        nullptr
    );
    if (FAILED(hr)) return E_FAIL;

    // Initialize unit quad vertices
    COLORVERTEX rectVertices[] = {
        { 0.0f, 0.0f, 0.0f, 0xFFFFFFFF }, // Top-left
        { 1.0f, 0.0f, 0.0f, 0xFFFFFFFF }, // Top-right
        { 0.0f, 1.0f, 0.0f, 0xFFFFFFFF }, // Bottom-left
        { 0.0f, 1.0f, 0.0f, 0xFFFFFFFF }, // Bottom-left
        { 1.0f, 0.0f, 0.0f, 0xFFFFFFFF }, // Top-right
        { 1.0f, 1.0f, 0.0f, 0xFFFFFFFF }  // Bottom-right
    };

    // Lock and copy vertex data
    COLORVERTEX* pVertices;
    s_pRectVB->Lock(0, 0, (void**)&pVertices, 0);
    memcpy(pVertices, rectVertices, sizeof(rectVertices));
    s_pRectVB->Unlock();

    s_bInitialized = true;
    return S_OK;
}

// Initialize texture rendering resources
static HRESULT InitTexResources() {
    if (s_bTexInitialized) 
        return S_OK;

    HRESULT hr;

    // Compile and create vertex shader
    ID3DXBuffer* pVertexShaderCode = nullptr;
    ID3DXBuffer* pVertexErrorMsg = nullptr;
    hr = D3DXCompileShader(s_strTexVertexShaderProgram, 
                           (UINT)strlen(s_strTexVertexShaderProgram),
                           nullptr, 
                           nullptr, 
                           "main", 
                           "vs_2_0", 
                           0, 
                           &pVertexShaderCode, 
                           &pVertexErrorMsg, 
                           nullptr);
    if (FAILED(hr)) {
        if (pVertexErrorMsg) 
            OutputDebugStringA((char*)pVertexErrorMsg->GetBufferPointer());
        if (pVertexErrorMsg) pVertexErrorMsg->Release();
        if (pVertexShaderCode) pVertexShaderCode->Release();
        return E_FAIL;
    }

    hr = g_pd3dDevice->CreateVertexShader(
        (DWORD*)pVertexShaderCode->GetBufferPointer(), 
        &s_pTexVertexShader
    );
    pVertexShaderCode->Release();
    if (FAILED(hr)) return E_FAIL;

    // Compile and create pixel shader
    ID3DXBuffer* pPixelShaderCode = nullptr;
    ID3DXBuffer* pPixelErrorMsg = nullptr;
    hr = D3DXCompileShader(s_strTexPixelShaderProgram, 
                           (UINT)strlen(s_strTexPixelShaderProgram),
                           nullptr, 
                           nullptr, 
                           "main", 
                           "ps_2_0", 
                           0, 
                           &pPixelShaderCode, 
                           &pPixelErrorMsg,
                           nullptr);
    if (FAILED(hr)) {
        if (pPixelErrorMsg) 
            OutputDebugStringA((char*)pPixelErrorMsg->GetBufferPointer());
        if (pPixelErrorMsg) pPixelErrorMsg->Release();
        if (pPixelShaderCode) pPixelShaderCode->Release();
        return E_FAIL;
    }

    hr = g_pd3dDevice->CreatePixelShader(
        (DWORD*)pPixelShaderCode->GetBufferPointer(), 
        &s_pTexPixelShader
    );
    pPixelShaderCode->Release();
    if (FAILED(hr)) return E_FAIL;

    // Create vertex declaration for textured vertices
    D3DVERTEXELEMENT9 TexVertexElements[] = {
        { 0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };
    hr = g_pd3dDevice->CreateVertexDeclaration(TexVertexElements, &s_pTexVertexDecl);
    if (FAILED(hr)) return E_FAIL;

    // Create unit quad vertex buffer with texture coordinates
    hr = g_pd3dDevice->CreateVertexBuffer(
        6 * sizeof(TEXTUREVERTEX),
        D3DUSAGE_WRITEONLY,
        0,
        D3DPOOL_MANAGED,
        &s_pTexVB,
        nullptr
    );
    if (FAILED(hr)) return E_FAIL;

    // Initialize unit quad vertices with texture coordinates
    TEXTUREVERTEX texVertices[] = {
        // Position          // UV
        { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }, // Top-left
        { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f }, // Top-right
        { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }, // Bottom-left
        { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f }, // Bottom-left
        { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f }, // Top-right
        { 1.0f, 1.0f, 0.0f, 1.0f, 1.0f }  // Bottom-right
    };

    // Lock and copy vertex data
    TEXTUREVERTEX* pVertices;
    s_pTexVB->Lock(0, 0, (void**)&pVertices, 0);
    memcpy(pVertices, texVertices, sizeof(texVertices));
    s_pTexVB->Unlock();

    // Set texture sampling states
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

    s_bTexInitialized = true;
    return S_OK;
}

// Draw a 2D rectangle (pixel coordinates)
void XD2D_DrawRectangle(int width, int height, int x, int y, DWORD rgba) {
    if (!g_pd3dDevice) 
        return;

    // Initialize resources if needed
    if (FAILED(InitRectResources()))
        return;

    // Update vertex colors
    COLORVERTEX* pVertices;
    if (SUCCEEDED(s_pRectVB->Lock(0, 0, (void**)&pVertices, 0))) {
        for (int i = 0; i < 6; i++) {
            pVertices[i].Color = rgba;
        }
        s_pRectVB->Unlock();
    }

    // Calculate transformation matrices
    XMMATRIX matScale = XMMatrixScaling(
        static_cast<float>(width),
        static_cast<float>(height),
        1.0f
    );
    XMMATRIX matTranslate = XMMatrixTranslation(
        static_cast<float>(x),
        static_cast<float>(y),
        0.0f
    );
    XMMATRIX matWorld = matScale * matTranslate;
    XMMATRIX matWVP = matWorld * s_matOrtho;

    // Set shader parameters
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);

    // Configure rendering pipeline
    g_pd3dDevice->SetVertexDeclaration(s_pRectVertexDecl);
    g_pd3dDevice->SetStreamSource(0, s_pRectVB, 0, sizeof(COLORVERTEX));
    g_pd3dDevice->SetVertexShader(s_pRectVertexShader);
    g_pd3dDevice->SetPixelShader(s_pRectPixelShader);

    // Draw the rectangle
    g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
}

// Load texture from memory
XD2D_Texture* XD2D_LoadTextureFromMemory(const BYTE* data, DWORD size, int orig_w, int orig_h, int pot_w, int pot_h) {
    if (!g_pd3dDevice || !data || size == 0)
        return nullptr;

    // Create texture object
    XD2D_Texture* tex = new XD2D_Texture;
    tex->original_width = orig_w;
    tex->original_height = orig_h;
    tex->pot_width = pot_w;
    tex->pot_height = pot_h;
    
    // Create texture from DDS data
    if (FAILED(D3DXCreateTextureFromFileInMemory(
        g_pd3dDevice,
        data,
        size,
        &tex->texture
    ))) {
        delete tex;
        return nullptr;
    }

    return tex;
}


// Draw a texture at specified position
void XD2D_DrawTexture(XD2D_Texture* tex, int x, int y) {
    if (!g_pd3dDevice || !tex || !tex->texture)
        return;

    // Initialize resources if needed
    if (FAILED(InitTexResources()))
        return;

    // Save current blend state
    DWORD prevAlphaBlendEnable, prevSrcBlend, prevDestBlend;
    g_pd3dDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &prevAlphaBlendEnable);
    g_pd3dDevice->GetRenderState(D3DRS_SRCBLEND, &prevSrcBlend);
    g_pd3dDevice->GetRenderState(D3DRS_DESTBLEND, &prevDestBlend);

    // Enable alpha blending
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

    // Calculate UV scale factors for POT padding
    float u_scale = static_cast<float>(tex->original_width) / tex->pot_width;
    float v_scale = static_cast<float>(tex->original_height) / tex->pot_height;
    float uv_scale[4] = {u_scale, v_scale, 0.0f, 0.0f};
    
    // Set UV scaling constant
    g_pd3dDevice->SetVertexShaderConstantF(4, uv_scale, 1);

    // Calculate transformation matrices
    XMMATRIX matScale = XMMatrixScaling(
        static_cast<float>(tex->original_width),
        static_cast<float>(tex->original_height),
        1.0f
    );
    XMMATRIX matTranslate = XMMatrixTranslation(
        static_cast<float>(x),
        static_cast<float>(y),
        0.0f
    );
    XMMATRIX matWorld = matScale * matTranslate;
    XMMATRIX matWVP = matWorld * s_matOrtho;

    // Set shader parameters
    g_pd3dDevice->SetVertexShaderConstantF(0, (float*)&matWVP, 4);

    // Set texture
    g_pd3dDevice->SetTexture(0, tex->texture);

    // Configure rendering pipeline
    g_pd3dDevice->SetVertexDeclaration(s_pTexVertexDecl);
    g_pd3dDevice->SetStreamSource(0, s_pTexVB, 0, sizeof(TEXTUREVERTEX));
    g_pd3dDevice->SetVertexShader(s_pTexVertexShader);
    g_pd3dDevice->SetPixelShader(s_pTexPixelShader);

    // Draw the textured quad
    g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);

    // Restore original blend state
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, prevAlphaBlendEnable);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, prevSrcBlend);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, prevDestBlend);
}


// Release texture resources
void XD2D_ReleaseTexture(XD2D_Texture* tex) {
    if (tex) {
        if (tex->texture) tex->texture->Release();
        delete tex;
    }
}