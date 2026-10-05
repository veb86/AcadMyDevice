// MyDeviceProperties.h — свойства MyDevice для палитры свойств.
//
// Здесь собрано всё, что палитре свойств нужно знать о свойствах, кроме COM:
// список свойств (категория «MyDevice» с блоком, категории «Text 1»/«Text 2»
// с текстами: имя, описание), чтение и запись значений, списки блоков, состояний
// видимости, слоёв и текстовых стилей чертежа и создание отсутствующего слоя.
// COM-обёртка для AutoCAD находится в MyDeviceOPM.cpp; этот файл не зависит
// от COM и проверяется модульными тестами.
#pragma once

#include "StdAfx.h"
#include "MyDevice.h"

#include <vector>

class MyDeviceProperties
{
public:
    // Свойство одного текста.
    enum Field
    {
        kText = 0,    // содержимое (строка)
        kHeight,      // высота (число)
        kPositionX,   // локальная X (число)
        kPositionY,   // локальная Y (число)
        kLayer,       // имя слоя (строка); отсутствующий слой создаётся
        kFont,        // имя текстового стиля (выбор из списка стилей чертежа)
        kFieldCount
    };

    struct Descriptor
    {
        int textIndex;             // MyDevice::kText1 или MyDevice::kText2
        Field field;
        const ACHAR* category;     // «Text 1» или «Text 2»
        const ACHAR* name;         // имя в палитре
        const ACHAR* description;  // подсказка в палитре
    };

    // Все свойства текстов: сначала шесть свойств Text1, затем шесть свойств Text2.
    static int count();
    static const Descriptor& at(int index);

    // Свойства блока (категория «MyDevice», в палитре — над текстами).
    enum BlockField
    {
        kBlockName = 0,   // имя блока (выбор из списка блоков чертежа)
        kVisibility,      // состояние видимости Dynamic Block (выбор из списка состояний)
        kBlockFieldCount
    };

    struct BlockDescriptor
    {
        BlockField field;
        const ACHAR* category;     // «MyDevice»
        const ACHAR* name;         // имя в палитре
        const ACHAR* description;  // подсказка в палитре
    };

    static int blockCount();
    static const BlockDescriptor& blockAt(int index);

    // Имя блока. Запись определяет тип блока заново и сбрасывает Visibility
    // в состояние по умолчанию нового блока (повторный выбор того же блока
    // сохраняет текущее состояние). Объект должен быть открыт на запись.
    static AcString blockName(const MyDevice& device);
    static Acad::ErrorStatus setBlockName(MyDevice& device, const AcString& name);

    // Есть ли у блока параметр видимости (только у Dynamic Block с Visibility).
    static bool hasVisibility(const MyDevice& device);
    // Текущее состояние видимости; пусто, если Visibility недоступно.
    static AcString visibility(const MyDevice& device);
    //  * eNotApplicable — у блока нет параметра видимости;
    //  * eInvalidInput — такого состояния у блока нет.
    static Acad::ErrorStatus setVisibility(MyDevice& device, const AcString& state);
    // Состояния видимости блока в порядке параметра; пусто, если Visibility недоступно.
    static Acad::ErrorStatus visibilityStates(const MyDevice& device, std::vector<AcString>& states);

    // Блоки, которые можно выбрать в BlockName: без листов, анонимных блоков и внешних ссылок.
    static Acad::ErrorStatus blockNames(AcDbDatabase* pDb, std::vector<AcString>& names);

    // Числовое ли свойство (высота и координаты).
    static bool isNumeric(Field field);

    // Чтение значения. Для строкового свойства getDouble() возвращает 0,
    // для числового getString() — пустую строку.
    static AcString getString(const MyDevice& device, int textIndex, Field field);
    static double getDouble(const MyDevice& device, int textIndex, Field field);

    // Запись значения. Объект должен быть открыт на запись.
    //  * eInvalidInput — неверный номер текста, тип свойства или значение
    //    (высота <= 0, недопустимое имя слоя и т. п.);
    //  * eKeyNotFound — текстового стиля с таким именем нет в чертеже.
    // Для свойства Layer отсутствующий слой создаётся в базе данных объекта.
    // Пустое имя слоя означает «слой самого MyDevice».
    static Acad::ErrorStatus setString(MyDevice& device, int textIndex, Field field,
                                       const AcString& value);
    static Acad::ErrorStatus setDouble(MyDevice& device, int textIndex, Field field,
                                       double value);

    // Имена слоёв и текстовых стилей чертежа в порядке таблицы.
    // Стили-описания форм (SHX shape files) в список не входят: ими нельзя писать текст.
    static Acad::ErrorStatus layerNames(AcDbDatabase* pDb, std::vector<AcString>& names);
    static Acad::ErrorStatus textStyleNames(AcDbDatabase* pDb, std::vector<AcString>& names);

    // Создаёт слой name, если его нет. Пустое имя — ничего не делать.
    static Acad::ErrorStatus ensureLayer(AcDbDatabase* pDb, const AcString& name);

    // База данных объекта, а если он ещё не добавлен в чертёж — рабочая.
    static AcDbDatabase* databaseOf(const MyDevice& device);
};
