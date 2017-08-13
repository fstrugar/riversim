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

#include "DXUT.h"

#include "DxEventNotifier.h"

//DxEventNotifier * DxEventNotifier::s_pInstance = 0;
//DxEventNotifier DxEventNotifier::s_Instance;

DxEventNotifier & DxEventNotifier::Instance()
{
   static DxEventNotifier me;
   return me;
}

DxEventNotifier::DxEventNotifier(void)
{
	d3ddcnt_pd3dDevice = NULL;
	d3ddcnt_deviceLost = true;
}
//
DxEventNotifier::~DxEventNotifier(void)
{
   assert( DXNotifyTargets.size() == 0 );
}
//
HRESULT DxEventNotifier::PostCreateDevice(IDirect3DDevice9* pd3dDevice)
{
   DxEventNotifier::Instance().d3ddcnt_pd3dDevice = pd3dDevice;
	DxEventNotifier::Instance().d3ddcnt_deviceLost = false;
	HRESULT hr;
	for( list<DxEventReceiver*>::iterator it = DxEventNotifier::Instance().DXNotifyTargets.begin(); it != DxEventNotifier::Instance().DXNotifyTargets.end(); it++ )
		V_RETURN( (*it)->OnCreateDevice() );
	return S_OK;
}
//
HRESULT	DxEventNotifier::PostResetDevice(const D3DSURFACE_DESC* pBackBufferSurfaceDesc)
{
	DxEventNotifier::Instance().d3ddcnt_deviceLost = false;
	HRESULT hr;
	for( list<DxEventReceiver*>::iterator it = DxEventNotifier::Instance().DXNotifyTargets.begin(); it != DxEventNotifier::Instance().DXNotifyTargets.end(); it++ )
		V_RETURN( (*it)->OnResetDevice(pBackBufferSurfaceDesc) );
	return S_OK;
}
//
void DxEventNotifier::PostLostDevice()
{
	if( DxEventNotifier::Instance().d3ddcnt_deviceLost ) return;
	for( list<DxEventReceiver*>::reverse_iterator  it = DxEventNotifier::Instance().DXNotifyTargets.rbegin(); it != DxEventNotifier::Instance().DXNotifyTargets.rend(); it++ )
		(*it)->OnLostDevice();
	DxEventNotifier::Instance().d3ddcnt_deviceLost = true;
}
//
void DxEventNotifier::PostDestroyDevice()
{
	DxEventNotifier::Instance().d3ddcnt_pd3dDevice = NULL;
	DxEventNotifier::Instance().d3ddcnt_deviceLost = true;
	for( list<DxEventReceiver*>::reverse_iterator it = DxEventNotifier::Instance().DXNotifyTargets.rbegin(); it != DxEventNotifier::Instance().DXNotifyTargets.rend(); it++ )
		(*it)->OnDestroyDevice();
}
//
void DxEventNotifier::RegisterNotifyTarget(DxEventReceiver * rh)
{
	DXNotifyTargets.push_back( rh );
}
//
void DxEventNotifier::UnregisterNotifyTarget(DxEventReceiver * rh)
{
	list<DxEventReceiver*>::iterator it = find(DXNotifyTargets.begin(), DXNotifyTargets.end(), rh );
	if( it != DXNotifyTargets.end() ) 
		DXNotifyTargets.erase( it );
}
//

//
DxEventReceiver::DxEventReceiver( )
{
   DxEventNotifier::Instance().RegisterNotifyTarget( this );
}

DxEventReceiver::~DxEventReceiver()
{
	DxEventNotifier::Instance().UnregisterNotifyTarget( this );
}
