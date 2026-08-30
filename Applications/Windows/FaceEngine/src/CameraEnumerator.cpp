#include "CameraEnumerator.h"

#include <windows.h>
#include <dshow.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace fe
{
  namespace
  {
    /// @brief Reads one string property of a device, empty when the device does not carry it
    std::wstring ReadProperty(IPropertyBag* iBag, const wchar_t* iName)
    {
      if (!iBag) return {};

      VARIANT value;
      VariantInit(&value);

      std::wstring result;
      if (SUCCEEDED(iBag->Read(iName, &value, nullptr)) && value.vt == VT_BSTR && value.bstrVal)
      {
        result.assign(value.bstrVal, SysStringLen(value.bstrVal));
      }

      VariantClear(&value);
      return result;
    }
  }

  std::vector<CameraEnumerator::Device> CameraEnumerator::Enumerate()
  {
    std::vector<Device> devices;

    // The engine thread that calls this may or may not have initialised COM already. S_FALSE
    // says it was, and still counts as a reference this function has to give back; only a
    // hard failure - an apartment this thread cannot enter - leaves nothing to balance.
    const HRESULT comInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool ownsCom = SUCCEEDED(comInit);

    ComPtr<ICreateDevEnum> devEnum;
    if (SUCCEEDED(CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&devEnum))))
    {
      ComPtr<IEnumMoniker> monikers;

      // S_FALSE means the category exists but is empty, which is not an error
      if (devEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &monikers, 0) == S_OK)
      {
        ComPtr<IMoniker> moniker;
        int index = 0;

        while (monikers->Next(1, moniker.ReleaseAndGetAddressOf(), nullptr) == S_OK)
        {
          ComPtr<IPropertyBag> bag;
          if (SUCCEEDED(moniker->BindToStorage(nullptr, nullptr, IID_PPV_ARGS(&bag))))
          {
            Device device;
            device.index = index;
            device.name = ReadProperty(bag.Get(), L"FriendlyName");
            device.path = ReadProperty(bag.Get(), L"DevicePath");

            if (device.name.empty())
            {
              device.name = L"Camera " + std::to_wstring(index);
            }

            devices.emplace_back(std::move(device));
          }

          ++index;
        }
      }
    }

    if (ownsCom) CoUninitialize();

    return devices;
  }
}
