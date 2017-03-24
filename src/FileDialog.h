#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include "windows.h"
#include <string>
#include <vector>

#include <v8.h>
#include <nan.h>

using namespace std;

using v8::Value;
using v8::Local;
using v8::Array;
using v8::Object;

using Nan::FunctionCallbackInfo;

enum DialogType {Show, Save};

class FileDialog {
private:
    HRESULT hr;
    IFileDialog* pDialog;
    IShellItem* pDirectory;
    IShellItem* pDefaultDirectory;
    wstring type;
    wstring directory;
    wstring filename;
    wstring extension;
    wstring title;
    wstring filenameLabel;
    wstring buttonLabel;
    wstring defaultDirectory;
    vector<wstring> results;
    bool clear;
    FILEOPENDIALOGOPTIONS flags;

    void Release();

public:
    FileDialog(Local<Object> options);
    ~FileDialog();

    void Init();
    void Show();
    void GetResults();
    Local<Array> Results();

    class InitFailed {};
    class CreateInstanceFailed {};
    class SetDirectoryFailed {};
    class SetDefaultDirectoryFailed {};
    class ShowFailed {};
    class SelectionFailed {};
    class SetFlagsFailed {};
    class SetClearFailed {};
    class GetResultsFailed {};
};

#endif