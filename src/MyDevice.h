// MyDevice.h — пользовательский объект MyDevice (наследник AcDbEntity).
//
// MyDevice хранит точку вставки, ориентацию, два текста (Text1, Text2) и имя блока
// и сам рисует прямоугольник 100x50, блок и оба текста в subWorldDraw(),
// не создавая отдельных AcDbText/AcDbMText/AcDbBlockReference.
#pragma once

#include "StdAfx.h"
#include "MyDeviceBlock.h"

// Параметры одного текста MyDevice.
// Высота и положение задаются в локальной системе координат объекта:
// при перемещении, повороте и масштабировании MyDevice они не меняются,
// а текст преобразуется вместе с объектом.
struct MyDeviceText
{
    AcString text;       // содержимое
    double   height;     // высота текста
    double   x;          // начало базовой линии, локальная X
    double   y;          // начало базовой линии, локальная Y
    AcString layer;      // имя слоя; пустая строка — слой самого MyDevice
    AcString textStyle;  // имя текстового стиля AutoCAD (шрифт)
};

class MyDevice : public AcDbEntity
{
public:
    ACRX_DECLARE_MEMBERS(MyDevice);

    // Версия формата данных MyDevice в DWG/DXF.
    // Увеличивать при каждом изменении набора сохраняемых полей.
    // 1 — этап 1 (только строки Text1/Text2), 2 — полные свойства текстов,
    // 3 — имя блока и состояние видимости.
    static constexpr Adesk::Int16 kCurrentVersion = 3;

    // Геометрия в локальной системе координат объекта (единицы чертежа).
    static const double kWidth;       // ширина прямоугольника
    static const double kHeight;      // высота прямоугольника
    static const double kTextHeight;  // высота текста по умолчанию
    static const double kTextMargin;  // отступ текста от левого края по умолчанию
    static const double kNotFoundTextHeight; // высота надписи «BLOCK NOT FOUND»

    // Номера текстов для textAt()/setTextAt().
    enum TextIndex { kText1 = 0, kText2 = 1, kTextCount = 2 };

    // Текстовый стиль по умолчанию — есть в любом чертеже.
    static const ACHAR* const kDefaultTextStyle;

    MyDevice();
    explicit MyDevice(const AcGePoint3d& position);
    ~MyDevice() override;

    // Точка вставки (левый нижний угол прямоугольника) в МСК.
    AcGePoint3d position() const;
    Acad::ErrorStatus setPosition(const AcGePoint3d& position);

    // Направление оси X объекта и нормаль к его плоскости (единичные векторы, МСК).
    AcGeVector3d xDirection() const;
    AcGeVector3d normal() const;
    Acad::ErrorStatus setOrientation(const AcGeVector3d& xDirection,
                                     const AcGeVector3d& normal);

    // Масштаб объекта (изменяется командой SCALE).
    double scale() const;
    Acad::ErrorStatus setScale(double scale);

    AcString text1() const;
    Acad::ErrorStatus setText1(const AcString& text);

    AcString text2() const;
    Acad::ErrorStatus setText2(const AcString& text);

    // Все параметры текста index (kText1 или kText2).
    // setTextAt() отклоняет неверный номер, высоту <= 0, нечисловые координаты
    // и пустое имя стиля (eInvalidInput); объект при этом не меняется.
    // Слой и стиль хранятся по имени и в базе данных не создаются.
    MyDeviceText textAt(int index) const;
    Acad::ErrorStatus setTextAt(int index, const MyDeviceText& data);

    // Параметры текста по умолчанию (как на этапе 1).
    static MyDeviceText defaultText(int index);

    // Имя блока, который показывается внутри MyDevice; пустое — блока нет.
    // Базовая точка блока совпадает с началом локальной системы координат объекта,
    // блок перемещается, поворачивается и масштабируется вместе с ним.
    // setBlockName() принимает и имя отсутствующего блока (показывается «BLOCK NOT FOUND»)
    // и сбрасывает Visibility в состояние по умолчанию нового блока (или в пустую
    // строку, если у блока нет параметра видимости). Повторный выбор того же блока
    // сохраняет текущее состояние.
    AcString blockName() const;
    Acad::ErrorStatus setBlockName(const AcString& name);

    // Сохранённое состояние видимости Dynamic Block (пустое — параметра видимости нет).
    // Это не самостоятельный параметр: если состояния с таким именем в блоке уже нет,
    // показывается состояние по умолчанию (см. MyDeviceBlock::Info::effectiveState).
    // setVisibility():
    //  * eNotApplicable — у блока нет параметра видимости (или блок не найден);
    //  * eInvalidInput — такого состояния у блока нет; объект при этом не меняется.
    AcString visibility() const;
    Acad::ErrorStatus setVisibility(const AcString& state);

    // Блок BlockName в базе данных объекта (или в рабочей, если объект не в чертеже).
    Acad::ErrorStatus findBlock(MyDeviceBlock::Info& info) const;

    // Матрица перехода из локальной системы координат объекта в МСК.
    AcGeMatrix3d localToWorld() const;

    // Сохранение и загрузка DWG.
    Acad::ErrorStatus dwgOutFields(AcDbDwgFiler* pFiler) const override;
    Acad::ErrorStatus dwgInFields(AcDbDwgFiler* pFiler) override;

    // Сохранение и загрузка DXF.
    Acad::ErrorStatus dxfOutFields(AcDbDxfFiler* pFiler) const override;
    Acad::ErrorStatus dxfInFields(AcDbDxfFiler* pFiler) override;

protected:
    // Остальные перегрузки базового класса остаются доступными.
    using AcDbEntity::subGetGripPoints;
    using AcDbEntity::subMoveGripPointsAt;

    Adesk::Boolean subWorldDraw(AcGiWorldDraw* pWd) override;
    void subList() const override;
    Acad::ErrorStatus subTransformBy(const AcGeMatrix3d& xform) override;
    Acad::ErrorStatus subGetGeomExtents(AcDbExtents& extents) const override;
    Acad::ErrorStatus subGetGripPoints(AcGePoint3dArray& gripPoints,
                                       AcDbIntArray& osnapModes,
                                       AcDbIntArray& geomIds) const override;
    Acad::ErrorStatus subMoveGripPointsAt(const AcDbIntArray& indices,
                                          const AcGeVector3d& offset) override;

private:
    // Углы прямоугольника в МСК: [0] — точка вставки, далее против часовой стрелки.
    void getCorners(AcGePoint3d corners[4]) const;

    // Рисует один текст его стилем и на его слое.
    void drawText(AcGiWorldDraw* pWd, AcDbDatabase* pDb, const MyDeviceText& text) const;

    // Рисует блок BlockName (или «BLOCK NOT FOUND») на слое объекта.
    void drawBlock(AcGiWorldDraw* pWd, AcDbDatabase* pDb) const;

    // База данных объекта, а если он ещё не добавлен в чертёж — рабочая.
    AcDbDatabase* databaseOrWorking() const;

    // Матрица перехода из системы координат определения блока в МСК:
    // базовая точка блока попадает в начало локальной системы координат объекта.
    AcGeMatrix3d blockToWorld(const MyDeviceBlock::Info& info) const;

    AcGePoint3d  m_position;
    AcGeVector3d m_xDirection;
    AcGeVector3d m_normal;
    double       m_scale;
    MyDeviceText m_texts[kTextCount];
    AcString     m_blockName;
    AcString     m_visibility;
};
