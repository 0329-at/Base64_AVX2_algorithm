{
    files = {
        [[build\.objs\test03\windows\x64\release\src\main.cpp.obj]],
        [[build\.objs\test03\windows\x64\release\tests\test_api_consistency.cpp.obj]],
        [[build\.objs\test03\windows\x64\release\tests\test_byte_values.cpp.obj]],
        [[build\.objs\test03\windows\x64\release\tests\test_cross_validate.cpp.obj]],
        [[build\.objs\test03\windows\x64\release\tests\test_decode_length.cpp.obj]],
        [[build\.objs\test03\windows\x64\release\tests\test_encode_vs_ref.cpp.obj]],
        [[build\.objs\test03\windows\x64\release\tests\test_invalid_chars.cpp.obj]],
        [[build\.objs\test03\windows\x64\release\tests\test_known_vectors.cpp.obj]],
        [[build\.objs\test03\windows\x64\release\tests\test_mutation.cpp.obj]],
        [[build\.objs\test03\windows\x64\release\tests\test_padding_positions.cpp.obj]],
        [[build\.objs\test03\windows\x64\release\tests\test_plus_slash.cpp.obj]],
        [[build\.objs\test03\windows\x64\release\tests\test_roundtrip.cpp.obj]],
        [[build\.objs\test03\windows\x64\release\tests\test_urlsafe.cpp.obj]],
        [[build\.objs\test03\windows\x64\release\tests\test_validate.cpp.obj]]
    },
    values = {
        [[C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Tools\MSVC\14.51.36231\bin\HostX64\x64\link.exe]],
        {
            "-nologo",
            "-dynamicbase",
            "-nxcompat",
            "-machine:x64",
            [[-libpath:C:\Users\30655\AppData\Local\.xmake\packages\g\gtest\v1.17.0\a14402329573418185bf52f17f8d405b\lib]],
            "/opt:ref",
            "/opt:icf",
            "gmock.lib",
            "gtest.lib"
        }
    }
}