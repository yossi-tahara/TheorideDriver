# マクロとlibToolingでリフレクションにトライしてみた

このリポジトリは、[Qiita](https://qiita.com/trend)の[C++ Advent Calendar 2017](https://qiita.com/advent-calendar/2017/cpp)12月6日の[投稿](https://qiita.com/Chironian/items/fb146738412a10def7f8)で解説したプログラムです。

タイトルの通りです。

## 次のようなプログラムを書くと、

```
#define _CRT_SECURE_NO_WARNINGS
#define THEORIDE_DO_PROCESS

//----------------------------------------------------------------------------
//      基本型の出力用
//----------------------------------------------------------------------------

#include <iostream>
#include <sstream>

template<typename tType>
std::string to_string(tType iValue , unsigned=0)
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

//----------------------------------------------------------------------------
//      非侵入型
//----------------------------------------------------------------------------

#include <ctime>

// 自動生成指示
void GenerationMarkerStart(tm, char const* inc="non_intrusive.inc");

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
};

//----------------------------------------------------------------------------
//      メイン・ルーチン
//----------------------------------------------------------------------------

int main()
{
    Derived   aDerived(EnumTest::Symbol1, 123);
    std::cout << "aDerived :" << aDerived << "\n";
}
```

## こんな出力を得られる簡単なサンプルがついてます

```
{
    "<Class Name>":"Derived",
    "(Base)":
    {
        "<Class Name>":"Base",
        "mInt":12300
    },
    "mTm":
    {
        "<Class Name>":"tm",
        "tm_sec":30,
        "tm_min":19,
        "tm_hour":22,
        "tm_mday":5,
        "tm_mon":11,
        "tm_year":117,
        "tm_wday":2,
        "tm_yday":338,
        "tm_isdst":0
    },
    "mPrivate":"EnumTest::Symbol1",
    "mProtected":1230,
    "mPublic":123
}
```

## 何をするかは使う人の発想しだいっす