#pragma once
#include <windows.h>
#include <credentialprovider.h>
#include <string>

class OpenBioCredential final : public ICredentialProviderCredential {
public:
    OpenBioCredential();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE Advise(IQueryContinueWithStatus*, ICredentialProviderCredentialEvents*) override;
    HRESULT STDMETHODCALLTYPE UnAdvise() override;
    HRESULT STDMETHODCALLTYPE SetSelected(BOOL* autoLogon) override;
    HRESULT STDMETHODCALLTYPE SetDeselected() override;
    HRESULT STDMETHODCALLTYPE GetFieldState(DWORD, CREDENTIAL_PROVIDER_FIELD_STATE*, CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE*) override;
    HRESULT STDMETHODCALLTYPE GetBitmapValue(DWORD, HBITMAP*) override;
    HRESULT STDMETHODCALLTYPE GetCheckboxValue(DWORD, BOOL*, PWSTR*) override;
    HRESULT STDMETHODCALLTYPE GetSubmitButtonValue(DWORD, DWORD*) override;
    HRESULT STDMETHODCALLTYPE GetComboBoxValueCount(DWORD, DWORD*, DWORD*) override;
    HRESULT STDMETHODCALLTYPE GetComboBoxValueAt(DWORD, DWORD, PWSTR*) override;
    HRESULT STDMETHODCALLTYPE GetStringValue(DWORD, PWSTR*) override;
    HRESULT STDMETHODCALLTYPE GetECPFriendlyName(PWSTR*) override;
    HRESULT STDMETHODCALLTYPE GetSerialization(CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE*, CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION*, PWSTR*, CREDENTIAL_PROVIDER_STATUS_ICON*) override;
    HRESULT STDMETHODCALLTYPE ReportResult(NTSTATUS, NTSTATUS, PWSTR*, CREDENTIAL_PROVIDER_STATUS_ICON*) override;
private:
    LONG references_;
};

class OpenBioCredentialProvider final : public ICredentialProvider {
public:
    OpenBioCredentialProvider();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE SetUsageScenario(CREDENTIAL_PROVIDER_USAGE_SCENARIO, DWORD) override;
    HRESULT STDMETHODCALLTYPE SetSerialization(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION*) override;
    HRESULT STDMETHODCALLTYPE Advise(ICredentialProviderEvents*, UINT_PTR) override;
    HRESULT STDMETHODCALLTYPE UnAdvise() override;
    HRESULT STDMETHODCALLTYPE GetFieldDescriptorCount(DWORD*) override;
    HRESULT STDMETHODCALLTYPE GetFieldDescriptorAt(DWORD, CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR**) override;
    HRESULT STDMETHODCALLTYPE GetCredentialCount(DWORD*, DWORD*, BOOL*) override;
    HRESULT STDMETHODCALLTYPE GetCredentialAt(DWORD, ICredentialProviderCredential**) override;
private:
    LONG references_;
    OpenBioCredential* credential_;
};
