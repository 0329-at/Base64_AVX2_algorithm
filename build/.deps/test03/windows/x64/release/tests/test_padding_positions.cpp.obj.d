{
    depfiles_format = "gcc",
    files = {
        [[tests\test_padding_positions.cpp]]
    },
    values = {
        [[C:\Program Files\Microsoft Visual Studio\18\Professional\VC\Tools\Llvm\x64\bin\clang-cl]],
        {
            "--target=x86_64-pc-windows-msvc",
            "-MD",
            "-W3",
            "-O2",
            "-std:c++20",
            "-Iinclude",
            [[-Itests\common]],
            "/EHsc",
            "/utf-8",
            "-external:W0",
            [[-external:IC:\Users\30655\AppData\Local\.xmake\packages\g\gtest\v1.17.0\a14402329573418185bf52f17f8d405b\include]],
            "-march=native",
            "-DNDEBUG"
        }
    },
    depfiles = "build\\.objs\\test03\\windows\\x64\\release\\tests\\test_padding_positions.cpp.obj: \\\
  tests\\test_padding_positions.cpp include\\Base64_algorithm.hpp \\\
  tests\\common\\ref_base64.hpp\
"
}