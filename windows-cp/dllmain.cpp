#include "CredentialProvider.h"
#include <new>

#pragma comment(linker, "/EXPORT:DllGetClassObject")
#pragma comment(linker, "/EXPORT:DllCanUnloadNow")

static HMODULE module_handle = nullptr;
static LONG module_locks = 0;

class OpenBioClassFactory final : public IClassFactory {
public:
    OpenBioClassFactory() : references_(1) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (!object) return E_POINTER;
        *object = nullptr;
        if (riid == IID_IUnknown || riid == IID_IClassFactory) { *object = static_cast<IClassFactory*>(this); AddRef(); return S_OK; }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&references_); }
    ULONG STDMETHODCALLTYPE Release() override { const ULONG value = InterlockedDecrement(&references_); if (!value) delete this; return value; }
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID riid, void** object) override {
        if (outer || !object) return CLASS_E_NOAGGREGATION;
        *object = nullptr;
        auto* provider = new (std::nothrow) OpenBioCredentialProvider();
        if (!provider) return E_OUTOFMEMORY;
        const HRESULT result = provider->QueryInterface(riid, object);
        provider->Release();
        return result;
    }
    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override { if (lock) InterlockedIncrement(&module_locks); else InterlockedDecrement(&module_locks); return S_OK; }
private:
    LONG references_;
};

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) { module_handle = instance; DisableThreadLibraryCalls(instance); }
    return TRUE;
}

extern "C" HRESULT STDAPICALLTYPE DllGetClassObject(REFCLSID clsid, REFIID riid, void** object) {
    static const CLSID clsid_open_bio = {0xd1aa6d25, 0x6f49, 0x4a52, {0xa2, 0xcf, 0x6f, 0x22, 0xd9, 0xd1, 0x90, 0x01}};
    if (clsid != clsid_open_bio) return CLASS_E_CLASSNOTAVAILABLE;
    auto* factory = new (std::nothrow) OpenBioClassFactory();
    if (!factory) return E_OUTOFMEMORY;
    const HRESULT result = factory->QueryInterface(riid, object);
    factory->Release();
    return result;
}

extern "C" HRESULT STDAPICALLTYPE DllCanUnloadNow() {
    return module_locks == 0 ? S_OK : S_FALSE;
}
