//############################################################################
//      Theorideドライバー
//          ドライバーは、インクルードが非常に重いため、
//          コンパイル単位を１つだけとする。
//          当ファイルがコンパイル単位の中心である。
//
//          役割：
//              関連ソースのインクルード
//              パラメータ解析
//              AST解析-ソース修正呼び出し
//              元のコンパイラ呼び出し
/*
    © 2017 Theoride Technology (http://theolizer.com/) All Rights Reserved.

    General Public License Version 3 ("GPLv3")
        You may use this file in accordance with the terms and conditions of 
        GPLv3 published by Free Software Foundation.
        Please confirm the contents of GPLv3 at https://www.gnu.org/licenses/gpl.txt .
        A copy of GPLv3 is also saved in a LICENSE.TXT file.

    General Public License Version 3(以下GPLv3)
        Free Software Foundationが公表するGPLv3の使用条件に従って、
        あなたはこのファイルを取り扱うことができます。
        GPLv3の内容を https://www.gnu.org/licenses/gpl.txt にて確認して下さい。
        またGPLv3のコピーをLICENSE.TXTファイルにおいてます。
*/
//############################################################################

#if defined(_MSC_VER)   // disabling MSVC warnings
    #pragma warning(disable:4100 4127 4714 4996)
#endif

#if defined(__GNUC__)
    #pragma GCC diagnostic ignored "-Wunused-result"    // chdirのため
#endif

#include "header_includes.h"    // 必要な外部ヘッダのインクルード
#include "utility.h"            // ユーティリティ
//#include "modify.h"             // ソース修正
#include "parse.h"              // AST解析と必要情報収集

// ***************************************************************************
//      ドライバー・モード
// ***************************************************************************

enum DriverMode {
    none,               // 対応無し
    gpp,                // g++
    cl,                 // cl
    cpp                 // cpp
};

DriverMode getDriverMode(std::string const& iStem)
{
    std::string aStem = StringRef(iStem).rtrim("-0123456789.").lower();

    DriverMode  ret = none;
    if (StringRef(aStem).endswith("g++"))
    {
       ret = gpp;
    }
    if (StringRef(aStem).endswith("cl"))
    {
       ret = cl;
    }
    if (StringRef(aStem).endswith("cpp"))
    {
       ret = cpp;
    }

    return ret;
}

// ***************************************************************************
//      元コンパイラのリネーム時フル・パス生成
// ***************************************************************************

std::string makeRenamePath(std::string const& iExePath)
{
    std::string aRenamePath = llvmS::path::parent_path(iExePath);
    aRenamePath.append("/");
    aRenamePath.append(llvmS::path::stem(iExePath));
    aRenamePath.append("RenamedByTheoride");
    aRenamePath.append(llvmS::path::extension(iExePath));

    return aRenamePath;
}

// ***************************************************************************
//      実行管理クラス
//          指定exeを実行し、結果を一時ファイルへ保存する。
//          デストラクタで一時ファイルを削除する。
// ***************************************************************************

class ExecuteMan
{
    SmallString<128>    mTempFilePath;
    bool                mError;
    std::string         mErrorMessage;
    std::string         mResult;
    char*               mCurrentDir;

public:
    // コンストラクタ(指定コマンド実行)
    ExecuteMan(std::string const& iExePath, llvm::opt::ArgStringList& ioArgv)
    {
        mError=true;
        mErrorMessage="Unknown error";
        mResult = "";
        mCurrentDir=GetCurrentDirName();

        // 一時ファイル名生成
        std::error_code ec = llvmS::fs::createTemporaryFile("Theoride", "txt", mTempFilePath);
        if (ec)
        {
            mErrorMessage=ec.message();
    return;
        }

        // リダイレクト
        const StringRef *aRedirects[3];
        StringRef       aEmpty;
        aRedirects[0] = &aEmpty;
        StringRef       aTempFilePath(mTempFilePath);
        aRedirects[1] = &aTempFilePath;
        aRedirects[2] = &aTempFilePath;

        ioArgv.push_back(nullptr);

        // 実行
        std::string aCurrentDir = llvmS::path::parent_path(iExePath);
        if (chdir(aCurrentDir.c_str()))
        {
            mErrorMessage="Can not chdir.";
    return;
        }

        bool execution_failed;
        llvmS::ExecuteAndWait(iExePath, ioArgv.data(), nullptr,
                                        aRedirects, 0, 0, &mErrorMessage,
                                        &execution_failed);
        if (execution_failed)
    return;

        // 結果読み出し
        {
            std::ifstream ifs(mTempFilePath.str());
            if (ifs.fail())
            {
                mErrorMessage="Temporary file read error!";
    return;
            }
            mError=false;

            std::istreambuf_iterator<char> it(ifs);
            std::istreambuf_iterator<char> last;
            mResult = std::string(it, last);
        }
    }

    // デストラクタ(一時ファイル削除)
    ~ExecuteMan()
    {
        // 一時ファイル削除
        if (!mError)
        {
            llvmS::fs::remove(mTempFilePath);
        }

        // カレント・フォルダを元に戻しておく
        chdir(mCurrentDir);
        free(mCurrentDir);
    }

    // 結果返却
    std::string const& GetResult()       const { return mResult; }
    bool GetError()                      const { return mError; }
    std::string const& GetErrorMessage() const { return mErrorMessage; }
};

// ***************************************************************************
//      TheorideDriverであることを確認する
// ***************************************************************************

enum CheckResult
{
    ecrExecError,
    ecrIsTheoride,
    ecrNotTheoride
};

CheckResult CheckTheoride(std::string const& iExePath)
{
    // パラメータ生成
    llvm::opt::ArgStringList aArgv;
    aArgv.push_back(iExePath.c_str());
    aArgv.push_back(kTheorideVersionParam);

    // iExePath実行
    ExecuteMan  aExecuteMan(iExePath, aArgv);
    if (aExecuteMan.GetError())
    {
        llvm::errs() << kDiagMarker << aExecuteMan.GetErrorMessage() << "\n";
return ecrExecError;
    }

    // 結果判定
    if (aExecuteMan.GetResult().find(kTheorideMarker) != std::string::npos)
return ecrIsTheoride;

    return ecrNotTheoride;
}

// ***************************************************************************
//      シンボリック・リンクを展開する
// ***************************************************************************

#ifdef _WIN32
std::wstring convertUtf8ToUtf16(std::string const& iString);
std::string convertUtf16ToUtf8(std::wstring const& iString);
#endif

std::string getCanonicalPath(std::string const& iPath)
{
#ifdef _WIN32
    // 文字コード変換する(UTF-8 → UTF-16)
    std::wstring aPath=convertUtf8ToUtf16(iPath);
    boost::system::error_code ec;
    boostF::path aCanonicalPath = boostF::canonical(aPath, ec);
    if (ec)
    {
        llvm::errs() << kDiagMarker << "Not found " << iPath << "\n";
    }
    // 文字コード変換する(UTF-16 → UTF-8)
    return convertUtf16ToUtf8(aCanonicalPath.wstring());
#else
    boost::system::error_code ec;
    boostF::path aCanonicalPath = boostF::canonical(iPath, ec);
    if (ec)
    {
        llvm::errs() << kDiagMarker << "Not found " << iPath << "\n";
    }
    return aCanonicalPath.string();
#endif
}

// ***************************************************************************
//      ファイルの状態（パーミッション）を取り出す
// ***************************************************************************

boostF::file_status getFileStatus(std::string const& iPath)
{
#ifdef _WIN32
    // 文字コード変換する(UTF-8 → UTF-16)
    std::wstring aPath=convertUtf8ToUtf16(iPath);
    return boostF::status(aPath);
#else
    return boostF::status(iPath);
#endif
}

// ***************************************************************************
//      TheorideDriver専用処理
// ***************************************************************************

#define THEORIDE_INTERNAL_PRODUCT_NAME "TheorideDriver "
#define THEORIDE_INTERNAL_COPYRIGHT    "Copyright (C) 2017 Yohinori Tahara (Theoride Technology)"

std::string getVersionString()
{
    std::string ret(THEORIDE_INTERNAL_PRODUCT_NAME);
    ret += THEORIDE_INTERNAL_COPYRIGHT "\n";

    return ret;
}

int TheorideProc(std::string const& iExePath, char const* iArg)
{
//----------------------------------------------------------------------------
//      TheorideDriverであることを応答する(バージョン表示を兼ねる)
//----------------------------------------------------------------------------

    if (StringRef(iArg).equals(kTheorideVersionParam))
    {

        llvm::outs() << getVersionString() << "\n";
return 0;
    }

//----------------------------------------------------------------------------
//      バラメータ取り出し
//----------------------------------------------------------------------------

#ifndef DISABLE_REPLACE
    bool    aDoReplace = StringRef(iArg).startswith(kTheorideReplaceParam);
    bool    aDoRestore = StringRef(iArg).startswith(kTheorideRestoreParam);

    if (!aDoReplace && !aDoRestore)
    {
        llvm::errs() << kDiagMarker 
                     << " Unknown Theoride parameter(" << iArg << ")\n";
return 1;
    }

//----------------------------------------------------------------------------
//      元コンパイラの置換／回復
//----------------------------------------------------------------------------

    std::pair<StringRef, StringRef> aCurrent = StringRef(iArg).split('=');
    while (true)
    {
        if (aCurrent.second.empty())
    break;

        aCurrent = aCurrent.second.split(';');

        std::string aTargetPath = aCurrent.first;
        aTargetPath=getCanonicalPath(aTargetPath);
        if (aTargetPath.empty())
return 1;
        std::string aReplacePath = makeRenamePath(aTargetPath);
        CheckResult cr=CheckTheoride(aTargetPath);
        std::error_code ec;
        switch (cr)
        {
        case ecrExecError:
            break;

        case ecrNotTheoride:
            // 相手方がTheorideDriver.exeでないなら置換する
            if (aDoReplace)
            {
                llvm::outs() << "Replacing " << aTargetPath << " ...\n";

                // 元のコンパイラのファイル名を変える
                ec=llvmS::fs::rename(aTargetPath, aReplacePath);
                if (ec)
                {
                    llvm::errs() << kDiagMarker << "Can not rename "
                                 << aTargetPath << " to " << aReplacePath
                                 << "(" << ec.message() << ")\n";
return 1;
                }
                llvm::outs() << "    Renamed " << aTargetPath << " to "
                             << llvmS::path::filename(aReplacePath).str() << "\n";

                // TheorideDriver.exeをターゲットへコピーする
                ec=llvmS::fs::copy_file(iExePath, aTargetPath);
                if (ec)
                {
                    llvm::errs() << kDiagMarker << "Can not copy to "
                                 << aTargetPath << "(" << ec.message() << ")\n";
return 1;
                }
                // パーミッションをTheorideDriver.exeと合わせる。
                boostF::file_status file_status=getFileStatus(iExePath);
                boostF::permissions(aTargetPath, file_status.permissions());
                llvm::outs() << "    Copied " << iExePath << " to "
                             << llvmS::path::filename(aTargetPath).str() << "\n";
                llvm::outs() << "Completed !\n";
            }
            else
            {
                llvm::outs() << "Already restored " << aTargetPath << "\n";
            }
            break;

        case ecrIsTheoride:
            // 相手方がTheorideDriver.exeなら回復する
            if (aDoRestore)
            {
                llvm::outs() << "Restoring " << aTargetPath << " ...\n";

                // ターゲットがTheorideDriver.exeなので削除する
                ec=llvmS::fs::remove(aTargetPath);
                if (ec)
                {
                    llvm::errs() << kDiagMarker << "Can not remove "
                                 << aTargetPath << "(" << ec.message() << ")\n";
return 1;
                }
                llvm::outs() << "    Removed " << aTargetPath << "\n";

                // 元のコンパイラを元の名前へ回復する
                ec=llvmS::fs::rename(aReplacePath, aTargetPath);
                if (ec)
                {
                    llvm::errs() << kDiagMarker << "Can not rename "
                                 << aReplacePath << " to " << aTargetPath
                                 << "(" << ec.message() << ")\n";
return 1;
                }
                llvm::outs() << "    Renamed " << aReplacePath << " to "
                             << llvmS::path::filename(aTargetPath).str() << "\n";
                llvm::outs() << "Completed !\n";
            }
            else
            {
                llvm::outs() << "Already replaced " << aTargetPath << " by "
                             << llvmS::path::filename(iExePath).str() << "\n";
            }
            break;

        default:
            DRIVER_ABORT("Unknown return code : CheckTheoride() = %d\n", cr);
        }
    }
#endif

    return 0;
}

// ***************************************************************************
//      g++の情報取り出し
// ***************************************************************************

std::vector<std::string> getGppInfo(std::string const& iExePath, std::string& oTarget)
{
    std::vector<std::string> includes;

    // パラメータ生成
    llvm::opt::ArgStringList aArgv;
    aArgv.push_back(iExePath.c_str());
    aArgv.push_back("-v");
    aArgv.push_back("-x");
    aArgv.push_back("c++");
    aArgv.push_back("-E");
    aArgv.push_back("-");

    ExecuteMan  aExecuteMan(iExePath, aArgv);
    DRIVER_ASSERT(!aExecuteMan.GetError(),
                  "Execution %s error : %s\n", 
                  llvmS::path::filename(iExePath).str().c_str(),
                  aExecuteMan.GetErrorMessage().c_str());

    // システム・インクルード開始位置
    StringRef aResult = aExecuteMan.GetResult();
    std::size_t start = aResult.find("#include <...>");
    if (start == StringRef::npos)
return includes;

    std::size_t end = aResult.find("End of search list.", start);
    if (end == StringRef::npos)
return includes;

    // パスを取り出す
    std::pair<StringRef, StringRef> aCurrent = aResult.substr(start, end-start).split('\n');
    while (true)
    {
        if (aCurrent.second.empty())
    break;
        aCurrent = aCurrent.second.split('\n');
        StringRef aPath = aCurrent.first.trim();
        if (aPath.size() == 0)
    continue;

        includes.push_back(aPath);
    }

    // ターゲットを取り出す
    static const char aTargetString[]="Target: ";
    static const std::size_t aTargetSringLen=sizeof(aTargetString)-1;
    std::size_t aTargetPos = aResult.find(aTargetString);
    if (aTargetPos != StringRef::npos)
    {
        aCurrent = aResult.substr(aTargetPos+aTargetSringLen).split('\n');
        oTarget=aCurrent.first;
    }

    return includes;
}

// ***************************************************************************
//      コンパイラー情報獲得
//          g++ の場合、'echo "" | g++ -v -x c++ -E -'にて取り出す
//              システム・インクルード："#include <...>
//              ターゲット            ："Target: "
//          msvcの場合
//              システム・インクルード：環境変数INCLUDE
//              ターゲット            ："無し
// ***************************************************************************

std::vector<std::string> GetCompilerInfo
(
    std::string const& iExePath,
    DriverMode iDriverMode,
    std::string& oTarget
)
{
    std::vector<std::string> aIncludes;
    oTarget = "";

    // g++なら、-vにて取り出す
    if (iDriverMode == gpp)
    {
        aIncludes = getGppInfo(iExePath, oTarget);
    }

    // それ以外なら、環境変数INCLUDEから取り出す
    else
    {
        char const* env = ::getenv("INCLUDE");
        if (env)
        {
            std::pair<StringRef, StringRef> aCurrent = StringRef(env).split(';');
            while (!aCurrent.second.empty())
            {
                if (!aCurrent.first.empty())
                {
                    aIncludes.push_back(aCurrent.first);
                }
                aCurrent = aCurrent.second.split(';');
            }
        }
    }

    return aIncludes;
}

// ***************************************************************************
//      元コンパイラ・パラメータ判定
//          iArgv+2がkTheorideOrigCompParamで始まっている時、true返却
//              msvc  :/Dtheoride_original_compiler=<元コンパイラのパス>
//              その他:--theoride_original_compiler=<元コンパイラのパス>
// ***************************************************************************

bool isOrigCompParam(char const* iArgv)
{
    if (!iArgv)
return false;
    if (!*iArgv)
return false;
    if (!*(iArgv+1))
return false;

    return StringRef(iArgv+2).startswith(kTheorideOrigCompParam);
}

// ***************************************************************************
//      メイン
// ***************************************************************************

int callParse
(
    std::string const&              iExecPath,
    llvm::opt::ArgStringList const& iArgs,
    std::string const&              iTarget
);

int main(int iArgc, const char **iArgv)
{
    gExclusiveControl.reset(new ExclusiveControl(kTheorideFileLock));

//----------------------------------------------------------------------------
//      自exeパス名操作
//----------------------------------------------------------------------------

    // 自モジュールのパスを自exeのフル・パスとする
    void *aFuncPtr = (void*)(intptr_t)main;
    std::string aExePath = llvmS::fs::getMainExecutable(iArgv[0], &aFuncPtr);
    aExePath=getCanonicalPath(aExePath);
    if (aExePath.empty())
return 1;

//----------------------------------------------------------------------------
//      パラメータ取り出し
//----------------------------------------------------------------------------

    for (int i=0; i < iArgc; ++i)
    {
        llvm::outs() << "    iArgv[" << i << "] = ";
        if (iArgv[i] == nullptr)
        {
            llvm::outs() << "<nullptr>";
        }
        else
        {
            llvm::outs() << "\"" << iArgv[i] << "\"";
        }
        llvm::outs() << "\n";
    }
    llvm::outs().flush();

    // パラメータをaArgvへ設定する
    SmallVector<char const*, 256> aArgv;
    llvm::SpecificBumpPtrAllocator<char> ArgAllocator;  // ここが実体を記録している
    {
        auto array_ref = llvm::makeArrayRef(iArgv, iArgc);

        // GetArgumentVector()
        //  aArgvにはアクセスしていない。
        //  コマンド・ラインをUnicodeで取り出し、
        //  ワイルド・カード展開後、char型で保存している。
        std::error_code ec = llvmS::Process::GetArgumentVector(aArgv, array_ref, ArgAllocator);
        if (ec)
        {
            llvm::errs() << kDiagMarker
                         << "Couldn't get arguments = " << ec.message() << "\n";
            return 1;
        }
    }

    // 応答ファイルをaArgvへ展開する
#if (LLVM_VERSION_MAJOR < 3) || ((LLVM_VERSION_MAJOR == 3) && (LLVM_VERSION_MINOR < 7))
    #error "Not support LLVM Version"
#elif (LLVM_VERSION_MAJOR == 3) && (LLVM_VERSION_MINOR == 7)    // 3.7.0
    llvm::BumpPtrAllocator aAllocator;
    llvm::BumpPtrStringSaver Saver(aAllocator);
#else
    llvm::BumpPtrAllocator A;
    llvm::StringSaver Saver(A);
#endif
#if defined(_WIN32)
    llvm::cl::ExpandResponseFiles(Saver, llvm::cl::TokenizeWindowsCommandLine, aArgv, true);
#else
    llvm::cl::ExpandResponseFiles(Saver, llvm::cl::TokenizeGNUCommandLine, aArgv, true);
#endif

//----------------------------------------------------------------------------
//      Theoride用パラメータ処理
//----------------------------------------------------------------------------

    // ドライバー・モード設定とマクロ定義パラメータ形式決定
    DriverMode aDriverMode = getDriverMode(llvmS::path::stem(aExePath));

    // Theorideドライバ・モード判定準備
    bool aDefining=false;

    // パラメータ・チェック
    bool aIsClangHelp = false;
    bool aIsVersion = false;
    bool aIsNostdinc = false;
    bool aIsNostdincpp = false;
    bool aIsOptionv = false;
    std::string aOriginalPath;

    for (auto&& arg : aArgv)
    {
        if (arg == nullptr)
    continue;

        // オリジナル・コンパイラ取り出し
        if ((aDefining && StringRef(arg).startswith(kTheorideOrigCompParam))
              || (isOrigCompParam(arg)))
        {
            aDefining=false;

            std::pair<StringRef, StringRef> aCurrent = StringRef(arg).split('=');
            if (aCurrent.second.empty())
            {
                llvm::errs() << kDiagMarker
                             << kTheorideOrigCompParam << " path is null.\n";
return 1;
            }

            aOriginalPath=aCurrent.second;
            aOriginalPath=getCanonicalPath(aOriginalPath);
            aDriverMode = getDriverMode(llvmS::path::stem(aOriginalPath));
    continue;
        }

        // -D(/D)処理中なら、現在はは定義シンボルなので解析スキップ
        if (aDefining)
        {
            aDefining=false;
    continue;
        }

        // -D(/D)の次にブランクのある場合の対処
        if (StringRef(arg+1).equals("D"))
        {
            aDefining=true;
    continue;
        }
        aDefining=false;

        if (StringRef(arg).startswith(ARG_THEORIDE))
return TheorideProc(aExePath, arg);
        if (StringRef(arg).equals("-help"))         aIsClangHelp  = true;
        if (StringRef(arg).equals("--help"))        aIsClangHelp  = true;
        if (StringRef(arg).equals("--version"))     aIsVersion    = true;
        if (StringRef(arg).equals("-nostdinc"))     aIsNostdinc   = true;
        if (StringRef(arg).equals("-nostdinc++"))   aIsNostdincpp = true;
        if (StringRef(arg).equals("-v"))            aIsOptionv    = true;
    }

#ifndef DISABLE_REPLACE
    // 元コンパイラのパス名をaExePathから生成する
    if (aOriginalPath.empty())
    {
        aOriginalPath = makeRenamePath(aExePath);
    }
#else
    if (aOriginalPath.empty())
    {
        llvm::errs() << kDiagMarker
                     << "No " << kTheorideOrigCompParam << " option.\n";
return 1;
    }
#endif

//----------------------------------------------------------------------------
//      Theoride解析実行判定
//----------------------------------------------------------------------------

    if (true)
    {
        // ライセンス表示
#if 0
        if ((aDriverMode != gpp) || (aIsVersion) || (aIsOptionv))
        {
            llvm::outs() << getVersionString() << "\n";
            llvm::outs() << "\n";
            llvm::outs().flush();
        }
#endif

//----------------------------------------------------------------------------
//      パラメータをclang用に変換する
//----------------------------------------------------------------------------

        // clang helpが含まれていたら、clangのhelp messageを出したくないので、
        // スキップする。
        int aRet=0;
        if (!aIsClangHelp)
        {
            // 警告・エラー表示環境準備(最低限)
            IntrusiveRefCntPtr<DiagnosticOptions> aDiagOpts = new DiagnosticOptions;
            DiagnosticConsumer* diag_client =
                            new TheorideDiagnosticConsumer(llvm::errs(), &*aDiagOpts);
            IntrusiveRefCntPtr<DiagnosticIDs> aDiagId(new DiagnosticIDs());
            DiagnosticsEngine Diags(aDiagId, &*aDiagOpts, diag_client);

            // ドライバー生成
            clangD::Driver TheDriver(aExePath, llvmS::getDefaultTargetTriple(), Diags);

            // ドライバー・モード用パラメータ設定
            switch (aDriverMode)
            {
            case gpp:
                aArgv.insert(aArgv.begin()+1, "--driver-mode=g++");
                break;

            case cl:
                aArgv.insert(aArgv.begin()+1, "--driver-mode=cl");
                aArgv.push_back("-Wno-ignored-attributes");// 標準ライブラリ警告を防ぐ
                aArgv.push_back("-Wno-deprecated-declarations");
                aArgv.push_back("-Wno-implicit-exception-spec-mismatch");
                aArgv.push_back("-Wno-delete-incomplete");
                aArgv.push_back("-Wno-microsoft");
                aArgv.push_back("-Wno-undefined-inline");
                aArgv.push_back("-Wno-writable-strings");
                aArgv.push_back("-Wno-dllimport-static-field-def");
                aArgv.push_back("-Wno-dangling-else");
                break;

            case cpp:
                aArgv.insert(aArgv.begin()+1, "--driver-mode=cpp");
                break;

            default:
                break;
            }

            // システム・インクルード・パス追加
            //  msvcはclangに任せる(環境変数だけではうまくいかない)
            if ((aDriverMode != cl) && !aIsNostdinc)    aArgv.push_back("-nostdinc");
            if ((aDriverMode != cl) && !aIsNostdincpp)  aArgv.push_back("-nostdinc++");

            // システム・インクルード・パス生成
            std::string  aTarget;
            std::vector<std::string> aIncludes=
                GetCompilerInfo(aOriginalPath, aDriverMode, aTarget);

            // インクルード・パス追加
            for(auto& include : aIncludes)
            {
                if (aDriverMode == cl)
                {
                    include.insert(0, "/I");
                    aArgv.push_back(include.c_str());
                }
                else
                {
                    aArgv.push_back("-I");
                    aArgv.push_back(include.c_str());
                }
            }

            if (aTarget.size())
            {
                aTarget.insert(0, "-aTarget=");
                aArgv.push_back(aTarget.c_str());
            }

            // Jobs分解する
            std::unique_ptr<clangD::Compilation> compilation(TheDriver.BuildCompilation(aArgv));

            if (aIsVersion)
            {
                llvm::outs() << "\n";
            }

//----------------------------------------------------------------------------
//      AST解析とソース修正呼び出し処理
//----------------------------------------------------------------------------

            clangD::JobList &jobs = compilation->getJobs();

            // ジョブ毎に処理
            int i=-1;
            for(const auto &job : jobs)
            {
                ++i;

                // コマンド取り出し
                const clangD::Command *command = dyn_cast<clangD::Command>(&job);
                if (!command)
            continue;

                // Theoride Driver以外なら非対象
                if (aExePath != command->getExecutable())
            continue;

                // オプション取り出し
                const llvm::opt::ArgStringList &args = command->getArguments();

                // -cc1でなければ非対象
                if (!args.size() || (!StringRef(*args.begin()).equals("-cc1")))
            continue;

                int ret=0;
                do
                {
                    boostI::sharable_lock<ExclusiveControl> lock(*gExclusiveControl);
                    gRetryAST=false;

                    // AST解析とソース修正
                    ret=callParse(aExePath, args, aTarget);
                }
                while(gExclusiveControl->getRedoRequest() || gRetryAST);
                if (ret) aRet=ret;
            }
        }
        if (aRet)
return aRet;
    }

//----------------------------------------------------------------------------
//      元コンパイラ呼び出し
//----------------------------------------------------------------------------

    llvm::opt::ArgStringList   aArgvForCall;
    aArgvForCall.push_back(aOriginalPath.c_str());
    char const* aDefArg=nullptr;
    for (int i=1; i < iArgc; ++i)
    {
        if (StringRef(iArgv[i]).equals(""))
    continue;

        if ((aDefining && StringRef(iArgv[i]).startswith(kTheorideOrigCompParam))
         || (isOrigCompParam(iArgv[i])))
        {
            aDefArg=nullptr;
    continue;
        }

        if (aDefArg)
        {
            aArgvForCall.push_back(aDefArg);
            aArgvForCall.push_back(iArgv[i]);
            aDefArg=nullptr;
    continue;
        }

        if (StringRef(iArgv[i]+1).equals("D"))
        {
            aDefArg=iArgv[i];
    continue;
        }
        aDefArg=nullptr;

        if (StringRef(iArgv[i]).startswith(ARG_THEORIDE))
    continue;

        aArgvForCall.push_back(iArgv[i]);
    }

    // 番兵
    aArgvForCall.push_back(nullptr);

    std::string error_essage;
    bool execution_failed;

    int ret=0;
    {
        boostI::sharable_lock<ExclusiveControl> lock(*gExclusiveControl);

        ret = llvmS::ExecuteAndWait(aOriginalPath, aArgvForCall.data(), nullptr,
                                    nullptr, 0, 0, &error_essage,
                                    &execution_failed);
    }

    llvm::llvm_shutdown();

    return ret;
}

// ***************************************************************************
//      ソース不正修正エラー表示用マクロ(解放)
// ***************************************************************************

#undef ERROR
#undef ERROR_ST

// ***************************************************************************
//      Windows向け文字コード変換
// ***************************************************************************

#ifdef _WIN32
#include <boost/locale.hpp>
namespace boostLC=boost::locale::conv;

std::wstring convertUtf8ToUtf16(std::string const& iString)
{
    return boostLC::utf_to_utf<wchar_t, char>(iString);
}
std::string convertUtf16ToUtf8(std::wstring const& iString)
{
    return boostLC::utf_to_utf<char, wchar_t>(iString);
}
#endif

//############################################################################
//      コンパイラの相違点を吸収する
//############################################################################

// ***************************************************************************
//      POSIX関係
// ***************************************************************************

#ifdef _MSC_VER
#include <windows.h>
int chdir(char const* iPath)
{
    BOOL ret = SetCurrentDirectoryA(iPath);
    return (ret)?0:-1;
}

char* GetCurrentDirName()
{
    DWORD   len;
    len = GetCurrentDirectory(0, nullptr);
    char* ret = (char*)malloc(len);
    GetCurrentDirectoryA(len, ret);

    return ret;
}
#else
char* GetCurrentDirName()
{
    return getcwd(nullptr, 0);
}
#endif

//############################################################################
//      libToolingで発生した例外処理
//############################################################################

// ***************************************************************************
//      WindowsのSEHやLinuxのSIGNALを捕まえるため、boost::execution_monitorを使う
// ***************************************************************************

#ifdef _MSC_VER     // start of disabling MSVC warnings
    #pragma warning(disable:4702 4717 4718 4913)
#endif

#undef new          // 誰かがデバッグ・ビルド時に#defineしており、boostと干渉する

#define BOOST_NO_EXCEPTION  // boost 1.59.0対応
//  BOOST_NO_EXCEPTIONSではなくBOOST_NO_EXCEPTION(Sがない)でboot::throw_exceptionが定義される)

#define BOOST_TEST_NO_MAIN
#include <boost/test/included/prg_exec_monitor.hpp>

// ***************************************************************************
//      中継用ファンクタ
// ***************************************************************************

struct ParseCaller
{
    ParseCaller
    (
        std::string const&              iExecPath,
        llvm::opt::ArgStringList const& iArgs,
        std::string const&              iTarget
    ) : mExecPath(iExecPath),
        mArgs(iArgs),
        mTarget(iTarget)
    { }

    int operator()() { return parse(mExecPath, mArgs, mTarget); }

private:
    std::string const&              mExecPath;
    llvm::opt::ArgStringList const& mArgs;
    std::string const&              mTarget;
};

// ***************************************************************************
//      例外キャッチ用中継関数
// ***************************************************************************

int callParse
(
    std::string const&              iExecPath,
    llvm::opt::ArgStringList const& iArgs,
    std::string const&              iTarget
)
{
    int result=0;
    try
    {
        gLastErrorMessage="";
        result = ::boost::execution_monitor().execute(ParseCaller(iExecPath, iArgs, iTarget));
    }
    catch(std::exception& e)
    {
        llvm::errs() << "\n**** std::exception : " << e.what() << "\n";
        result = 1;
    }
    catch(...)
    {
        llvm::errs() << "\n**** Unknown exception from clang :  " << gLastErrorMessage << "\n";
        result = 2;
    }

    return result;
}
