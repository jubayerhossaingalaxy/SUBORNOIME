#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <msctf.h>
#include <string>
#include <vector>
#include <wil/com.h>
#include <wil/resource.h>

#include "suborno/dictionary/dictionary.hpp"
#include "suborno/platform/windows/app_paths.hpp"
#include "suborno/platform/windows/tsf/guids.hpp"
extern "C" _Check_return_ HRESULT WINAPI DllCanUnloadNow();
extern "C" _Check_return_ HRESULT WINAPI DllGetClassObject(_In_ REFCLSID class_id,
                                                           _In_ REFIID interface_id,
                                                           _Outptr_ void** object);
extern "C" HRESULT WINAPI DllRegisterServer();
extern "C" HRESULT WINAPI DllUnregisterServer();

namespace {

class ComApartment final {
  public:
    ComApartment()
        : result_(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {}

    ~ComApartment() {
        if (SUCCEEDED(result_)) {
            ::CoUninitialize();
        }
    }

    bool initialized() const noexcept {
        return SUCCEEDED(result_) || result_ == RPC_E_CHANGED_MODE;
    }

  private:
    HRESULT result_;
};

}  // namespace

TEST(WindowsBuildSmokeTest, LoadsTheNativeWindowsModuleTarget) {
    EXPECT_NE(::GetModuleHandleW(L"suborno_windows_tsf.dll"), nullptr);
}

TEST(WindowsUserDataTest, MigratesSettingsAndSqliteDataWithoutReplacingNewerFiles) {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path()
        / (L"SubornoMigration-" + std::to_wstring(::GetCurrentProcessId()) + L"-"
           + std::to_wstring(::GetTickCount64()));
    const std::filesystem::path legacy_directory = root / L"Local";
    const std::filesystem::path user_directory = root / L"Roaming";
    std::error_code filesystem_error;
    std::filesystem::create_directories(legacy_directory, filesystem_error);
    ASSERT_FALSE(filesystem_error);

    const std::filesystem::path legacy_database = legacy_directory / L"dictionary.sqlite3";
    {
        suborno::dictionary::Dictionary dictionary(legacy_database.string());
        dictionary.add_entry("বাংলা", "বাংলা", 3);
    }
    {
        std::ofstream settings(legacy_directory / L"settings.ini", std::ios::binary);
        ASSERT_TRUE(settings.is_open());
        settings << "legacy-settings";
    }

    ASSERT_EQ(
        suborno::platform::windows::prepare_user_data_directory(user_directory, legacy_directory),
        S_OK);
    const auto new_database = user_directory / L"dictionaries" / L"user_candidates.db";
    ASSERT_TRUE(std::filesystem::exists(new_database));
    ASSERT_TRUE(std::filesystem::exists(user_directory / L"dictionaries" / L"suborno_learnt_bn"));
    ASSERT_TRUE(std::filesystem::exists(user_directory / L"layouts"));
    {
        std::ifstream settings(user_directory / L"settings.ini", std::ios::binary);
        ASSERT_TRUE(settings.is_open());
        EXPECT_EQ(std::string(std::istreambuf_iterator<char>(settings), {}), "legacy-settings");
    }

    {
        suborno::dictionary::Dictionary dictionary(new_database.string());
        EXPECT_EQ(dictionary.find_matches("বাংলা"), (std::vector<std::string>{"বাংলা"}));
        dictionary.add_entry("বই", "বই", 1);
    }

    {
        std::ofstream settings(user_directory / L"settings.ini",
                               std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(settings.is_open());
        settings << "new-settings";
    }
    ASSERT_EQ(
        suborno::platform::windows::prepare_user_data_directory(user_directory, legacy_directory),
        S_OK);
    {
        std::ifstream settings(user_directory / L"settings.ini", std::ios::binary);
        ASSERT_TRUE(settings.is_open());
        EXPECT_EQ(std::string(std::istreambuf_iterator<char>(settings), {}), "new-settings");
    }
    {
        suborno::dictionary::Dictionary dictionary(new_database.string());
        EXPECT_EQ(dictionary.find_matches("বই"), (std::vector<std::string>{"বই"}));
    }

    std::filesystem::remove_all(root, filesystem_error);
    EXPECT_FALSE(filesystem_error);
}

TEST(WindowsSharedDataTest, ReportsProgramDataSharedDirectories) {
    std::filesystem::path shared;
    ASSERT_EQ(suborno::platform::windows::get_shared_data_directory(shared), S_OK);
    EXPECT_EQ(shared.filename(), L"Suborno");

    std::filesystem::path assets, fonts, themes, dictionaries, layouts;
    ASSERT_EQ(suborno::platform::windows::get_shared_assets_directory(assets), S_OK);
    ASSERT_EQ(suborno::platform::windows::get_shared_fonts_directory(fonts), S_OK);
    ASSERT_EQ(suborno::platform::windows::get_shared_themes_directory(themes), S_OK);
    ASSERT_EQ(suborno::platform::windows::get_shared_dictionaries_directory(dictionaries), S_OK);
    ASSERT_EQ(suborno::platform::windows::get_shared_layouts_directory(layouts), S_OK);

    EXPECT_EQ(assets, shared / L"assets");
    EXPECT_EQ(fonts, shared / L"assets" / L"fonts");
    EXPECT_EQ(themes, shared / L"assets" / L"themes");
    EXPECT_EQ(dictionaries, shared / L"dictionaries");
    EXPECT_EQ(layouts, shared / L"layouts");
}

TEST(WindowsUserDataPathTest, ReportsRoamingAppDataUserPaths) {
    std::filesystem::path user;
    ASSERT_EQ(suborno::platform::windows::get_user_data_directory(user), S_OK);
    EXPECT_EQ(user.filename(), L"Suborno");

    std::filesystem::path settings, dictionary, user_layouts;
    ASSERT_EQ(suborno::platform::windows::get_user_settings_path(settings), S_OK);
    ASSERT_EQ(suborno::platform::windows::get_user_dictionary_path(dictionary), S_OK);
    ASSERT_EQ(suborno::platform::windows::get_user_layouts_directory(user_layouts), S_OK);

    EXPECT_EQ(settings, user / L"settings.ini");
    EXPECT_EQ(dictionary, user / L"dictionaries" / L"user_candidates.db");
    EXPECT_EQ(user_layouts, user / L"layouts");
}

TEST(WindowsTsfContractTest, CreatesTheTextServiceAndTracksServerLocks) {
    EXPECT_EQ(DllCanUnloadNow(), S_OK);

    wil::com_ptr<IClassFactory> factory;
    ASSERT_EQ(DllGetClassObject(suborno::platform::windows::CLSID_SubornoTextService,
                                IID_IClassFactory,
                                reinterpret_cast<void**>(factory.put())),
              S_OK);
    ASSERT_NE(factory.get(), nullptr);
    EXPECT_EQ(DllCanUnloadNow(), S_FALSE);

    EXPECT_EQ(factory->LockServer(TRUE), S_OK);
    EXPECT_EQ(factory->LockServer(FALSE), S_OK);

    wil::com_ptr<ITfTextInputProcessor> service;
    ASSERT_EQ(factory->CreateInstance(
                  nullptr, IID_ITfTextInputProcessor, reinterpret_cast<void**>(service.put())),
              S_OK);
    ASSERT_NE(service.get(), nullptr);

    wil::com_ptr<ITfKeyEventSink> key_sink;
    EXPECT_EQ(
        service->QueryInterface(IID_ITfKeyEventSink, reinterpret_cast<void**>(key_sink.put())),
        S_OK);
    key_sink.reset();
    service.reset();
    factory.reset();
    EXPECT_EQ(DllCanUnloadNow(), S_OK);
}

TEST(WindowsTsfContractTest, RejectsUnknownClassesAndBalancesServerLocks) {
    int placeholder = 0;
    void* object = &placeholder;
    EXPECT_EQ(DllGetClassObject(GUID_NULL, IID_IClassFactory, &object), CLASS_E_CLASSNOTAVAILABLE);
    EXPECT_EQ(object, nullptr);

    wil::com_ptr<IClassFactory> factory;
    ASSERT_EQ(DllGetClassObject(suborno::platform::windows::CLSID_SubornoTextService,
                                IID_IClassFactory,
                                reinterpret_cast<void**>(factory.put())),
              S_OK);
    EXPECT_EQ(factory->LockServer(FALSE), E_UNEXPECTED);
}

TEST(WindowsTsfContractTest, RegistersAndUnregistersTheLanguageProfile) {
    ComApartment apartment;
    ASSERT_TRUE(apartment.initialized());

    DllUnregisterServer();
    const HRESULT registered = DllRegisterServer();
    EXPECT_EQ(registered, S_OK);

    if (SUCCEEDED(registered)) {
        wil::com_ptr<ITfInputProcessorProfiles> profiles;
        const HRESULT profiles_result = ::CoCreateInstance(CLSID_TF_InputProcessorProfiles,
                                                           nullptr,
                                                           CLSCTX_INPROC_SERVER,
                                                           IID_PPV_ARGS(profiles.put()));
        EXPECT_EQ(profiles_result, S_OK);
        if (SUCCEEDED(profiles_result)) {
            wil::unique_bstr description;
            const HRESULT description_result = profiles->GetLanguageProfileDescription(
                suborno::platform::windows::CLSID_SubornoTextService,
                MAKELANGID(0x45, 0x02),
                suborno::platform::windows::GUID_LANGUAGE_PROFILE_SUBORNO_BENGALI,
                description.put());
            EXPECT_EQ(description_result, S_OK);
            if (SUCCEEDED(description_result) && description.get() != nullptr) {
                EXPECT_STREQ(description.get(), L"Suborno (Bengali, Bangladesh)");
                BOOL enabled = FALSE;
                EXPECT_EQ(profiles->IsEnabledLanguageProfile(
                              suborno::platform::windows::CLSID_SubornoTextService,
                              MAKELANGID(0x45, 0x02),
                              suborno::platform::windows::GUID_LANGUAGE_PROFILE_SUBORNO_BENGALI,
                              &enabled),
                          S_OK);
                EXPECT_NE(enabled, FALSE);
            } else {
                ADD_FAILURE() << "The registered language profile has no description.";
            }
        }
    }

    EXPECT_EQ(DllUnregisterServer(), S_OK);
}
