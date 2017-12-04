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
};

int main()
{
}
