#include "suborno/platform/windows/tsf/class_factory.hpp"

#include "../com/module_lifetime.hpp"
#include "suborno/platform/windows/tsf/text_service.hpp"

namespace suborno::platform::windows {

SubornoClassFactory::SubornoClassFactory() {
    com::module_lifetime().object_created();
}

SubornoClassFactory::~SubornoClassFactory() {
    com::module_lifetime().object_destroyed();
}

STDMETHODIMP SubornoClassFactory::QueryInterface(REFIID riid, void** ppv) {
    if (ppv == nullptr) {
        return E_POINTER;
    }
    *ppv = nullptr;

    if (riid == IID_IUnknown || riid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) SubornoClassFactory::AddRef() {
    return static_cast<ULONG>(::InterlockedIncrement(&m_refCount));
}

STDMETHODIMP_(ULONG) SubornoClassFactory::Release() {
    const ULONG count = static_cast<ULONG>(::InterlockedDecrement(&m_refCount));
    if (count == 0) {
        delete this;
    }
    return count;
}

STDMETHODIMP SubornoClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) {
    if (ppv == nullptr) {
        return E_POINTER;
    }
    *ppv = nullptr;

    // No aggregation support
    if (pUnkOuter != nullptr) {
        return CLASS_E_NOAGGREGATION;
    }

    return SubornoTextService::CreateInstance(riid, ppv);
}

STDMETHODIMP SubornoClassFactory::LockServer(BOOL fLock) {
    return com::module_lifetime().lock_server(fLock != FALSE);
}

HRESULT SubornoClassFactory::CreateInstance(REFIID riid, void** ppv) {
    if (ppv == nullptr) {
        return E_POINTER;
    }
    *ppv = nullptr;

    SubornoClassFactory* factory = new (std::nothrow) SubornoClassFactory();
    if (factory == nullptr) {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = factory->QueryInterface(riid, ppv);
    factory->Release();
    return hr;
}

}  // namespace suborno::platform::windows
