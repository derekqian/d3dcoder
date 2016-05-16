//=============================================================================
// SpriteDemo.cpp by Frank Luna (C) 2005 All Rights Reserved.
//
// Demonstrates how to draw sprites with the ID3DXSprite interface, and how
// to move the sprite around and fire missiles with Direct Input using
// the mouse and keyboard.
//=============================================================================

#include "d3dApp.h"
#include "DirectInput.h"
#include <crtdbg.h>
#include "GfxStats.h"
#include <list>

struct BulletInfo
{
	D3DXVECTOR3 pos;
	float rotation;
	float life;
};

class SpriteDemo : public D3DApp
{
public:
	SpriteDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP);
	~SpriteDemo();

	bool checkDeviceCaps();
	void onLostDevice();
	void onResetDevice();
	void updateScene(float dt);
	void drawScene();

	// Helper functions.
	void updateShip(float dt);
	void updateBullets(float dt);
	void drawBkgd();
	void drawShip();
	void drawBullets();
private:
	GfxStats* mGfxStats;
	
	ID3DXSprite* mSprite;

	IDirect3DTexture9* mBkgdTex;
	D3DXVECTOR3 mBkgdCenter;

	IDirect3DTexture9* mShipTex;
	D3DXVECTOR3 mShipCenter;
	D3DXVECTOR3 mShipPos;
	float       mShipSpeed;
	float       mShipRotation;

	IDirect3DTexture9* mBulletTex;
	D3DXVECTOR3 mBulletCenter;
	std::list<BulletInfo> mBulletList;

	const float BULLET_SPEED;
	const float MAX_SHIP_SPEED;
	const float SHIP_ACCEL;
	const float SHIP_DRAG; 
};


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif

	SpriteDemo app(hInstance, "Sprite Demo", D3DDEVTYPE_HAL, D3DCREATE_HARDWARE_VERTEXPROCESSING);
	gd3dApp = &app;

	DirectInput di(DISCL_NONEXCLUSIVE|DISCL_FOREGROUND, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
	gDInput = &di;

	return gd3dApp->run();
}

SpriteDemo::SpriteDemo(HINSTANCE hInstance, std::string winCaption, D3DDEVTYPE devType, DWORD requestedVP)
: D3DApp(hInstance, winCaption, devType, requestedVP), BULLET_SPEED(2500.0f), MAX_SHIP_SPEED(1500.0f),
	SHIP_ACCEL(1000.0f), SHIP_DRAG(0.85f)
{
	if(!checkDeviceCaps())
	{
		MessageBox(0, "checkDeviceCaps() Failed", 0, 0);
		PostQuitMessage(0);
	}

	mGfxStats = new GfxStats();

	HR(D3DXCreateSprite(gd3dDevice, &mSprite));

	HR(D3DXCreateTextureFromFile(gd3dDevice, "bkgd1.bmp", &mBkgdTex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "alienship.bmp", &mShipTex));
	HR(D3DXCreateTextureFromFile(gd3dDevice, "bullet.bmp", &mBulletTex));

	mBkgdCenter = D3DXVECTOR3(256.0f, 256.0f, 0.0f);
	mShipCenter = D3DXVECTOR3(64.0f, 64.0f, 0.0f);
	mBulletCenter = D3DXVECTOR3(32.0f, 32.0f, 0.0f);

	mShipPos      = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	mShipSpeed    = 0.0f;
	mShipRotation = 0.0f;
	
	onResetDevice();
}

SpriteDemo::~SpriteDemo()
{
	delete mGfxStats;
	ReleaseCOM(mSprite);
	ReleaseCOM(mBkgdTex);
	ReleaseCOM(mShipTex);
	ReleaseCOM(mBulletTex);
}

bool SpriteDemo::checkDeviceCaps()
{
	// Nothing to check.
	return true;
}

void SpriteDemo::onLostDevice()
{
	mGfxStats->onLostDevice();
	HR(mSprite->OnLostDevice());
}

void SpriteDemo::onResetDevice()
{
	// Call the onResetDevice of other objects.
	mGfxStats->onResetDevice();
	HR(mSprite->OnResetDevice());

	// Sets up the camera 1000 units back looking at the origin.
	D3DXMATRIX V;
	D3DXVECTOR3 pos(0.0f, 0.0f, -1000.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXMatrixLookAtLH(&V, &pos, &target, &up);
	HR(gd3dDevice->SetTransform(D3DTS_VIEW, &V));

	// The following code defines the volume of space the camera sees.
	D3DXMATRIX P;
	RECT R;
	GetClientRect(mhMainWnd, &R);
	float width  = (float)R.right;
	float height = (float)R.bottom;
	D3DXMatrixPerspectiveFovLH(&P, D3DX_PI*0.25f, width/height, 1.0f, 5000.0f);
	HR(gd3dDevice->SetTransform(D3DTS_PROJECTION, &P));

	// This code sets texture filters, which helps to smooth out distortions
	// when you scale a texture.  
	HR(gd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR));
	HR(gd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR));
	HR(gd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR));

	// This line of code disables Direct3D lighting.
	HR(gd3dDevice->SetRenderState(D3DRS_LIGHTING, false));
	
	// The following code specifies an alpha test and reference value.
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHAREF, 10));
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER));

	// The following code is used to setup alpha blending.
	HR(gd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE));
	HR(gd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1));
	HR(gd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA));
	HR(gd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA));

	// Indicates that we are using 2D texture coordinates.
	HR(gd3dDevice->SetTextureStageState(
		0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2));
}

void SpriteDemo::updateScene(float dt)
{
	// Two triangles for each sprite--two for background,
	// two for ship, and two for each bullet.  Similarly,
	// 4 vertices for each sprite.
	mGfxStats->setTriCount(4 + mBulletList.size()*2);
	mGfxStats->setVertexCount(8 + mBulletList.size()*4);
	mGfxStats->update(dt);

	// Get snapshot of input devices.
	gDInput->poll();

	// Update game objects.
	updateShip(dt);
	updateBullets(dt);
}

void SpriteDemo::updateShip(float dt)
{
	// Check input.
	if( gDInput->keyDown(DIK_A) )	 mShipRotation += 4.0f * dt;
	if( gDInput->keyDown(DIK_D) )	 mShipRotation -= 4.0f * dt;
	if( gDInput->keyDown(DIK_W) )	 mShipSpeed    += SHIP_ACCEL * dt;
	if( gDInput->keyDown(DIK_S) )	 mShipSpeed    -= SHIP_ACCEL * dt;

	// Clamp top speed.
	if( mShipSpeed > MAX_SHIP_SPEED )	 mShipSpeed =  MAX_SHIP_SPEED;
	if( mShipSpeed < -MAX_SHIP_SPEED )	 mShipSpeed = -MAX_SHIP_SPEED;

	// Rotate counterclockwise when looking down -z axis (i.e., rotate
	// clockwise when looking down the +z axis.
	D3DXVECTOR3 shipDir(-sinf(mShipRotation), cosf(mShipRotation), 0.0f);

	// Update position and speed based on time.
	mShipPos   += shipDir * mShipSpeed * dt;
	mShipSpeed -= SHIP_DRAG * mShipSpeed * dt;
}

void SpriteDemo::updateBullets(float dt)
{
	// Make static so that its value persists across function calls.
	static float fireDelay = 0.0f;

	// Accumulate time.
	fireDelay += dt;

	// Did the user press the spacebar key and has 0.1 seconds passed?
	// We can only fire one bullet every 0.1 seconds.  If we do not
	// put this delay in, the ship will fire bullets way too fast.
	if( gDInput->keyDown(DIK_SPACE) && fireDelay > 0.1f )
	{
		BulletInfo bullet;

		// Remember the ship is always drawn at the center of the window--
		// the origin.  Therefore, bullets originate from the origin.
		bullet.pos      = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

		// The bullets rotation should match the ship's rotating at the
		// instant it is fired.  
		bullet.rotation = mShipRotation;

		// Bullet just born.
		bullet.life     = 0.0f;

		// Add the bullet to the list.
		mBulletList.push_back(bullet);

		// A bullet was just fired, so reset the fire delay.
		fireDelay = 0.0f;
	}

	// Now loop through each bullet, and update its position.
	std::list<BulletInfo>::iterator i = mBulletList.begin();
	while( i != mBulletList.end() )
	{
		// Accumualte the time the bullet has lived.
		i->life += dt;

		// If the bullet has lived for two seconds, kill it.  By now the
		// bullet should have flown off the screen and cannot be seen.
		if(i->life >= 2.0f)
			i = mBulletList.erase(i);

		// Otherwise, update its position by moving along its directional
		// path.  Code similar to how we move the ship--but no drag.
		else
		{
			D3DXVECTOR3 dir(-sinf(i->rotation), cosf(i->rotation), 0.0f);
			i->pos += dir * BULLET_SPEED * dt;
			++i;
		}
	}
}

void SpriteDemo::drawScene()
{
	// Clear the backbuffer and depth buffer.
	HR(gd3dDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0));

	HR(gd3dDevice->BeginScene());
	HR(mSprite->Begin(D3DXSPRITE_OBJECTSPACE|D3DXSPRITE_DONOTMODIFY_RENDERSTATE));
	drawBkgd();
	drawShip();	
	drawBullets();
	mGfxStats->display();
	HR(mSprite->End());
	HR(gd3dDevice->EndScene());

	// Present the backbuffer.
	HR(gd3dDevice->Present(0, 0, 0, 0));
}

void SpriteDemo::drawBkgd()
{
	// Set a texture coordinate scaling transform.  Here we scale the texture 
	// coordinates by 10 in each dimension.  This tiles the texture 
	// ten times over the sprite surface.
	D3DXMATRIX texScaling;
	D3DXMatrixScaling(&texScaling, 10.0f, 10.0f, 0.0f);
	HR(gd3dDevice->SetTransform(D3DTS_TEXTURE0, &texScaling));

	// Position and size the background sprite--remember that 
	// we always draw the ship in the center of the client area 
	// rectangle. To give the illusion that the ship is moving,
	// we translate the background in the opposite direction.
	D3DXMATRIX T, S;
	D3DXMatrixTranslation(&T, -mShipPos.x, -mShipPos.y, -mShipPos.z);
	D3DXMatrixScaling(&S, 20.0f, 20.0f, 0.0f);
	HR(mSprite->SetTransform(&(S*T)));

	// Draw the background sprite.
	HR(mSprite->Draw(mBkgdTex, 0, &mBkgdCenter, 0, D3DCOLOR_XRGB(255, 255, 255)));
	HR(mSprite->Flush());

	// Restore defaults texture coordinate scaling transform.
	D3DXMatrixScaling(&texScaling, 1.0f, -1.0f, 0.0f);
	HR(gd3dDevice->SetTransform(D3DTS_TEXTURE0, &texScaling));
}

void SpriteDemo::drawShip()
{
	// Turn on the alpha test.
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, true));

	// Set ships orientation.
	D3DXMATRIX R;
	D3DXMatrixRotationZ(&R, mShipRotation);
	HR(mSprite->SetTransform(&R));

	// Draw the ship.
	HR(mSprite->Draw(mShipTex, 0, &mShipCenter, 0, D3DCOLOR_XRGB(255, 255, 255)));
	HR(mSprite->Flush());

	// Turn off the alpha test.
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false));
}

void SpriteDemo::drawBullets()
{
	// Turn on alpha blending.
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true));

	// For each bullet...
	std::list<BulletInfo>::iterator i = mBulletList.begin();
	while( i != mBulletList.end() )
	{
		// Set its position and orientation.
		D3DXMATRIX T, R;
		D3DXMatrixRotationZ(&R, i->rotation);
		D3DXMatrixTranslation(&T, i->pos.x, i->pos.y, i->pos.z);
		HR(mSprite->SetTransform(&(R*T)));

		// Add it to the batch.
		HR(mSprite->Draw(mBulletTex, 0, &mBulletCenter, 0, D3DCOLOR_XRGB(255, 255, 255)));
		++i;
	}
	// Draw all the bullets at once.
	HR(mSprite->Flush());

	// Turn off alpha blending.
	HR(gd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false));
}