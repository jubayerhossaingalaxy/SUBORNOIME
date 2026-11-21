# Windows 10-এ SUBORNO IME build ও পরীক্ষা

এই গাইডটি Windows 10 x64 কম্পিউটারে বর্তমান SUBORNO branch build করে installer
তৈরি, install এবং হাতে-কলমে TSF typing যাচাই করার জন্য। এটি একটি **পরীক্ষামূলক
build**; live typing, clean-machine install এবং release signing এখনও সম্পূর্ণ
যাচাই হয়নি।

## ১. কী কী লাগবে

- Windows 10 x64।
- Visual Studio 2022; Installer থেকে **Desktop development with C++** workload,
  MSVC v143 এবং Windows SDK নির্বাচন করুন।
- Git for Windows।
- CMake 3.25 বা পরবর্তী সংস্করণ।
- Inno Setup 6 (installer তৈরির জন্য)।
- ইন্টারনেট সংযোগ; প্রথমবার vcpkg দিয়ে ICU, SQLite, WIL ও test dependency নামবে।

Visual Studio-এর **Developer PowerShell for VS 2022** খুলুন। যাচাই করুন:

```powershell
git --version
cmake --version
```

`cmake --version`-এ 3.25 বা পরবর্তী সংস্করণ থাকতে হবে।

## ২. সর্বশেষ branch নিন

নতুন checkout:

```powershell
git clone --branch copilot/complete-windows-tsf https://github.com/movielandbd1999-ops/SUBORNOIME.git
Set-Location SUBORNOIME
```

আগে থেকেই checkout থাকলে repository-র ভেতরে:

```powershell
git switch copilot/complete-windows-tsf
git pull
```

## ৩. নির্দিষ্ট vcpkg সংস্করণ প্রস্তুত করুন

repository-র `vcpkg.json` যে baseline নির্ধারণ করেছে, সেটিই ব্যবহার করতে হবে।
repository root থেকে চালান:

```powershell
git clone --filter=blob:none --no-checkout https://github.com/microsoft/vcpkg.git .vcpkg
git -C .vcpkg fetch --depth 1 origin 0df93a05b9de080129e653ec4776d7e14a1b2e4f
git -C .vcpkg checkout --detach 0df93a05b9de080129e653ec4776d7e14a1b2e4f
.\.vcpkg\bootstrap-vcpkg.bat -disableMetrics
$env:VCPKG_ROOT = (Resolve-Path .\.vcpkg).Path
```

যদি `.vcpkg` আগে থেকেই থাকে, আবার clone না করে নিশ্চিত করুন সেটি উপরের pinned
commit-এ আছে এবং বর্তমান PowerShell session-এ `VCPKG_ROOT` সেট করুন।

## ৪. Release build ও Windows test চালান

প্রথম configure-এ dependency download/build হতে সময় লাগতে পারে:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc-release --target suborno_unit_tests suborno_integration_tests suborno_windows_tests --parallel 2
ctest --preset windows-msvc-release --output-on-failure
cmake --build --preset windows-msvc-release --parallel 2
```

`ctest`-এ সব test pass না করলে installer install করে typing পরীক্ষা করবেন না।
Windows Debug test/build-ও চালাতে চাইলে:

```powershell
cmake --build --preset windows-msvc-debug --target suborno_unit_tests suborno_integration_tests suborno_windows_tests --parallel 2
ctest --preset windows-msvc-debug --output-on-failure
cmake --build --preset windows-msvc-debug --parallel 2
```

## ৫. Installer তৈরি করুন

Inno Setup 6 ইনস্টল করা থাকলে repository root থেকে:

```powershell
$iscc = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $iscc)) { throw "Inno Setup 6 ISCC.exe পাওয়া যায়নি।" }

$artifactDir = Join-Path $PWD "build\windows-msvc\artifacts"
New-Item -ItemType Directory -Force -Path $artifactDir | Out-Null
& $iscc /Q "/O$artifactDir" "/FSuborno-IME-0.1.0-x64" "installer/windows/suborno_installer.iss"
if ($LASTEXITCODE -ne 0) { throw "Installer compilation ব্যর্থ হয়েছে।" }

$installer = Join-Path $artifactDir "Suborno-IME-0.1.0-x64.exe"
if (-not (Test-Path $installer)) { throw "Installer ফাইল তৈরি হয়নি: $installer" }
Write-Host "Installer: $installer"
```

এই installer-এ administrator permission প্রয়োজন। আপাতত পরীক্ষার জন্য ব্যবহার
করুন; production-এ বিতরণের আগে signing এবং clean-machine install যাচাই বাকি।

## ৬. Install ও TSF profile চালু করুন

1. সম্ভব হলে আগে VM বা পরীক্ষার জন্য আলাদা Windows account-এ কাজ করুন।
2. `build\windows-msvc\artifacts\Suborno-IME-0.1.0-x64.exe` চালান এবং UAC
   prompt-এ সম্মতি দিন।
3. Windows-এর **Settings → Time & Language → Language & region** থেকে Bengali
   ভাষা যোগ করুন, যদি আগে যোগ করা না থাকে।
4. install শেষে Notepad খুলুন। `Win + Space` দিয়ে keyboard/input method তালিকা
   দেখুন এবং Suborno/Bengali profile নির্বাচন করুন। Profile না এলে sign out করে
   আবার sign in করে পুনরায় পরীক্ষা করুন।

Installer-টি COM/TSF registration নিজেই করে; পরীক্ষা করতে আলাদা করে
`regsvr32` চালাবেন না।

## ৭. হাতে-কলমে typing checklist

Notepad-এ পরীক্ষা করুন; তারপর সম্ভব হলে Word বা browser-এর text field-এও একই
পরীক্ষা করুন।

- [ ] `amar` বা `bangla`-র মতো phonetic অক্ষর লিখলে composition তৈরি হয়।
- [ ] composition-এর লেখা ঠিকভাবে update হয় এবং বাংলা Unicode text দেখা যায়।
- [ ] একাধিক candidate থাকলে `1`–`9` দিয়ে candidate নির্বাচন হয়।
- [ ] composition-এর সময় Backspace চাপলে শেষ input মুছে যায়।
- [ ] Space composition commit করে এবং একটি space যোগ করে।
- [ ] Escape composition বাতিল করে।
- [ ] অন্য punctuation/letter দিলে আগের composition commit হয়ে পরের input ঠিক থাকে।
- [ ] app বদলানো, একাধিক document window, এবং sign out/sign in-এর পর আচরণ নোট করুন।
- [ ] Bengali ও English input profile-এর মধ্যে `Win + Space` দিয়ে বদলানো যায়।

প্রতিটি ধাপের জন্য **Pass / Fail / Not tested** লিখুন। ভুল হলে ঠিক কী key
চাপলেন, কোন app-এ ঘটল, কী আশা করেছিলেন, কী ঘটেছে—এসবসহ screenshot বা error
সংরক্ষণ করুন। ব্যক্তিগত লেখা বা সংবেদনশীল তথ্য screenshot-এ রাখবেন না।

## ৮. Uninstall পরীক্ষা

Windows **Settings → Apps → Installed apps** থেকে Suborno uninstall করুন।
তারপর sign out/in করে যাচাই করুন:

- Suborno input profile আর তালিকায় নেই।
- English/Bengali-র অন্য input method আগের মতো কাজ করে।
- reinstall করলে setup এবং profile registration আবার সফল হয়।

সমস্যা হলে error, Windows version, build commit, test-এর ধাপ এবং সংশ্লিষ্ট
app-এর নাম লিখে রাখুন। এই তথ্য ছাড়া “Windows-এ কাজ করে” বলে production
readiness নিশ্চিত করা যাবে না।

## CI কী প্রমাণ করে—এবং কী করে না

GitHub Actions run [44766128685](https://github.com/movielandbd1999-ops/SUBORNOIME/actions/runs/44766128685)
Clang, GCC, MSVC Release/Debug build/test এবং installer compilation pass করেছে।
তবে CI কোনো desktop app-এ live typing করে না। বর্তমান workflow installer
compile করে, কিন্তু CI run সেটিকে downloadable Actions artifact হিসেবে upload
করে না। পরীক্ষার installer পেতে GitHub Actions-এর **Release** workflow-তে
`workflow_dispatch` দিয়ে `copilot/complete-windows-tsf` branch চালান; সফল হলে
run-এর Artifacts অংশে installer ১৪ দিন থাকবে। `v*` tag push করলে workflow
GitHub Release প্রকাশ করে—হাতে-কলমে যাচাই ও অনুমোদন ছাড়া tag push করবেন না।

Artifact না পাওয়া গেলে বা নিজে build করতে চাইলে উপরের local build ধাপ অনুসরণ
করুন।

এই হাতে-কলমে পরীক্ষা, clean-machine install/uninstall এবং release signing শেষ না
হওয়া পর্যন্ত প্রকল্পকে production-ready ধরা যাবে না।
