// MyDeviceOPM.cpp — динамические свойства MyDevice для палитры свойств AutoCAD.
//
// Каждое из 14 свойств — отдельный COM-объект, реализующий IDynamicProperty
// (имя, тип, чтение и запись значения) и ICategorizeProperties (категория
// «MyDevice», «Text 1» или «Text 2»). Свойства блока регистрируются первыми,
// поэтому категория «MyDevice» стоит над текстами.
//
// Раскрывающиеся списки (IDynamicEnumProperty):
//  * BlockName — блоки чертежа (без листов, анонимных блоков и внешних ссылок);
//  * Visibility — состояния видимости блока; свойство недоступно, если у блока
//    нет параметра видимости;
//  * Font — текстовые стили чертежа.
// Layer — строка: можно ввести имя любого слоя, отсутствующий слой создаётся.
//
// После записи значения объект закрывается, AutoCAD перерисовывает его,
// а IDynamicPropertyNotify::OnChanged() обновляет палитру.
#include "StdAfx.h"
#include "MyDeviceOPM.h"
#include "MyDevice.h"
#include "MyDeviceProperties.h"

#include <ole2.h>

#include "acdocman.h"
#include "dbobjptr.h"
#include "dynprops.h"
#include "category.h"

#include <vector>

namespace
{
    // Категории палитры: положительные номера принадлежат приложению.
    const PROPCAT kBlockCategory = 1000;  // «MyDevice»
    const PROPCAT kFirstCategory = 1001;  // «Text 1»; «Text 2» — kFirstCategory + 1

    // Номера GUID свойств блока: после свойств текстов (0..11), с запасом.
    const int kFirstBlockGuid = 0x10;

    // GUID свойства index: базовый GUID, последний байт которого равен номеру свойства.
    GUID propertyGuid(int index)
    {
        // {0A80016C-9B74-4E72-9854-5AF3DD408A00}
        GUID guid = { 0x0a80016c, 0x9b74, 0x4e72,
                      { 0x98, 0x54, 0x5a, 0xf3, 0xdd, 0x40, 0x8a, 0x00 } };
        guid.Data4[7] = static_cast<unsigned char>(index);
        return guid;
    }

    HRESULT allocString(const ACHAR* value, BSTR* pResult)
    {
        if (pResult == nullptr)
            return E_POINTER;
        *pResult = ::SysAllocString(value != nullptr ? value : L"");
        return *pResult != nullptr ? S_OK : E_OUTOFMEMORY;
    }

    AcDbObjectId objectIdOf(LONG_PTR objectID)
    {
        AcDbObjectId id;
        id.setFromOldId(static_cast<Adesk::IntDbId>(objectID));
        return id;
    }

    // Свойство текста (Text 1/Text 2) или свойство блока (категория MyDevice).
    class MyDeviceDynamicProperty : public IDynamicProperty,
                                    public IDynamicEnumProperty,
                                    public ICategorizeProperties
    {
    public:
        // Свойство текста: index — номер в MyDeviceProperties::at().
        explicit MyDeviceDynamicProperty(int index)
            : m_refCount(1), m_guidIndex(index), m_pText(&MyDeviceProperties::at(index)),
              m_pBlock(nullptr), m_pNotify(nullptr)
        {
        }

        // Свойство блока: field — MyDeviceProperties::kBlockName или kVisibility.
        explicit MyDeviceDynamicProperty(MyDeviceProperties::BlockField field)
            : m_refCount(1), m_guidIndex(kFirstBlockGuid + field), m_pText(nullptr),
              m_pBlock(&MyDeviceProperties::blockAt(field)), m_pNotify(nullptr)
        {
        }

        virtual ~MyDeviceDynamicProperty()
        {
            if (m_pNotify != nullptr)
                m_pNotify->Release();
        }

        // *** IUnknown ***
        STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override
        {
            if (ppvObject == nullptr)
                return E_POINTER;
            *ppvObject = nullptr;
            if (riid == __uuidof(IUnknown) || riid == __uuidof(IDynamicProperty))
                *ppvObject = static_cast<IDynamicProperty*>(this);
            else if (riid == __uuidof(ICategorizeProperties))
                *ppvObject = static_cast<ICategorizeProperties*>(this);
            else if (riid == __uuidof(IDynamicEnumProperty) && isEnum())
                *ppvObject = static_cast<IDynamicEnumProperty*>(this);
            else
                return E_NOINTERFACE;
            AddRef();
            return S_OK;
        }

        STDMETHODIMP_(ULONG) AddRef() override
        {
            return static_cast<ULONG>(::InterlockedIncrement(&m_refCount));
        }

        STDMETHODIMP_(ULONG) Release() override
        {
            const LONG count = ::InterlockedDecrement(&m_refCount);
            if (count == 0)
                delete this;
            return static_cast<ULONG>(count);
        }

        // *** IDynamicProperty ***
        STDMETHODIMP GetGUID(GUID* propGUID) override
        {
            if (propGUID == nullptr)
                return E_POINTER;
            *propGUID = propertyGuid(m_guidIndex);
            return S_OK;
        }

        STDMETHODIMP GetDisplayName(BSTR* bstrName) override
        {
            return allocString(name(), bstrName);
        }

        STDMETHODIMP IsPropertyEnabled(LONG_PTR objectID, BOOL* pbEnabled) override
        {
            if (pbEnabled == nullptr)
                return E_POINTER;
            *pbEnabled = TRUE;
            if (isBlockField(MyDeviceProperties::kVisibility))
            {
                // Visibility есть только у Dynamic Block с параметром видимости.
                m_lastObjectId = objectIdOf(objectID);
                AcDbObjectPointer<MyDevice> pDevice(m_lastObjectId, AcDb::kForRead);
                *pbEnabled = pDevice.openStatus() == Acad::eOk
                    && MyDeviceProperties::hasVisibility(*pDevice) ? TRUE : FALSE;
            }
            return S_OK;
        }

        STDMETHODIMP IsPropertyReadOnly(BOOL* pbReadonly) override
        {
            if (pbReadonly == nullptr)
                return E_POINTER;
            *pbReadonly = FALSE;
            return S_OK;
        }

        STDMETHODIMP GetDescription(BSTR* bstrName) override
        {
            return allocString(m_pText != nullptr ? m_pText->description : m_pBlock->description,
                               bstrName);
        }

        STDMETHODIMP GetCurrentValueName(BSTR* /*pbstrName*/) override
        {
            return E_NOTIMPL;
        }

        STDMETHODIMP GetCurrentValueType(VARTYPE* pVarType) override
        {
            if (pVarType == nullptr)
                return E_POINTER;
            *pVarType = isNumeric() ? VT_R8 : VT_BSTR;
            return S_OK;
        }

        STDMETHODIMP GetCurrentValueData(LONG_PTR objectID, VARIANT* pvarData) override
        {
            if (pvarData == nullptr)
                return E_POINTER;
            // Список состояний Visibility строится для последнего запрошенного объекта.
            m_lastObjectId = objectIdOf(objectID);
            AcDbObjectPointer<MyDevice> pDevice(m_lastObjectId, AcDb::kForRead);
            if (pDevice.openStatus() != Acad::eOk)
                return E_FAIL;

            ::VariantInit(pvarData);
            if (isNumeric())
            {
                V_VT(pvarData) = VT_R8;
                V_R8(pvarData) = MyDeviceProperties::getDouble(*pDevice, m_pText->textIndex,
                                                                m_pText->field);
                return S_OK;
            }
            AcString value;
            if (isBlockField(MyDeviceProperties::kBlockName))
                value = MyDeviceProperties::blockName(*pDevice);
            else if (isBlockField(MyDeviceProperties::kVisibility))
                value = MyDeviceProperties::visibility(*pDevice);
            else
                value = MyDeviceProperties::getString(*pDevice, m_pText->textIndex, m_pText->field);
            V_VT(pvarData) = VT_BSTR;
            return allocString(value.kACharPtr(), &V_BSTR(pvarData));
        }

        STDMETHODIMP SetCurrentValueData(LONG_PTR objectID, const VARIANT varData) override
        {
            const AcDbObjectId id = objectIdOf(objectID);
            // Палитра может вызвать запись вне команды: документ блокируется на время записи.
            AcApDocument* pDoc = id.database() != nullptr ? acDocManager->document(id.database())
                                                          : nullptr;
            const bool locked = pDoc != nullptr
                && acDocManager->lockDocument(pDoc, AcAp::kWrite) == Acad::eOk;
            const HRESULT hr = setValue(id, varData);
            if (locked)
                acDocManager->unlockDocument(pDoc);

            if (SUCCEEDED(hr) && m_pNotify != nullptr)
                m_pNotify->OnChanged(this);
            return hr;
        }

        STDMETHODIMP Connect(IDynamicPropertyNotify* pSink) override
        {
            if (pSink != nullptr)
                pSink->AddRef();
            if (m_pNotify != nullptr)
                m_pNotify->Release();
            m_pNotify = pSink;
            return S_OK;
        }

        STDMETHODIMP Disconnect() override
        {
            if (m_pNotify != nullptr)
            {
                m_pNotify->Release();
                m_pNotify = nullptr;
            }
            return S_OK;
        }

        // *** IDynamicEnumProperty (BlockName, Visibility, Font) ***
        STDMETHODIMP GetNumPropertyValues(LONG* numValues) override
        {
            if (numValues == nullptr)
                return E_POINTER;
            // Список перечитывается при каждом открытии: блоки, состояния и стили
            // могли добавить или удалить.
            AcDbDatabase* pDb = acdbHostApplicationServices()->workingDatabase();
            if (isBlockField(MyDeviceProperties::kBlockName))
            {
                MyDeviceProperties::blockNames(pDb, m_values);
            }
            else if (isBlockField(MyDeviceProperties::kVisibility))
            {
                m_values.clear();
                AcDbObjectPointer<MyDevice> pDevice(m_lastObjectId, AcDb::kForRead);
                if (pDevice.openStatus() == Acad::eOk)
                    MyDeviceProperties::visibilityStates(*pDevice, m_values);
            }
            else
            {
                MyDeviceProperties::textStyleNames(pDb, m_values);
            }
            *numValues = static_cast<LONG>(m_values.size());
            return S_OK;
        }

        STDMETHODIMP GetPropValueName(LONG index, BSTR* valueName) override
        {
            if (index < 0 || static_cast<size_t>(index) >= m_values.size())
                return E_INVALIDARG;
            return allocString(m_values[index].kACharPtr(), valueName);
        }

        STDMETHODIMP GetPropValueData(LONG index, VARIANT* valueData) override
        {
            if (valueData == nullptr)
                return E_POINTER;
            if (index < 0 || static_cast<size_t>(index) >= m_values.size())
                return E_INVALIDARG;
            ::VariantInit(valueData);
            V_VT(valueData) = VT_BSTR;
            return allocString(m_values[index].kACharPtr(), &V_BSTR(valueData));
        }

        // *** ICategorizeProperties ***
        STDMETHODIMP MapPropertyToCategory(DISPID /*dispid*/, PROPCAT* ppropcat) override
        {
            if (ppropcat == nullptr)
                return E_POINTER;
            *ppropcat = m_pText != nullptr ? kFirstCategory + m_pText->textIndex : kBlockCategory;
            return S_OK;
        }

        STDMETHODIMP GetCategoryName(PROPCAT propcat, LCID /*lcid*/, BSTR* pbstrName) override
        {
            if (propcat == kBlockCategory)
                return allocString(MyDeviceProperties::blockAt(MyDeviceProperties::kBlockName)
                                       .category, pbstrName);
            const int textIndex = propcat - kFirstCategory;
            if (textIndex < 0 || textIndex >= MyDevice::kTextCount)
                return E_INVALIDARG;
            return allocString(MyDeviceProperties::at(textIndex * MyDeviceProperties::kFieldCount)
                                   .category, pbstrName);
        }

    private:
        const ACHAR* name() const { return m_pText != nullptr ? m_pText->name : m_pBlock->name; }
        const ACHAR* category() const
        {
            return m_pText != nullptr ? m_pText->category : m_pBlock->category;
        }
        bool isBlockField(MyDeviceProperties::BlockField field) const
        {
            return m_pBlock != nullptr && m_pBlock->field == field;
        }
        bool isNumeric() const
        {
            return m_pText != nullptr && MyDeviceProperties::isNumeric(m_pText->field);
        }
        bool isEnum() const
        {
            return m_pBlock != nullptr || m_pText->field == MyDeviceProperties::kFont;
        }

        Acad::ErrorStatus setString(MyDevice& device, const AcString& value) const
        {
            if (isBlockField(MyDeviceProperties::kBlockName))
                return MyDeviceProperties::setBlockName(device, value);
            if (isBlockField(MyDeviceProperties::kVisibility))
                return MyDeviceProperties::setVisibility(device, value);
            return MyDeviceProperties::setString(device, m_pText->textIndex, m_pText->field, value);
        }

        HRESULT setValue(const AcDbObjectId& id, const VARIANT& varData)
        {
            const bool numeric = isNumeric();
            VARIANT value;
            ::VariantInit(&value);
            if (FAILED(::VariantChangeType(&value, const_cast<VARIANT*>(&varData), 0,
                                           numeric ? VT_R8 : VT_BSTR)))
                return E_INVALIDARG;

            HRESULT hr = E_FAIL;
            AcDbObjectPointer<MyDevice> pDevice(id, AcDb::kForWrite);
            if (pDevice.openStatus() == Acad::eOk)
            {
                const Acad::ErrorStatus es = numeric
                    ? MyDeviceProperties::setDouble(*pDevice, m_pText->textIndex,
                                                    m_pText->field, V_R8(&value))
                    : setString(*pDevice, AcString(V_BSTR(&value) != nullptr
                                                   ? V_BSTR(&value) : L""));
                if (es == Acad::eOk)
                {
                    hr = S_OK;
                }
                else
                {
                    acutPrintf(_T("\nMyDevice: недопустимое значение свойства %s (%s)."),
                               name(), category());
                    hr = E_INVALIDARG;
                }
            }
            ::VariantClear(&value);
            return hr;
        }

        LONG m_refCount;
        const int m_guidIndex;
        const MyDeviceProperties::Descriptor* m_pText;        // свойство текста или nullptr
        const MyDeviceProperties::BlockDescriptor* m_pBlock;  // свойство блока или nullptr
        IDynamicPropertyNotify* m_pNotify;
        AcDbObjectId m_lastObjectId;       // объект, для которого строится список Visibility
        std::vector<AcString> m_values;    // значения раскрывающегося списка
    };

    // Свойства, добавленные в палитру; по одной ссылке на каждое хранит приложение.
    std::vector<MyDeviceDynamicProperty*> g_properties;

    // Менеджер свойств класса MyDevice или nullptr, если палитра недоступна.
    // Возвращённый указатель нужно освободить через Release().
    IPropertyManager* propertyManager()
    {
        OPMPropertyExtensionFactory* pFactory = GET_OPMEXTENSION_CREATE_PROTOCOL();
        if (pFactory == nullptr)
            return nullptr;
        OPMPropertyExtension* pExtension = pFactory->CreateOPMObjectProtocol(MyDevice::desc());
        return pExtension != nullptr ? pExtension->GetPropertyManager() : nullptr;
    }
}

bool registerMyDeviceProperties()
{
    if (!g_properties.empty())
        return true;
    IPropertyManager* pManager = propertyManager();
    if (pManager == nullptr)
        return false;
    // Порядок добавления задаёт порядок в палитре: сначала MyDevice, затем тексты.
    std::vector<MyDeviceDynamicProperty*> properties;
    for (int i = 0; i < MyDeviceProperties::blockCount(); ++i)
        properties.push_back(new MyDeviceDynamicProperty(MyDeviceProperties::blockAt(i).field));
    for (int i = 0; i < MyDeviceProperties::count(); ++i)
        properties.push_back(new MyDeviceDynamicProperty(i));
    for (MyDeviceDynamicProperty* pProperty : properties)
    {
        if (SUCCEEDED(pManager->AddProperty(pProperty)))
            g_properties.push_back(pProperty);
        else
            pProperty->Release();
    }
    pManager->Release();
    return !g_properties.empty();
}

void unregisterMyDeviceProperties()
{
    IPropertyManager* pManager = propertyManager();
    for (MyDeviceDynamicProperty* pProperty : g_properties)
    {
        if (pManager != nullptr)
            pManager->RemoveProperty(pProperty);
        pProperty->Release();
    }
    g_properties.clear();
    if (pManager != nullptr)
        pManager->Release();
}
