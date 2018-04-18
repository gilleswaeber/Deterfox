This is a fork of Deterfox (http://deterfox.com/ -- https://github.com/nkdxczh/gecko-dev/tree/deterfox), changed to allow compilation on Windows using Visual Studio 2017.

The default documentation works fine for Linux.
On Ubuntu, the compilation required those packages: build-essential autoconf2.13 libgconf2-dev libpulse-dev libasound2-dev yasm libdbus-glib-1-dev gtk-2.0 gtk-3.0 glib-2.0 libxt-dev.

Instructions for Windows (64-bit):
1. Follow the Windows Prerequisities Guide at https://developer.mozilla.org/en-US/docs/Mozilla/Developer_guide/Build_Instructions/Windows_Prerequisites — install Visual Studio 2017 (do include the MFC/ATL support), install Rust, install mozilla-build
2. In In C:\mozilla-build\nsis-3.01\Bin, copy makensis-*.exe into makensis.exe
3. In %UserProfile%, create a .profile file with the following content (between the backticks):

```
export PATH=$PATH:~/.cargo/bin
# Adapt the version string to match the installed MSVC
export PATH=$PATH:/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/2017/Community/VC/Tools/MSVC/14.13.26128/bin/Hostx64/x64
# Adapt the version string to match the installed Windows Kit
export PATH=$PATH:/c/Program\ Files\ \(x86\)/Windows\ Kits/10/bin/10.0.15063.0/x64
export PATH=$PATH:/c/mozilla-build/nsis-3.01/Bin

# Adapt the version string to match the installed MSVC
export LIB=/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ Studio/2017/Community/VC/Tools/MSVC/14.13.26128/lib/x64

export INCLUDE=/c/Program\ Files\ \(x86\)/Microsoft\ Visual\ 
# Adapt the version string to match the installed MSVC
Studio/2017/Community/VC/Tools/MSVC/14.13.26128/include
# Adapt the version strings to match the installed Windows Kit
export INCLUDE=$INCLUDE:/c/Program\ Files\ \(x86\)/Windows\ Kits/10/Include/10.0.15063.0/ucrt
export INCLUDE=$INCLUDE:/c/Program\ Files\ \(x86\)/Windows\ Kits/10/Include/10.0.15063.0/um
export INCLUDE=$INCLUDE:/c/Program\ Files\ \(x86\)/Windows\ Kits/10/Include/10.0.15063.0/shared

# Adapt this command to point to the sources folder
export MOZBUILD_STATE_PATH=/c/mozilla-build-state
# Adapt this command to point to the sources
cd /c/mozilla-source
```

4. Launch `./mach bootstrap` with mode 2 in the mozilla shell (C:\mozilla-build\start-shell.bat)
5. In C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\...\bin\Hostx64\x86, create a symbolic link to `..\x64` named `x86_amd64` (old naming convention for Visual Studio VC++), do the same for Hostx86
6. In the source folder, create a file name mozconfig with the following content

```
# Compile Firefox x64
ac_add_options --target=x86_64-pc-mingw32
ac_add_options --host=x86_64-pc-mingw32

# Single thread run
#mk_add_options MOZ_MAKE_FLAGS="-j1"
```

7. Launch `./mach build` in the source folder.

If the build fails mid-way with an error message about a locked file, uncomment the last line in mozconfig, run `./mach build` and wait for a while until the problematic files are compiled in single threaded mode, then kill the build, recomment the line and relaunch it. You can also build everything in signle threaded mode but this will take longer.


-----This is the original README-----

An explanation of the Mozilla Source Code Directory Structure and links to
project pages with documentation can be found at:

    https://developer.mozilla.org/en/Mozilla_Source_Code_Directory_Structure

For information on how to build Mozilla from the source code, see:

    http://developer.mozilla.org/en/docs/Build_Documentation

To have your bug fix / feature added to Mozilla, you should create a patch and
submit it to Bugzilla (https://bugzilla.mozilla.org). Instructions are at:

    http://developer.mozilla.org/en/docs/Creating_a_patch
    http://developer.mozilla.org/en/docs/Getting_your_patch_in_the_tree

If you have a question about developing Mozilla, and can't find the solution
on http://developer.mozilla.org, you can try asking your question in a
mozilla.* Usenet group, or on IRC at irc.mozilla.org. [The Mozilla news groups
are accessible on Google Groups, or news.mozilla.org with a NNTP reader.]

You can download nightly development builds from the Mozilla FTP server.
Keep in mind that nightly builds, which are used by Mozilla developers for
testing, may be buggy. Firefox nightlies, for example, can be found at:

    https://archive.mozilla.org/pub/firefox/nightly/latest-mozilla-central/
            - or -
    http://nightly.mozilla.org/
