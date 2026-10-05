// MyDeviceProperties.cpp — чтение и запись свойств MyDevice.
#include "StdAfx.h"
#include "MyDeviceProperties.h"

namespace
{
    const ACHAR kCategoryText1[] = _T("Text 1");
    const ACHAR kCategoryText2[] = _T("Text 2");

    const MyDeviceProperties::Descriptor kDescriptors[] = {
        { MyDevice::kText1, MyDeviceProperties::kText, kCategoryText1, _T("Text"),
          _T("Содержимое Text1") },
        { MyDevice::kText1, MyDeviceProperties::kHeight, kCategoryText1, _T("Height"),
          _T("Высота Text1 в единицах MyDevice") },
        { MyDevice::kText1, MyDeviceProperties::kPositionX, kCategoryText1, _T("Position X"),
          _T("Начало базовой линии Text1: X в системе координат MyDevice") },
        { MyDevice::kText1, MyDeviceProperties::kPositionY, kCategoryText1, _T("Position Y"),
          _T("Начало базовой линии Text1: Y в системе координат MyDevice") },
        { MyDevice::kText1, MyDeviceProperties::kLayer, kCategoryText1, _T("Layer"),
          _T("Слой Text1. Отсутствующий слой создаётся; пусто — слой MyDevice") },
        { MyDevice::kText1, MyDeviceProperties::kFont, kCategoryText1, _T("Font"),
          _T("Текстовый стиль (шрифт) Text1") },

        { MyDevice::kText2, MyDeviceProperties::kText, kCategoryText2, _T("Text"),
          _T("Содержимое Text2") },
        { MyDevice::kText2, MyDeviceProperties::kHeight, kCategoryText2, _T("Height"),
          _T("Высота Text2 в единицах MyDevice") },
        { MyDevice::kText2, MyDeviceProperties::kPositionX, kCategoryText2, _T("Position X"),
          _T("Начало базовой линии Text2: X в системе координат MyDevice") },
        { MyDevice::kText2, MyDeviceProperties::kPositionY, kCategoryText2, _T("Position Y"),
          _T("Начало базовой линии Text2: Y в системе координат MyDevice") },
        { MyDevice::kText2, MyDeviceProperties::kLayer, kCategoryText2, _T("Layer"),
          _T("Слой Text2. Отсутствующий слой создаётся; пусто — слой MyDevice") },
        { MyDevice::kText2, MyDeviceProperties::kFont, kCategoryText2, _T("Font"),
          _T("Текстовый стиль (шрифт) Text2") },
    };

    const int kDescriptorCount = static_cast<int>(sizeof(kDescriptors) / sizeof(kDescriptors[0]));

    const ACHAR kCategoryBlock[] = _T("MyDevice");

    const MyDeviceProperties::BlockDescriptor kBlockDescriptors[] = {
        { MyDeviceProperties::kBlockName, kCategoryBlock, _T("BlockName"),
          _T("Блок, отображаемый внутри MyDevice; базовая точка блока — в начале координат MyDevice") },
        { MyDeviceProperties::kVisibility, kCategoryBlock, _T("Visibility"),
          _T("Состояние видимости Dynamic Block") },
    };

    const int kBlockDescriptorCount =
        static_cast<int>(sizeof(kBlockDescriptors) / sizeof(kBlockDescriptors[0]));

    bool isValidTextIndex(int textIndex)
    {
        return textIndex >= 0 && textIndex < MyDevice::kTextCount;
    }

    bool hasTextStyle(AcDbDatabase* pDb, const AcString& name)
    {
        AcDbTextStyleTable* pTable = nullptr;
        if (pDb == nullptr || pDb->getTextStyleTable(pTable, AcDb::kForRead) != Acad::eOk)
            return false;
        const bool found = pTable->has(name.kACharPtr());
        pTable->close();
        return found;
    }
}

int MyDeviceProperties::count()
{
    return kDescriptorCount;
}

const MyDeviceProperties::Descriptor& MyDeviceProperties::at(int index)
{
    if (index < 0 || index >= kDescriptorCount)
        index = 0;
    return kDescriptors[index];
}

int MyDeviceProperties::blockCount()
{
    return kBlockDescriptorCount;
}

const MyDeviceProperties::BlockDescriptor& MyDeviceProperties::blockAt(int index)
{
    if (index < 0 || index >= kBlockDescriptorCount)
        index = 0;
    return kBlockDescriptors[index];
}

AcString MyDeviceProperties::blockName(const MyDevice& device)
{
    return device.blockName();
}

Acad::ErrorStatus MyDeviceProperties::setBlockName(MyDevice& device, const AcString& name)
{
    return device.setBlockName(name);
}

bool MyDeviceProperties::hasVisibility(const MyDevice& device)
{
    MyDeviceBlock::Info info;
    device.findBlock(info);
    return info.hasVisibility;
}

AcString MyDeviceProperties::visibility(const MyDevice& device)
{
    // Сохранённое состояние, которого у блока больше нет, показывается как состояние по умолчанию —
    // так же, как блок рисуется.
    MyDeviceBlock::Info info;
    device.findBlock(info);
    if (!info.hasVisibility)
        return AcString();
    return info.states[static_cast<size_t>(info.effectiveState(device.visibility()))].name;
}

Acad::ErrorStatus MyDeviceProperties::setVisibility(MyDevice& device, const AcString& state)
{
    return device.setVisibility(state);
}

Acad::ErrorStatus MyDeviceProperties::visibilityStates(const MyDevice& device,
                                                       std::vector<AcString>& states)
{
    states.clear();
    MyDeviceBlock::Info info;
    device.findBlock(info);
    for (const MyDeviceBlock::State& state : info.states)
        states.push_back(state.name);
    return Acad::eOk;
}

Acad::ErrorStatus MyDeviceProperties::blockNames(AcDbDatabase* pDb, std::vector<AcString>& names)
{
    return MyDeviceBlock::names(pDb, names);
}

bool MyDeviceProperties::isNumeric(Field field)
{
    return field == kHeight || field == kPositionX || field == kPositionY;
}

AcString MyDeviceProperties::getString(const MyDevice& device, int textIndex, Field field)
{
    if (!isValidTextIndex(textIndex))
        return AcString();
    const MyDeviceText text = device.textAt(textIndex);
    switch (field)
    {
    case kText:
        return text.text;
    case kLayer:
        return text.layer;
    case kFont:
        return text.textStyle;
    default:
        return AcString();
    }
}

double MyDeviceProperties::getDouble(const MyDevice& device, int textIndex, Field field)
{
    if (!isValidTextIndex(textIndex))
        return 0.0;
    const MyDeviceText text = device.textAt(textIndex);
    switch (field)
    {
    case kHeight:
        return text.height;
    case kPositionX:
        return text.x;
    case kPositionY:
        return text.y;
    default:
        return 0.0;
    }
}

Acad::ErrorStatus MyDeviceProperties::setString(MyDevice& device, int textIndex, Field field,
                                                const AcString& value)
{
    if (!isValidTextIndex(textIndex))
        return Acad::eInvalidInput;
    MyDeviceText text = device.textAt(textIndex);
    switch (field)
    {
    case kText:
        text.text = value;
        break;
    case kLayer:
    {
        // Слой создаётся до изменения объекта: если имя недопустимо, объект не меняется.
        const Acad::ErrorStatus es = ensureLayer(databaseOf(device), value);
        if (es != Acad::eOk)
            return es;
        text.layer = value;
        break;
    }
    case kFont:
        if (!hasTextStyle(databaseOf(device), value))
            return Acad::eKeyNotFound;
        text.textStyle = value;
        break;
    default:
        return Acad::eInvalidInput;
    }
    return device.setTextAt(textIndex, text);
}

Acad::ErrorStatus MyDeviceProperties::setDouble(MyDevice& device, int textIndex, Field field,
                                                double value)
{
    if (!isValidTextIndex(textIndex))
        return Acad::eInvalidInput;
    MyDeviceText text = device.textAt(textIndex);
    switch (field)
    {
    case kHeight:
        text.height = value;
        break;
    case kPositionX:
        text.x = value;
        break;
    case kPositionY:
        text.y = value;
        break;
    default:
        return Acad::eInvalidInput;
    }
    // setTextAt() отклоняет высоту <= 0 и нечисловые значения.
    return device.setTextAt(textIndex, text);
}

Acad::ErrorStatus MyDeviceProperties::layerNames(AcDbDatabase* pDb, std::vector<AcString>& names)
{
    names.clear();
    if (pDb == nullptr)
        return Acad::eNullObjectPointer;

    AcDbLayerTable* pTable = nullptr;
    Acad::ErrorStatus es = pDb->getLayerTable(pTable, AcDb::kForRead);
    if (es != Acad::eOk)
        return es;
    AcDbLayerTableIterator* pIter = nullptr;
    es = pTable->newIterator(pIter);
    pTable->close();
    if (es != Acad::eOk)
        return es;

    for (pIter->start(); !pIter->done(); pIter->step())
    {
        AcDbLayerTableRecord* pRecord = nullptr;
        if (pIter->getRecord(pRecord, AcDb::kForRead) != Acad::eOk)
            continue;
        AcString name;
        if (pRecord->getName(name) == Acad::eOk && !name.isEmpty())
            names.push_back(name);
        pRecord->close();
    }
    delete pIter;
    return Acad::eOk;
}

Acad::ErrorStatus MyDeviceProperties::textStyleNames(AcDbDatabase* pDb, std::vector<AcString>& names)
{
    names.clear();
    if (pDb == nullptr)
        return Acad::eNullObjectPointer;

    AcDbTextStyleTable* pTable = nullptr;
    Acad::ErrorStatus es = pDb->getTextStyleTable(pTable, AcDb::kForRead);
    if (es != Acad::eOk)
        return es;
    AcDbTextStyleTableIterator* pIter = nullptr;
    es = pTable->newIterator(pIter);
    pTable->close();
    if (es != Acad::eOk)
        return es;

    for (pIter->start(); !pIter->done(); pIter->step())
    {
        AcDbTextStyleTableRecord* pRecord = nullptr;
        if (pIter->getRecord(pRecord, AcDb::kForRead) != Acad::eOk)
            continue;
        // Записи форм (SHAPE) и безымянные служебные стили не являются шрифтами текста.
        AcString name;
        if (!pRecord->isShapeFile() && pRecord->getName(name) == Acad::eOk && !name.isEmpty())
            names.push_back(name);
        pRecord->close();
    }
    delete pIter;
    return Acad::eOk;
}

Acad::ErrorStatus MyDeviceProperties::ensureLayer(AcDbDatabase* pDb, const AcString& name)
{
    if (name.isEmpty())
        return Acad::eOk;
    if (pDb == nullptr)
        return Acad::eNullObjectPointer;
    if (acdbSymUtil()->validateSymbolName(name.kACharPtr(), false) != Acad::eOk)
        return Acad::eInvalidInput;

    AcDbLayerTable* pTable = nullptr;
    Acad::ErrorStatus es = pDb->getLayerTable(pTable, AcDb::kForRead);
    if (es != Acad::eOk)
        return es;
    const bool exists = pTable->has(name.kACharPtr());
    pTable->close();
    if (exists)
        return Acad::eOk;

    // Новый слой получает свойства по умолчанию (цвет 7, тип линий Continuous).
    AcDbLayerTableRecord* pRecord = new AcDbLayerTableRecord();
    es = pRecord->setName(name.kACharPtr());
    if (es != Acad::eOk)
    {
        delete pRecord;
        return Acad::eInvalidInput;
    }
    if ((es = pDb->getLayerTable(pTable, AcDb::kForWrite)) != Acad::eOk)
    {
        delete pRecord;
        return es;
    }
    es = pTable->add(pRecord);
    pTable->close();
    if (es != Acad::eOk)
    {
        delete pRecord;
        // Слой мог появиться между проверкой и добавлением — это не ошибка.
        return es == Acad::eDuplicateRecordName ? Acad::eOk : es;
    }
    pRecord->close();
    return Acad::eOk;
}

AcDbDatabase* MyDeviceProperties::databaseOf(const MyDevice& device)
{
    AcDbDatabase* pDb = device.database();
    return pDb != nullptr ? pDb : acdbHostApplicationServices()->workingDatabase();
}
