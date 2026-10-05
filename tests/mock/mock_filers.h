// mock_filers.h — филеры DWG/DXF в памяти и записывающий AcGiWorldDraw для тестов.
//
// MemoryDwgFiler хранит последовательность типизированных значений и проверяет при
// чтении, что тип совпадает с записанным — так обнаруживается расхождение порядка
// полей в dwgOutFields()/dwgInFields(), которое в AutoCAD повредило бы чертёж.
//
// MemoryDxfFiler хранит пары «групповой код — значение» и повторяет протокол
// AcDbDxfFiler: readResBuf() возвращает eEndOfFile в конце данных,
// pushBackItem() возвращает последнюю прочитанную группу.
#pragma once

#include "arx_mock.h"

#include <string>
#include <vector>

class MemoryDwgFiler : public AcDbDwgFiler
{
public:
    enum class Type { String, Int16, Int32, Double, Point3d, Vector3d };

    struct Item
    {
        Type type;
        std::wstring str;
        double d[3];
        Adesk::Int32 i;
    };

    std::vector<Item> items;
    size_t cursor = 0;
    Acad::ErrorStatus status = Acad::eOk;

    void rewind() { cursor = 0; status = Acad::eOk; }

    Acad::ErrorStatus filerStatus() const override { return status; }

    Acad::ErrorStatus writeString(const AcString& val) override
    {
        Item it = make(Type::String);
        it.str = val.kACharPtr();
        items.push_back(it);
        return status;
    }
    Acad::ErrorStatus readString(AcString& val) override
    {
        const Item* it = next(Type::String);
        if (it)
            val = it->str.c_str();
        return status;
    }

    Acad::ErrorStatus writeInt16(Adesk::Int16 val) override
    {
        Item it = make(Type::Int16);
        it.i = val;
        items.push_back(it);
        return status;
    }
    Acad::ErrorStatus readInt16(Adesk::Int16* pVal) override
    {
        const Item* it = next(Type::Int16);
        if (it)
            *pVal = static_cast<Adesk::Int16>(it->i);
        return status;
    }

    Acad::ErrorStatus writeInt32(Adesk::Int32 val) override
    {
        Item it = make(Type::Int32);
        it.i = val;
        items.push_back(it);
        return status;
    }
    Acad::ErrorStatus readInt32(Adesk::Int32* pVal) override
    {
        const Item* it = next(Type::Int32);
        if (it)
            *pVal = it->i;
        return status;
    }

    Acad::ErrorStatus writeDouble(double val) override
    {
        Item it = make(Type::Double);
        it.d[0] = val;
        items.push_back(it);
        return status;
    }
    Acad::ErrorStatus readDouble(double* pVal) override
    {
        const Item* it = next(Type::Double);
        if (it)
            *pVal = it->d[0];
        return status;
    }

    Acad::ErrorStatus writePoint3d(const AcGePoint3d& val) override
    {
        Item it = make(Type::Point3d);
        it.d[0] = val.x; it.d[1] = val.y; it.d[2] = val.z;
        items.push_back(it);
        return status;
    }
    Acad::ErrorStatus readPoint3d(AcGePoint3d* pVal) override
    {
        const Item* it = next(Type::Point3d);
        if (it)
            pVal->set(it->d[0], it->d[1], it->d[2]);
        return status;
    }

    Acad::ErrorStatus writeVector3d(const AcGeVector3d& val) override
    {
        Item it = make(Type::Vector3d);
        it.d[0] = val.x; it.d[1] = val.y; it.d[2] = val.z;
        items.push_back(it);
        return status;
    }
    Acad::ErrorStatus readVector3d(AcGeVector3d* pVal) override
    {
        const Item* it = next(Type::Vector3d);
        if (it)
            *pVal = AcGeVector3d(it->d[0], it->d[1], it->d[2]);
        return status;
    }

private:
    static Item make(Type type)
    {
        Item it;
        it.type = type;
        it.d[0] = it.d[1] = it.d[2] = 0.0;
        it.i = 0;
        return it;
    }

    const Item* next(Type type)
    {
        if (status != Acad::eOk)
            return nullptr;
        if (cursor >= items.size())
        {
            status = Acad::eEndOfFile;
            return nullptr;
        }
        const Item& it = items[cursor++];
        if (it.type != type)
        {
            status = Acad::eInvalidResBuf;
            return nullptr;
        }
        return &it;
    }
};

class MemoryDxfFiler : public AcDbDxfFiler
{
public:
    enum class Type { String, Int16, Double, Point3d };

    struct Item
    {
        int code;
        Type type;
        std::wstring str;
        double d[3];
        Adesk::Int16 i;
        int precision;
    };

    std::vector<Item> items;
    size_t cursor = 0;
    bool canPushBack = false;
    Acad::ErrorStatus status = Acad::eOk;
    std::wstring errorMessage;

    void rewind() { cursor = 0; canPushBack = false; status = Acad::eOk; errorMessage.clear(); }

    // Ищет группу с кодом code (после записи). Возвращает nullptr, если её нет.
    const Item* find(int code) const
    {
        for (const Item& it : items)
            if (it.code == code)
                return &it;
        return nullptr;
    }

    // Добавление групп вручную — для имитации DXF-файла, записанного извне.
    void addString(int code, const wchar_t* s) { Item it = make(code, Type::String); it.str = s; items.push_back(it); }
    void addInt16(int code, Adesk::Int16 v) { Item it = make(code, Type::Int16); it.i = v; items.push_back(it); }
    void addDouble(int code, double v) { Item it = make(code, Type::Double); it.d[0] = v; items.push_back(it); }
    void addPoint(int code, double x, double y, double z)
    {
        Item it = make(code, Type::Point3d);
        it.d[0] = x; it.d[1] = y; it.d[2] = z;
        items.push_back(it);
    }

    Acad::ErrorStatus filerStatus() const override { return status; }

    Acad::ErrorStatus setError(Acad::ErrorStatus es, const ACHAR* errMsg, ...) override
    {
        status = es;
        errorMessage = errMsg ? errMsg : L"";
        return status;
    }

    Acad::ErrorStatus readResBuf(resbuf* pRb) override
    {
        if (cursor >= items.size())
        {
            canPushBack = false;
            return Acad::eEndOfFile;
        }
        Item& it = items[cursor++];
        canPushBack = true;
        pRb->rbnext = nullptr;
        pRb->restype = static_cast<short>(it.code);
        switch (it.type)
        {
        case Type::String:
            // Как и в AutoCAD, строкой владеет филер.
            pRb->resval.rstring = const_cast<ACHAR*>(it.str.c_str());
            break;
        case Type::Int16:
            pRb->resval.rint = it.i;
            break;
        case Type::Double:
            pRb->resval.rreal = it.d[0];
            break;
        case Type::Point3d:
            pRb->resval.rpoint[0] = it.d[0];
            pRb->resval.rpoint[1] = it.d[1];
            pRb->resval.rpoint[2] = it.d[2];
            break;
        }
        return Acad::eOk;
    }

    Acad::ErrorStatus pushBackItem() override
    {
        if (!canPushBack || cursor == 0)
            return Acad::eInvalidInput;
        --cursor;
        canPushBack = false;
        return Acad::eOk;
    }

    bool atSubclassData(const ACHAR* subClassName) override
    {
        if (cursor < items.size() && items[cursor].code == AcDb::kDxfSubclass
            && items[cursor].str == subClassName)
        {
            ++cursor;
            canPushBack = false;
            return true;
        }
        return false;
    }

    Acad::ErrorStatus writeString(AcDb::DxfCode code, const ACHAR* pVal) override
    {
        addString(code, pVal ? pVal : L"");
        return status;
    }
    Acad::ErrorStatus writeString(AcDb::DxfCode code, const AcString& val) override
    {
        addString(code, val.kACharPtr());
        return status;
    }
    Acad::ErrorStatus writeInt16(AcDb::DxfCode code, Adesk::Int16 val) override
    {
        addInt16(code, val);
        return status;
    }
    Acad::ErrorStatus writeDouble(AcDb::DxfCode code, double val, int prec) override
    {
        addDouble(code, val);
        items.back().precision = prec;
        return status;
    }
    Acad::ErrorStatus writePoint3d(AcDb::DxfCode code, const AcGePoint3d& val, int prec) override
    {
        addPoint(code, val.x, val.y, val.z);
        items.back().precision = prec;
        return status;
    }
    Acad::ErrorStatus writeVector3d(AcDb::DxfCode code, const AcGeVector3d& val, int prec) override
    {
        addPoint(code, val.x, val.y, val.z);
        items.back().precision = prec;
        return status;
    }

private:
    static Item make(int code, Type type)
    {
        Item it;
        it.code = code;
        it.type = type;
        it.d[0] = it.d[1] = it.d[2] = 0.0;
        it.i = 0;
        it.precision = AcDb::kDfltPrec;
        return it;
    }
};

// Записывает все графические примитивы, которые объект выдаёт в subWorldDraw(),
// вместе со слоем, установленным через subEntityTraits() на момент вызова.
// Пустой идентификатор слоя означает «слой самого объекта» (setLayer не вызывался).
//
// Матрицы pushModelTransform() накапливаются в стеке; каждый примитив запоминает
// матрицу, действующую в момент вызова (transform), а worldPoints() пересчитывает
// точки полилинии в МСК. draw() записывает вложенный объект и, как AutoCAD,
// рисует его через worldDraw() с тем же стеком матриц.
class RecordingWorldDraw : public AcGiWorldDraw, public AcGiWorldGeometry,
                           public AcGiSubEntityTraits
{
public:
    struct Polyline
    {
        std::vector<AcGePoint3d> points;
        bool hasNormal;
        AcGeVector3d normal;
        AcDbObjectId layerId;
        AcGeMatrix3d transform;

        std::vector<AcGePoint3d> worldPoints() const
        {
            std::vector<AcGePoint3d> result = points;
            for (AcGePoint3d& p : result)
                p.transformBy(transform);
            return result;
        }
    };

    struct Drawn
    {
        const AcGiDrawable* drawable;
        AcGeMatrix3d transform;
        AcDbObjectId layerId;
    };

    struct Text
    {
        AcGePoint3d position;
        AcGeVector3d normal;
        AcGeVector3d direction;
        double height;
        double width;
        double oblique;
        std::wstring message;
        AcDbObjectId layerId;
        // Только для вызова со стилем: имя стиля, признак загрузки шрифта, length и raw.
        bool hasStyle = false;
        std::wstring styleName;
        bool styleLoaded = false;
        Adesk::Int32 length = 0;
        bool raw = false;
        AcGeMatrix3d transform;
    };

    mutable std::vector<Polyline> polylines;
    mutable std::vector<Text> texts;
    bool abort = false;
    AcDbObjectId currentLayer;
    int setLayerCalls = 0;
    mutable std::vector<Drawn> drawn;
    // Текущий стек матриц модели (пустой — единичная матрица).
    std::vector<AcGeMatrix3d> transforms;
    int pushCalls = 0;
    int popCalls = 0;
    // Предел вложенности draw(): защищает тест от бесконечной рекурсии.
    static const int kMaxDepth = 16;
    mutable int depth = 0;
    mutable bool depthExceeded = false;

    AcGeMatrix3d currentTransform() const
    {
        return transforms.empty() ? AcGeMatrix3d() : transforms.back();
    }

    Adesk::Boolean pushModelTransform(const AcGeMatrix3d& xMat) override
    {
        pushCalls++;
        transforms.push_back(currentTransform() * xMat);
        return Adesk::kFalse;
    }
    Adesk::Boolean popModelTransform() override
    {
        popCalls++;
        if (!transforms.empty())
            transforms.pop_back();
        return Adesk::kFalse;
    }
    Adesk::Boolean draw(AcGiDrawable* pDrawable) const override
    {
        Drawn d;
        d.drawable = pDrawable;
        d.transform = currentTransform();
        d.layerId = currentLayer;
        drawn.push_back(d);
        AcDbEntity* pEntity = dynamic_cast<AcDbEntity*>(pDrawable);
        if (pEntity == nullptr)
            return Adesk::kFalse;
        if (depth >= kMaxDepth)
        {
            depthExceeded = true;
            return Adesk::kFalse;
        }
        RecordingWorldDraw& self = const_cast<RecordingWorldDraw&>(*this);
        const AcDbObjectId savedLayer = currentLayer;
        depth++;
        pEntity->worldDraw(&self);
        depth--;
        self.currentLayer = savedLayer;
        return Adesk::kFalse;
    }

    AcGiWorldGeometry& geometry() const override
    {
        return const_cast<RecordingWorldDraw&>(*this);
    }
    AcGiSubEntityTraits& subEntityTraits() const override
    {
        return const_cast<RecordingWorldDraw&>(*this);
    }
    Adesk::Boolean regenAbort() const override { return abort; }

    void setLayer(const AcDbObjectId layerId) override
    {
        currentLayer = layerId;
        setLayerCalls++;
    }
    AcDbObjectId layerId() const override { return currentLayer; }

    Adesk::Boolean polyline(const Adesk::UInt32 nbPoints, const AcGePoint3d* pVertexList,
                            const AcGeVector3d* pNormal, Adesk::LongPtr) const override
    {
        Polyline pl;
        pl.points.assign(pVertexList, pVertexList + nbPoints);
        pl.hasNormal = pNormal != nullptr;
        if (pNormal)
            pl.normal = *pNormal;
        pl.layerId = currentLayer;
        pl.transform = currentTransform();
        polylines.push_back(pl);
        return Adesk::kFalse;
    }

    Adesk::Boolean text(const AcGePoint3d& position, const AcGeVector3d& normal,
                        const AcGeVector3d& direction, const double height,
                        const double width, const double oblique,
                        const ACHAR* pMsg) const override
    {
        Text t;
        t.position = position;
        t.normal = normal;
        t.direction = direction;
        t.height = height;
        t.width = width;
        t.oblique = oblique;
        t.message = pMsg ? pMsg : L"";
        t.layerId = currentLayer;
        t.transform = currentTransform();
        texts.push_back(t);
        return Adesk::kFalse;
    }

    Adesk::Boolean text(const AcGePoint3d& position, const AcGeVector3d& normal,
                        const AcGeVector3d& direction, const ACHAR* pMsg,
                        const Adesk::Int32 length, const Adesk::Boolean raw,
                        const AcGiTextStyle& textStyle) const override
    {
        Text t;
        t.position = position;
        t.normal = normal;
        t.direction = direction;
        t.height = textStyle.textSize();
        t.width = 1.0;
        t.oblique = 0.0;
        t.message = pMsg ? pMsg : L"";
        t.layerId = currentLayer;
        t.hasStyle = true;
        t.styleName = textStyle.styleName();
        t.styleLoaded = textStyle.mockLoaded();
        t.length = length;
        t.raw = raw;
        texts.push_back(t);
        return Adesk::kFalse;
    }
};
