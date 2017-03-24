{
  "targets": [
    {
      "target_name": "<(module_name)",
      "sources": [
        "src/addon.cc",
        "src/FileDialog.cc"
      ],
      "include_dirs" : [
        "<!(node -e \"require('nan')\")"
      ],
      'VCCLCompilerTool': {
          'ExceptionHandling': 1 # /EHsc
      },
      'configurations': {
        'Release': {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ExceptionHandling': 1,
            }
          }
        }
      },
      "defines":["UNICODE","_UNICODE", "MODULE_NAME=<(module_name)"]
    },
    {
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [ "<(module_name)" ],
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/<(module_name).node" ],
          "destination": "<(module_path)"
        }
      ]
    }
  ]
}
