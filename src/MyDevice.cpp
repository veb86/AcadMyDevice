// MyDevice.cpp — реализация пользовательского объекта MyDevice.
#include "StdAfx.h"
#include "MyDevice.h"

#include <cmath>

ACRX_DXF_DEFINE_MEMBERS(
    MyDevice, AcDbEntity,
    AcDb::kDHL_CURRENT, AcDb::kMReleaseCurrent,
    AcDbProxyEntity::kNoOperation, MYDEVICE,
    MyDeviceApp
    |Product Desc: MyDevice custom entity
    |Company: veb86
)

const double MyDevice::kWidth = 100.0;
const double MyDevice::kHeight = 50.0;
const double MyDevice::kTextHeight = 10.0;
const double MyDevice::kTextMargin = 5.0;
const double MyDevice::kNotFoundTextHeight = 5.0;
const ACHAR* const MyDevice::kDefaultTextStyle = _T("Standard");

namespace
{
    // Базовые линии текстов в локальной системе координат объекта.
    const double kText1BaselineY = 30.0;
    const double kText2BaselineY = 10.0;

    const double kTolerance = 1.0e-10;

    // Групповые коды DXF подкласса MyDevice.
    const AcDb::DxfCode kDxfVersion = AcDb::kDxfInt16;                                   // 70
    const AcDb::DxfCode kDxfPosition = AcDb::kDxfXCoord;                                 // 10
    const AcDb::DxfCode kDxfXDirection = static_cast<AcDb::DxfCode>(AcDb::kDxfXCoord + 1); // 11
    const AcDb::DxfCode kDxfScale = AcDb::kDxfReal;                                      // 40
    const AcDb::DxfCode kDxfNormal = AcDb::kDxfNormalX;                                  // 210
    const AcDb::DxfCode kDxfText1 = AcDb::kDxfXTextString;                               // 300
    const AcDb::DxfCode kDxfText2 = static_cast<AcDb::DxfCode>(AcDb::kDxfXTextString + 1); // 301

    // Свойства текстов (версия 2), по одному коду на Text1 и Text2.
    AcDb::DxfCode dxfCode(int base, int index)
    {
        return static_cast<AcDb::DxfCode>(base + index);
    }
    const int kDxfTextHeightBase = AcDb::kDxfReal + 1;            // 41, 42
    const int kDxfTextPositionBase = AcDb::kDxfXCoord + 2;        // 12, 13
    const int kDxfTextLayerBase = AcDb::kDxfXTextString + 2;      // 302, 303
    const int kDxfTextStyleBase = AcDb::kDxfXTextString + 4;      // 304, 305

    // Блок (версия 3).
    const AcDb::DxfCode kDxfBlockName = static_cast<AcDb::DxfCode>(AcDb::kDxfXTextString + 6);  // 306
    const AcDb::DxfCode kDxfVisibility = static_cast<AcDb::DxfCode>(AcDb::kDxfXTextString + 7); // 307

    // Надпись на месте отсутствующего блока.
    const ACHAR kBlockNotFound[] = _T("BLOCK NOT FOUND");

    const ACHAR kDxfSubclassName[] = _T("MyDevice");

    AcGePoint3d asPoint(const resbuf& rb)
    {
        return AcGePoint3d(rb.resval.rpoint[0], rb.resval.rpoint[1], rb.resval.rpoint[2]);
    }

    AcGeVector3d asVector(const resbuf& rb)
    {
        return AcGeVector3d(rb.resval.rpoint[0], rb.resval.rpoint[1], rb.resval.rpoint[2]);
    }

    // Приводит пару (ось X, нормаль) к ортонормированному виду.
    // Возвращает false, если векторы вырождены или параллельны.
    bool normalizeOrientation(AcGeVector3d& xDirection, AcGeVector3d& normal)
    {
        const double normalLength = normal.length();
        if (normalLength < kTolerance)
            return false;
        normal *= 1.0 / normalLength;

        // Убираем из оси X составляющую вдоль нормали.
        xDirection -= normal * xDirection.dotProduct(normal);
        const double xLength = xDirection.length();
        if (xLength < kTolerance)
            return false;
        xDirection *= 1.0 / xLength;
        return true;
    }

    // Высота и положение должны быть конечными числами, высота — положительной,
    // стиль — непустым (без стиля AutoCAD не знает, каким шрифтом рисовать).
    bool isValidText(const MyDeviceText& text)
    {
        return text.height > kTolerance && std::isfinite(text.height)
            && std::isfinite(text.x) && std::isfinite(text.y)
            && !text.textStyle.isEmpty();
    }

    // Идентификатор слоя по имени или пустой идентификатор. Ничего не создаёт:
    // отрисовка не должна менять базу данных.
    AcDbObjectId findLayer(AcDbDatabase* pDb, const AcString& name)
    {
        AcDbObjectId id;
        AcDbLayerTable* pTable = nullptr;
        if (pDb == nullptr || name.isEmpty()
            || pDb->getLayerTable(pTable, AcDb::kForRead) != Acad::eOk)
        {
            return id;
        }
        if (pTable->getAt(name.kACharPtr(), id) != Acad::eOk)
            id = AcDbObjectId();
        pTable->close();
        return id;
    }

    // Идентификатор текстового стиля по имени или пустой идентификатор.
    AcDbObjectId findTextStyle(AcDbDatabase* pDb, const AcString& name)
    {
        AcDbObjectId id;
        AcDbTextStyleTable* pTable = nullptr;
        if (pDb == nullptr || name.isEmpty()
            || pDb->getTextStyleTable(pTable, AcDb::kForRead) != Acad::eOk)
        {
            return id;
        }
        if (pTable->getAt(name.kACharPtr(), id) != Acad::eOk)
            id = AcDbObjectId();
        pTable->close();
        return id;
    }
}

MyDevice::MyDevice()
    : m_position(AcGePoint3d::kOrigin)
    , m_xDirection(AcGeVector3d::kXAxis)
    , m_normal(AcGeVector3d::kZAxis)
    , m_scale(1.0)
{
    for (int i = 0; i < kTextCount; ++i)
        m_texts[i] = defaultText(i);
}

MyDevice::MyDevice(const AcGePoint3d& position)
    : MyDevice()
{
    m_position = position;
}

MyDevice::~MyDevice()
{
}

AcGePoint3d MyDevice::position() const
{
    assertReadEnabled();
    return m_position;
}

Acad::ErrorStatus MyDevice::setPosition(const AcGePoint3d& position)
{
    assertWriteEnabled();
    m_position = position;
    return Acad::eOk;
}

AcGeVector3d MyDevice::xDirection() const
{
    assertReadEnabled();
    return m_xDirection;
}

AcGeVector3d MyDevice::normal() const
{
    assertReadEnabled();
    return m_normal;
}

Acad::ErrorStatus MyDevice::setOrientation(const AcGeVector3d& xDirection,
                                           const AcGeVector3d& normal)
{
    AcGeVector3d x = xDirection;
    AcGeVector3d n = normal;
    if (!normalizeOrientation(x, n))
        return Acad::eInvalidInput;

    assertWriteEnabled();
    m_xDirection = x;
    m_normal = n;
    return Acad::eOk;
}

double MyDevice::scale() const
{
    assertReadEnabled();
    return m_scale;
}

Acad::ErrorStatus MyDevice::setScale(double scale)
{
    if (!(scale > kTolerance))
        return Acad::eInvalidInput;

    assertWriteEnabled();
    m_scale = scale;
    return Acad::eOk;
}

AcString MyDevice::text1() const
{
    assertReadEnabled();
    return m_texts[kText1].text;
}

Acad::ErrorStatus MyDevice::setText1(const AcString& text)
{
    assertWriteEnabled();
    m_texts[kText1].text = text;
    return Acad::eOk;
}

AcString MyDevice::text2() const
{
    assertReadEnabled();
    return m_texts[kText2].text;
}

Acad::ErrorStatus MyDevice::setText2(const AcString& text)
{
    assertWriteEnabled();
    m_texts[kText2].text = text;
    return Acad::eOk;
}

MyDeviceText MyDevice::textAt(int index) const
{
    assertReadEnabled();
    if (index < 0 || index >= kTextCount)
        return defaultText(kText1);
    return m_texts[index];
}

Acad::ErrorStatus MyDevice::setTextAt(int index, const MyDeviceText& data)
{
    if (index < 0 || index >= kTextCount || !isValidText(data))
        return Acad::eInvalidInput;

    assertWriteEnabled();
    m_texts[index] = data;
    return Acad::eOk;
}

MyDeviceText MyDevice::defaultText(int index)
{
    // Положение и высота совпадают с отрисовкой этапа 1.
    MyDeviceText text;
    text.text = (index == kText2) ? _T("TEXT2") : _T("TEXT1");
    text.height = kTextHeight;
    text.x = kTextMargin;
    text.y = (index == kText2) ? kText2BaselineY : kText1BaselineY;
    text.textStyle = kDefaultTextStyle;
    return text;
}

AcString MyDevice::blockName() const
{
    assertReadEnabled();
    return m_blockName;
}

Acad::ErrorStatus MyDevice::setBlockName(const AcString& name)
{
    // Новый блок может быть другого типа: тип и состояния видимости определяются заново.
    MyDeviceBlock::Info info;
    MyDeviceBlock::find(databaseOrWorking(), name, info);

    // Новый блок начинается с состояния по умолчанию; тот же блок сохраняет своё состояние.
    AcString visibility = info.hasVisibility ? info.states.front().name : AcString();
    if (name == blockName() && info.findState(m_visibility) >= 0)
        visibility = m_visibility;

    assertWriteEnabled();
    m_blockName = name;
    m_visibility = visibility;
    return Acad::eOk;
}

AcString MyDevice::visibility() const
{
    assertReadEnabled();
    return m_visibility;
}

Acad::ErrorStatus MyDevice::setVisibility(const AcString& state)
{
    MyDeviceBlock::Info info;
    findBlock(info);
    if (!info.hasVisibility)
        return Acad::eNotApplicable;
    if (info.findState(state) < 0)
        return Acad::eInvalidInput;

    assertWriteEnabled();
    m_visibility = state;
    return Acad::eOk;
}

Acad::ErrorStatus MyDevice::findBlock(MyDeviceBlock::Info& info) const
{
    assertReadEnabled();
    return MyDeviceBlock::find(databaseOrWorking(), m_blockName, info);
}

AcDbDatabase* MyDevice::databaseOrWorking() const
{
    AcDbDatabase* pDb = database();
    return pDb != nullptr ? pDb : acdbHostApplicationServices()->workingDatabase();
}

AcGeMatrix3d MyDevice::blockToWorld(const MyDeviceBlock::Info& info) const
{
    return localToWorld() * AcGeMatrix3d::translation(AcGePoint3d::kOrigin - info.origin);
}

AcGeMatrix3d MyDevice::localToWorld() const
{
    assertReadEnabled();
    const AcGeVector3d yDirection = m_normal.crossProduct(m_xDirection);
    AcGeMatrix3d xform;
    xform.setCoordSystem(m_position,
                         m_xDirection * m_scale,
                         yDirection * m_scale,
                         m_normal * m_scale);
    return xform;
}

void MyDevice::getCorners(AcGePoint3d corners[4]) const
{
    const AcGeMatrix3d xform = localToWorld();
    corners[0].set(0.0, 0.0, 0.0);
    corners[1].set(kWidth, 0.0, 0.0);
    corners[2].set(kWidth, kHeight, 0.0);
    corners[3].set(0.0, kHeight, 0.0);
    for (int i = 0; i < 4; ++i)
        corners[i].transformBy(xform);
}

// ---------------------------------------------------------------------------
// DWG
// ---------------------------------------------------------------------------

Acad::ErrorStatus MyDevice::dwgOutFields(AcDbDwgFiler* pFiler) const
{
    assertReadEnabled();

    Acad::ErrorStatus es = AcDbEntity::dwgOutFields(pFiler);
    if (es != Acad::eOk)
        return es;

    // Версия всегда записывается первой.
    pFiler->writeInt16(kCurrentVersion);
    pFiler->writePoint3d(m_position);
    pFiler->writeVector3d(m_xDirection);
    pFiler->writeVector3d(m_normal);
    pFiler->writeDouble(m_scale);
    pFiler->writeString(m_texts[kText1].text);
    pFiler->writeString(m_texts[kText2].text);
    // Версия 2: остальные свойства текстов дописываются после полей версии 1.
    for (int i = 0; i < kTextCount; ++i)
    {
        pFiler->writeDouble(m_texts[i].height);
        pFiler->writeDouble(m_texts[i].x);
        pFiler->writeDouble(m_texts[i].y);
        pFiler->writeString(m_texts[i].layer);
        pFiler->writeString(m_texts[i].textStyle);
    }
    // Версия 3: блок и его состояние видимости.
    pFiler->writeString(m_blockName);
    pFiler->writeString(m_visibility);

    return pFiler->filerStatus();
}

Acad::ErrorStatus MyDevice::dwgInFields(AcDbDwgFiler* pFiler)
{
    assertWriteEnabled();

    Acad::ErrorStatus es = AcDbEntity::dwgInFields(pFiler);
    if (es != Acad::eOk)
        return es;

    Adesk::Int16 version = 0;
    if ((es = pFiler->readInt16(&version)) != Acad::eOk)
        return es;
    // Объект сохранён более новой версией приложения — пусть станет прокси.
    if (version > kCurrentVersion || version < 1)
        return Acad::eMakeMeProxy;

    pFiler->readPoint3d(&m_position);
    pFiler->readVector3d(&m_xDirection);
    pFiler->readVector3d(&m_normal);
    pFiler->readDouble(&m_scale);

    // В версии 1 были только строки — остальное берётся по умолчанию.
    MyDeviceText texts[kTextCount] = { defaultText(kText1), defaultText(kText2) };
    pFiler->readString(texts[kText1].text);
    pFiler->readString(texts[kText2].text);
    if (version >= 2)
    {
        for (int i = 0; i < kTextCount; ++i)
        {
            pFiler->readDouble(&texts[i].height);
            pFiler->readDouble(&texts[i].x);
            pFiler->readDouble(&texts[i].y);
            pFiler->readString(texts[i].layer);
            pFiler->readString(texts[i].textStyle);
        }
    }
    // До версии 3 блока не было. Тип блока и состояния здесь не проверяются:
    // при чтении DWG определение блока может быть ещё не загружено, поэтому
    // они определяются заново при отрисовке и в палитре свойств.
    AcString blockName, visibility;
    if (version >= 3)
    {
        pFiler->readString(blockName);
        pFiler->readString(visibility);
    }
    if ((es = pFiler->filerStatus()) != Acad::eOk)
        return es;

    for (int i = 0; i < kTextCount; ++i)
        m_texts[i] = texts[i];
    m_blockName = blockName;
    m_visibility = visibility;
    return Acad::eOk;
}

// ---------------------------------------------------------------------------
// DXF
// ---------------------------------------------------------------------------

Acad::ErrorStatus MyDevice::dxfOutFields(AcDbDxfFiler* pFiler) const
{
    assertReadEnabled();

    Acad::ErrorStatus es = AcDbEntity::dxfOutFields(pFiler);
    if (es != Acad::eOk)
        return es;

    pFiler->writeItem(AcDb::kDxfSubclass, kDxfSubclassName);
    pFiler->writeInt16(kDxfVersion, kCurrentVersion);
    pFiler->writePoint3d(kDxfPosition, m_position);
    pFiler->writeVector3d(kDxfXDirection, m_xDirection, 16);
    pFiler->writeDouble(kDxfScale, m_scale);
    // Нормаль всегда пишется с максимальной точностью.
    pFiler->writeVector3d(kDxfNormal, m_normal, 16);
    pFiler->writeString(kDxfText1, m_texts[kText1].text);
    pFiler->writeString(kDxfText2, m_texts[kText2].text);
    for (int i = 0; i < kTextCount; ++i)
    {
        const MyDeviceText& text = m_texts[i];
        pFiler->writeDouble(dxfCode(kDxfTextHeightBase, i), text.height);
        pFiler->writePoint3d(dxfCode(kDxfTextPositionBase, i), AcGePoint3d(text.x, text.y, 0.0));
        pFiler->writeString(dxfCode(kDxfTextLayerBase, i), text.layer);
        pFiler->writeString(dxfCode(kDxfTextStyleBase, i), text.textStyle);
    }
    pFiler->writeString(kDxfBlockName, m_blockName);
    pFiler->writeString(kDxfVisibility, m_visibility);

    return pFiler->filerStatus();
}

Acad::ErrorStatus MyDevice::dxfInFields(AcDbDxfFiler* pFiler)
{
    assertWriteEnabled();

    if (AcDbEntity::dxfInFields(pFiler) != Acad::eOk
        || !pFiler->atSubclassData(kDxfSubclassName))
    {
        return pFiler->filerStatus();
    }

    // Значения по умолчанию на случай отсутствия необязательных групп.
    Adesk::Int16 version = kCurrentVersion;
    AcGePoint3d position = AcGePoint3d::kOrigin;
    AcGeVector3d xDirection = AcGeVector3d::kXAxis;
    AcGeVector3d normal = AcGeVector3d::kZAxis;
    double scale = 1.0;
    MyDeviceText texts[kTextCount] = { defaultText(kText1), defaultText(kText2) };
    AcString blockName, visibility;

    Acad::ErrorStatus es = Acad::eOk;
    resbuf rb;
    while (es == Acad::eOk && (es = pFiler->readResBuf(&rb)) == Acad::eOk)
    {
        switch (rb.restype)
        {
        case kDxfVersion:
            version = rb.resval.rint;
            break;
        case kDxfPosition:
            position = asPoint(rb);
            break;
        case kDxfXDirection:
            xDirection = asVector(rb);
            break;
        case kDxfScale:
            scale = rb.resval.rreal;
            break;
        case kDxfNormal:
            normal = asVector(rb);
            break;
        case kDxfText1:
            texts[kText1].text = rb.resval.rstring;
            break;
        case kDxfText2:
            texts[kText2].text = rb.resval.rstring;
            break;
        case kDxfTextHeightBase + kText1:
        case kDxfTextHeightBase + kText2:
            texts[rb.restype - kDxfTextHeightBase].height = rb.resval.rreal;
            break;
        case kDxfTextPositionBase + kText1:
        case kDxfTextPositionBase + kText2:
            texts[rb.restype - kDxfTextPositionBase].x = rb.resval.rpoint[0];
            texts[rb.restype - kDxfTextPositionBase].y = rb.resval.rpoint[1];
            break;
        case kDxfTextLayerBase + kText1:
        case kDxfTextLayerBase + kText2:
            texts[rb.restype - kDxfTextLayerBase].layer = rb.resval.rstring;
            break;
        case kDxfTextStyleBase + kText1:
        case kDxfTextStyleBase + kText2:
            texts[rb.restype - kDxfTextStyleBase].textStyle = rb.resval.rstring;
            break;
        case kDxfBlockName:
            blockName = rb.resval.rstring;
            break;
        case kDxfVisibility:
            visibility = rb.resval.rstring;
            break;
        default:
            // Чужая группа — возвращаем её, чтобы её прочитал следующий подкласс.
            pFiler->pushBackItem();
            es = Acad::eEndOfFile;
            break;
        }
    }

    // Здесь es должен быть eEndOfFile — либо от readResBuf(), либо от pushBackItem().
    if (es != Acad::eEndOfFile)
        return Acad::eInvalidResBuf;

    if (version > kCurrentVersion || version < 1)
        return Acad::eMakeMeProxy;

    if (!normalizeOrientation(xDirection, normal))
    {
        pFiler->setError(Acad::eInvalidDxfCode,
                         _T("\nMyDevice: invalid direction or normal vector."));
        return pFiler->filerStatus();
    }
    if (!(scale > kTolerance))
    {
        pFiler->setError(Acad::eInvalidDxfCode,
                         _T("\nMyDevice: invalid scale %g."), scale);
        return pFiler->filerStatus();
    }
    for (int i = 0; i < kTextCount; ++i)
    {
        if (!isValidText(texts[i]))
        {
            pFiler->setError(Acad::eInvalidDxfCode,
                             _T("\nMyDevice: invalid height, position or text style of Text%d."),
                             i + 1);
            return pFiler->filerStatus();
        }
    }

    m_position = position;
    m_xDirection = xDirection;
    m_normal = normal;
    m_scale = scale;
    for (int i = 0; i < kTextCount; ++i)
        m_texts[i] = texts[i];
    m_blockName = blockName;
    m_visibility = visibility;

    return pFiler->filerStatus();
}

// ---------------------------------------------------------------------------
// Графика и редактирование
// ---------------------------------------------------------------------------

Adesk::Boolean MyDevice::subWorldDraw(AcGiWorldDraw* pWd)
{
    assertReadEnabled();

    if (pWd->regenAbort())
        return Adesk::kTrue;

    // Прямоугольник 100x50: замкнутая полилиния из 5 точек.
    AcGePoint3d outline[5];
    getCorners(outline);
    outline[4] = outline[0];
    pWd->geometry().polyline(5, outline, &m_normal);

    // Блок рисуется поверх прямоугольника, тексты — поверх блока.
    AcDbDatabase* pDb = databaseOrWorking();
    drawBlock(pWd, pDb);

    // Тексты рисуются примитивами AcGi, без создания AcDbText/AcDbMText.
    // Прямоугольник уже нарисован на слое объекта, поэтому слой меняется только здесь.
    for (int i = 0; i < kTextCount; ++i)
        drawText(pWd, pDb, m_texts[i]);

    return Adesk::kTrue;
}

void MyDevice::drawBlock(AcGiWorldDraw* pWd, AcDbDatabase* pDb) const
{
    MyDeviceBlock::Info info;
    MyDeviceBlock::find(pDb, m_blockName, info);
    if (info.kind == MyDeviceBlock::kNoBlock)
        return;
    if (info.kind == MyDeviceBlock::kNotFound)
    {
        // Надпись стоит на месте базовой точки блока, стилем Standard и на слое объекта.
        MyDeviceText message;
        message.text = kBlockNotFound;
        message.height = kNotFoundTextHeight;
        message.x = 0.0;
        message.y = 0.0;
        message.textStyle = kDefaultTextStyle;
        drawText(pWd, pDb, message);
        return;
    }

    // Примитивы определения блока рисуются как есть (без создания вставки блока)
    // в системе координат объекта. Определение блока не меняется.
    const AcDbObjectId deviceLayerId = layerId();
    if (!deviceLayerId.isNull())
        pWd->subEntityTraits().setLayer(deviceLayerId);
    const int stateIndex = info.effectiveState(m_visibility);
    pWd->geometry().pushModelTransform(blockToWorld(info));
    MyDeviceBlock::forEachVisibleEntity(pDb, info, stateIndex, [pWd](AcDbEntity* pEntity) {
        pWd->geometry().draw(pEntity);
    });
    pWd->geometry().popModelTransform();
}

void MyDevice::drawText(AcGiWorldDraw* pWd, AcDbDatabase* pDb, const MyDeviceText& text) const
{
    // Слой текста, если он есть в чертеже, иначе — слой самого MyDevice.
    // Слой назначается каждому тексту: иначе текст унаследовал бы слой предыдущего.
    AcDbObjectId textLayerId = findLayer(pDb, text.layer);
    if (textLayerId.isNull())
        textLayerId = layerId();
    if (!textLayerId.isNull())
        pWd->subEntityTraits().setLayer(textLayerId);

    // Шрифт задаётся стандартным текстовым стилем AutoCAD; если стиль удалён — Standard.
    AcGiTextStyle style(pDb);
    AcDbObjectId styleId = findTextStyle(pDb, text.textStyle);
    if (styleId.isNull())
        styleId = findTextStyle(pDb, kDefaultTextStyle);
    if (!styleId.isNull())
        fromAcDbTextStyle(style, styleId);
    // Высота задаётся свойством, а не стилем; масштаб объекта её увеличивает.
    style.setTextSize(text.height * m_scale);
    style.loadStyleRec(pDb);

    AcGePoint3d position(text.x, text.y, 0.0);
    position.transformBy(localToWorld());
    pWd->geometry().text(position, m_normal, m_xDirection,
                         text.text.kACharPtr(), -1, Adesk::kFalse, style);
}

void MyDevice::subList() const
{
    assertReadEnabled();
    AcDbEntity::subList();

    acutPrintf(_T("%18s%16s X = %-9.16q0, Y = %-9.16q0, Z = %-9.16q0\n"),
               _T(""), _T("Insertion point:"),
               m_position.x, m_position.y, m_position.z);

    MyDeviceBlock::Info info;
    findBlock(info);
    const ACHAR* kind = _T("");
    switch (info.kind)
    {
    case MyDeviceBlock::kNotFound:
        kind = _T(" (BLOCK NOT FOUND)");
        break;
    case MyDeviceBlock::kOrdinary:
        kind = _T(" (block)");
        break;
    case MyDeviceBlock::kDynamic:
        kind = _T(" (dynamic block)");
        break;
    default:
        break;
    }
    acutPrintf(_T("%18s%16s %s%s\n"), _T(""), _T("Block name:"),
               m_blockName.isEmpty() ? _T("(none)") : m_blockName.kACharPtr(), kind);
    if (info.hasVisibility)
    {
        const int stateIndex = info.effectiveState(m_visibility);
        acutPrintf(_T("%18s%16s %s\n"), _T(""), _T("Visibility:"),
                   info.states[static_cast<size_t>(stateIndex)].name.kACharPtr());
    }

    for (int i = 0; i < kTextCount; ++i)
    {
        const MyDeviceText& text = m_texts[i];
        acutPrintf(_T("%18sText%d: %s\n"), _T(""), i + 1, text.text.kACharPtr());
        acutPrintf(_T("%18s%16s %-9.16q0\n"), _T(""), _T("Height:"), text.height);
        acutPrintf(_T("%18s%16s X = %-9.16q0, Y = %-9.16q0\n"), _T(""), _T("Position:"),
                   text.x, text.y);
        acutPrintf(_T("%18s%16s %s\n"), _T(""), _T("Layer:"),
                   text.layer.isEmpty() ? _T("(MyDevice layer)") : text.layer.kACharPtr());
        acutPrintf(_T("%18s%16s %s\n"), _T(""), _T("Text style:"), text.textStyle.kACharPtr());
    }
}

Acad::ErrorStatus MyDevice::subTransformBy(const AcGeMatrix3d& xform)
{
    // Неравномерное масштабирование исказило бы текст — запрещаем его.
    if (!xform.isUniScaledOrtho())
        return Acad::eCannotScaleNonUniformly;

    AcGeVector3d xAxis = m_xDirection * m_scale;
    AcGeVector3d yAxis = m_normal.crossProduct(m_xDirection) * m_scale;
    xAxis.transformBy(xform);
    yAxis.transformBy(xform);

    const double newScale = xAxis.length();
    if (newScale < kTolerance)
        return Acad::eInvalidInput;

    assertWriteEnabled();
    m_position.transformBy(xform);
    m_xDirection = xAxis * (1.0 / newScale);
    // При зеркалировании нормаль меняет знак, так что геометрия отражается корректно.
    m_normal = xAxis.crossProduct(yAxis).normal();
    m_scale = newScale;
    return Acad::eOk;
}

Acad::ErrorStatus MyDevice::subGetGeomExtents(AcDbExtents& extents) const
{
    assertReadEnabled();

    AcGePoint3d corners[4];
    getCorners(corners);
    for (int i = 0; i < 4; ++i)
        extents.addPoint(corners[i]);

    // Тексты могут выходить за прямоугольник. Ширина текста зависит от шрифта,
    // поэтому учитываются только начало базовой линии и верх первого символа.
    const AcGeMatrix3d xform = localToWorld();
    for (int i = 0; i < kTextCount; ++i)
    {
        AcGePoint3d base(m_texts[i].x, m_texts[i].y, 0.0);
        AcGePoint3d top(m_texts[i].x, m_texts[i].y + m_texts[i].height, 0.0);
        extents.addPoint(base.transformBy(xform));
        extents.addPoint(top.transformBy(xform));
    }

    // Видимые примитивы блока: углы их габаритов в системе координат блока.
    MyDeviceBlock::Info info;
    findBlock(info);
    const AcGeMatrix3d blockXform = blockToWorld(info);
    MyDeviceBlock::forEachVisibleEntity(
        databaseOrWorking(), info, info.effectiveState(m_visibility),
        [&extents, &blockXform](AcDbEntity* pEntity) {
            AcDbExtents entityExtents;
            if (pEntity->getGeomExtents(entityExtents) != Acad::eOk)
                return;
            const AcGePoint3d lo = entityExtents.minPoint();
            const AcGePoint3d hi = entityExtents.maxPoint();
            for (int corner = 0; corner < 8; ++corner)
            {
                AcGePoint3d p((corner & 1) ? hi.x : lo.x,
                              (corner & 2) ? hi.y : lo.y,
                              (corner & 4) ? hi.z : lo.z);
                extents.addPoint(p.transformBy(blockXform));
            }
        });
    return Acad::eOk;
}

Acad::ErrorStatus MyDevice::subGetGripPoints(AcGePoint3dArray& gripPoints,
                                             AcDbIntArray& /*osnapModes*/,
                                             AcDbIntArray& /*geomIds*/) const
{
    assertReadEnabled();
    // Одна ручка в точке вставки — перемещает весь объект.
    gripPoints.append(m_position);
    return Acad::eOk;
}

Acad::ErrorStatus MyDevice::subMoveGripPointsAt(const AcDbIntArray& indices,
                                                const AcGeVector3d& offset)
{
    if (indices.length() == 0)
        return Acad::eOk;

    assertWriteEnabled();
    m_position += offset;
    return Acad::eOk;
}
