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
//      基底クラス
//----------------------------------------------------------------------------

class PublicBase
{
    unsigned long   mULong;
public:
    PublicBase(unsigned long iULong) : mULong(iULong) { }

    // 自動生成指示
    void GenerationMarkerStart(PublicBase, char const* inc="intrusive.inc");
    #define THEORIDE_CLASS PublicBase
    #define THEORIDE_BASE_LIST()
    #define THEORIDE_ELEMENT_LIST()\
        THEORIDE_ELEMENT_PRIVATE(mULong)
    #include "intrusive.inc"
    #undef THEORIDE_ELEMENT_LIST
    #undef THEORIDE_BASE_LIST
    #undef THEORIDE_CLASS
    void GenerationMarkerEnd(PublicBase);
};

//----------------------------------------------------------------------------
//      基底クラス
//----------------------------------------------------------------------------

class ProtectedBase
{
    unsigned int    mUInt;
public:
    ProtectedBase(unsigned int iUInt) : mUInt(iUInt) { }

    // 自動生成指示
    void GenerationMarkerStart(ProtectedBase, char const* inc="intrusive.inc");
    #define THEORIDE_CLASS ProtectedBase
    #define THEORIDE_BASE_LIST()
    #define THEORIDE_ELEMENT_LIST()\
        THEORIDE_ELEMENT_PRIVATE(mUInt)
    #include "intrusive.inc"
    #undef THEORIDE_ELEMENT_LIST
    #undef THEORIDE_BASE_LIST
    #undef THEORIDE_CLASS
    void GenerationMarkerEnd(ProtectedBase);
};

//----------------------------------------------------------------------------
//      基底クラス
//----------------------------------------------------------------------------

class PrivateBase
{
    unsigned short  mUShort;
public:
    PrivateBase(unsigned short iUShort) : mUShort(iUShort) { }

    // 自動生成指示
    void GenerationMarkerStart(PrivateBase, char const* inc="intrusive.inc");
    #define THEORIDE_CLASS PrivateBase
    #define THEORIDE_BASE_LIST()
    #define THEORIDE_ELEMENT_LIST()\
        THEORIDE_ELEMENT_PRIVATE(mUShort)
    #include "intrusive.inc"
    #undef THEORIDE_ELEMENT_LIST
    #undef THEORIDE_BASE_LIST
    #undef THEORIDE_CLASS
    void GenerationMarkerEnd(PrivateBase);
};

//----------------------------------------------------------------------------
//      派生クラス
//----------------------------------------------------------------------------

class ClassTest : public PublicBase, protected ProtectedBase, private PrivateBase
{
    EnumTest    mEnumTest;
    int         mInt;
public:
    ClassTest(EnumTest iEnumTest, int iInt) :
        PublicBase(iInt*1000),
        ProtectedBase(iInt*100),
        PrivateBase(static_cast<unsigned short>(iInt*10)),
        mEnumTest(iEnumTest),
        mInt(iInt)
    { }

    // 自動生成指示
    void GenerationMarkerStart(ClassTest, char const* inc="intrusive.inc");
    #define THEORIDE_CLASS ClassTest
    #define THEORIDE_BASE_LIST()\
        THEORIDE_BASE_PUBLIC(PublicBase)\
        THEORIDE_BASE_PROTECTED(ProtectedBase)\
        THEORIDE_BASE_PRIVATE(PrivateBase)
    #define THEORIDE_ELEMENT_LIST()\
        THEORIDE_ELEMENT_PRIVATE(mEnumTest)\
        THEORIDE_ELEMENT_PRIVATE(mInt)
    #include "intrusive.inc"
    #undef THEORIDE_ELEMENT_LIST
    #undef THEORIDE_BASE_LIST
    #undef THEORIDE_CLASS
    void GenerationMarkerEnd(ClassTest);
};

int main()
{
    ClassTest   aClassTest(EnumTest::Symbol1, 123);
    std::cout << "aClassTest :" << aClassTest << "\n";
}
