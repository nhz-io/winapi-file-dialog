#include "windows.h"

#include <v8.h>
#include <nan.h>

#include "FileDialog.h"

using namespace std;

using v8::Function;
using v8::FunctionTemplate;
using v8::Local;
using v8::Number;
using v8::Value;
using v8::Array;
using v8::Object;
using v8::String;
using v8::External;

using Nan::AsyncQueueWorker;
using Nan::AsyncWorker;
using Nan::ObjectWrap;
using Nan::Callback;
using Nan::HandleScope;
using Nan::ErrnoException;
using Nan::ThrowError;
using Nan::ThrowTypeError;
using Nan::Null;
using Nan::Persistent;
using Nan::GetFunction;
using Nan::Set;
using Nan::New;
using Nan::SetPrototypeMethod;
using Nan::FunctionCallbackInfo;

class FileDialogWorker : public AsyncWorker, public ObjectWrap {
private:
	FileDialog* dialog;

public:
    FileDialogWorker(FileDialog* dialog, Callback *callback) :
        AsyncWorker(callback), dialog(dialog) {};

    ~FileDialogWorker() {
		delete dialog;
	};

	void Execute() {
		try {
			dialog->Init();	
		}
		catch(FileDialog::InitFailed) {
			SetErrorMessage("Init failed");
			return;
		}
		catch(FileDialog::CreateInstanceFailed) {
			SetErrorMessage("Failed to create dialog instance");
			return;
		}
		catch(FileDialog::SetFlagsFailed) {
			SetErrorMessage("Failed to set flags");
			return;
		}
		catch(FileDialog::SetDirectoryFailed) {
			SetErrorMessage("Failed to set directory");
			return;
		}
		catch(FileDialog::SetDefaultDirectoryFailed) {
			SetErrorMessage("Failed to set default diretory");
			return;
		}
		catch(FileDialog::SetClearFailed) {
			SetErrorMessage("Failed to clear data");
			return;
		}
		catch(...) {
			SetErrorMessage("Init failed with unknown exception");
		}

		try {
			dialog->Show();
		}
		catch(FileDialog::ShowFailed) {
			SetErrorMessage("Failed to open dialog window");
			return;
		}
		catch(...) {
			SetErrorMessage("Show failed with unknown exception");
		}

		try {
			dialog->GetResults();
		}
		catch(FileDialog::GetResultsFailed) {
			SetErrorMessage("Failed to get results");
			return;
		}
		catch(...) {
			SetErrorMessage("GetResults failed with unknown exception");
		}
    }

    void HandleOKCallback() {
        Local<Value> argv[] = {Null(), dialog->Results()};
        callback->Call(2, argv);
    }

	void HandleErrorCallback() {
		Local<Value> argv[] = {New(ErrorMessage()).ToLocalChecked()};
		callback->Call(1, argv);
	}
};

NAN_METHOD(OpenFileDialogAsync) {
	if (info[0]->IsFunction()) {
		AsyncQueueWorker(new FileDialogWorker(
			new FileDialog(New<Object>()), 
			new Callback(info[0].As<Function>())
		));

		return;
	}

    if (info[1]->IsFunction()) {
        AsyncQueueWorker(new FileDialogWorker(
            new FileDialog(info[0].As<Object>()),
            new Callback(info[1].As<Function>())
        ));		

		return;
    }

	ThrowTypeError("Expected callback");
}

NAN_MODULE_INIT(InitAll) {
    Set(
		target,
		New("openFileDialogAsync").ToLocalChecked(),
		GetFunction(New<FunctionTemplate>(OpenFileDialogAsync)).ToLocalChecked()
	);
}

NODE_MODULE(MODULE_NAME, InitAll);


