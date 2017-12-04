#define THEORIDE_DO_PROCESS

#include <iostream>
#include <sstream>

template<typename tType>
std::string getString(tType iValue , unsigned=0)
{
    return std::to_string(iValue);
}

constexpr char const*   kStr="abc";
const char kStr2[]="def";

enum class EnumTest
{
    Symbol0,
    Symbol1,
    Symbol2
};

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

class PublicBase
{
    unsigned long   mULong;
public:
    PublicBase(unsigned long iULong) : mULong(iULong) { }
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

class ProtectedBase
{
    unsigned int    mUInt;
public:
    ProtectedBase(unsigned int iUInt) : mUInt(iUInt) { }
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
class PrivateBase
{
    unsigned short  mUShort;
public:
    PrivateBase(unsigned short iUShort) : mUShort(iUShort) { }
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
    EnumTest aEnumTest = EnumTest::Symbol2;
    std::cout << "aEnumTest = " << aEnumTest << "\n";

    PublicBase   aPublicBase(123);
    std::cout << "aPublicBase :" << aPublicBase << "\n";

    ClassTest   aClassTest(EnumTest::Symbol1, 123);
    std::cout << "aClassTest :" << aClassTest << "\n";
}
