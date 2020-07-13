{
    "targets": [{
        "target_name": "addon",
        "cflags!": [ "-fno-exceptions" ],
        "cflags_cc!": [ "-fno-exceptions" ],
        "msvs_settings": {
                "VCCLCompilerTool": {
                  "ExceptionHandling": 1,
                  "AdditionalOptions": [ "/std:c++17" ]
                }
              },
        "sources": [
            "src/cpp/permissions/FolderPermissionsManager.cpp",
            "src/cpp/helper/helper.cpp",
            "src/cpp/addon.cpp"
        ],
        'include_dirs': [
            "<!@(node -p \"require('node-addon-api').include\")"
        ],
        'dependencies': [
            "<!(node -p \"require('node-addon-api').gyp\")"
        ]
    }]
}