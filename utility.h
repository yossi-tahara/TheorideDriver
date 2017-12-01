//############################################################################
//      ユーティリティ／共有クラス定義
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

#if !defined(THEOLIZER_UTILITY_H)
#define THEOLIZER_UTILITY_H

// ***************************************************************************
//      重要定義
// ***************************************************************************

#define DISABLE_REPLACE                             // replace/restore機能を禁止

//----------------------------------------------------------------------------
//      各種定義
//----------------------------------------------------------------------------

// TheolizerDriver.exe判定用文字列
char const* kTheolizerMarker = "Theolizer";

// TheolizerDriver専用パラメータ
char const* kTheolizerOrigCompParam = "theolizer_original_compiler";// =<OrigPath>
char const* kTheolizerDoProcessParam= "THEOLIZER_DO_PROCESS";       // none

#define ARG_THEOLIZER   "--theolizer"                           // 継続パラメータ
char const* kTheolizerVersionParam  = ARG_THEOLIZER "-version"; // none
#ifndef DISABLE_REPLACE
char const* kTheolizerReplaceParam  = ARG_THEOLIZER "-replace"; // =<Path>;<Path>;...
char const* kTheolizerRestoreParam  = ARG_THEOLIZER "-restore"; // =<Path>;<Path>;...
#endif

// エラー出力ヘッダ
char const*  kDiagMarker = "[" "Theolizer" "] ";

// 排他制御用リソース名
char const* kTheolizerFileLock = "file_lock";

//############################################################################
//      コンパイラの相違点を吸収する
//############################################################################

// ***************************************************************************
//      POSIX関係
// ***************************************************************************

#ifdef _MSC_VER
    #include <io.h>
    int chdir(char const* iPath);
    char *GetCurrentDirName();
#else
    #include <unistd.h>
    char *GetCurrentDirName();
#endif

//############################################################################
//      汎用ルーチン
//############################################################################

extern SourceManager*           gSourceManager; // デバッグ用

// ***************************************************************************
//      ASSERT群
// ***************************************************************************

void DriverAbort(char const* iDate, char const* iFile, const int iLine, 
                     const char *iFormat, ...)
{
    std::string      aOutputString;
    int         aOutputNum;
    char        aBuff[2048];
    const int   aBuffSize((int)sizeof(aBuff));

    va_list args;
    va_start(args, iFormat);
    aOutputNum = snprintf(aBuff, aBuffSize, "%s(%d) [%s] ", iFile, iLine, iDate);
    aOutputString = aBuff;
    if ((aOutputNum < 0) || (aBuffSize <= aOutputNum))
    {
        aOutputString += "[[TRANCATED]]";
    }
    aOutputNum = vsnprintf(aBuff, aBuffSize, iFormat, args);
    aOutputString += aBuff;
    if ((aOutputNum < 0) || (aBuffSize <= aOutputNum))
    {
        aOutputString += "[[TRANCATED]]";
    }
    va_end(args);

    llvm::errs() << aOutputString << "\n";
    std::exit(1);
}

#define DRIVER_ABORT(...)                                          \
    DriverAbort(__DATE__, __FILE__, __LINE__, __VA_ARGS__);
#define DRIVER_ASSERT(iCondition, ...) if (!(iCondition))          \
    DriverAbort(__DATE__, __FILE__, __LINE__, __VA_ARGS__);

// ***************************************************************************
//      アクセス権付与
// ***************************************************************************

void setPermissions(char const* iPath)
{
    chmod(iPath, 0777);
}

// ***************************************************************************
//      テンポラリ・フォルダ管理
//          一度だけ動けば良いのでシングルトンとする
// ***************************************************************************

class TemporaryDir
{
private:
    SmallString<128>    mTemporaryDir;
    TemporaryDir()
    {
        // テンポラリ・フォルダのパス生成
        SmallString<128>    temp_dir;
        llvmS::path::system_temp_directory(true, temp_dir);
        mTemporaryDir = temp_dir;
        mTemporaryDir += "/theolizer/";

        // テンポラリ・フォルダの生成
        std::string aTemporaryDir(mTemporaryDir.begin());
        boost::system::error_code error;
        boostF::create_directories(aTemporaryDir, error);
        setPermissions(aTemporaryDir.c_str());
    }
    TemporaryDir(const TemporaryDir&) = delete;
    void operator =(const TemporaryDir&) = delete;

public:
    static char const* get()
    {
        static TemporaryDir instance;
        return instance.mTemporaryDir.c_str();
    }
};

// ***************************************************************************
//      ユーティリティ
// ***************************************************************************

//----------------------------------------------------------------------------
//      スコープを抜けたらfalseへ戻す
//----------------------------------------------------------------------------

struct AutoFalse
{
    bool&   mTarget;
    AutoFalse(bool& iTarget) :
        mTarget(iTarget)
    {
        mTarget=true;
    }
    ~AutoFalse()
    {
        mTarget=false;
    }
};

// ***************************************************************************
//      AST解析用ユーティリティ
// ***************************************************************************

//############################################################################
//      主要共有クラス
//############################################################################

// ***************************************************************************
//      ソース保存排他制御オブジェクト
//          ミューテックスと各フラグはアトミックに設定する必要があるが、
//          Theolizerドライバーはシングル・スレッドなのでアトミック化を省略。
// ***************************************************************************

class ExclusiveControl
{
//----------------------------------------------------------------------------
//      排他制御用のファイル生成
//----------------------------------------------------------------------------

private:
    struct FileCreator
    {
        SmallString<128>    mFilePathSharable;
        SmallString<128>    mFilePathUpgradable;
        FileCreator(char const* iPrefix) : 
                        mFilePathSharable(TemporaryDir::get()),
                        mFilePathUpgradable(TemporaryDir::get())
        {
            mFilePathSharable += iPrefix;
            mFilePathSharable += "_sharable";

            mFilePathUpgradable += iPrefix;
            mFilePathUpgradable += "_upgradable";

            int result_fd;
            llvmS::fs::openFileForWrite(StringRef(mFilePathSharable),
                                        result_fd, llvmS::fs::F_Append);
            llvmS::Process::SafelyCloseFileDescriptor(result_fd);
            setPermissions(mFilePathSharable.c_str());

            llvmS::fs::openFileForWrite(StringRef(mFilePathUpgradable),
                                        result_fd, llvmS::fs::F_Append);
            llvmS::Process::SafelyCloseFileDescriptor(result_fd);
            setPermissions(mFilePathUpgradable.c_str());
        }
    };
    FileCreator             mFileCreator;
    boostI::file_lock       mFileLockSharable;
    boostI::file_lock       mFileLockUpgradable;
    bool                    mSharableLocked;
    bool                    mUpgradableLocked;
    bool                    mLocked;

    bool                    mRedoRequest;

//----------------------------------------------------------------------------
//      生成と削除
//          デストラクト時ロックを開放しているが、これは必要なロジックではない。
//----------------------------------------------------------------------------

public:
    ExclusiveControl(char const* iName) :
        mFileCreator(iName),
        mFileLockSharable(mFileCreator.mFilePathSharable.c_str()),
        mFileLockUpgradable(mFileCreator.mFilePathUpgradable.c_str()),
        mSharableLocked(false),
        mUpgradableLocked(false),
        mLocked(false),
        mRedoRequest(false)
    { }
    ~ExclusiveControl()
    {
        unlock();
        unlock_upgradable();
        unlock_sharable();
    }

//----------------------------------------------------------------------------
//      Sharable Lock
//----------------------------------------------------------------------------

//      ---<<< Lock >>>---

    void lock_sharable()
    {
        {
            boostI::sharable_lock<boostI::file_lock> lock(mFileLockUpgradable);
            mFileLockSharable.lock_sharable();
        }
        mSharableLocked = true;
        mRedoRequest=false;
    }

//      ---<<< Unlock >>>---

    void unlock_sharable()
    {
        if (!mSharableLocked)
    return;
        mSharableLocked = false;
        mFileLockSharable.unlock_sharable();
    }

//----------------------------------------------------------------------------
//      Upgradable Lock
//----------------------------------------------------------------------------

//      ---<<< Try Lock >>>---
//          Upgradeできないということは、既に他のプロセスがソース修正中である。
//          この場合、AST解析からやり直す必要がある。

    bool try_lock_upgradable()
    {
        if (mFileLockUpgradable.try_lock())
        {
            mUpgradableLocked = true;
            unlock_sharable();
    return true;
        }
        mRedoRequest=true;
        return false;
    }

//      ---<<< Lock >>>---
//          排他制御機能のデバッグ／動作検証用

    void lock_upgradable()
    {
        mFileLockUpgradable.lock();
        mUpgradableLocked = true;
        unlock_sharable();
    }

//      ---<<< Unlock >>>---

    void unlock_upgradable()
    {
        if (!mUpgradableLocked)
    return;
        mUpgradableLocked = false;
        lock_sharable();
        mFileLockUpgradable.unlock();
    }

//----------------------------------------------------------------------------
//      Exclusive Lock
//----------------------------------------------------------------------------

//      ---<<< Lock >>>---

    void lock()
    {
        mFileLockSharable.lock();
        mLocked = true;
        mUpgradableLocked = false;
    }

//      ---<<< Unlock >>>---

    void unlock()
    {
        if (!mLocked)
    return;
        mUpgradableLocked = true;
        mLocked = false;
        mFileLockSharable.unlock();
    }

//----------------------------------------------------------------------------
//      ドライバー専用機能
//          try_lock_upgradableに失敗していたらtrueを返却する。
//----------------------------------------------------------------------------

    bool getRedoRequest()
    {
        return mRedoRequest;
    }
};

// ***************************************************************************
//      診断結果表示用DiagnosticConsumer
// ***************************************************************************

extern ASTContext*  gASTContext;
std::string         gLastErrorMessage;

class TheolizerDiagnosticConsumer : public clang::TextDiagnosticPrinter
{
private:
    bool                    mHasErrorOccurred;
    SmallVector<char, 128>  mMessage;

public:
    TheolizerDiagnosticConsumer(clang::raw_ostream &iOStream, clang::DiagnosticOptions *iDiagOpt) :
        TextDiagnosticPrinter(iOStream, iDiagOpt),
        mHasErrorOccurred(false)
    { }
    void clear() {mHasErrorOccurred=false;}
    bool hasErrorOccurred() {return mHasErrorOccurred;}

    void HandleDiagnostic(clang::DiagnosticsEngine::Level iDiagLevel,
                          clang::Diagnostic const& iInfo) override
    {
        mMessage.clear();
        iInfo.FormatDiagnostic(mMessage);
        std::string aMessage(mMessage.begin(), mMessage.size());

        std::string aLocation;
        if (iInfo.hasSourceManager())
        {
            aLocation=gASTContext->getFullLoc(iInfo.getLocation()).getSpellingLoc().
                    printToString(iInfo.getSourceManager());
        }
        else
        {
            aLocation="<no-source>";
        }

        // 最後のErrorとFatalを記録する
        if (clang::DiagnosticsEngine::Level::Error <= iDiagLevel)
        {
            gLastErrorMessage=aMessage + "\n" + aLocation;
        }

        // 致命的エラーやTheolizerエラーを反映する
        if ((iDiagLevel == clang::DiagnosticsEngine::Level::Fatal)
         || (StringRef(aMessage).startswith(kDiagMarker)))
        {
            clang::TextDiagnosticPrinter::HandleDiagnostic(iDiagLevel, iInfo);
            mHasErrorOccurred=true;
        }
    }
};

// ***************************************************************************
//      診断メーセッジ出力
// ***************************************************************************

class CustomDiag
{
private:
    DiagnosticsEngine*  mDiags;

    DiagnosticBuilder Report(DiagnosticIDs::Level iLevel,
                             SourceLocation iLoc,
                             char const* iFormat)
    {
        unsigned custom_id = mDiags->getDiagnosticIDs()->getCustomDiagID(
                                                iLevel, StringRef(iFormat));
        return mDiags->Report(iLoc, custom_id);
    }
    std::string mBuff;
    char const* getFormat(std::string const& iFormat)
    {
        mBuff = kDiagMarker;
        mBuff.append(iFormat);
        return mBuff.c_str();
    }
    TheolizerDiagnosticConsumer*        mClient;
    std::unique_ptr<DiagnosticConsumer> mClientBackup;

public:
    CustomDiag() : mDiags(nullptr), mClient(nullptr)
    { }

    void setDiagnosticsEngine(DiagnosticsEngine* iDiags)
    {

        mDiags = iDiags;
        mClient= static_cast<TheolizerDiagnosticConsumer*>(mDiags->getClient());
        DRIVER_ASSERT(mClient, "");
        mClient->clear();
    }
    void resetDiagnosticsEngine()
    {
    }

    bool hasErrorOccurred()
    {
        DRIVER_ASSERT(mClient, "");
        return mClient->hasErrorOccurred();
    }

    DiagnosticBuilder IgnoredReport(SourceLocation iLoc, std::string const& iFormat)
    {
        return Report(DiagnosticIDs::Ignored, iLoc, getFormat(iFormat));
    }
    DiagnosticBuilder NoteReport(SourceLocation iLoc, std::string const& iFormat)
    {
        return Report(DiagnosticIDs::Note, iLoc, getFormat(iFormat));
    }
    DiagnosticBuilder RemarkReport(SourceLocation iLoc, std::string const& iFormat)
    {
        return Report(DiagnosticIDs::Remark, iLoc, getFormat(iFormat));
    }
    DiagnosticBuilder WarningReport(SourceLocation iLoc, std::string const& iFormat)
    {
        return Report(DiagnosticIDs::Warning, iLoc, getFormat(iFormat));
    }
    DiagnosticBuilder ErrorReport(SourceLocation iLoc, std::string const& iFormat)
    {
        return Report(DiagnosticIDs::Error, iLoc, getFormat(iFormat));
    }
    DiagnosticBuilder FatalReport(SourceLocation iLoc, std::string const& iFormat)
    {
        return Report(DiagnosticIDs::Fatal, iLoc, getFormat(iFormat));
    }

    DiagnosticsEngine& getDiags() { return *mDiags; }

//      ---<<< 複数箇所で用いるメッセージの定義 >>>---

    static inline char const*   getNotSupportUnicon()
        { return "theolizer does not support union.";}
    static inline char const*   getNotSupportNoName()
        { return "theolizer does not support no-named class/struct";}
};

//----------------------------------------------------------------------------
//      グローバル変数
//----------------------------------------------------------------------------

CustomDiag              gCustomDiag;

//----------------------------------------------------------------------------
//      ソース不正修正エラー表示用マクロ
//----------------------------------------------------------------------------

#define ERROR(dCond, dDecl, ...)                                            \
    do {                                                                    \
        if (dCond) {                                                        \
            gCustomDiag.ErrorReport(dDecl->getLocation(),                   \
                "Do not modify the source generated by Theolizer.(%0)")     \
            << __LINE__;                                                    \
    return __VA_ARGS__;                                                     \
        }                                                                   \
    } while(0)

#define ERROR_ST(dCond, dStmt, ...)                                         \
    do {                                                                    \
        if (dCond) {                                                        \
            gCustomDiag.ErrorReport(dStmt->getLocStart(),                   \
                "Do not modify the source generated by Theolizer.(%0)")     \
            << __LINE__;                                                    \
    return __VA_ARGS__;                                                     \
        }                                                                   \
    } while(0)

#define ERROR_LOC(dCond, dLoc, ...)                                         \
    do {                                                                    \
        if (dCond) {                                                        \
            gCustomDiag.ErrorReport(dLoc,                                   \
                "Do not modify the source generated by Theolizer.(%0)")     \
            << __LINE__;                                                    \
    return __VA_ARGS__;                                                     \
        }                                                                   \
    } while(0)

#define WARNING(dCond, dDecl, ...)                                          \
    if (dCond) {                                                            \
        gCustomDiag.WarningReport(dDecl->getLocation(),                     \
            "Theolizer unkown pattern. Please report to Theolizer developper.(%0)")\
        << __LINE__;                                                        \
    return __VA_ARGS__;                                                     \
    }                                                                       \

#define WARNING_CONT(dCond, dDecl)                                          \
    if (dCond) {                                                            \
        gCustomDiag.WarningReport(dDecl->getLocation(),                     \
            "Theolizer unkown pattern. Please report to Theolizer developper.(%0)")\
        << __LINE__;                                                        \
    continue;                                                               \
    }                                                                       \

#define NOT_SUPPORT(dCond, dDecl, ...)                                      \
    if (dCond) {                                                            \
        gCustomDiag.WarningReport(dDecl->getLocation(),                     \
            "Theolizer does not support pattern.");                         \
    return __VA_ARGS__;                                                     \
    }                                                                       \

#define NOT_SUPPORT_CONT(dCond, dDecl)                                      \
    if (dCond) {                                                            \
        gCustomDiag.WarningReport(dDecl->getLocation(),                     \
            "Theolizer does not support pattern.");                         \
    continue;                                                               \
    }                                                                       \

// ***************************************************************************
//      AST解析処理からの出力
// ***************************************************************************

struct AstInterface
{
//      ---<<< プリプロセスからの出力 >>>---
//          first : #undef、second : #defineとする
//          #define～#undefの間にあるキーを見つけるようにするため。

    bool    mNotParse;      // 解析処理を行わない時true THEOLIZER_NO_ANALYZE

//      ---<<< 定義終了位置 >>>---

    std::map<clang::TagDecl*, FullSourceLoc>    mEndMarkerList;
    void addEndMarker(clang::TagDecl* iTagDecl, SourceLocation iLoc)
    {
        FullSourceLoc aLoc = gASTContext->getFullLoc(iLoc).getSpellingLoc();
        mEndMarkerList.emplace(iTagDecl, aLoc);
    }

//      ---<<< コンストラクタ >>>---

    AstInterface() : 
        mNotParse(false)
    { }
};

// ***************************************************************************
//      グローバル変数群
// ***************************************************************************

//      ---<<< コンパイル対象ファイル >>>---

std::string             gCompilingSourceName;

//      ---<<< AST処理用 >>>---

CompilerInstance*       gCompilerInstance=nullptr;
ASTContext*             gASTContext=nullptr;
SourceManager*          gSourceManager=nullptr;

// AST解析をリトライする
//  バージョンアップを検出した時、最新版のマクロが未生成の状態でAST解析するため、
//  旧バージョンのみで保存されているクラスのsave/loadが生成されない。
//  その場合にTHEOLIZER_GENERATED_NO_COMPILEが付いてしまう問題に対処する
// 2016/09/12 これだけではダメだった。
//  派生クラスからのみ保存されていた基底クラスがあり、
//  それをバーションアップ時に派生クラスから解除した場合、
//  save/load検出がうまく行っていない。
//  暫定処置として、THEOLIZER_GENERATED_NO_COMPILEを付けないようにしている。
bool                    gRetryAST;

//      ---<<< ソース・ファイルの排他制御 >>>---

std::unique_ptr<ExclusiveControl>   gExclusiveControl;

//      ---<<< 改行 >>>---

#if defined(_WIN32)
    static StringRef const  kLineEnding{"\r\n"};
#else
    static StringRef const  kLineEnding{"\n"};
#endif

#endif
