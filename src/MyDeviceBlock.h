// MyDeviceBlock.h — блок, который MyDevice показывает внутри себя (этап 3).
//
// Поиск определения блока по имени, определение его типа (обычный или Dynamic Block)
// и чтение параметра видимости (Visibility) динамического блока.
//
// Параметр видимости хранится в определении блока: словарь расширений записи блока ->
// запись ACAD_ENHANCEDBLOCK (граф вычислений) -> узел BLOCKVISIBILITYPARAMETER.
// Его данные читаются через acdbEntGet() — открытого API ObjectARX для параметров
// определения динамического блока нет (AcDbDynBlockReference работает только
// со вставками блока, а MyDevice вставку не создаёт).
#pragma once

#include "StdAfx.h"

#include <functional>
#include <vector>

namespace MyDeviceBlock
{
    enum Kind
    {
        kNoBlock = 0,  // BlockName пустое — блок не показывается
        kNotFound,     // блока нет в чертеже (или это лист, анонимный блок, внешняя ссылка)
        kOrdinary,     // обычный блок
        kDynamic       // Dynamic Block (Visibility — только если есть параметр видимости)
    };

    // Состояние видимости: имя и примитивы, видимые в этом состоянии.
    struct State
    {
        AcString name;
        std::vector<AcDbObjectId> visible;
    };

    struct Info
    {
        Kind kind = kNoBlock;
        AcString name;              // имя записи таблицы блоков (как в чертеже)
        AcDbObjectId blockId;
        AcGePoint3d origin;         // базовая точка блока
        bool hasVisibility = false; // есть параметр видимости хотя бы с одним состоянием
        AcString parameterName;     // имя параметра видимости (например, «Visibility1»)
        std::vector<AcDbObjectId> controlled; // примитивы, которыми управляет параметр
        std::vector<State> states;            // первое состояние — состояние по умолчанию

        // Номер состояния name или -1.
        int findState(const AcString& name) const;
        // Номер состояния, которое нужно показывать для сохранённого значения stored:
        // само stored, если такое состояние есть, иначе состояние по умолчанию.
        // -1 — параметра видимости нет.
        int effectiveState(const AcString& stored) const;
        // Виден ли примитив id определения блока в состоянии stateIndex.
        // Примитивы, которыми параметр видимости не управляет, видны всегда.
        bool isVisible(const AcDbObjectId& id, int stateIndex) const;
    };

    // Ищет блок name в pDb (без учёта регистра) и заполняет info.
    //  * eOk — пустое имя (kNoBlock) или блок найден (kOrdinary/kDynamic);
    //  * eKeyNotFound — блока нет или его нельзя показывать (kNotFound);
    //  * eNullObjectPointer — нет базы данных (kNotFound).
    // База данных только читается.
    Acad::ErrorStatus find(AcDbDatabase* pDb, const AcString& name, Info& info);

    // Вызывает fn для каждого примитива определения блока, видимого в состоянии
    // stateIndex (кроме определений атрибутов: их тег — не графика блока).
    // Примитив открыт на чтение только на время вызова fn.
    // Блок, который уже обходится выше по стеку (блок содержит MyDevice, показывающий
    // этот же блок), пропускается: иначе отрисовка ушла бы в бесконечную рекурсию.
    Acad::ErrorStatus forEachVisibleEntity(AcDbDatabase* pDb, const Info& info, int stateIndex,
                                           const std::function<void(AcDbEntity*)>& fn);

    // Имена блоков, которые можно выбрать в BlockName, в порядке таблицы блоков:
    // без пространств модели и листов, анонимных блоков и внешних ссылок.
    Acad::ErrorStatus names(AcDbDatabase* pDb, std::vector<AcString>& names);
}
