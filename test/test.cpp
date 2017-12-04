#define THEORIDE_DO_PROCESS

//#include <iostream>

constexpr char const*   kStr="abc";
const char kStr2[]="def";

enum EnumTest
{
    Symbol0,
    Symbol1,
    Symbol2
};

void GenerationMarkerStart(EnumTest, char const* inc="Enum.inc");
#define THEORIDE_ENUM EnumTest
#define THEORIDE_ENUM_LIST()\
    THEORIDE_ELEMENT(Symbol0)\
    THEORIDE_ELEMENT(Symbol1)\
    THEORIDE_ELEMENT(Symbol2)
#include "Enum.inc"
#undef THEORIDE_ENUM_LIST
#undef THEORIDE_ENUM
void GenerationMarkerEnd(EnumTest);

class PublicBase
{
    unsigned int    mUInt;
};
class ProtectedBase
{
    unsigned short  mUShort;
};
class PrivateBase
{
    unsigned char   mUChar;
};

class ClassTest : public PublicBase, protected ProtectedBase, private PrivateBase
{
    EnumTest    mEnumTest;
    int         mInt;
public:
    ClassTest(EnumTest iEnumTest, int iInt) :
        mEnumTest(iEnumTest),
        mInt(iInt)
    { }

    void GenerationMarkerStart(ClassTest, char const* inc="Class.inc");
#define THEORIDE_CLASS ClassTest
#define THEORIDE_BASE_LIST()\
    THEORIDE_BASE_PUBLIC(PublicBase)\
    THEORIDE_BASE_PROTECTED(ProtectedBase)\
    THEORIDE_BASE_PRIVATE(PrivateBase)
#define THEORIDE_ELEMENT_LIST()\
    THEORIDE_ELEMENT_PRIVATE(mEnumTest)\
    THEORIDE_ELEMENT_PRIVATE(mInt)
#include "Class.inc"
#undef THEORIDE_ELEMENT_LIST
#undef THEORIDE_BASE_LIST
#undef THEORIDE_CLASS
void GenerationMarkerEnd(ClassTest);
};

int main()
{
}
