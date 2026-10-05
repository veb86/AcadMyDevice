// MyDeviceBlock.cpp — поиск блока и чтение параметра видимости Dynamic Block.
#include "StdAfx.h"
#include "MyDeviceBlock.h"

#include <algorithm>
#include <memory>

namespace
{
    // Группы DXF, по которым читается параметр видимости.
    const short kDxfEntityType = 0;        // тип объекта
    const short kDxfDictionaryKey = 3;     // ключ записи словаря
    const short kDxfSubclass = 100;        // маркер подкласса
    const short kDxfParameterName = 301;   // имя параметра видимости
    const short kDxfStateName = 303;       // имя состояния видимости
    const short kDxfControlled = 331;      // примитив, которым управляет параметр
    const short kDxfVisibleInState = 332;  // примитив, видимый в текущем состоянии
    const short kDxfSoftOwner = 350;       // объект словаря (мягкое владение)
    const short kDxfHardOwner = 360;       // объект словаря (жёсткое владение) / узел графа

    const ACHAR kEnhancedBlockKey[] = _T("ACAD_ENHANCEDBLOCK");
    const ACHAR kVisibilityParameterType[] = _T("BLOCKVISIBILITYPARAMETER");
    const ACHAR kVisibilitySubclass[] = _T("AcDbBlockVisibilityParameter");

    // Список групп acdbEntGet(), освобождаемый acutRelRb().
    struct ReleaseResbuf
    {
        void operator()(resbuf* pList) const { acutRelRb(pList); }
    };
    typedef std::unique_ptr<resbuf, ReleaseResbuf> ResbufList;

    ResbufList entGet(const AcDbObjectId& id)
    {
        ads_name name;
        if (id.isNull() || acdbGetAdsName(name, id) != Acad::eOk)
            return ResbufList();
        return ResbufList(acdbEntGet(name));
    }

    bool isText(const resbuf* pRb, short code, const ACHAR* value)
    {
        return pRb->restype == code && pRb->resval.rstring != nullptr
            && AcString(pRb->resval.rstring) == value;
    }

    AcDbObjectId objectIdOf(const resbuf* pRb)
    {
        // В MSVC x64 rlname и ads_name — оба int64_t[2]; поэлементное копирование
        // не зависит от того, как заголовки SDK выбирают тип ads_name.
        ads_name name;
        name[0] = pRb->resval.rlname[0];
        name[1] = pRb->resval.rlname[1];
        AcDbObjectId id;
        if (acdbGetObjectId(id, name) != Acad::eOk)
            return AcDbObjectId();
        return id;
    }

    // Граф вычислений динамического блока: запись ACAD_ENHANCEDBLOCK словаря расширений.
    AcDbObjectId findEvaluationGraph(const AcDbObjectId& dictionaryId)
    {
        ResbufList dictionary = entGet(dictionaryId);
        for (const resbuf* pRb = dictionary.get(); pRb != nullptr; pRb = pRb->rbnext)
        {
            if (!isText(pRb, kDxfDictionaryKey, kEnhancedBlockKey))
                continue;
            const resbuf* pValue = pRb->rbnext;
            if (pValue != nullptr
                && (pValue->restype == kDxfHardOwner || pValue->restype == kDxfSoftOwner))
            {
                return objectIdOf(pValue);
            }
            break;
        }
        return AcDbObjectId();
    }

    // Читает данные узла BLOCKVISIBILITYPARAMETER. Группы 301 встречаются и в данных
    // базового подкласса (AcDbBlock1PtParameter), поэтому разбор начинается после
    // маркера AcDbBlockVisibilityParameter (если маркера нет — с начала списка).
    void readVisibilityParameter(const resbuf* pList, MyDeviceBlock::Info& info)
    {
        const resbuf* pStart = pList;
        for (const resbuf* pRb = pList; pRb != nullptr; pRb = pRb->rbnext)
        {
            if (isText(pRb, kDxfSubclass, kVisibilitySubclass))
            {
                pStart = pRb->rbnext;
                break;
            }
        }

        bool hasName = false;
        for (const resbuf* pRb = pStart; pRb != nullptr; pRb = pRb->rbnext)
        {
            switch (pRb->restype)
            {
            case kDxfParameterName:
                if (!hasName && pRb->resval.rstring != nullptr)
                {
                    info.parameterName = pRb->resval.rstring;
                    hasName = true;
                }
                break;
            case kDxfControlled:
                // Список управляемых примитивов идёт до первого состояния.
                if (info.states.empty())
                    info.controlled.push_back(objectIdOf(pRb));
                break;
            case kDxfStateName:
            {
                MyDeviceBlock::State state;
                if (pRb->resval.rstring != nullptr)
                    state.name = pRb->resval.rstring;
                info.states.push_back(state);
                break;
            }
            case kDxfVisibleInState:
                if (!info.states.empty())
                    info.states.back().visible.push_back(objectIdOf(pRb));
                break;
            default:
                break;
            }
        }
        info.hasVisibility = !info.states.empty();
    }

    // Является ли объект узлом BLOCKVISIBILITYPARAMETER (группа 0 — тип объекта;
    // перед ней acdbEntGet() ставит группу -1 с именем объекта).
    bool isVisibilityParameter(const resbuf* pList)
    {
        for (const resbuf* pRb = pList; pRb != nullptr; pRb = pRb->rbnext)
            if (pRb->restype == kDxfEntityType)
                return isText(pRb, kDxfEntityType, kVisibilityParameterType);
        return false;
    }

    // Ищет параметр видимости среди узлов графа вычислений динамического блока.
    void readVisibility(const AcDbObjectId& dictionaryId, MyDeviceBlock::Info& info)
    {
        ResbufList graph = entGet(findEvaluationGraph(dictionaryId));
        for (const resbuf* pRb = graph.get(); pRb != nullptr; pRb = pRb->rbnext)
        {
            if (pRb->restype != kDxfHardOwner)
                continue;
            ResbufList node = entGet(objectIdOf(pRb));
            if (isVisibilityParameter(node.get()))
            {
                readVisibilityParameter(node.get(), info);
                return;
            }
        }
    }

    // Блоки, которые MyDevice не показывает: пространства модели и листов
    // (они содержат сам MyDevice), анонимные блоки и внешние ссылки.
    bool isSelectable(const AcDbBlockTableRecord* pBlock)
    {
        return !pBlock->isLayout() && !pBlock->isAnonymous() && !pBlock->isFromExternalReference();
    }

    // Определения блоков, которые сейчас обходит forEachVisibleEntity().
    std::vector<AcDbObjectId>& blocksInProgress()
    {
        static std::vector<AcDbObjectId> blocks;
        return blocks;
    }

    class BlockGuard
    {
    public:
        explicit BlockGuard(const AcDbObjectId& blockId)
            : m_entered(std::find(blocksInProgress().begin(), blocksInProgress().end(), blockId)
                        == blocksInProgress().end())
        {
            if (m_entered)
                blocksInProgress().push_back(blockId);
        }
        ~BlockGuard()
        {
            if (m_entered)
                blocksInProgress().pop_back();
        }
        bool entered() const { return m_entered; }

    private:
        BlockGuard(const BlockGuard&) = delete;
        BlockGuard& operator=(const BlockGuard&) = delete;

        bool m_entered;
    };
}

int MyDeviceBlock::Info::findState(const AcString& name) const
{
    for (size_t i = 0; i < states.size(); ++i)
        if (states[i].name == name)
            return static_cast<int>(i);
    return -1;
}

int MyDeviceBlock::Info::effectiveState(const AcString& stored) const
{
    if (!hasVisibility)
        return -1;
    const int index = findState(stored);
    return index >= 0 ? index : 0;
}

bool MyDeviceBlock::Info::isVisible(const AcDbObjectId& id, int stateIndex) const
{
    if (!hasVisibility || stateIndex < 0 || stateIndex >= static_cast<int>(states.size()))
        return true;
    if (std::find(controlled.begin(), controlled.end(), id) == controlled.end())
        return true;
    const std::vector<AcDbObjectId>& visible = states[static_cast<size_t>(stateIndex)].visible;
    return std::find(visible.begin(), visible.end(), id) != visible.end();
}

Acad::ErrorStatus MyDeviceBlock::find(AcDbDatabase* pDb, const AcString& name, Info& info)
{
    info = Info();
    if (name.isEmpty())
        return Acad::eOk;
    info.kind = kNotFound;
    if (pDb == nullptr)
        return Acad::eNullObjectPointer;

    AcDbBlockTable* pTable = nullptr;
    Acad::ErrorStatus es = pDb->getBlockTable(pTable, AcDb::kForRead);
    if (es != Acad::eOk)
        return es;
    AcDbBlockTableRecord* pBlock = nullptr;
    es = pTable->getAt(name.kACharPtr(), pBlock, AcDb::kForRead);
    pTable->close();
    if (es != Acad::eOk)
        return Acad::eKeyNotFound;

    const bool selectable = isSelectable(pBlock);
    AcDbObjectId dictionaryId;
    if (selectable)
    {
        pBlock->getName(info.name);
        info.blockId = pBlock->objectId();
        info.origin = pBlock->origin();
        dictionaryId = pBlock->extensionDictionary();
    }
    pBlock->close();
    if (!selectable)
        return Acad::eKeyNotFound;

    info.kind = kOrdinary;
    if (AcDbDynBlockReference::isDynamicBlock(info.blockId))
    {
        info.kind = kDynamic;
        readVisibility(dictionaryId, info);
    }
    return Acad::eOk;
}

Acad::ErrorStatus MyDeviceBlock::forEachVisibleEntity(AcDbDatabase* pDb, const Info& info,
                                                      int stateIndex,
                                                      const std::function<void(AcDbEntity*)>& fn)
{
    if (info.kind != kOrdinary && info.kind != kDynamic)
        return Acad::eKeyNotFound;
    if (pDb == nullptr)
        return Acad::eNullObjectPointer;
    BlockGuard guard(info.blockId);
    if (!guard.entered())
        return Acad::eOk;

    AcDbBlockTable* pTable = nullptr;
    Acad::ErrorStatus es = pDb->getBlockTable(pTable, AcDb::kForRead);
    if (es != Acad::eOk)
        return es;
    AcDbBlockTableRecord* pBlock = nullptr;
    es = pTable->getAt(info.name.kACharPtr(), pBlock, AcDb::kForRead);
    pTable->close();
    if (es != Acad::eOk)
        return es;
    AcDbBlockTableRecordIterator* pIter = nullptr;
    es = pBlock->newIterator(pIter);
    if (es != Acad::eOk)
    {
        pBlock->close();
        return es;
    }

    for (pIter->start(); !pIter->done(); pIter->step())
    {
        AcDbEntity* pEntity = nullptr;
        if (pIter->getEntity(pEntity, AcDb::kForRead) != Acad::eOk)
            continue;
        if (AcDbAttributeDefinition::cast(pEntity) == nullptr
            && info.isVisible(pEntity->objectId(), stateIndex))
        {
            fn(pEntity);
        }
        pEntity->close();
    }
    delete pIter;
    pBlock->close();
    return Acad::eOk;
}

Acad::ErrorStatus MyDeviceBlock::names(AcDbDatabase* pDb, std::vector<AcString>& names)
{
    names.clear();
    if (pDb == nullptr)
        return Acad::eNullObjectPointer;

    AcDbBlockTable* pTable = nullptr;
    Acad::ErrorStatus es = pDb->getBlockTable(pTable, AcDb::kForRead);
    if (es != Acad::eOk)
        return es;
    AcDbBlockTableIterator* pIter = nullptr;
    es = pTable->newIterator(pIter);
    pTable->close();
    if (es != Acad::eOk)
        return es;

    for (pIter->start(); !pIter->done(); pIter->step())
    {
        AcDbBlockTableRecord* pRecord = nullptr;
        if (pIter->getRecord(pRecord, AcDb::kForRead) != Acad::eOk)
            continue;
        AcString name;
        if (isSelectable(pRecord) && pRecord->getName(name) == Acad::eOk && !name.isEmpty())
            names.push_back(name);
        pRecord->close();
    }
    delete pIter;
    return Acad::eOk;
}
