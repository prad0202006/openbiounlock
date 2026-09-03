#include "CredentialProvider.h"
#include <sddl.h>

static constexpr wchar_t kPipe[] = L"\\\\.\\pipe\\OpenBioUnlockPipe";

static HRESULT PipeApproval() {
    HANDLE pipe = CreateFileW(kPipe, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(GetLastError());
    const char request[] = "{\"type\":\"challenge\"}\n";
    DWORD written = 0;
    BOOL ok = WriteFile(pipe, request, static_cast<DWORD>(sizeof(request) - 1), &written, nullptr);
    CloseHandle(pipe);
    return ok && written == sizeof(request) - 1 ? S_OK : E_FAIL;
}

OpenBioCredential::OpenBioCredential() : references_(1) {}
HRESULT STDMETHODCALLTYPE OpenBioCredential::QueryInterface(REFIID riid, void** object) { if (!object) return E_POINTER; if (riid == IID_IUnknown || riid == IID_ICredentialProviderCredential) { *object = static_cast<ICredentialProviderCredential*>(this); AddRef(); return S_OK; } *object = nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE OpenBioCredential::AddRef() { return InterlockedIncrement(&references_); }
ULONG STDMETHODCALLTYPE OpenBioCredential::Release() { ULONG value = InterlockedDecrement(&references_); if (!value) delete this; return value; }
HRESULT STDMETHODCALLTYPE OpenBioCredential::Advise(IQueryContinueWithStatus*, ICredentialProviderCredentialEvents*) { return S_OK; }
HRESULT STDMETHODCALLTYPE OpenBioCredential::UnAdvise() { return S_OK; }
HRESULT STDMETHODCALLTYPE OpenBioCredential::SetSelected(BOOL* autoLogon) { if (!autoLogon) return E_POINTER; *autoLogon = FALSE; return S_OK; }
HRESULT STDMETHODCALLTYPE OpenBioCredential::SetDeselected() { return S_OK; }
HRESULT STDMETHODCALLTYPE OpenBioCredential::GetFieldState(DWORD, CREDENTIAL_PROVIDER_FIELD_STATE* state, CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* interactive) { if (!state || !interactive) return E_POINTER; *state = CPFS_DISPLAY_IN_SELECTED_TILE; *interactive = CPFIS_NONE; return S_OK; }
HRESULT STDMETHODCALLTYPE OpenBioCredential::GetBitmapValue(DWORD, HBITMAP*) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE OpenBioCredential::GetCheckboxValue(DWORD, BOOL*, PWSTR*) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE OpenBioCredential::GetSubmitButtonValue(DWORD, DWORD*) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE OpenBioCredential::GetComboBoxValueCount(DWORD, DWORD*, DWORD*) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE OpenBioCredential::GetComboBoxValueAt(DWORD, DWORD, PWSTR*) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE OpenBioCredential::GetStringValue(DWORD, PWSTR* value) { if (!value) return E_POINTER; *value = nullptr; return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE OpenBioCredential::GetECPFriendlyName(PWSTR*) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE OpenBioCredential::GetSerialization(CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* response, CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION*, PWSTR*, CREDENTIAL_PROVIDER_STATUS_ICON*) { if (!response) return E_POINTER; *response = CPGSR_NO_CREDENTIAL_NOT_FINISHED; return PipeApproval(); }
HRESULT STDMETHODCALLTYPE OpenBioCredential::ReportResult(NTSTATUS, NTSTATUS, PWSTR*, CREDENTIAL_PROVIDER_STATUS_ICON*) { return S_OK; }

OpenBioCredentialProvider::OpenBioCredentialProvider() : references_(1), credential_(new OpenBioCredential()) {}
HRESULT STDMETHODCALLTYPE OpenBioCredentialProvider::QueryInterface(REFIID riid, void** object) { if (!object) return E_POINTER; if (riid == IID_IUnknown || riid == IID_ICredentialProvider) { *object = static_cast<ICredentialProvider*>(this); AddRef(); return S_OK; } *object = nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE OpenBioCredentialProvider::AddRef() { return InterlockedIncrement(&references_); }
ULONG STDMETHODCALLTYPE OpenBioCredentialProvider::Release() { ULONG value = InterlockedDecrement(&references_); if (!value) delete this; return value; }
HRESULT STDMETHODCALLTYPE OpenBioCredentialProvider::SetUsageScenario(CREDENTIAL_PROVIDER_USAGE_SCENARIO, DWORD) { return S_OK; }
HRESULT STDMETHODCALLTYPE OpenBioCredentialProvider::SetSerialization(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION*) { return E_NOTIMPL; }
HRESULT STDMETHODCALLTYPE OpenBioCredentialProvider::Advise(ICredentialProviderEvents*, UINT_PTR) { return S_OK; }
HRESULT STDMETHODCALLTYPE OpenBioCredentialProvider::UnAdvise() { return S_OK; }
HRESULT STDMETHODCALLTYPE OpenBioCredentialProvider::GetFieldDescriptorCount(DWORD* count) { if (!count) return E_POINTER; *count = 0; return S_OK; }
HRESULT STDMETHODCALLTYPE OpenBioCredentialProvider::GetFieldDescriptorAt(DWORD, CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR**) { return E_BOUNDS; }
HRESULT STDMETHODCALLTYPE OpenBioCredentialProvider::GetCredentialCount(DWORD* count, DWORD* default_index, BOOL* auto_logon) { if (!count || !default_index || !auto_logon) return E_POINTER; *count = 1; *default_index = 0; *auto_logon = FALSE; return S_OK; }
HRESULT STDMETHODCALLTYPE OpenBioCredentialProvider::GetCredentialAt(DWORD index, ICredentialProviderCredential** credential) { if (index != 0 || !credential) return E_BOUNDS; *credential = credential_; credential_->AddRef(); return S_OK; }
