#include "windows.h"

#include <string>
#include <v8.h>
#include <nan.h>

#include "FileDialog.h"

using namespace std;

using v8::Local;
using v8::Number;
using v8::Value;
using v8::Array;
using v8::Object;
using v8::String;
using v8::Boolean;

using Nan::AsyncQueueWorker;
using Nan::AsyncWorker;
using Nan::New;

void FileDialog::Release() {
    #define RELEASE_COM(name)       \
        if ((name) != nullptr) {    \
            (name)->Release();      \
            (name) = nullptr;       \
        }

    RELEASE_COM(pDefaultDirectory)
    RELEASE_COM(pDirectory)
    RELEASE_COM(pDialog)

    #undef RELEASE_COM
}

FileDialog::FileDialog(Local<Object> options) : 
    hr(E_FAIL), 
    pDialog(nullptr), 
    pDirectory(nullptr), 
    pDefaultDirectory(nullptr),
    flags(0),
    clear(false)
{
    Local<String> key;
    Local<Value> val;
   
    #define SET_OPT(name, keyName)                                              \
        key = New(keyName).ToLocalChecked();                                    \
        if (options->Has(key)) {                                                \
            val = options->Get(key);                                            \
            if (val->IsString()) {                                              \
                (name) = wstring((wchar_t*) * String::Value(val.As<String>())); \
            }                                                                   \
        }                                                                       
    
    SET_OPT(type, "type")
    SET_OPT(title, "title")
    SET_OPT(filename, "filename")
    SET_OPT(extension, "extension")
    SET_OPT(buttonLabel, "buttonLabel")
    SET_OPT(directory, "directory")
    SET_OPT(defaultDirectory, "defaultDirectory")
    SET_OPT(filenameLabel, "filenameLabel")
    SET_OPT(buttonLabel, "buttonLabel")

    #undef SET_OPT

    key = New("flags").ToLocalChecked();
    if (options->Has(key)) {
        val = options->Get(key);

        if (val->IsObject()) {
            #define SET_FLAG(name, value)                               \
                key = New(name).ToLocalChecked();                       \
                if (val.As<Object>()->Has(key) &&                       \
                    val.As<Object>()->Get(key).As<Boolean>()->Value())  \
                        flags |= (value);                             

            SET_FLAG("overwritePrompt", FOS_OVERWRITEPROMPT)
            SET_FLAG("strictFiletypes", FOS_STRICTFILETYPES)
            SET_FLAG("noChangeDir", FOS_NOCHANGEDIR)
            SET_FLAG("pickFolders", FOS_PICKFOLDERS)
            SET_FLAG("noValidate", FOS_NOVALIDATE)
            SET_FLAG("allowMultiselect", FOS_ALLOWMULTISELECT)
            SET_FLAG("pathMustExist", FOS_PATHMUSTEXIST)
            SET_FLAG("fileMustExist", FOS_FILEMUSTEXIST)
            SET_FLAG("createPrompt", FOS_CREATEPROMPT)
            SET_FLAG("noReadonlyReturn", FOS_NOREADONLYRETURN)
            SET_FLAG("noTestFileCreate", FOS_NOTESTFILECREATE)
            SET_FLAG("hideMRUPlaces", FOS_HIDEMRUPLACES)
            SET_FLAG("hidePinnedPlaces", FOS_HIDEPINNEDPLACES)
            SET_FLAG("noDereferenceLinks", FOS_NODEREFERENCELINKS)
            SET_FLAG("dontAddToRecent", FOS_DONTADDTORECENT)
            SET_FLAG("forceShowHidden", FOS_FORCESHOWHIDDEN)
            SET_FLAG("defaultNoMiniMode", FOS_DEFAULTNOMINIMODE)
            SET_FLAG("forcePreviewPaneOn", FOS_FORCEPREVIEWPANEON)

            #undef SET_FLAG
        }
    }

    key = New("clear").ToLocalChecked();
    if (options->Has(key)) {                                             
        val = options->Get(key);                   

        if (val->IsString()) {                                            
            clear = val.As<Boolean>()->Value();
        }                                                               
    }    
}    

FileDialog::~FileDialog() {
    Release();

    if(SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
        CoUninitialize();
    }
}

void FileDialog::Init() {
    if (!SUCCEEDED(hr)) {
            hr = CoInitializeEx(
            NULL,
            COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE
        );
    }

    if(!SUCCEEDED(hr)) {
        Release();
        CoUninitialize();            
        throw InitFailed();
    }

    hr = CoCreateInstance(
        type == L"save" ? CLSID_FileSaveDialog : CLSID_FileOpenDialog,
        NULL,
        CLSCTX_ALL,
        type == L"save" ? IID_IFileSaveDialog : IID_IFileOpenDialog,
        reinterpret_cast<void **>(&pDialog)
    );

    if(!SUCCEEDED(hr)) {
        Release();
        CoUninitialize();
        throw CreateInstanceFailed();
    };

    #define SET_OPT(name, setter) \
        if ((name).length() > 0) setter((name).c_str());  

    SET_OPT(title, pDialog->SetTitle)
    SET_OPT(filename, pDialog->SetFileName)
    SET_OPT(extension, pDialog->SetDefaultExtension)
    SET_OPT(filenameLabel, pDialog->SetFileNameLabel)
    SET_OPT(buttonLabel, pDialog->SetOkButtonLabel)

    #undef SET_OPT

    if (clear)
        pDialog->ClearClientData();
    
    if (flags) {
        hr = pDialog->SetOptions(flags);
        if (!SUCCEEDED(hr)) {
            Release();
            CoUninitialize();
            throw SetFlagsFailed();
        }
    }

    #define SET_OPT(name, target, setter, ex)   \
        if((name).length() > 0) {               \
            hr = SHCreateItemFromParsingName(   \
                (name).c_str(),                 \
                NULL,                           \
                IID_IShellItem,                 \
                (void**)&(target)               \
            );                                  \
            if (!SUCCEEDED(hr)) {               \
                Release();                      \
                CoUninitialize();               \
                throw ex ();                    \
            };                                  \
        };                                      \
        hr = setter(target);                    \
        if (!SUCCEEDED(hr)) {                   \
            Release();                          \
            CoUninitialize();                   \
            throw ex ();                        \
        };

    SET_OPT(directory, pDirectory, pDialog->SetFolder, SetDirectoryFailed)
    SET_OPT(defaultDirectory, pDefaultDirectory, pDialog->SetDefaultFolder, SetDefaultDirectoryFailed)

    #undef SET_OPT
}

void FileDialog::GetResults() {
    if (SUCCEEDED(hr)) {
        PWSTR pszPath;
        IShellItem *pItem;

        if (type == L"save") {
            hr = pDialog->GetResult(&pItem);

            if (!SUCCEEDED(hr)) {
                Release();
                CoUninitialize();
                throw GetResultsFailed();
            }

            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);

            if (!SUCCEEDED(hr)) {
                pItem->Release();
                Release();
                CoUninitialize();
                throw GetResultsFailed();
            }

            results.push_back(wstring(pszPath));
            
            pItem->Release();
            CoTaskMemFree(pszPath);

            return;
        }

        IShellItemArray *pItems;

        hr = ((IFileOpenDialog*)pDialog)->GetResults(&pItems);

        if (!SUCCEEDED(hr)) {
            Release();
            CoUninitialize();
            throw GetResultsFailed();
        }

        DWORD count, i;

        hr = pItems->GetCount(&count);

        if (!SUCCEEDED(hr)) {
            pItems->Release();
            Release();
            CoUninitialize();
            throw GetResultsFailed();
        }

        for (i = 0; i < count; i++) {
            hr = pItems->GetItemAt(i, &pItem);

            if (!SUCCEEDED(hr)) {
                pItems->Release();
                Release();
                CoUninitialize();
                throw GetResultsFailed();
            }

            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);

            if (!SUCCEEDED(hr)) {
                pItem->Release();
                pItems->Release();
                Release();
                CoUninitialize();
                throw GetResultsFailed();
            }

            results.push_back(wstring(pszPath));
            CoTaskMemFree(pszPath);

            pItem->Release();
        }
    }
}

Local<Array> FileDialog::Results() {
    if (!SUCCEEDED(hr)) {
        return New<Array>();
    }

    unsigned int i;
    size_t length = results.size();

    Local<Array> _results = New<Array>((unsigned int)length);

    for (i = 0; i < length; i++) {
        _results->Set(New(i), New<String>((uint16_t*)results.at(i).c_str()).ToLocalChecked());
    }

    return _results;
}

void FileDialog::Show() {
    if (SUCCEEDED(hr)) {
        hr = pDialog->Show(NULL);

        if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            return;

        if (!SUCCEEDED(hr)) {
            Release();
            CoUninitialize();
            throw ShowFailed();
        }
    }
}