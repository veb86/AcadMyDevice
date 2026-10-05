// MyDeviceOPM.h — свойства MyDevice в палитре свойств AutoCAD (OPM).
//
// Для класса MyDevice регистрируются 14 динамических свойств (IDynamicProperty):
// категория «MyDevice» (BlockName, Visibility), затем категории «Text 1» и «Text 2»,
// в каждой Text, Height, Position X, Position Y, Layer и Font.
// Значения читаются и записываются через MyDeviceProperties.
#pragma once

// Регистрирует свойства в палитре свойств. Возвращает false, если палитра
// недоступна (например, в AcCoreConsole); объект при этом работает как обычно.
bool registerMyDeviceProperties();

// Удаляет свойства, зарегистрированные registerMyDeviceProperties().
void unregisterMyDeviceProperties();
