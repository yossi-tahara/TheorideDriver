//############################################################################
//      AST解析
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

#if !defined(THEOLIZER_PARSE_H)
#define THEOLIZER_PARSE_H

// ***************************************************************************
//      デバッグ用グローバル変数
// ***************************************************************************

StringRef   gProcessingFile="";     // 処理中ソース・ファイル名

// ***************************************************************************
//      Decl処理
// ***************************************************************************

class PPCallbacksTracker : public PPCallbacks
{
public:
    PPCallbacksTracker(AstInterface& oAstInterface) :
        mAstInterface(oAstInterface)
    {
    }

private:
    AstInterface&       mAstInterface;      // ソース修正処理とのI/F

//----------------------------------------------------------------------------
//      #define位置獲得
//----------------------------------------------------------------------------

    void MacroDefined(Token const& iMacroNameTok, MacroDirective const* iMD) override
    {
        StringRef aMacroName=StringRef(iMacroNameTok.getIdentifierInfo()->getNameStart());

        // 解析しない
        if (aMacroName.equals("THEOLIZER_NO_ANALYZE"))
        {
            mAstInterface.mNotParse=true;
        }
    }
};

// ***************************************************************************
//      ASTConsumer派生クラス
// ***************************************************************************

class ASTVisitor : public DeclVisitor<ASTVisitor, bool>
{
public:

//----------------------------------------------------------------------------
//      class/structの処理
//          enumは非侵入型処理なので、structで実装されることになる。
//          2015/05/14現在enumは非対応。
//
//          2016/01/06
//          TheolizerVersion<>も直接呼ばれる。
//              恐らく、クラス外定義しているから呼ばれると思う。
//          また、TheolizerVersion<>のdecls()に再度TheolizerVersion<>が現れる。
//              こちらはgetQualifiedNameAsString()でテンプレート・パラメータも
//              含まれる。取り除く方法がないか幾つかトライしたが失敗。
//          入口でTheolizerVersion<>判定して処理する。
//----------------------------------------------------------------------------

public:
    virtual bool VisitCXXRecordDecl(CXXRecordDecl *iCXXRecordDecl)
    {
        if (iCXXRecordDecl->isThisDeclarationADefinition())
        {
#if 0
            for (auto decl : iCXXRecordDecl->decls())
            {
                Visit(decl);
            }
#else
            enumerateDecl(iCXXRecordDecl, mSearchEnd);
#endif
        }
        return true;
    }

//----------------------------------------------------------------------------
//      関数処理
//          GenerationMarkerStart(), GenerationMarkerEnd()の位置を取り出す。
//----------------------------------------------------------------------------

//      ---<<< 文字列獲得 >>>---

    std::string pullupString(clang::Expr* iExpr)
    {
        // StmtClass名テーブル
        constexpr static char const* StmtClassTable[]=
        {
            "NoStmtClass",
            #define ABSTRACT_STMT(STMT)
            #define STMT(CLASS, PARENT) #CLASS,
            #include "clang/AST/StmtNodes.inc"
            nullptr
        };

llvm::outs() << "pullupString()--------------\n";
llvm::outs().flush();

llvm::outs() << "    iExpr->getStmtClass()=" << iExpr->getStmtClass()
             << " (" << StmtClassTable[iExpr->getStmtClass()] << ")\n";

        switch(iExpr->getStmtClass())
        {
        case clang::Stmt::ImplicitCastExprClass:
            {
                clang::ImplicitCastExpr* ice = static_cast<clang::ImplicitCastExpr*>(iExpr);
                clang::Expr* aSubExpr = ice->getSubExpr();
    return pullupString(aSubExpr);
            }

        case clang::Stmt::StringLiteralClass:
            {
                StringRef aString = static_cast<clang::StringLiteral*>(iExpr)->getString();
    return aString.str();
            }

        case clang::Stmt::DeclRefExprClass:
            {
                clang::ValueDecl* vd =
                    static_cast<clang::DeclRefExpr*>(iExpr)->getDecl();
llvm::outs() << "    vd=" << vd << "\n";
                if (!vd)
    return "";
                clang::VarDecl* var = dyn_cast<clang::VarDecl>(vd);
llvm::outs() << "    var=" << var << "\n";
                if (!var)
    return "";
llvm::outs().flush();
var->dump();

                clang::Expr* init = var->getInit();
llvm::outs() << "    init=" << init << "\n";
                if (!init)
    return "";

    return pullupString(init);
            }
        }

        return "";
    }

//      ---<<< 本体 >>>---

    virtual bool VisitFunctionDecl(clang::FunctionDecl* iFunctionDecl)
    {
        if (!iFunctionDecl->getIdentifier())
    return true;

        if (!iFunctionDecl->getName().startswith("GenerationMarker"))
    return true;

        llvm::outs() << iFunctionDecl->getNameAsString() << "\n";
llvm::outs().flush();

//      ---<<< パラメータ取り出し >>>---

        // GenerationMarkerEnd()処理中
        if (mSearchEnd)
        {
            if ((iFunctionDecl->getName() == "GenerationMarkerEnd")
             && (iFunctionDecl->param_size() != 1))
            {
                gCustomDiag.ErrorReport(iFunctionDecl->getLocation(),
                    "Do not modify the source generated.");
    return true;
            }
        }

        // GenerationMarkerStart()処理中
        else
        {
            if ((iFunctionDecl->getName() == "GenerationMarkerStart")
             && (iFunctionDecl->param_size() != 2))
            {
                gCustomDiag.ErrorReport(iFunctionDecl->getLocation(),
                    "GenerationMarkerStart() has 2 parameters. Second is inc-file path.");
    return true;
            }

            // 第2パラメータがchar const*であることをチェック
            bool aIsCharConstPointer = false;
            clang::ParmVarDecl* aParam=iFunctionDecl->getParamDecl(1);
            QualType qt = aParam->getType();
llvm::outs() << "GenerationMarkerStart( ," << qt.getAsString() << ")\n";
llvm::outs() << "    " << qt->isPointerType() << "\n";

            clang::PointerType const* pt = qt->getAs<clang::PointerType>();
            if (pt)
            {
                QualType pointee = pt->getPointeeType();
llvm::outs() << "    " << pointee.getAsString() << "\n";
llvm::outs() << "    " << pointee.isConstQualified() << ", " << pointee->isCharType() << "\n";
                if (pointee.isConstQualified() && pointee->isCharType())
                {
                    aIsCharConstPointer = true;
llvm::outs() << "    OK!!\n";
                }
            }
llvm::outs().flush();
            if (!aIsCharConstPointer)
            {
                gCustomDiag.ErrorReport(aParam->getLocation(),
                    "GenerationMarkerStart() second parameter is %0.(modify to 'char const*')")
                    << qt.getAsString();
    return true;
            }


#if 1
aParam->dump();
            clang::Expr* aDefault = aParam->getDefaultArg();
            if (aDefault == nullptr)
            {
                gCustomDiag.ErrorReport(aParam->getLocation(),
                    "GenerationMarkerStart() second parameter has default.(inc-file path)");
    return true;
            }
            std::string aString = pullupString(aDefault);
llvm::outs() << "    aString=" << aString << "\n";
llvm::outs().flush();
            if (aString.empty())
            {
                gCustomDiag.ErrorReport(aParam->getLocation(),
                    "GenerationMarkerStart() unknown error.");
    return true;
            }
#else
            if (aDefault == nullptr)
            {
                gCustomDiag.ErrorReport(aParam->getLocation(),
                    "GenerationMarkerStart() second parameter has default.(inc-file path)");
    return true;
            }
llvm::outs() << "    aDefault->getStmtClass()=" << aDefault->getStmtClass()
             << " (" << StmtClassTable[aDefault->getStmtClass()] << ")\n";
            if (aDefault->getStmtClass() != clang::Stmt::ImplicitCastExprClass)
            {
            }
            clang::ImplicitCastExpr* ice = static_cast<clang::ImplicitCastExpr*>(aDefault);
            clang::Expr* aSubExpr = ice->getSubExpr();

llvm::outs() << "    aSubExpr->getStmtClass()=" << aSubExpr->getStmtClass()
             << " (" << StmtClassTable[aSubExpr->getStmtClass()] << ")\n";
llvm::outs() << "    StringLiteralClass=" << clang::Stmt::StringLiteralClass << "\n";

            std::string aIncPath;
            if (aSubExpr->getStmtClass() == clang::Stmt::StringLiteralClass)
            {
                StringRef aString = static_cast<clang::StringLiteral*>(aSubExpr)->getString();
                aIncPath = aString.str();
            }
            else if (aSubExpr->getStmtClass() == clang::Stmt::DeclRefExprClass)
            {
                clang::ValueDecl* vd =
                    static_cast<clang::DeclRefExpr*>(aSubExpr)->getDecl();
vd->dump();
            }
            else
            {
                gCustomDiag.ErrorReport(aParam->getLocation(),
                    "GenerationMarkerStart() unknown error.(StringLiteral)");
    return true;
            }
llvm::outs() << "    aIncPath=" << aIncPath << "\n";
llvm::outs().flush();
aParam->dump();
#endif

        }

        // 対象の型取り出し
        clang::ParmVarDecl* aParam=iFunctionDecl->getParamDecl(0);
        QualType qt = aParam->getType();
        switch (qt->getTypeClass())
        {
        case Type::Enum:
            {
llvm::outs() << "enum : " << qt.getAsString() << "\n";
llvm::outs().flush();
                EnumType const* et = qt->getAs<EnumType>();
                ERROR(!et, iFunctionDecl, true);

                EnumDecl* aTargetEnum = et->getDecl();
                ERROR(!aTargetEnum, iFunctionDecl, true);
                if (mSearchEnd)
                {
                    mAstInterface.addEndMarker(aTargetEnum, iFunctionDecl->getLocation());
                }
                else
                {
                }
            }
            break;

        case Type::Record:
llvm::outs() << "class : " << qt.getAsString() << "\n";
llvm::outs().flush();
            break;
        }

#if 0

//      ---<<< 自動生成位置記録 >>>---

FullSourceLoc loc = gASTContext->getFullLoc(iFunctionDecl->getLocStart());
ASTANALYZE_OUTPUT("    TheorideMarker(",
                  wUniqueClass->getQualifiedNameAsString(), ") : ",
                  loc.printToString(*gSourceManager));
ASTANALYZE_OUTPUT("    wUniqueClass=", wUniqueClass);
        mAstOutput.mGeneratingLocations.emplace
        (
            wUniqueClass,
            iFunctionDecl
        );
#endif
        return true;
    }

//----------------------------------------------------------------------------
//      各種追跡処理
//----------------------------------------------------------------------------

//      ---<<< namespace >>>---

    virtual bool VisitNamespaceDecl(NamespaceDecl *iNamespaceDecl)
    {
        StringRef name = iNamespaceDecl->getName();

        // std名前空間は処理しない
        // 他にも不要な名前空間は処理しない方がよいが、特定できない。
        if (name.equals("std"))
        {
            AutoFalse auto_false(mIsStdSpace);
            enumerateDecl(iNamespaceDecl, mSearchEnd);
    return true;
        }

        enumerateDecl(iNamespaceDecl, mSearchEnd);
        return true;
    }

//      ---<<< extern "C"/"C++" >>>---

    virtual bool VisitLinkageSpecDecl(LinkageSpecDecl *iLinkageSpecDecl)
    {
        switch (iLinkageSpecDecl->getLanguage())
        {
        case LinkageSpecDecl::lang_c:           // "C" : NOP
            enumerateDecl(iLinkageSpecDecl, mSearchEnd);
            break;

        case LinkageSpecDecl::lang_cxx:         // "C++"
            enumerateDecl(iLinkageSpecDecl, mSearchEnd);
            break;
        }
        return true;
    }

//----------------------------------------------------------------------------
//      Declリスト1レベルの枚挙処理
//----------------------------------------------------------------------------

    bool mSearchEnd;
public:
    void enumerateDecl(DeclContext *iDeclContext, bool iSearchEnd)
    {
        mSearchEnd = iSearchEnd;
        for (auto decl : iDeclContext->decls())
        {
            Visit(decl);
        }
    }

//----------------------------------------------------------------------------
//      基本
//----------------------------------------------------------------------------

//      ---<<< 出力 >>>---

private:
    AstInterface&       mAstInterface;          // ソース修正処理とのI/F

//      ---<<< 内部処理 >>>---

    bool    mIsStdSpace;                        // std名前空間処理中

public:
    ASTVisitor(AstInterface& iAstInterface) :
        mSearchEnd(false),
        mAstInterface(iAstInterface),
        mIsStdSpace(false)
    { }
};

// ***************************************************************************
//      プリプロセッサからのコールバック処理
//          AST解析を実行するかどうか判定する
// ***************************************************************************

class TheolizerASTConsumer : public ASTConsumer
{
private:

//----------------------------------------------------------------------------
//      解析実行判定
//----------------------------------------------------------------------------

    // AST解析結果の受取(トップレベル毎に呼ばれる)
    virtual bool HandleTopLevelDecl (DeclGroupRef iDeclGroupRef)
    {
        // 致命的エラーが発生していたら解析しない
        if (gCustomDiag.getDiags().hasFatalErrorOccurred())
        {
            mAstInterface.mNotParse=true;
        }

        return !mAstInterface.mNotParse;
    }

//----------------------------------------------------------------------------
//      AST解析とソース修正処理呼び出し
//----------------------------------------------------------------------------

    bool    mSearchEnd;
    virtual void HandleTranslationUnit(ASTContext &iContext)
    {
        // GenerationMarkerEnd()を見つけておく
        mASTVisitor.enumerateDecl(iContext.getTranslationUnitDecl(), true);

        // GenerationMarkerStart()を処理する
        mASTVisitor.enumerateDecl(iContext.getTranslationUnitDecl(), false);
    }

//----------------------------------------------------------------------------
//      生成と削除
//----------------------------------------------------------------------------

private:
    Preprocessor&       mPreprocessor;      // プリプロセッサ
    ASTVisitor          mASTVisitor;        // Visitor処理用クラス
    AstInterface        mAstInterface;      // AST解析処理とのI/F
    bool                mMadeDefaultSource; // デフォルトの自動生成ソースを作成した

public:
    explicit TheolizerASTConsumer(CompilerInstance *iCompilerInstance) :
                    mPreprocessor(iCompilerInstance->getPreprocessor()),
                    mASTVisitor(mAstInterface),
                    mMadeDefaultSource(false)
    {
        // グローバル変数群の初期化
        gCompilerInstance = iCompilerInstance;
        gASTContext = &(gCompilerInstance->getASTContext());
        gSourceManager = &(gASTContext->getSourceManager());
        gCustomDiag.setDiagnosticsEngine(&(gCompilerInstance->getDiagnostics()));

        // プリプロセッサからのコールバック登録
        mPreprocessor.addPPCallbacks(std::unique_ptr<PPCallbacksTracker>(
            new PPCallbacksTracker(mAstInterface)));
    }
    ~TheolizerASTConsumer()
    {
        gCustomDiag.resetDiagnosticsEngine();
    }
};

//----------------------------------------------------------------------------
//      メイン処理からの中継
//----------------------------------------------------------------------------

class TheolizerFrontendAction : public SyntaxOnlyAction /*ASTFrontendAction*/
{
public:
    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(
                        CompilerInstance &iCompilerInstance, StringRef file)
    {
        return llvm::make_unique<TheolizerASTConsumer>(&iCompilerInstance); 
    }
};

// ***************************************************************************
//      AST解析用パラメータのスキップ解析
// ***************************************************************************

struct SkipList
{
  const char *name;
  const int  skip_num;
};

int check_skip(char const* iArg)
{
    static const SkipList skip_list[] =
    {
        // unsupported
        {"--dependent-lib", 1},

        // unknowm
        {"-cc1", 1},
        {"-triple", 2},
        {"-main-file-name", 2},
        {"-mrelocation-model", 2},
        {"-mdisable-fp-elim", 1},
        {"-pic-level", 2},
        {"-relaxed-aliasing", 1},
        {"-masm-verbose", 1},
        {"-mconstructor-aliases", 1},
        {"-munwind-tables", 1},
        {"-target-cpu", 2},
        {"-fdiagnostics-format", 2},
        {"-fdeprecated-macro", 1},
        {"-fdebug-compilation-dir", 2},
//      {"-ferror-limit", 2},
        {"-fmessage-length", 2},
        {"-coverage-file", 2},  // Visual Studioからビルドする時、BuildCompilationで生成された。
        {"-vectorize-loops", 1},// Visual Studioでコンパイル・エラーになってしまうので取り除く。
        {"-vectorize-slp", 1},
        {"-fno-rtti-data", 1},

        // unused
        {"-emit-obj", 1},           // a.oファイルを出力する
        {"-mrelax-all", 1},         // 内部アセンブラ用オプション
        {"-disable-free", 1},       // 終了時、メモリ開放しない
        {"-fms-volatile", 1},       // 意味が分からない
        {"-dwarf-column-info", 1},  // DWARFなので恐らくリンカー・オプション
//      {"-internal-isystem", 2},   // システム・インクルード指定だが使ってない模様
    };

    for (std::size_t i = 0; i < llvm::array_lengthof(skip_list); ++i)
    {
        if (StringRef(iArg).startswith(skip_list[i].name))
return skip_list[i].skip_num;
    }

    return 0;
}

// ***************************************************************************
//      AST解析メイン処理
// ***************************************************************************

static llvm::cl::OptionCategory MyToolCategory("Theolizer Driver");
int parse
(
    std::string const&              iExecPath,
    llvm::opt::ArgStringList const& iArgs,
    std::string const&              iTarget
)
{

//----------------------------------------------------------------------------
//      CommonOptionsParserへ渡すためのパラメータ生成
//----------------------------------------------------------------------------
llvm::outs() << "parse(" << iTarget << ")\n";
llvm::outs().flush();

    llvm::opt::ArgStringList aArgStringList;

    aArgStringList.push_back(iExecPath.c_str());    // argv[0]=exe

    auto aFileArg=iArgs.end()-1;
    gProcessingFile = StringRef(*aFileArg);
    aArgStringList.push_back(*aFileArg);            // argv[1]=ファイル名
    aArgStringList.push_back("--");

    // デフォルトの自動生成先ファイル名
    gCompilingSourceName=std::string(*aFileArg);

    // CommonOptionsParser::getSourcePathList()は、
    // 内部にstaticにソース・リストを保持しており、
    // コンストラクトされる度にソース・リストへ追加される。
    // 対策のため、別途ソース・リストを用意する。
    std::vector<std::string>  aSourceList;
    aSourceList.push_back(*aFileArg);

    bool        x_flag = false;
    int         skip_num = 0;                       // 解析スキップするargvの数
    std::string error_limit;                        // -error-limit記憶領域(最後のみ有効)
    std::string message_length;                     // -message-length記憶領域(最後のみ有効)
    for (auto arg = iArgs.begin(); arg != iArgs.end(); ++arg)
    {
        // ファイル名はスキップする。
        if (arg == aFileArg)
    continue;

        // -xフラグ確認(cppのみ処理する。プリプロセス済のものは処理しない。)
        if (x_flag)
        {
            x_flag = false;

            if ((!StringRef(*arg).equals("c++"))
             && (!StringRef(*arg).equals("c++-header")))
return 0;
        }

        if (StringRef(*arg).equals("-x"))
            x_flag=true;

        // '-ferror-limit'のフォーマットをCommonOptionsParser用に戻す
        if (StringRef(*arg).equals("-ferror-limit"))
        {
            // ユーザ・クラスのバージョンアップ時に多数のエラーが出るのでエラー停止無し
            error_limit = "-ferror-limit=0";
//          error_limit = error_limit + *(arg+1);
            aArgStringList.push_back(error_limit.c_str());
            skip_num=2;
        }

        // '-fmessage-length'のフォーマットをCommonOptionsParser用に戻す
        if (StringRef(*arg).equals("-fmessage-length"))
        {
            message_length = "-fmessage-length=";
            message_length = message_length + *(arg+1);
            aArgStringList.push_back(message_length.c_str());
            skip_num=2;
        }

        // これがないと、char16_t, char32_tについて未定義エラーとなる(llvm 3.7.0+MSVC 2015)
        if (StringRef(*arg).equals("-fms-compatibility-version=18"))
        {
            aArgStringList.push_back("-fms-compatibility-version=19");
            skip_num=1;
        }

        // 解析スキップ処理しつつ、パラメータ登録
        if (!skip_num)
        {
            skip_num = check_skip(*arg);
        }

        if (skip_num)
        {
            --skip_num;
        }
        else
        {
            aArgStringList.push_back(*arg);
        }
    }

    // これがないと、tryとthrowで例外対応していないエラーが出る(llvm 3.7.0+MSVC 2015)
    aArgStringList.push_back("-fexceptions");

    // ターゲットをあわせる
    if (iTarget.size())
    {
        aArgStringList.push_back(iTarget.c_str());
    }

//----------------------------------------------------------------------------
//      AST解析実行
//----------------------------------------------------------------------------

    int argc = static_cast<int>(aArgStringList.size());
    char const** argv = aArgStringList.begin();

    // パラメータ解析と実行
    clang::tooling::CommonOptionsParser op(argc, argv, MyToolCategory);
    clang::tooling::ClangTool Tool(op.getCompilations(), aSourceList);
    IntrusiveRefCntPtr<DiagnosticOptions>   diag_opts = new DiagnosticOptions;
    TheolizerDiagnosticConsumer    DiagClient(llvm::errs(), &*diag_opts);
    Tool.setDiagnosticConsumer(&DiagClient);
    int ret=Tool.run(clang::tooling::newFrontendActionFactory<TheolizerFrontendAction>().get());

    return ret;
}

#endif
