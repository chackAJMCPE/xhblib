// XboxDraw2D.h
#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <xnamath.h>

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 1280
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 720
#endif

// Forward declarations
struct XD2D_Texture;

extern LPDIRECT3D9       g_pD3D;
extern LPDIRECT3DDEVICE9 g_pd3dDevice;

// Core functions
HRESULT XD2D_Init();
HRESULT XD2D_Render();
HRESULT XD2D_End();

// Drawing functions
void XD2D_DrawRectangle(int width, int height, int x, int y, DWORD rgba);

// Texture functions
XD2D_Texture* XD2D_LoadTextureFromMemory(const BYTE* data, DWORD size, int orig_w, int orig_h, int pot_w, int pot_h);
void XD2D_DrawTexture(XD2D_Texture* tex, int x, int y);
void XD2D_ReleaseTexture(XD2D_Texture* tex);