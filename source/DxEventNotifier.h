//////////////////////////////////////////////////////////////////////aatags<aa78hjy>//
// AdVantage Terrain SDK, Copyright (C) 2004 - 2008 Filip Strugar.
// 
// Distributed as a FREE PUBLIC part of the SDK source code under the following rules:
// 
// * This code file can be used on 'as-is' basis. Author is not liable for any damages arising from its use.
// * The distribution of a modified version of the software is subject to the following restrictions:
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote the original 
//     software. If you use this software in a product, an acknowledgment in the product documentation would be 
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such and must not be misrepresented as being the original
//     software.
//  3. The license notice must not be removed from any source distributions.
//////////////////////////////////////////////////////////////////////aatage<aa78hjy>//

#pragma once

#pragma warning (disable : 4995)
#include <list>
#include <algorithm>
#pragma warning (default : 4995)
using namespace std;

class DxEventReceiver;

// Singleton class
class DxEventNotifier
{
private:
	IDirect3DDevice9 *         d3ddcnt_pd3dDevice;
	bool                       d3ddcnt_deviceLost;		
	list<DxEventReceiver*>     DXNotifyTargets;
	//
	DxEventNotifier();
   ~DxEventNotifier();
   //
public:
	//
	static HRESULT             PostCreateDevice(IDirect3DDevice9* pd3dDevice);
	static HRESULT             PostResetDevice(const D3DSURFACE_DESC* pBackBufferSurfaceDesc);
	static void                PostLostDevice();
	static void                PostDestroyDevice();
	//
private:
	friend class DxEventReceiver;
	//
	void                       RegisterNotifyTarget(DxEventReceiver * rh);
	void                       UnregisterNotifyTarget(DxEventReceiver * rh);	
	//
public:
   static IDirect3DDevice9 *  GetD3DDevice()                                  { return Instance().d3ddcnt_pd3dDevice; }
   static bool                IsD3DDeviceLost()                               { return Instance().d3ddcnt_deviceLost; }
   //
private:
   //static DxEventNotifier     s_Instance;
   static DxEventNotifier &   Instance();
};

//
class DxEventReceiver
{
protected:
	friend class DxEventNotifier;
	//
protected:
	//
	DxEventReceiver();
	virtual ~DxEventReceiver();
	//
   IDirect3DDevice9 *         GetD3DDevice()                                  { return DxEventNotifier::GetD3DDevice(); }
	bool                       IsD3DDeviceLost()                               { return DxEventNotifier::IsD3DDeviceLost(); }
	//
	virtual HRESULT 				OnCreateDevice()                                { return S_OK; };
	virtual HRESULT            OnResetDevice(const D3DSURFACE_DESC* pBackBufferSurfaceDesc)   { return S_OK; };
	virtual void               OnLostDevice()                                  {};
	virtual void               OnDestroyDevice()                               {};
};
