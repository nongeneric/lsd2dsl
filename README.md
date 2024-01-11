lsd2dsl
=======

**lsd2dsl** is a decompiler for ABBYY Lingvo's and Duden proprietary dictionaries.

The following Lingvo versions are supported

<table>
  <thead>
    <tr>
      <th>Application</th>
      <th style="text-align:center">User</th>
      <th style="text-align:center">System</th>
      <th style="text-align:center">Abbreviations</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>Lingvo x6</td>
      <td style="text-align:center">152001</td>
      <td style="text-align:center">151005</td>
      <td style="text-align:center">155001</td>
    </tr>
    <tr>
      <td>Lingvo x5</td>
      <td style="text-align:center">142001</td>
      <td style="text-align:center">141004</td>
      <td style="text-align:center">145001</td>
    </tr>
    <tr>
      <td>Lingvo x3</td>
      <td style="text-align:center">132001</td>
      <td colspan="2" style="text-align:center">131001</td>
    </tr>
    <tr>
      <td>Lingvo 12</td>
      <td colspan="3" style="text-align:center">120001</td>
    </tr>
    <tr>
      <td>Lingvo 11</td>
      <td colspan="3" style="text-align:center">110001</td>
    </tr>
  </tbody>
</table>

In addition, audio archives (.lsa) are also supported.

Decompiling an LSD file produces the following output

* dictionary itself (.dsl)
* (optional) description in one or more languages (.ann)
* (optional) icon (.bmp)
* (optional) images and other data that are referenced from articles (.dsl.files.zip)

Decompiling an LSA file produces a single directory containing .wav audio files.

The following Duden versions have been tested

<table>
  <thead>
    <tr>
      <th>File Type</th>
      <th style="text-align:center">Versions</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>INF</td>
      <td style="text-align:center">1, 100, 101, 103, 111, 114, 200, 201, 202, 203, 204, 205, 206, 212, 213, 216, 219, 221, 224, 225, 226, 227, 228, 290, 300, 302, 303, 311, 400, 403, 410, 418, 500</td>
    </tr>
    <tr>
      <td>HIC</td>
      <td style="text-align:center">4, 5, 6</td>
    </tr>
  </tbody>
</table>

## Building on Fedora

Install required libraries

    dnf install boost-devel cmake zlib-devel qt6-qt5compat-devel qt6-qtbase-devel libvorbis-devel libsndfile-devel

Compile

    cmake .
    make

## Building on Windows

Install Qt6 using the official online installer. Install the Qt5Compat, Pdf and WebEngine modules.

Install Visual Studio 2022 (don't install VCPKG).

Install VCPKG from the official website and use it to build all the dependencies except Qt6:

    vcpkg install fmt libogg libvorbis libsndfile gtest zlib boost

Use the "x64 Native Tools Command Prompt for VS 2022" shortcut to start the terminal, then create a build directory and configure lsd2dsl using cmake (change the paths as needed):

    cmake -DCMAKE_TOOLCHAIN_FILE=C:/install/vcpkg/scripts/buildsystems/vcpkg.cmake -DQt6_ROOT=C:/Qt/6.6.1/msvc2019_64 -DENABLE_DUDEN=1 ..\lsd2dsl\

Note that setting ENABLE_DUDEN to 0 will disable Duden support, which in turn will make the installation package much smaller, as it won't need to contain the Qt6 WebEngine.

Build the installation package

    msbuild PACKAGE.vcxproj /property:Configuration=Release