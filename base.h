﻿//############################################################################
//      Theolizer全体のベース部
/*
    © 2016 Theoride Technology (http://theolizer.com/) All Rights Reserved.
    "Theolizer" is a registered trademark of Theoride Technology.

    "Theolizer" License
        In the case where you are in possession of a valid “Theolizer” License,
        you may use this file in accordance with the terms and conditions of 
        the use license determined by Theoride Technology.

    General Public License Version 3 ("GPLv3")
        You may use this file in accordance with the terms and conditions of 
        GPLv3 published by Free Software Foundation.
        Please confirm the contents of GPLv3 at https://www.gnu.org/licenses/gpl.txt .
        A copy of GPLv3 is also saved in a LICENSE.TXT file.

    商用ライセンス
        あなたが有効なTheolizer商用ライセンスを保持している場合、
        セオライド テクノロジーの定める使用許諾書の条件に従って、
        このファイルを取り扱うことができます。

    General Public License Version 3(以下GPLv3)
        Free Software Foundationが公表するGPLv3の使用条件に従って、
        あなたはこのファイルを取り扱うことができます。
        GPLv3の内容を https://www.gnu.org/licenses/gpl.txt にて確認して下さい。
        またGPLv3のコピーをLICENSE.TXTファイルにおいてます。
*/
//############################################################################

#if !defined(THEOLIZER_INTERNAL_BASE_H)
#define THEOLIZER_INTERNAL_BASE_H

#if defined(__GNUC__)
    #include <cxxabi.h> // for Demangle for GCC
#endif

#include "rbfor.h"   // インクルード漏れ防止のため最初

#include <iostream>
#include <string>
#include <vector>
#include <memory>       // for unique_ptr
#include <chrono>

#include <type_traits>
#include <typeindex>

//############################################################################
//      Begin
//############################################################################

namespace theolizer
{

//############################################################################
//      メモリリーク検出用(MSVCのみ有効)
//          boost 1.64.0を使う時、これが原因でエラーになるので止める。
//############################################################################

#if defined(_MSC_VER) && defined(_DEBUG)
//  #define new new(_NORMAL_BLOCK, THEOLIZER_INTERNAL_FILE, __LINE__)
#endif

//############################################################################
//      管理用定義
//############################################################################

// ***************************************************************************
//      文字列
// ***************************************************************************

#define THEOLIZER_INTERNAL_LICENSE      ""  //"licensed under GPLv3 or lator."

#define THEOLIZER_INTERNAL_PRODUCT_NAME "Theolizer"

#define THEOLIZER_INTERNAL_COPYRIGHT    "Copyright (C) 2016 Yohinori Tahara (Theoride Technology)"

#define THEOLIZER_INTERNAL_BUG          "Theolizer bug. Please report to http://theoride.com"

// ***************************************************************************
//      重要機能の許可／禁止
// ***************************************************************************

// メタ・シリアライザ・コードを出力する
#define THEOLIZER_INTERNAL_ENABLE_META_SERIALIZER

//############################################################################
//      ユーティリティ
//############################################################################

namespace internal
{
#ifndef THEOLIZER_INTERNAL_DOXYGEN

// ***************************************************************************
//      MSVCの警告を止める
//          意図的な使用に対して警告を止めるために用いる
// ***************************************************************************

#if defined(__GNUC__)
    #define THEOLIZER_INTERNAL_UNUSED   __attribute__ ((unused))
#else
    #define THEOLIZER_INTERNAL_UNUSED
#endif

// ***************************************************************************
//      文字列化
// ***************************************************************************

#define THEOLIZER_INTERNAL_STRINGIZE(...)       THEOLIZER_INTERNAL_STRINGIZE_I(__VA_ARGS__)
#define THEOLIZER_INTERNAL_STRINGIZE_I(...)     #__VA_ARGS__ ""

// ***************************************************************************
//      シンボルの結合
// ***************************************************************************

#define THEOLIZER_INTERNAL_CAT(a, b)            THEOLIZER_INTERNAL_CAT_I(a, b)
#define THEOLIZER_INTERNAL_CAT_I(a, b)          a ## b

// ***************************************************************************
//      先頭取り出し
// ***************************************************************************

#define THEOLIZER_INTERNAL_FIRST(dFirst, ...)   dFirst

// ***************************************************************************
//      マクロ展開処理
// ***************************************************************************

#define THEOLIZER_INTERNAL_EXPAND(P)  P

// ***************************************************************************
//      マクロFOR
//          1個から8個までループ可能
//          THEOLIZER_INTERNAL_FOR(MACRO, ...)
//              ...部に1個から8個の要素を書くことで、下記が要素数分展開される
//              MACRO(要素)
// ***************************************************************************

#define THEOLIZER_INTERNAL_FOR(S,...)                                       \
    static_assert(                                                          \
        (!theolizer::internal::isEmpty(THEOLIZER_INTERNAL_STRINGIZE(__VA_ARGS__)))\
     && (theolizer::internal::countComma(THEOLIZER_INTERNAL_STRINGIZE(__VA_ARGS__))<8),\
        "Bad number of elements(1..8) in THEOLIZER_INTERNAL_FOR.");         \
    THEOLIZER_INTERNAL_FOR_I(THEOLIZER_INTERNAL_FORR(__VA_ARGS__),S,__VA_ARGS__)

//      ---<<< 内部処理用 >>>---

#define THEOLIZER_INTERNAL_FORX1(S,P,...) S(P)
#define THEOLIZER_INTERNAL_FORX2(S,P,...) S(P);\
    THEOLIZER_INTERNAL_EXPAND(THEOLIZER_INTERNAL_FORX1(S,__VA_ARGS__))
#define THEOLIZER_INTERNAL_FORX3(S,P,...) S(P);\
    THEOLIZER_INTERNAL_EXPAND(THEOLIZER_INTERNAL_FORX2(S,__VA_ARGS__))
#define THEOLIZER_INTERNAL_FORX4(S,P,...) S(P);\
    THEOLIZER_INTERNAL_EXPAND(THEOLIZER_INTERNAL_FORX3(S,__VA_ARGS__))
#define THEOLIZER_INTERNAL_FORX5(S,P,...) S(P);\
    THEOLIZER_INTERNAL_EXPAND(THEOLIZER_INTERNAL_FORX4(S,__VA_ARGS__))
#define THEOLIZER_INTERNAL_FORX6(S,P,...) S(P);\
    THEOLIZER_INTERNAL_EXPAND(THEOLIZER_INTERNAL_FORX5(S,__VA_ARGS__))
#define THEOLIZER_INTERNAL_FORX7(S,P,...) S(P);\
    THEOLIZER_INTERNAL_EXPAND(THEOLIZER_INTERNAL_FORX6(S,__VA_ARGS__))
#define THEOLIZER_INTERNAL_FORX8(S,P,...) S(P);\
    THEOLIZER_INTERNAL_EXPAND(THEOLIZER_INTERNAL_FORX7(S,__VA_ARGS__))

#define THEOLIZER_INTERNAL_FORN(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define THEOLIZER_INTERNAL_FORR_I(...)\
    THEOLIZER_INTERNAL_EXPAND(THEOLIZER_INTERNAL_FORN(__VA_ARGS__))
#define THEOLIZER_INTERNAL_FORR(...)\
    THEOLIZER_INTERNAL_FORR_I(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define THEOLIZER_INTERNAL_FOR_I(N,S,...)                                   \
    THEOLIZER_INTERNAL_EXPAND                                               \
    (                                                                       \
        THEOLIZER_INTERNAL_CAT(THEOLIZER_INTERNAL_FORX,N)(S,__VA_ARGS__)    \
    )

//----------------------------------------------------------------------------
//      文字列処理constexpr関数
//----------------------------------------------------------------------------

//      ---<<< 文字列が空かどうか判定する >>>---

constexpr bool isEmpty(char const* p)
{
    return (*p == 0);
}

//      ---<<< 文字列内の','の数を返却する >>>---

constexpr unsigned countComma(char const* p)
{
    return  (*p == 0)?              // if
                0
            :(*p == ',')?           // else if
                countComma(p+1)+1
            :                       // else
                countComma(p+1);
}

// ***************************************************************************
//      静的にファイル名抽出
// ***************************************************************************

constexpr char const* stripPathImpl(char const* iPath, char const* iLast)
{
    return  (!*iPath)?
                iLast
            : ((*iPath == '/') || (*iPath == '\\'))?
                stripPathImpl(iPath+1, iPath+1)
            :
                stripPathImpl(iPath+1, iLast);
}

constexpr char const* stripPath(char const* iPath)
{
    return stripPathImpl(iPath, iPath);
}

#define THEOLIZER_INTERNAL_FILE    theolizer::internal::stripPath(__FILE__)

// ***************************************************************************
//      高精度タイマ・クラス
//          コンストラクトから、もしくは、前回取得した時からの経過時間を返却する。
//          iUpdateがfalseの時、経過時間を更新しない。
// ***************************************************************************

class FineTimer
{
private:
    std::chrono::system_clock::time_point   mStartTime;

public:
    FineTimer() { mStartTime=std::chrono::system_clock::now(); }

    int64_t GetmSec(bool iUpdate=true)
    {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        auto duration = now - mStartTime; 
        if (iUpdate) mStartTime=now;
        return std::chrono::duration_cast<
                        std::chrono::milliseconds>(duration).count();
    }

    int64_t GetuSec(bool iUpdate=true)
    {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        auto duration = now - mStartTime; 
        if (iUpdate) mStartTime=now;
        return std::chrono::duration_cast<
                        std::chrono::microseconds>(duration).count();
    }
};

// ***************************************************************************
//      型情報取得(TYPENAMEマクロ)
// ***************************************************************************

// libToolingとリンクする時、RTTIを有効にできない。
//  ドライバのみlibToolingとリンクするのでドライバでは、型情報取得機能を削除する

#ifndef THEOLIZER_INTERNAL_DRIVER

// ---------------------------------------------------------------------------
//      GCC用
// ---------------------------------------------------------------------------

#if defined(__GNUC__)

template<typename tType>
class GccTypeName
{
private:
    static void         use(GccTypeName const&) {}

    char*   mName;
    GccTypeName() : mName(0)
    {
        int status = 0;
        mName = abi::__cxa_demangle(typeid(tType).name(), 0, 0, &status);
    }
    ~GccTypeName() {std::free(mName);}

public:
    static GccTypeName& getInstance()
    {
        static GccTypeName instance;
        use(instance);
        return instance;
    }
    char const* get()
    {
        return (mName)?mName:"\"demagle error\"";
    }
    // コピー／ムーブ禁止
    GccTypeName(GccTypeName const&)  = delete;
    GccTypeName(GccTypeName&&)       = delete;
    GccTypeName& operator=(GccTypeName const&)  = delete;
    GccTypeName& operator=(GccTypeName&&)       = delete;
};

// 中継
template<typename tType>
char const* getTypeNameImpl()
{
    return GccTypeName<tType>::getInstance().get();
}

#if 0   // 不要な筈(未テスト)
// type_indexから生成
std::string getCppTypeName(std::type_index iStdTypeIndex)
{
    int status = 0;
    char* aName = abi::__cxa_demangle(iStdTypeIndex.name(), 0, 0, &status);
    std::string ret(aName);
    free(aName);
    return ret;
}
#endif

// ---------------------------------------------------------------------------
//      その他のコンパイラ用
// ---------------------------------------------------------------------------

#else

template<typename tType>
char const* getTypeNameImpl()
{
    return typeid(tType).name();
}

#if 0   // 不要な筈(未テスト)
// type_indexから生成
std::string getCppTypeName(std::type_index iStdTypeIndex)
{
    return std::string(iStdTypeIndex.name());
}
#endif

#endif

// ---------------------------------------------------------------------------
//      中継クラス
// ---------------------------------------------------------------------------

template<typename tType>
struct TypeNameImpl
{
    static char const* get(bool iIsRough=false)
    {
        if (iIsRough)
        {
            return  getTypeNameImpl<tType>();
        }
        else
        {
            char const* aName=getTypeNameImpl<TypeNameImpl>();
            char const* p;
            for (p=aName; (*p != '<') && (*p != 0); ++p)
                ;
            return  (*p)?p:aName;
        }
    }
};

// ---------------------------------------------------------------------------
//      マクロ
//          VAR無しは、型を与える。
//          VAR付きは、インスタンスを与える。
//          ROUGH付きは、constと参照がついていてもを区別しない。
// ---------------------------------------------------------------------------

#define THEOLIZER_INTERNAL_TYPE_NAME(dType)                                 \
    theolizer::internal::TypeNameImpl<dType>::get()

#define THEOLIZER_INTERNAL_TYPE_NAME_VAR(dVar)                              \
    theolizer::internal::TypeNameImpl<decltype(dVar)>::get()

#define THEOLIZER_INTERNAL_TYPE_NAME_ROUGH(dType)                           \
    theolizer::internal::TypeNameImpl<dType>::get(true)

#define THEOLIZER_INTERNAL_TYPE_NAME_VAR_ROUGH(dVar)                        \
    theolizer::internal::TypeNameImpl<decltype(dVar)>::get(true)

// ***************************************************************************
//      型情報取得(getNameByTypeInfo)
// ***************************************************************************

THEOLIZER_INTERNAL_DLL std::string getNameByTypeInfo(std::type_index aTypeIndex);

// ***************************************************************************
//      ドライバ向けダミー定義
// ***************************************************************************

#else

#define THEOLIZER_INTERNAL_TYPE_NAME(dType)             ""
#define THEOLIZER_INTERNAL_TYPE_NAME_VAR(dVar)          ""
#define THEOLIZER_INTERNAL_TYPE_NAME_ROUGH(dType)       ""
#define THEOLIZER_INTERNAL_TYPE_NAME_VAR_ROUGH(dVar)    ""
inline std::string getNameByTypeInfo(std::type_index aTypeIndex) {return "";}

#endif  // THEOLIZER_INTERNAL_DRIVER

// ***************************************************************************
//      テンプレート・メタ・プログラミング補助ツール群
// ***************************************************************************

//----------------------------------------------------------------------------
//      std::enable_if<>短縮
//----------------------------------------------------------------------------

template<bool tBool, class tThen = void>
using EnableIf=typename std::enable_if<tBool, tThen>::type;

//----------------------------------------------------------------------------
//      常にvoidとなる型
//----------------------------------------------------------------------------

template <class>
struct Ignore
{
    typedef void type;
    static const bool kTrue = true;
    static const bool kFalse = false;
};

//----------------------------------------------------------------------------
//      const/volatile外し
//          ポインタ、参照、配列の型からconst/volatileを外す
//----------------------------------------------------------------------------

template<typename tType, class tEnable=void>
class RemoveCV
{
public:
    typedef tType   type;
};
template<typename tType>
class RemoveCV<tType, EnableIf<std::is_pointer<tType>::value>>
{
    typedef typename std::remove_pointer<tType>::type       Type0;
    typedef typename std::remove_cv<Type0>::type            Type1;
    typedef typename RemoveCV<Type1>::type                  Type2;
public:
    typedef typename std::add_pointer<Type2>::type          type;
};
template<typename tType>
class RemoveCV<tType,
               EnableIf<std::is_lvalue_reference<tType>::value>>
{
    typedef typename std::remove_reference<tType>::type     Type0;
    typedef typename std::remove_cv<Type0>::type            Type1;
    typedef typename RemoveCV<Type1>::type                  Type2;
public:
    typedef typename std::add_lvalue_reference<Type2>::type type;
};
template<typename tType>
class RemoveCV<tType, EnableIf<std::is_array<tType>::value>>
{
    static size_t const kExtent=std::extent<tType>::value;

    typedef typename std::remove_extent<tType>::type        Type0;
    typedef typename std::remove_cv<Type0>::type            Type1;
    typedef typename RemoveCV<Type1>::type                  Type2;
public:
    typedef typename RemoveCV::Type2                        type[kExtent];
};

//----------------------------------------------------------------------------
//      ()解除マクロ
//----------------------------------------------------------------------------

#define THEOLIZER_INTERNAL_UNPAREN(...) __VA_ARGS__

//----------------------------------------------------------------------------
//      大小比較演算子
//          <や>を使うとtemplateの<>の対応付けが難しくなる
//----------------------------------------------------------------------------

#define THEOLIZER_GE     >=
#define THEOLIZER_GT     >
#define THEOLIZER_LE     <=
#define THEOLIZER_LT     <

//----------------------------------------------------------------------------
//      関数テンプレートのオーバーロード
//          使い方例
//              template<class tClass, THEOLIZER_INTERNAL_OVERLOAD((bool値))>
//              void FooFunc(tClass& iInstance) { }
//          bool値がtrueになる時、オーバーロード関数が実体化される。
//----------------------------------------------------------------------------

#define THEOLIZER_INTERNAL_OVERLOAD(dJudge)                                 \
    typename std::enable_if<THEOLIZER_INTERNAL_UNPAREN dJudge, std::nullptr_t>::type=nullptr

//----------------------------------------------------------------------------
//      各種データ型のコピー
//----------------------------------------------------------------------------

template<typename tClass>
void copyData(tClass& oDestination, tClass const& iSource);

// ***************************************************************************
//      自動回復ツール
// ***************************************************************************

template<typename tType>
class AutoRestore
{
    tType   mBackup;
    tType&  mTarget;

public:
    explicit AutoRestore(tType& iTarget, tType iInitialValue) :
                    mBackup(iTarget),   // iTargetの値をmBackupへバックアップ
                    mTarget(iTarget)    // iTargetのアドレスをmTargetへ設定
    {
        mTarget=iInitialValue;
    }
    ~AutoRestore()
    {
        mTarget=mBackup;
    }
    AutoRestore(AutoRestore const&) = delete;
    AutoRestore& operator=(AutoRestore const&) = delete;
};

#endif  // THEOLIZER_INTERNAL_DOXYGEN
}   // namespace internal

//############################################################################
//      バージョン管理情報
//############################################################################

// Theolizerドライバのビルド時は定義されていない
#if !defined(THEOLIZER_INTERNAL_DLL)
    #define THEOLIZER_INTERNAL_DLL
#endif

#ifndef THEOLIZER_INTERNAL_EXCLUDE_VERSION_H

// namespace内で定義するため、ここでインクルード
namespace
{
    #include "version.h"
}

//! バージョン文字列獲得
THEOLIZER_INTERNAL_DLL std::string getVersionString();

//! バージョン番号獲得（バージョン文字列獲得の一部）
THEOLIZER_INTERNAL_DLL std::string getVersionNo();

//! ソース・コードのMD5ハッシュ値獲得（バージョン文字列獲得の一部）
THEOLIZER_INTERNAL_DLL std::string getSourcesHash();

//! ライブラリ・ヘッダのMD5ハッシュ値獲得（バージョン文字列獲得の一部）
THEOLIZER_INTERNAL_DLL std::string getLibraryHash();

/*!
@brief      ライブラリ・バイナリとライブラリ・ヘッダのバージョンが一致している時、true返却
@details    不一致の場合、正常に動作しない恐れがあるため、共有ライブラリを使用する時は、
            checkLibraryHash()がtrueであることをチェックすることを推奨する。
*/
THEOLIZER_INTERNAL_DLL bool checkLibraryHash(char const* iLibraryHash=kTheolizerLibraryHash);

#endif  //THEOLIZER_INTERNAL_EXCLUDE_VERSION_H

//############################################################################
//      End
//############################################################################

}   // namespace theolizer

#endif  // THEOLIZER_INTERNAL_BASE_H
