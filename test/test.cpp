#define _CRT_SECURE_NO_WARNINGS
#define THEORIDE_DO_PROCESS

//----------------------------------------------------------------------------
//      基本型の出力用
//----------------------------------------------------------------------------

#include <iostream>
#include <sstream>

template<typename tType>
std::string getString(tType iValue , unsigned=0)
{
    return std::to_string(iValue);
}

//----------------------------------------------------------------------------
//      enum型
//----------------------------------------------------------------------------

enum class EnumTest
{
    Symbol0,
    Symbol1,
    Symbol2
};

// 自動生成指示
void GenerationMarkerStart(EnumTest, char const* inc="enum.inc");
#define THEORIDE_ENUM EnumTest
#define THEORIDE_ENUM_LIST()\
    THEORIDE_ELEMENT(Symbol0)\
    THEORIDE_ELEMENT(Symbol1)\
    THEORIDE_ELEMENT(Symbol2)
#include "enum.inc"
#undef THEORIDE_ENUM_LIST
#undef THEORIDE_ENUM
void GenerationMarkerEnd(EnumTest);

//----------------------------------------------------------------------------
//      非侵入型
//----------------------------------------------------------------------------

#include <ctime>

// 自動生成指示
void GenerationMarkerStart(tm, char const* inc="non_intrusive.inc");
#define THEORIDE_CLASS tm
#define THEORIDE_BASE_LIST()
#define THEORIDE_ELEMENT_LIST()\
    THEORIDE_ELEMENT_PUBLIC(tm_sec)\
    THEORIDE_ELEMENT_PUBLIC(tm_min)\
    THEORIDE_ELEMENT_PUBLIC(tm_hour)\
    THEORIDE_ELEMENT_PUBLIC(tm_mday)\
    THEORIDE_ELEMENT_PUBLIC(tm_mon)\
    THEORIDE_ELEMENT_PUBLIC(tm_year)\
    THEORIDE_ELEMENT_PUBLIC(tm_wday)\
    THEORIDE_ELEMENT_PUBLIC(tm_yday)\
    THEORIDE_ELEMENT_PUBLIC(tm_isdst)
#include "non_intrusive.inc"
#undef THEORIDE_ELEMENT_LIST
#undef THEORIDE_BASE_LIST
#undef THEORIDE_CLASS
void GenerationMarkerEnd(tm);

//----------------------------------------------------------------------------
//      基底クラス
//----------------------------------------------------------------------------

class Base
{
    int     mInt;
public:
    Base(int iInt) : mInt(iInt) { }

    // 自動生成指示
    void GenerationMarkerStart(Base, char const* inc="intrusive.inc");
    #define THEORIDE_CLASS Base
    #define THEORIDE_BASE_LIST()
    #define THEORIDE_ELEMENT_LIST()\
        THEORIDE_ELEMENT_PRIVATE(mInt)
    #include "intrusive.inc"
    #undef THEORIDE_ELEMENT_LIST
    #undef THEORIDE_BASE_LIST
    #undef THEORIDE_CLASS
    void GenerationMarkerEnd(Base);
};

//----------------------------------------------------------------------------
//      派生クラス
//----------------------------------------------------------------------------

class Derived : public Base
{
    tm          mTm;
    EnumTest    mPrivate;
protected:
    int         mProtected;
public:
    long        mPublic;
    Derived(EnumTest iEnumTest, int iData) :
        Base(iData*100),
        mPrivate(iEnumTest),
        mProtected(iData*10),
        mPublic(iData)
    {
        time_t  now = time(nullptr);
        mTm = *localtime(&now);
    }

    // 自動生成指示
    void GenerationMarkerStart(Derived, char const* inc="intrusive.inc");
    #define THEORIDE_CLASS Derived
    #define THEORIDE_BASE_LIST()\
        THEORIDE_BASE_PUBLIC(Base)
    #define THEORIDE_ELEMENT_LIST()\
        THEORIDE_ELEMENT_PRIVATE(mTm)\
        THEORIDE_ELEMENT_PRIVATE(mPrivate)\
        THEORIDE_ELEMENT_PROTECTED(mProtected)\
        THEORIDE_ELEMENT_PUBLIC(mPublic)
    #include "intrusive.inc"
    #undef THEORIDE_ELEMENT_LIST
    #undef THEORIDE_BASE_LIST
    #undef THEORIDE_CLASS
    void GenerationMarkerEnd(Derived);
};

int main()
{
    Derived   aDerived(EnumTest::Symbol1, 123);
    std::cout << "aDerived :" << aDerived << "\n";
}
