#!/usr/bin/env bash
# Убеждается, что модульные тесты ловят типичные ошибки: вносит в копию исходников
# (build/src) по одной мутации и проверяет, что тесты падают.
# Требует предварительного: cmake -S tests -B build
set -u
cd "$(dirname "$0")/.."
B=build/src
mutate() {  # file sed-expr description
    cp "$B/$1" "$B/$1.orig"
    sed -i "$2" "$B/$1"
    if cmp -s "$B/$1" "$B/$1.orig"; then
        echo "NOT APPLIED: $3"; status=1
    elif ! cmake --build build >/dev/null 2>&1; then
        echo "NO BUILD:    $3"; status=1   # мутация должна компилироваться
    elif ./build/MyDeviceTests >/dev/null; then
        echo "SURVIVED:    $3"; status=1
    else
        echo "killed:      $3"
    fi
    mv "$B/$1.orig" "$B/$1"
    touch "$B/$1"
}
status=0
# Этап 1
mutate MyDevice.cpp 's/pFiler->readString(texts\[kText2\].text);/pFiler->readString(texts[kText1].text);/' "Text2 читается в Text1 (DWG)"
mutate MyDevice.cpp 's/readString(texts\[kText1\].text);/TMPX/;s/readString(texts\[kText2\].text);/readString(texts[kText1].text);/;s/TMPX/readString(texts[kText2].text);/' "порядок Text1/Text2 в dwgIn"
mutate MyDevice.cpp 's/_T("TEXT1")/_T("TXT1")/' "значение по умолчанию Text1"
mutate MyDevice.cpp 's/corners\[2\].set(kWidth, kHeight, 0.0);/corners[2].set(kWidth, kWidth, 0.0);/' "размер прямоугольника"
mutate MyDevice.cpp 's/^            pFiler->pushBackItem();/            ;/' "нет pushBackItem в dxfIn"
mutate MyDevice.cpp 's/if (!xform.isUniScaledOrtho())/if (false)/' "нет проверки неравномерного масштаба"
mutate MyDeviceCommand.cpp 's/es = pModelSpace->appendAcDbEntity(entityId, pEntity);/es = Acad::eOk; (void)entityId;/' "объект не добавляется в модель"
mutate MyDeviceCommand.cpp 's/^            pEntity->close();/            ;/' "объект не закрывается после добавления"
# Этап 2: данные, отрисовка и сохранение текстов
mutate MyDevice.cpp 's/if (version >= 2)/if (version > 2)/' "поля версии 2 не читаются из DWG"
mutate MyDevice.cpp 's/pFiler->readDouble(&texts\[i\].y);/pFiler->readDouble(\&texts[i].x);/' "Y текста читается в X (DWG)"
mutate MyDevice.cpp 's/texts\[rb.restype - kDxfTextLayerBase\].layer/texts[kText1].layer/' "слой Text2 читается в Text1 (DXF)"
mutate MyDevice.cpp 's/return text.height > kTolerance && std::isfinite(text.height)/return std::isfinite(text.height)/' "нет проверки высоты > 0"
mutate MyDevice.cpp 's/pWd->subEntityTraits().setLayer(textLayerId);/(void)textLayerId;/' "текст не получает свой слой"
mutate MyDevice.cpp 's/textLayerId = layerId();/(void)0;/' "нет возврата к слою MyDevice"
mutate MyDevice.cpp 's/styleId = findTextStyle(pDb, kDefaultTextStyle);/(void)0;/' "нет возврата к стилю Standard"
mutate MyDevice.cpp 's/style.setTextSize(text.height \* m_scale);/style.setTextSize(text.height);/' "высота текста не масштабируется"
mutate MyDeviceCommand.cpp 's/text.layer = currentLayer;/;/' "тексты нового объекта не на текущем слое"
# Этап 2: палитра свойств
mutate MyDeviceProperties.cpp 's/        text.x = value;/        text.y = value;/' "Position X пишется в Y"
mutate MyDeviceProperties.cpp 's/const Acad::ErrorStatus es = ensureLayer(databaseOf(device), value);/const Acad::ErrorStatus es = Acad::eOk;/' "Layer не создаёт отсутствующий слой"
mutate MyDeviceProperties.cpp 's/es = pTable->add(pRecord);/es = Acad::eDuplicateRecordName;/' "ensureLayer не добавляет слой"
mutate MyDeviceProperties.cpp 's/if (!hasTextStyle(databaseOf(device), value))/if (!hasTextStyle(databaseOf(device), value) \&\& false)/' "Font принимает несуществующий стиль"
mutate MyDeviceProperties.cpp 's/if (!pRecord->isShapeFile() && /if (/' "формы SHAPE в списке шрифтов"
mutate acrxEntryPoint.cpp 's/^        registerMyDeviceProperties();/        ;/' "свойства палитры не регистрируются"
mutate acrxEntryPoint.cpp '/^        unregisterMyDeviceProperties();/d;s/^        deleteAcRxClass(MyDevice::desc());/&\n        unregisterMyDeviceProperties();/' "свойства удаляются после удаления класса"
# Этап 3: блок и Visibility
mutate MyDevice.cpp 's/if (version >= 3)/if (version > 3)/' "BlockName/Visibility не читаются из DWG"
mutate MyDevice.cpp 's/^    pFiler->writeString(m_visibility);/    pFiler->writeString(m_blockName);/' "Visibility не сохраняется в DWG"
mutate MyDevice.cpp 's/            visibility = rb.resval.rstring;/            blockName = rb.resval.rstring;/' "Visibility читается в BlockName (DXF)"
mutate MyDevice.cpp 's/AcGeMatrix3d::translation(AcGePoint3d::kOrigin - info.origin)/AcGeMatrix3d::translation(info.origin - info.origin)/' "базовая точка блока не в начале координат MyDevice"
mutate MyDevice.cpp 's/return localToWorld() \* AcGeMatrix3d::translation/return AcGeMatrix3d::translation/' "блок не следует за перемещением и поворотом"
mutate MyDevice.cpp 's/^    drawBlock(pWd, pDb);/    (void)0;/' "блок не рисуется"
mutate MyDevice.cpp 's/^        drawText(pWd, pDb, message);/        (void)message;/' "нет надписи BLOCK NOT FOUND"
mutate MyDevice.cpp 's/AcString visibility = info.hasVisibility ? info.states.front().name : AcString();/AcString visibility = m_visibility;/' "смена BlockName не обновляет Visibility"
mutate MyDevice.cpp 's/if (name == blockName() \&\& info.findState(m_visibility) >= 0)/if (false)/' "повторный выбор того же блока сбрасывает Visibility"
mutate MyDevice.cpp 's/if (info.findState(state) < 0)/if (false)/' "Visibility принимает несуществующее состояние"
mutate MyDevice.cpp 's/        pWd->subEntityTraits().setLayer(deviceLayerId);/        (void)0;/' "блок рисуется не на слое MyDevice"
mutate MyDeviceBlock.cpp 's/info.hasVisibility = !info.states.empty();/info.hasVisibility = false;/' "параметр видимости не распознаётся"
mutate MyDeviceBlock.cpp 's/return std::find(visible.begin(), visible.end(), id) != visible.end();/return std::find(visible.begin(), visible.end(), id) != visible.end() || true;/' "Visibility не скрывает примитивы"
mutate MyDeviceBlock.cpp 's/if (std::find(controlled.begin(), controlled.end(), id) == controlled.end())/if (false)/' "неуправляемые примитивы скрыты"
mutate MyDeviceBlock.cpp 's/if (AcDbAttributeDefinition::cast(pEntity) == nullptr/if (true/' "определения атрибутов рисуются"
mutate MyDeviceBlock.cpp 's/return !pBlock->isLayout() \&\& /return /' "пространства модели и листов считаются блоками"
mutate MyDeviceBlock.cpp 's/                pStart = pRb->rbnext;/                (void)0;/' "имя параметра берётся не из подкласса видимости"
# Без защиты габариты вложенного MyDevice рекурсивны: тест завершается переполнением стека.
mutate MyDeviceBlock.cpp 's/^    if (!guard.entered())/    if (false)/' "нет защиты от рекурсии блока"
mutate MyDeviceBlock.cpp 's/^    pBlock->close();\n    return Acad::eOk;/X/;s/^    delete pIter;/    delete pIter; return Acad::eOk;/' "определение блока не закрывается"
cmake --build build >/dev/null
exit $status
