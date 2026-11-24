# Drops mp.dll/client.dll into reTS\binaries\ts\, the staging folder xash.bat's assemble step
# copies from.
#
# Default: copy the pristine release_binaries build. reTS was extracted from a zip, not cloned,
# so it carries no git history to pin a source commit to, and the release binaries are the
# known-good pair for the shipped client. Set RETS_REBUILD=1 to build from source instead
# (picks up the ts_matchmode-facing source patches in ts_powerup.cpp/ts_gun.cpp and the
# interface alias below).
#
# Interface: source builds define RETS_INTERFACE_VERSION 6, but the shipped reTSX module
# (tsx_amxx.dll) still calls ReTS_GetInterface(5). dlls/rets_api.cpp therefore serves the v6
# table for 6 and a v5 copy of the same table (version field = 5) for 5. v6 only appended
# PowerUpTouch after every v5 slot, so the layouts are compatible and v5 callers see correct
# offsets. Without the alias, every ReTS_GetInterface-dependent native silently returns null
# (ts_setusercash, weaponrestriction, ...), which is why source builds used to be off by
# default. Keep the alias in sync if a future interface version inserts rather than appends.
# xash.bat's assemble step sets RETS_REBUILD=1 so the H9XASH package gets the source build
# (the local ts_matchmode patches and the disarm fix) rather than the older release pair.
$ErrorActionPreference = "Stop"
$reTS = Split-Path -Parent $PSScriptRoot
$dlls = Join-Path $reTS "binaries\ts\dlls"
$cldlls = Join-Path $reTS "binaries\ts\cl_dlls"
$release = Join-Path $reTS "release_binaries\ts"
New-Item -ItemType Directory -Force -Path $dlls, $cldlls | Out-Null

if ($env:RETS_REBUILD -ne "1") {
    Copy-Item (Join-Path $release "dlls\mp.dll") (Join-Path $dlls "mp.dll") -Force
    Copy-Item (Join-Path $release "cl_dlls\client.dll") (Join-Path $cldlls "client.dll") -Force
    Copy-Item (Join-Path $release "dlls\mp.dll.patch") (Join-Path $dlls "mp.dll.patch") -Force
    Copy-Item (Join-Path $release "cl_dlls\client.dll.patch") (Join-Path $cldlls "client.dll.patch") -Force
    # the release pair has no PrimeXT renderer: no pxrender.dll from an earlier source build may stay beside it
    Remove-Item (Join-Path $cldlls "pxrender.dll") -Force -ErrorAction SilentlyContinue
    Write-Host "  reTS: pristine release_binaries -> binaries\ts\ (RETS_REBUILD=1 builds from source instead)"
    exit 0
}

$buildDir = Join-Path $reTS "build-win32"

if (-not (Test-Path (Join-Path $buildDir "CMakeCache.txt"))) {
    cmake -B $buildDir -A Win32 -S $reTS
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }
}

cmake --build $buildDir --config Release --target hl client --parallel 2
if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }

Copy-Item (Join-Path $buildDir "dlls\Release\hl.dll") (Join-Path $dlls "mp.dll") -Force
Copy-Item (Join-Path $buildDir "cl_dll\Release\client.dll") (Join-Path $cldlls "client.dll") -Force
Copy-Item (Join-Path $release "dlls\mp.dll.patch") (Join-Path $dlls "mp.dll.patch") -Force
Copy-Item (Join-Path $release "cl_dlls\client.dll.patch") (Join-Path $cldlls "client.dll.patch") -Force

# The PrimeXT renderer (r_primext 1): client.dll loads cl_dlls\pxrender.dll, which the client target builds first
# (Win32 with MSVC), and pxrender.dll reads the shaders and data files below from the game folder. They go into
# binaries\ts\ so that xash.bat's layer 2 puts them in H9XASH\ts with the DLLs. The data files are H9's own, made by
# reTS\tools\primext-data (PRIMEXT-LICENCES.md).
$pxr = Join-Path $buildDir "cl_dll\primext\Release\pxrender.dll"
if (-not (Test-Path $pxr)) { throw "no pxrender.dll at $pxr" }
Copy-Item $pxr (Join-Path $cldlls "pxrender.dll") -Force
$game = Join-Path $reTS "game\ts"
$bin = Join-Path $reTS "binaries\ts"
foreach ($dir in "glsl", "gfx", "scripts") {
    Copy-Item (Join-Path $game $dir) $bin -Recurse -Force
}
New-Item -ItemType Directory -Force -Path (Join-Path $bin "sprites") | Out-Null
Copy-Item (Join-Path $game "sprites\null.spr") (Join-Path $bin "sprites\null.spr") -Force

Write-Host "  reTS built from source -> binaries\ts\dlls\mp.dll, binaries\ts\cl_dlls\client.dll and pxrender.dll, and the PrimeXT shaders and data (verify ReTS_GetInterface-dependent natives before shipping this)"
