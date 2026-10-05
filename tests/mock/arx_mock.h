// arx_mock.h — минимальная имитация ObjectARX 2021 для модульных тестов MyDevice.
//
// Это НЕ реализация ObjectARX: здесь объявлено ровно то подмножество API,
// которое использует проект, с теми же именами и сигнатурами. Благодаря этому
// исходники из src/ компилируются без изменений обычным g++/clang на Linux,
// а тесты проверяют логику сериализации DWG/DXF, отрисовки и команды MYDEVICE.
#pragma once

#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <map>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Базовые типы и макросы
// ---------------------------------------------------------------------------

#ifndef __declspec
#define __declspec(x)
#endif

typedef wchar_t ACHAR;
#define _T(x) L##x
#define __ACRX_T(x) L##x
#define ACRX_T(x) __ACRX_T(x)

namespace Adesk
{
    typedef int16_t Int16;
    typedef int32_t Int32;
    typedef uint32_t UInt32;
    typedef int64_t Int64;
    typedef intptr_t LongPtr;
    typedef bool Boolean;
    const Boolean kFalse = false;
    const Boolean kTrue = true;
}

namespace Acad
{
    enum ErrorStatus
    {
        eOk = 0,
        eNotApplicable = 3,
        eNullObjectId = 5,
        eInvalidInput = 4,
        eNullObjectPointer = 9,
        eOutOfMemory = 11,
        eInvalidDxfCode = 16,
        eMissingDxfField = 17,
        eInvalidResBuf = 18,
        eEndOfFile = 20,
        eMakeMeProxy = 80,
        eCannotScaleNonUniformly = 136,
        eDuplicateRecordName = 145,
        eKeyNotFound = 150,
    };
}

namespace AcDb
{
    enum OpenMode { kForRead = 0, kForWrite = 1, kForNotify = 2 };
    enum AcDbDwgVersion { kDHL_CURRENT = 36 };
    enum MaintenanceReleaseVersion { kMReleaseCurrent = 44 };

    enum DxfCode
    {
        kDxfInvalid = -9999,
        kDxfStart = 0,
        kDxfText = 1,
        kDxfLayerName = 8,
        kDxfXCoord = 10,
        kDxfReal = 40,
        kDxfInt16 = 70,
        kDxfInt32 = 90,
        kDxfSubclass = 100,
        kDxfNormalX = 210,
        kDxfXTextString = 300,
        kDxfXdAsciiString = 1000,
        kDxfRegAppName = 1001,
    };

    enum DxfPrecision { kDfltPrec = -1 };
}

// ---------------------------------------------------------------------------
// AcGe
// ---------------------------------------------------------------------------

class AcGeMatrix3d;

class AcGeVector3d
{
public:
    double x, y, z;

    AcGeVector3d() : x(0), y(0), z(0) {}
    AcGeVector3d(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}

    static const AcGeVector3d kIdentity;
    static const AcGeVector3d kXAxis;
    static const AcGeVector3d kYAxis;
    static const AcGeVector3d kZAxis;

    double length() const { return std::sqrt(x * x + y * y + z * z); }
    AcGeVector3d normal() const
    {
        const double len = length();
        return len == 0.0 ? *this : AcGeVector3d(x / len, y / len, z / len);
    }
    double dotProduct(const AcGeVector3d& v) const { return x * v.x + y * v.y + z * v.z; }
    AcGeVector3d crossProduct(const AcGeVector3d& v) const
    {
        return AcGeVector3d(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
    bool isEqualTo(const AcGeVector3d& v, double tol = 1.0e-9) const
    {
        return std::fabs(x - v.x) <= tol && std::fabs(y - v.y) <= tol && std::fabs(z - v.z) <= tol;
    }

    AcGeVector3d operator+(const AcGeVector3d& v) const { return AcGeVector3d(x + v.x, y + v.y, z + v.z); }
    AcGeVector3d operator-(const AcGeVector3d& v) const { return AcGeVector3d(x - v.x, y - v.y, z - v.z); }
    AcGeVector3d operator-() const { return AcGeVector3d(-x, -y, -z); }
    AcGeVector3d operator*(double s) const { return AcGeVector3d(x * s, y * s, z * s); }
    AcGeVector3d& operator+=(const AcGeVector3d& v) { x += v.x; y += v.y; z += v.z; return *this; }
    AcGeVector3d& operator-=(const AcGeVector3d& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    AcGeVector3d& operator*=(double s) { x *= s; y *= s; z *= s; return *this; }

    AcGeVector3d& transformBy(const AcGeMatrix3d& m);
};

inline AcGeVector3d operator*(double s, const AcGeVector3d& v) { return v * s; }

class AcGePoint3d
{
public:
    double x, y, z;

    AcGePoint3d() : x(0), y(0), z(0) {}
    AcGePoint3d(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}

    static const AcGePoint3d kOrigin;

    AcGePoint3d& set(double xx, double yy, double zz) { x = xx; y = yy; z = zz; return *this; }
    bool isEqualTo(const AcGePoint3d& p, double tol = 1.0e-9) const
    {
        return std::fabs(x - p.x) <= tol && std::fabs(y - p.y) <= tol && std::fabs(z - p.z) <= tol;
    }
    double distanceTo(const AcGePoint3d& p) const { return (*this - p).length(); }

    AcGePoint3d operator+(const AcGeVector3d& v) const { return AcGePoint3d(x + v.x, y + v.y, z + v.z); }
    AcGePoint3d operator-(const AcGeVector3d& v) const { return AcGePoint3d(x - v.x, y - v.y, z - v.z); }
    AcGeVector3d operator-(const AcGePoint3d& p) const { return AcGeVector3d(x - p.x, y - p.y, z - p.z); }
    AcGePoint3d& operator+=(const AcGeVector3d& v) { x += v.x; y += v.y; z += v.z; return *this; }

    AcGePoint3d& transformBy(const AcGeMatrix3d& m);
};

class AcGeMatrix3d
{
public:
    double entry[4][4];

    AcGeMatrix3d() { setToIdentity(); }

    AcGeMatrix3d& setToIdentity()
    {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                entry[i][j] = (i == j) ? 1.0 : 0.0;
        return *this;
    }

    // Столбцы матрицы — оси и начало новой системы координат.
    AcGeMatrix3d& setCoordSystem(const AcGePoint3d& origin, const AcGeVector3d& xAxis,
                                 const AcGeVector3d& yAxis, const AcGeVector3d& zAxis)
    {
        setToIdentity();
        const AcGeVector3d* axes[3] = { &xAxis, &yAxis, &zAxis };
        for (int c = 0; c < 3; ++c)
        {
            entry[0][c] = axes[c]->x;
            entry[1][c] = axes[c]->y;
            entry[2][c] = axes[c]->z;
        }
        entry[0][3] = origin.x;
        entry[1][3] = origin.y;
        entry[2][3] = origin.z;
        return *this;
    }

    AcGeMatrix3d operator*(const AcGeMatrix3d& m) const
    {
        AcGeMatrix3d r;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
            {
                double s = 0.0;
                for (int k = 0; k < 4; ++k)
                    s += entry[i][k] * m.entry[k][j];
                r.entry[i][j] = s;
            }
        return r;
    }

    bool isUniScaledOrtho() const
    {
        const double tol = 1.0e-9;
        if (std::fabs(entry[3][0]) > tol || std::fabs(entry[3][1]) > tol ||
            std::fabs(entry[3][2]) > tol || std::fabs(entry[3][3] - 1.0) > tol)
            return false;
        AcGeVector3d c[3];
        for (int i = 0; i < 3; ++i)
            c[i] = AcGeVector3d(entry[0][i], entry[1][i], entry[2][i]);
        const double len = c[0].length();
        if (len < tol)
            return false;
        for (int i = 0; i < 3; ++i)
        {
            if (std::fabs(c[i].length() - len) > tol * len)
                return false;
            for (int j = i + 1; j < 3; ++j)
                if (std::fabs(c[i].dotProduct(c[j])) > tol * len * len)
                    return false;
        }
        return true;
    }

    static AcGeMatrix3d translation(const AcGeVector3d& v)
    {
        AcGeMatrix3d m;
        m.entry[0][3] = v.x;
        m.entry[1][3] = v.y;
        m.entry[2][3] = v.z;
        return m;
    }

    static AcGeMatrix3d scaling(double s, const AcGePoint3d& center = AcGePoint3d::kOrigin)
    {
        AcGeMatrix3d m;
        for (int i = 0; i < 3; ++i)
            m.entry[i][i] = s;
        m.entry[0][3] = center.x * (1.0 - s);
        m.entry[1][3] = center.y * (1.0 - s);
        m.entry[2][3] = center.z * (1.0 - s);
        return m;
    }

    // Поворот вокруг оси, проходящей через center (формула Родрига).
    static AcGeMatrix3d rotation(double angle, const AcGeVector3d& axis,
                                 const AcGePoint3d& center = AcGePoint3d::kOrigin)
    {
        const AcGeVector3d a = axis.normal();
        const double c = std::cos(angle), s = std::sin(angle), t = 1.0 - c;
        AcGeMatrix3d r;
        r.entry[0][0] = t * a.x * a.x + c;
        r.entry[0][1] = t * a.x * a.y - s * a.z;
        r.entry[0][2] = t * a.x * a.z + s * a.y;
        r.entry[1][0] = t * a.x * a.y + s * a.z;
        r.entry[1][1] = t * a.y * a.y + c;
        r.entry[1][2] = t * a.y * a.z - s * a.x;
        r.entry[2][0] = t * a.x * a.z - s * a.y;
        r.entry[2][1] = t * a.y * a.z + s * a.x;
        r.entry[2][2] = t * a.z * a.z + c;
        const AcGeVector3d toCenter = center - AcGePoint3d::kOrigin;
        return translation(toCenter) * r * translation(-toCenter);
    }
};

inline AcGeVector3d& AcGeVector3d::transformBy(const AcGeMatrix3d& m)
{
    const double nx = m.entry[0][0] * x + m.entry[0][1] * y + m.entry[0][2] * z;
    const double ny = m.entry[1][0] * x + m.entry[1][1] * y + m.entry[1][2] * z;
    const double nz = m.entry[2][0] * x + m.entry[2][1] * y + m.entry[2][2] * z;
    x = nx; y = ny; z = nz;
    return *this;
}

inline AcGePoint3d& AcGePoint3d::transformBy(const AcGeMatrix3d& m)
{
    const double nx = m.entry[0][0] * x + m.entry[0][1] * y + m.entry[0][2] * z + m.entry[0][3];
    const double ny = m.entry[1][0] * x + m.entry[1][1] * y + m.entry[1][2] * z + m.entry[1][3];
    const double nz = m.entry[2][0] * x + m.entry[2][1] * y + m.entry[2][2] * z + m.entry[2][3];
    x = nx; y = ny; z = nz;
    return *this;
}

// ---------------------------------------------------------------------------
// AcArray, AcString, AcDbExtents
// ---------------------------------------------------------------------------

template <typename T>
class AcArray
{
public:
    int append(const T& value) { m_items.push_back(value); return length() - 1; }
    int length() const { return static_cast<int>(m_items.size()); }
    bool isEmpty() const { return m_items.empty(); }
    T& operator[](int i) { return m_items[static_cast<size_t>(i)]; }
    const T& operator[](int i) const { return m_items[static_cast<size_t>(i)]; }

private:
    std::vector<T> m_items;
};

typedef AcArray<AcGePoint3d> AcGePoint3dArray;
typedef AcArray<int> AcDbIntArray;

class AcString
{
public:
    AcString() {}
    AcString(const wchar_t* s) : m_str(s ? s : L"") {}
    AcString& operator=(const wchar_t* s) { m_str = s ? s : L""; return *this; }
    bool operator==(const wchar_t* s) const { return m_str == (s ? s : L""); }
    bool operator==(const AcString& s) const { return m_str == s.m_str; }
    bool operator!=(const wchar_t* s) const { return !(*this == s); }
    bool operator!=(const AcString& s) const { return !(*this == s); }
    bool isEmpty() const { return m_str.empty(); }
    const ACHAR* kACharPtr() const { return m_str.c_str(); }
    const wchar_t* kwszPtr() const { return m_str.c_str(); }
    int length() const { return static_cast<int>(m_str.size()); }

private:
    std::wstring m_str;
};

class AcDbExtents
{
public:
    AcDbExtents() : m_empty(true) {}
    void addPoint(const AcGePoint3d& p)
    {
        if (m_empty)
        {
            m_min = m_max = p;
            m_empty = false;
            return;
        }
        m_min.set(std::fmin(m_min.x, p.x), std::fmin(m_min.y, p.y), std::fmin(m_min.z, p.z));
        m_max.set(std::fmax(m_max.x, p.x), std::fmax(m_max.y, p.y), std::fmax(m_max.z, p.z));
    }
    AcGePoint3d minPoint() const { return m_min; }
    AcGePoint3d maxPoint() const { return m_max; }

private:
    bool m_empty;
    AcGePoint3d m_min, m_max;
};

// ---------------------------------------------------------------------------
// ADS
// ---------------------------------------------------------------------------

typedef double ads_real;
typedef ads_real ads_point[3];
enum { X = 0, Y = 1, Z = 2 };

typedef Adesk::Int64 ads_name[2];

#define RTNORM 5100
#define RTCAN (-5002)

union ads_u_val
{
    ads_real rreal;
    ads_real rpoint[3];
    short rint;
    ACHAR* rstring;
    Adesk::Int32 rlong;
    Adesk::Int64 rlname[2];
};

struct resbuf
{
    resbuf* rbnext;
    short restype;
    ads_u_val resval;
};

int acutPrintf(const ACHAR* format, ...);
int acedGetPoint(const ads_point pt, const ACHAR* prompt, ads_point result);
Acad::ErrorStatus acedGetCurrentUCS(AcGeMatrix3d& mat);
const ACHAR* acadErrorStatusText(Acad::ErrorStatus es);

// ---------------------------------------------------------------------------
// Филеры
// ---------------------------------------------------------------------------

class AcDbDwgFiler
{
public:
    virtual ~AcDbDwgFiler() {}
    virtual Acad::ErrorStatus filerStatus() const = 0;

    virtual Acad::ErrorStatus readString(AcString& val) = 0;
    virtual Acad::ErrorStatus writeString(const AcString& val) = 0;
    virtual Acad::ErrorStatus readInt16(Adesk::Int16* pVal) = 0;
    virtual Acad::ErrorStatus writeInt16(Adesk::Int16 val) = 0;
    virtual Acad::ErrorStatus readInt32(Adesk::Int32* pVal) = 0;
    virtual Acad::ErrorStatus writeInt32(Adesk::Int32 val) = 0;
    virtual Acad::ErrorStatus readDouble(double* pVal) = 0;
    virtual Acad::ErrorStatus writeDouble(double val) = 0;
    virtual Acad::ErrorStatus readPoint3d(AcGePoint3d* pVal) = 0;
    virtual Acad::ErrorStatus writePoint3d(const AcGePoint3d& val) = 0;
    virtual Acad::ErrorStatus readVector3d(AcGeVector3d* pVal) = 0;
    virtual Acad::ErrorStatus writeVector3d(const AcGeVector3d& val) = 0;
};

class AcDbDxfFiler
{
public:
    virtual ~AcDbDxfFiler() {}
    virtual Acad::ErrorStatus filerStatus() const = 0;
    virtual Acad::ErrorStatus setError(Acad::ErrorStatus es, const ACHAR* errMsg, ...) = 0;

    virtual Acad::ErrorStatus readResBuf(resbuf* pRb) = 0;
    virtual Acad::ErrorStatus pushBackItem() = 0;
    virtual bool atSubclassData(const ACHAR* subClassName) = 0;

    virtual Acad::ErrorStatus writeString(AcDb::DxfCode code, const ACHAR* pVal) = 0;
    virtual Acad::ErrorStatus writeString(AcDb::DxfCode code, const AcString& val) = 0;
    virtual Acad::ErrorStatus writeInt16(AcDb::DxfCode code, Adesk::Int16 val) = 0;
    virtual Acad::ErrorStatus writeDouble(AcDb::DxfCode code, double val, int prec = AcDb::kDfltPrec) = 0;
    virtual Acad::ErrorStatus writePoint3d(AcDb::DxfCode code, const AcGePoint3d& val, int prec = AcDb::kDfltPrec) = 0;
    virtual Acad::ErrorStatus writeVector3d(AcDb::DxfCode code, const AcGeVector3d& val, int prec = AcDb::kDfltPrec) = 0;

    Acad::ErrorStatus writeItem(AcDb::DxfCode code, const ACHAR* val) { return writeString(code, val); }
};

// ---------------------------------------------------------------------------
// Идентификатор объекта базы данных (нужен уже в AcGi)
// ---------------------------------------------------------------------------

class AcDbDatabase;

class AcDbObjectId
{
public:
    AcDbObjectId() : m_id(0) {}
    explicit AcDbObjectId(intptr_t id) : m_id(id) {}
    bool isNull() const { return m_id == 0; }
    intptr_t value() const { return m_id; }
    bool operator==(const AcDbObjectId& other) const { return m_id == other.m_id; }
    bool operator!=(const AcDbObjectId& other) const { return m_id != other.m_id; }
    bool operator<(const AcDbObjectId& other) const { return m_id < other.m_id; }

private:
    intptr_t m_id;
};

// Список групп DXF объекта (acdbEntGet) и преобразование идентификаторов в ads_name.
// В имитации ads_name — это идентификатор объекта; acdbEntGet возвращает группы,
// заданные тестом через mock::setEntGet(). Список освобождается через acutRelRb().
struct resbuf* acdbEntGet(const ads_name ent);
int acutRelRb(struct resbuf* rb);
Acad::ErrorStatus acdbGetAdsName(ads_name& objName, AcDbObjectId objId);
Acad::ErrorStatus acdbGetObjectId(AcDbObjectId& objId, const ads_name objName);

// ---------------------------------------------------------------------------
// AcGi
// ---------------------------------------------------------------------------

// Текстовый стиль для отрисовки. В имитации хранит только имя стиля и высоту.
class AcGiTextStyle
{
public:
    AcGiTextStyle(AcDbDatabase* pDb = nullptr) : m_pDb(pDb), m_textSize(1.0) {}
    virtual ~AcGiTextStyle() {}

    const ACHAR* styleName() const { return m_styleName.c_str(); }
    Acad::ErrorStatus setStyleName(const ACHAR* name)
    {
        m_styleName = name ? name : L"";
        return Acad::eOk;
    }
    double textSize() const { return m_textSize; }
    virtual void setTextSize(const double size) { m_textSize = size; }
    // В AutoCAD загружает шрифт; в имитации только запоминает вызов.
    virtual int loadStyleRec(AcDbDatabase* pDb = nullptr) const
    {
        m_loaded = true;
        if (pDb)
            m_pDb = pDb;
        return 0;
    }

    // Только для тестов.
    bool mockLoaded() const { return m_loaded; }
    AcDbDatabase* mockDatabase() const { return m_pDb; }

private:
    mutable AcDbDatabase* m_pDb;
    double m_textSize;
    std::wstring m_styleName;
    mutable bool m_loaded = false;
};

// Заполняет AcGiTextStyle по записи таблицы текстовых стилей (acgiutil.h).
Acad::ErrorStatus fromAcDbTextStyle(AcGiTextStyle& style, const AcDbObjectId& styleId);

class AcGiSubEntityTraits
{
public:
    virtual ~AcGiSubEntityTraits() {}
    virtual void setLayer(const AcDbObjectId layerId) = 0;
    virtual AcDbObjectId layerId() const = 0;
};

class AcGiDrawable;

class AcGiWorldGeometry
{
public:
    virtual ~AcGiWorldGeometry() {}
    // Матрица, применяемая ко всей последующей геометрии до popModelTransform().
    virtual Adesk::Boolean pushModelTransform(const AcGeMatrix3d& xMat) = 0;
    virtual Adesk::Boolean popModelTransform() = 0;
    // Рисует вложенный объект (например, примитив определения блока).
    virtual Adesk::Boolean draw(AcGiDrawable* pDrawable) const = 0;
    virtual Adesk::Boolean polyline(const Adesk::UInt32 nbPoints, const AcGePoint3d* pVertexList,
                                    const AcGeVector3d* pNormal = nullptr,
                                    Adesk::LongPtr lBaseSubEntMarker = -1) const = 0;
    virtual Adesk::Boolean text(const AcGePoint3d& position, const AcGeVector3d& normal,
                                const AcGeVector3d& direction, const double height,
                                const double width, const double oblique,
                                const ACHAR* pMsg) const = 0;
    virtual Adesk::Boolean text(const AcGePoint3d& position, const AcGeVector3d& normal,
                                const AcGeVector3d& direction, const ACHAR* pMsg,
                                const Adesk::Int32 length, const Adesk::Boolean raw,
                                const AcGiTextStyle& textStyle) const = 0;
};

class AcGiWorldDraw
{
public:
    virtual ~AcGiWorldDraw() {}
    virtual AcGiWorldGeometry& geometry() const = 0;
    virtual AcGiSubEntityTraits& subEntityTraits() const = 0;
    virtual Adesk::Boolean regenAbort() const = 0;
};

// ---------------------------------------------------------------------------
// AcRx
// ---------------------------------------------------------------------------

class AcRxObject;
typedef AcRxObject* (*AcRxCreateFunc)();

class AcRxClass
{
public:
    AcRxClass(const ACHAR* name, AcRxClass* parent, int dwgVer, int maintVer, int proxyFlags,
              AcRxCreateFunc create, const ACHAR* dxfName, const ACHAR* appName)
        : m_name(name), m_parent(parent), m_dwgVer(dwgVer), m_maintVer(maintVer),
          m_proxyFlags(proxyFlags), m_create(create), m_dxfName(dxfName ? dxfName : L""),
          m_appName(appName ? appName : L"") {}

    const ACHAR* name() const { return m_name.c_str(); }
    const ACHAR* dxfName() const { return m_dxfName.c_str(); }
    const ACHAR* appName() const { return m_appName.c_str(); }
    AcRxClass* myParent() const { return m_parent; }
    int proxyFlags() const { return m_proxyFlags; }
    void getClassVersion(int& dwgVer, int& maintVer) const { dwgVer = m_dwgVer; maintVer = m_maintVer; }
    AcRxObject* create() const { return m_create ? m_create() : nullptr; }
    bool isDerivedFrom(const AcRxClass* other) const
    {
        for (const AcRxClass* c = this; c; c = c->m_parent)
            if (c == other)
                return true;
        return false;
    }

private:
    std::wstring m_name;
    AcRxClass* m_parent;
    int m_dwgVer, m_maintVer, m_proxyFlags;
    AcRxCreateFunc m_create;
    std::wstring m_dxfName, m_appName;
};

class AcRxObject
{
public:
    virtual ~AcRxObject() {}
    virtual AcRxClass* isA() const = 0;
    bool isKindOf(const AcRxClass* c) const { return isA() && isA()->isDerivedFrom(c); }
};

// Всё, что умеет рисовать себя через AcGi; в ObjectARX — база AcDbObject.
class AcGiDrawable : public AcRxObject
{
};

// Реестр классов (имитация acrxClassDictionary).
AcRxClass* mockFindClass(const ACHAR* name);
AcRxClass* newAcRxClass(const ACHAR* className, const ACHAR* parentClassName,
                        int dwgVer, int maintVer, int proxyFlags, AcRxCreateFunc create,
                        const ACHAR* dxfName, const ACHAR* appName);
void deleteAcRxClass(AcRxClass* pClassObj);
void acrxBuildClassHierarchy();
bool acrxRegisterAppMDIAware(void* appId);
bool acrxUnlockApplication(void* appId);

template <class T>
AcRxObject* mockInstantiateClass() { return new T(); }

#define ACRX_DECLARE_MEMBERS(CLASS_NAME)                                         \
    AcRxClass* isA() const override;                                             \
    static AcRxClass* gpDesc;                                                    \
    static AcRxClass* desc();                                                    \
    static CLASS_NAME* cast(const AcRxObject* inPtr)                             \
    {                                                                            \
        return (inPtr == nullptr || !inPtr->isKindOf(CLASS_NAME::desc()))        \
            ? nullptr : (CLASS_NAME*)inPtr;                                      \
    }                                                                            \
    static void rxInit();                                                        \
    static const wchar_t* className() { return ACRX_T(#CLASS_NAME); }

#define ACRX_DXF_DEFINE_MEMBERS(CLASS_NAME, PARENT_CLASS, DWG_VERSION,           \
                                MAINTENANCE_VERSION, PROXY_FLAGS, DXF_NAME, APP) \
    AcRxClass* CLASS_NAME::gpDesc = nullptr;                                     \
    AcRxClass* CLASS_NAME::desc() { return gpDesc; }                             \
    AcRxClass* CLASS_NAME::isA() const { return gpDesc; }                        \
    void CLASS_NAME::rxInit()                                                    \
    {                                                                            \
        gpDesc = newAcRxClass(ACRX_T(#CLASS_NAME), ACRX_T(#PARENT_CLASS),        \
                              DWG_VERSION, MAINTENANCE_VERSION, PROXY_FLAGS,     \
                              &mockInstantiateClass<CLASS_NAME>,                 \
                              ACRX_T(#DXF_NAME), ACRX_T(#APP));                  \
    }

namespace AcRx
{
    enum AppMsgCode { kNullMsg = 0, kInitAppMsg = 1, kUnloadAppMsg = 2 };
    enum AppRetCode { kRetOK = 0, kRetError = 3 };
}

// ---------------------------------------------------------------------------
// AcDb
// ---------------------------------------------------------------------------

class AcDbObject : public AcGiDrawable
{
public:
    Acad::ErrorStatus close() { m_closeCount++; return Acad::eOk; }
    int closeCount() const { return m_closeCount; }
    // Сколько раз имитация «открывала» объект (итераторы, getAt с указателем);
    // у правильно закрытого объекта openCount() == closeCount().
    int openCount() const { return m_openCount; }
    void mockOpened() const { m_openCount++; }

    // Словарь расширений объекта; пустой идентификатор, если его нет.
    AcDbObjectId extensionDictionary() const { return m_extensionDictionary; }
    void mockSetExtensionDictionary(AcDbObjectId id) { m_extensionDictionary = id; }

    AcDbDatabase* database() const { return m_pDatabase; }
    AcDbObjectId objectId() const { return m_id; }
    // Только для имитации: объект становится резидентным в базе pDb.
    void mockAttach(AcDbDatabase* pDb, AcDbObjectId id) { m_pDatabase = pDb; m_id = id; }

    virtual Acad::ErrorStatus dwgInFields(AcDbDwgFiler* pFiler);
    virtual Acad::ErrorStatus dwgOutFields(AcDbDwgFiler* pFiler) const;
    virtual Acad::ErrorStatus dxfInFields(AcDbDxfFiler* pFiler);
    virtual Acad::ErrorStatus dxfOutFields(AcDbDxfFiler* pFiler) const;

protected:
    AcDbObject() : m_closeCount(0), m_pDatabase(nullptr) {}
    void assertReadEnabled() const {}
    void assertWriteEnabled() {}

private:
    int m_closeCount;
    mutable int m_openCount = 0;
    AcDbDatabase* m_pDatabase;
    AcDbObjectId m_id;
    AcDbObjectId m_extensionDictionary;
};

class AcDbEntity : public AcDbObject
{
public:
    static AcRxClass* desc();
    AcRxClass* isA() const override { return desc(); }

    // Публичные (в ObjectARX — sealed) обёртки над sub*-методами.
    Adesk::Boolean worldDraw(AcGiWorldDraw* pWd) { return subWorldDraw(pWd); }
    void list() const { subList(); }
    Acad::ErrorStatus transformBy(const AcGeMatrix3d& xform) { return subTransformBy(xform); }
    Acad::ErrorStatus getGeomExtents(AcDbExtents& extents) const { return subGetGeomExtents(extents); }
    Acad::ErrorStatus getGripPoints(AcGePoint3dArray& gripPoints, AcDbIntArray& osnapModes,
                                    AcDbIntArray& geomIds) const
    {
        return subGetGripPoints(gripPoints, osnapModes, geomIds);
    }
    Acad::ErrorStatus moveGripPointsAt(const AcDbIntArray& indices, const AcGeVector3d& offset)
    {
        return subMoveGripPointsAt(indices, offset);
    }

    // Как в AutoCAD, назначает объекту текущий слой базы данных (CLAYER).
    void setDatabaseDefaults(AcDbDatabase* pDb);
    AcDbDatabase* defaultsDatabase() const { return m_pDefaultsDb; }

    Acad::ErrorStatus layer(AcString& name) const { name = m_layer; return Acad::eOk; }
    // Идентификатор слоя по имени в базе объекта; пустой, если слоя нет.
    AcDbObjectId layerId() const;
    void setLayer(const AcString& layer) { m_layer = layer; }

    Acad::ErrorStatus dwgInFields(AcDbDwgFiler* pFiler) override;
    Acad::ErrorStatus dwgOutFields(AcDbDwgFiler* pFiler) const override;
    Acad::ErrorStatus dxfInFields(AcDbDxfFiler* pFiler) override;
    Acad::ErrorStatus dxfOutFields(AcDbDxfFiler* pFiler) const override;

protected:
    AcDbEntity() : m_pDefaultsDb(nullptr), m_layer(L"0") {}

    virtual Adesk::Boolean subWorldDraw(AcGiWorldDraw*) { return Adesk::kTrue; }
    virtual void subList() const {}
    virtual Acad::ErrorStatus subTransformBy(const AcGeMatrix3d&) { return Acad::eNotApplicable; }
    virtual Acad::ErrorStatus subGetGeomExtents(AcDbExtents&) const { return Acad::eNotApplicable; }
    virtual Acad::ErrorStatus subGetGripPoints(AcGePoint3dArray&, AcDbIntArray&, AcDbIntArray&) const
    {
        return Acad::eNotApplicable;
    }
    virtual Acad::ErrorStatus subGetGripPoints(void* /*grips*/, double /*curViewUnitSize*/) const
    {
        return Acad::eNotApplicable;
    }
    virtual Acad::ErrorStatus subMoveGripPointsAt(const AcDbIntArray&, const AcGeVector3d&)
    {
        return Acad::eNotApplicable;
    }
    virtual Acad::ErrorStatus subMoveGripPointsAt(void* /*gripAppData*/, const AcGeVector3d&, int)
    {
        return Acad::eNotApplicable;
    }

private:
    AcDbDatabase* m_pDefaultsDb;
    AcString m_layer;
};

class AcDbProxyEntity
{
public:
    enum GraphicsMetafileType { kNoMetafile = 0 };
    enum { kNoOperation = 0, kAllAllowedBits = 0x3FF };
};

#define ACDB_MODEL_SPACE _T("*Model_Space")

// Отрезок (dbents.h) — простой примитив для наполнения определений блоков в тестах.
class AcDbLine : public AcDbEntity
{
public:
    AcDbLine() {}
    AcDbLine(const AcGePoint3d& start, const AcGePoint3d& end) : m_start(start), m_end(end) {}
    AcGePoint3d startPoint() const { return m_start; }
    AcGePoint3d endPoint() const { return m_end; }

protected:
    Adesk::Boolean subWorldDraw(AcGiWorldDraw* pWd) override;
    Acad::ErrorStatus subGetGeomExtents(AcDbExtents& extents) const override;

private:
    AcGePoint3d m_start, m_end;
};

// Определение атрибута (dbents.h). Рисует свой тег, как в AutoCAD.
class AcDbAttributeDefinition : public AcDbEntity
{
public:
    static AcRxClass* desc();
    AcRxClass* isA() const override { return desc(); }
    static AcDbAttributeDefinition* cast(const AcRxObject* inPtr)
    {
        return (inPtr == nullptr || !inPtr->isKindOf(desc()))
            ? nullptr : (AcDbAttributeDefinition*)inPtr;
    }

    explicit AcDbAttributeDefinition(const ACHAR* tag = L"TAG") : m_tag(tag) {}
    const ACHAR* tagConst() const { return m_tag.c_str(); }

protected:
    Adesk::Boolean subWorldDraw(AcGiWorldDraw* pWd) override;
    Acad::ErrorStatus subGetGeomExtents(AcDbExtents& extents) const override;

private:
    std::wstring m_tag;
};

// Таблицы символов. Имена сравниваются без учёта регистра, как в AutoCAD.
class AcDbSymbolTableRecord : public AcDbObject
{
public:
    AcRxClass* isA() const override { return nullptr; }
    Acad::ErrorStatus getName(AcString& name) const { name = m_name.c_str(); return Acad::eOk; }
    Acad::ErrorStatus setName(const ACHAR* pName);
    const std::wstring& mockName() const { return m_name; }
    // Только для имитации: имя без проверки (например, "*Model_Space" или "*U1").
    void mockSetName(const ACHAR* pName) { m_name = pName; }

private:
    std::wstring m_name;
};

class AcDbBlockTableRecordIterator;

class AcDbBlockTableRecord : public AcDbSymbolTableRecord
{
public:
    ~AcDbBlockTableRecord() override;
    Acad::ErrorStatus appendAcDbEntity(AcDbObjectId& id, AcDbEntity* pEntity);
    Acad::ErrorStatus newIterator(AcDbBlockTableRecordIterator*& pIterator,
                                  bool atBeginning = true, bool skipDeleted = true) const;
    AcGePoint3d origin() const { return mockOrigin; }
    bool isLayout() const { return mockIsLayout; }
    bool isAnonymous() const { return mockIsAnonymous; }
    bool isFromExternalReference() const { return mockIsXref; }

    // Объекты, добавленные в запись; принадлежат записи и удаляются вместе с ней.
    std::vector<AcDbEntity*> entities;
    // Свойства определения блока, которые задаёт тест.
    AcGePoint3d mockOrigin;
    bool mockIsLayout = false;
    bool mockIsAnonymous = false;
    bool mockIsXref = false;
    // Признак динамического блока для AcDbDynBlockReference::isDynamicBlock().
    bool mockIsDynamic = false;
};

// Обход примитивов определения блока. getEntity() «открывает» примитив
// (AcDbObject::openCount), вызывающий обязан его закрыть.
class AcDbBlockTableRecordIterator
{
public:
    explicit AcDbBlockTableRecordIterator(const std::vector<AcDbEntity*>& entities)
        : m_entities(entities), m_index(0) {}

    void start(bool atBeginning = true, bool /*skipDeleted*/ = true)
    {
        m_index = atBeginning ? 0 : m_entities.size();
    }
    bool done() const { return m_index >= m_entities.size(); }
    void step(bool /*forward*/ = true, bool /*skipDeleted*/ = true) { ++m_index; }
    Acad::ErrorStatus getEntityId(AcDbObjectId& entityId) const;
    Acad::ErrorStatus getEntity(AcDbEntity*& pEntity, AcDb::OpenMode openMode = AcDb::kForRead,
                                bool openErasedEntity = false) const;

private:
    std::vector<AcDbEntity*> m_entities;
    size_t m_index;
};

// Определение динамического блока (dbdynblk.h).
class AcDbDynBlockReference
{
public:
    static bool isDynamicBlock(AcDbObjectId blockTableRecordId);
};

class AcDbLayerTableRecord : public AcDbSymbolTableRecord
{
};

class AcDbTextStyleTableRecord : public AcDbSymbolTableRecord
{
public:
    Adesk::Boolean isShapeFile() const { return m_isShapeFile; }
    void setIsShapeFile(Adesk::Boolean shape) { m_isShapeFile = shape; }

private:
    Adesk::Boolean m_isShapeFile = Adesk::kFalse;
};

class AcDbSymbolTable : public AcDbObject
{
public:
    explicit AcDbSymbolTable(AcDbDatabase* pDb) : m_pDb(pDb) {}
    ~AcDbSymbolTable() override;
    AcRxClass* isA() const override { return nullptr; }

    Acad::ErrorStatus getAt(const ACHAR* entryName, AcDbObjectId& recordId,
                            bool getErasedRecord = false) const;
    bool has(const ACHAR* name) const
    {
        AcDbObjectId id;
        return getAt(name, id) == Acad::eOk;
    }

    // Записи в порядке добавления; принадлежат таблице.
    std::vector<AcDbSymbolTableRecord*> records;
    int addCalls = 0;

protected:
    Acad::ErrorStatus addRecord(AcDbSymbolTableRecord* pRecord);

private:
    AcDbDatabase* m_pDb;
};

template <class RecordType>
class MockSymbolTableIterator
{
public:
    explicit MockSymbolTableIterator(const std::vector<AcDbSymbolTableRecord*>& records)
        : m_records(records), m_index(0) {}

    void start(bool atBeginning = true, bool /*skipDeleted*/ = true)
    {
        m_index = atBeginning ? 0 : m_records.size();
    }
    bool done() const { return m_index >= m_records.size(); }
    Acad::ErrorStatus step(bool /*forward*/ = true, bool /*skipDeleted*/ = true)
    {
        ++m_index;
        return Acad::eOk;
    }
    Acad::ErrorStatus getRecordId(AcDbObjectId& id) const
    {
        if (done())
            return Acad::eInvalidInput;
        id = m_records[m_index]->objectId();
        return Acad::eOk;
    }
    Acad::ErrorStatus getRecord(RecordType*& pRecord, AcDb::OpenMode /*openMode*/ = AcDb::kForRead,
                                bool /*openErasedRec*/ = false) const
    {
        if (done())
            return Acad::eInvalidInput;
        pRecord = static_cast<RecordType*>(m_records[m_index]);
        pRecord->mockOpened();
        return Acad::eOk;
    }

private:
    std::vector<AcDbSymbolTableRecord*> m_records;
    size_t m_index;
};

typedef MockSymbolTableIterator<AcDbLayerTableRecord> AcDbLayerTableIterator;
typedef MockSymbolTableIterator<AcDbTextStyleTableRecord> AcDbTextStyleTableIterator;

class AcDbLayerTable : public AcDbSymbolTable
{
public:
    using AcDbSymbolTable::AcDbSymbolTable;
    Acad::ErrorStatus add(AcDbLayerTableRecord* pRecord) { return addRecord(pRecord); }
    Acad::ErrorStatus newIterator(AcDbLayerTableIterator*& pIterator, bool atBeginning = true,
                                  bool skipDeleted = true) const
    {
        pIterator = new AcDbLayerTableIterator(records);
        pIterator->start(atBeginning, skipDeleted);
        return Acad::eOk;
    }
};

class AcDbTextStyleTable : public AcDbSymbolTable
{
public:
    using AcDbSymbolTable::AcDbSymbolTable;
    Acad::ErrorStatus add(AcDbTextStyleTableRecord* pRecord) { return addRecord(pRecord); }
    Acad::ErrorStatus newIterator(AcDbTextStyleTableIterator*& pIterator, bool atBeginning = true,
                                  bool skipDeleted = true) const
    {
        pIterator = new AcDbTextStyleTableIterator(records);
        pIterator->start(atBeginning, skipDeleted);
        return Acad::eOk;
    }
};

// Проверка имён символов (dbsymutl.h): пустые имена и запрещённые символы отклоняются.
namespace AcDbSymbolUtilities
{
    class Services
    {
    public:
        Acad::ErrorStatus validateSymbolName(const ACHAR* name, bool allowVerticalBar) const;
    };
}
const AcDbSymbolUtilities::Services* acdbSymUtil();

typedef MockSymbolTableIterator<AcDbBlockTableRecord> AcDbBlockTableIterator;

// Таблица блоков: пространство модели (принадлежит базе) и определения блоков
// из records. getAt() с указателем «открывает» запись — её нужно закрыть.
class AcDbBlockTable : public AcDbSymbolTable
{
public:
    using AcDbSymbolTable::AcDbSymbolTable;
    using AcDbSymbolTable::getAt;
    Acad::ErrorStatus add(AcDbBlockTableRecord* pRecord) { return addRecord(pRecord); }
    Acad::ErrorStatus getAt(const ACHAR* entryName, AcDbBlockTableRecord*& pRec,
                            AcDb::OpenMode openMode, bool openErasedRec = false) const;
    Acad::ErrorStatus newIterator(AcDbBlockTableIterator*& pIterator, bool atBeginning = true,
                                  bool skipDeleted = true) const;
    AcDbBlockTableRecord* modelSpace = nullptr;
    mutable AcDb::OpenMode lastOpenMode = AcDb::kForRead;
    mutable int getAtCalls = 0;
};

class AcDbDatabase
{
public:
    AcDbDatabase();
    ~AcDbDatabase();
    Acad::ErrorStatus getBlockTable(AcDbBlockTable*& pTable, AcDb::OpenMode mode);
    Acad::ErrorStatus getLayerTable(AcDbLayerTable*& pTable, AcDb::OpenMode mode);
    Acad::ErrorStatus getTextStyleTable(AcDbTextStyleTable*& pTable, AcDb::OpenMode mode);

    // Только для имитации: создаёт запись таблицы с заданным именем.
    AcDbObjectId mockAddLayer(const ACHAR* name);
    AcDbObjectId mockAddTextStyle(const ACHAR* name, bool isShapeFile = false);
    // Создаёт пустое определение блока (имя не проверяется); nullptr, если имя занято.
    AcDbBlockTableRecord* mockAddBlock(const ACHAR* name);

    AcDbBlockTable blockTable;
    AcDbBlockTableRecord modelSpace;
    // Слой "0" и стиль "Standard" есть в любой базе.
    AcDbLayerTable layerTable;
    AcDbTextStyleTable textStyleTable;
    // Текущий слой (CLAYER) для setDatabaseDefaults().
    AcString clayer;
    // Последний режим, в котором открывались таблицы слоёв и стилей.
    AcDb::OpenMode lastLayerTableMode = AcDb::kForRead;
    AcDb::OpenMode lastTextStyleTableMode = AcDb::kForRead;
};

class AcDbHostApplicationServices
{
public:
    AcDbDatabase* workingDatabase() const;
};

AcDbHostApplicationServices* acdbHostApplicationServices();

// ---------------------------------------------------------------------------
// Команды
// ---------------------------------------------------------------------------

#define ACRX_CMD_MODAL 0x00000000
typedef void (*AcRxFunctionPtr)();

class AcEdCommandStack
{
public:
    struct Command
    {
        std::wstring group, globalName, localName;
        Adesk::Int32 flags;
        AcRxFunctionPtr function;
    };

    Acad::ErrorStatus addCommand(const ACHAR* cmdGroupName, const ACHAR* cmdGlobalName,
                                 const ACHAR* cmdLocalName, Adesk::Int32 commandFlags,
                                 AcRxFunctionPtr functionAddr);
    Acad::ErrorStatus removeGroup(const ACHAR* groupName);
    const Command* lookupGlobalCmd(const ACHAR* cmdName) const;

    std::vector<Command> commands;
};

AcEdCommandStack* mockCommandStack();
#define acedRegCmds mockCommandStack()

// ---------------------------------------------------------------------------
// Управление имитацией из тестов
// ---------------------------------------------------------------------------

namespace mock
{
    // Результат следующего acedGetPoint().
    extern int getPointResult;
    extern AcGePoint3d getPointValue;
    extern std::wstring lastPrompt;
    // Матрица ПСК -> МСК, которую вернёт acedGetCurrentUCS().
    extern AcGeMatrix3d currentUcs;
    // Сообщения, выведенные через acutPrintf().
    extern std::vector<std::wstring> printed;
    // Счётчики вызовов служебных функций.
    extern int mdiAwareCalls;
    extern int unlockCalls;
    extern int buildHierarchyCalls;

    // Группа DXF, которую вернёт acdbEntGet() (см. setEntGet).
    struct DxfGroup
    {
        enum class Type { String, Name, Int16 };
        short code;
        Type type;
        std::wstring str;
        AcDbObjectId id;
        short i;

        static DxfGroup text(short code, const wchar_t* value);
        static DxfGroup name(short code, AcDbObjectId value);
        static DxfGroup int16(short code, short value);
    };

    // Задаёт ответ acdbEntGet() для объекта id (словаря, графа, узла параметра).
    // Перед группами автоматически добавляется (-1 . id), как в AutoCAD.
    void setEntGet(AcDbObjectId id, const std::vector<DxfGroup>& groups);
    // Новый уникальный идентификатор для неграфических объектов имитации.
    AcDbObjectId newId();
    // Число вызовов acdbEntGet() и ещё не освобождённых через acutRelRb() узлов resbuf.
    extern int entGetCalls;
    extern int liveResbufs;

    // Пересоздаёт рабочую базу данных и сбрасывает состояние имитации.
    void reset();
}
