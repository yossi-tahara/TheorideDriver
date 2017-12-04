//############################################################################
//      AST解析
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

#if !defined(THEORIDE_PARSE_H)
#define THEORIDE_PARSE_H

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
        if (aMacroName.equals("THEORIDE_NO_ANALYZE"))
        {
            mAstInterface.mNotParse=true;
        }
        // 処理する
        else if (aMacroName.equals("THEORIDE_DO_PROCESS"))
        {
            mAstInterface.mNotParse=false;
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
//          メソッドとしてのGenerationMarker*処理のため
//----------------------------------------------------------------------------

public:
    virtual bool VisitCXXRecordDecl(CXXRecordDecl *iCXXRecordDecl)
    {
        if (iCXXRecordDecl->isThisDeclarationADefinition())
        {
            enumerateDecl(iCXXRecordDecl);
        }
        return true;
    }

//----------------------------------------------------------------------------
//      文字列リテラル獲得
//----------------------------------------------------------------------------

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
                if (!vd)
    return "";
                clang::VarDecl* var = dyn_cast<clang::VarDecl>(vd);
                if (!var)
    return "";

                clang::Expr* init = var->getInit();
                if (!init)
    return "";

    return pullupString(init);
            }
        }

        return "";
    }

//----------------------------------------------------------------------------
//      関数処理
//          GenerationMarkerStart(), GenerationMarkerEnd()の位置を取り出す。
//----------------------------------------------------------------------------

    virtual bool VisitFunctionDecl(clang::FunctionDecl* iFunctionDecl)
    {
        if (!iFunctionDecl->getIdentifier())
    return true;

        if (!iFunctionDecl->getName().startswith("GenerationMarker"))
    return true;

        llvm::outs() << iFunctionDecl->getNameAsString() << "\n";

//      ---<<< 処理対象判定 >>>---

        // GenerationMarkerEnd()処理中(自動生成ソース範囲調査中)
        if (!mModifing)
        {
            if (iFunctionDecl->getName() != "GenerationMarkerEnd")
    return true;

            if (iFunctionDecl->param_size() != 1)
            {
                gCustomDiag.ErrorReport(iFunctionDecl->getLocation(),
                    "Do not modify the source generated.");
    return true;
            }
        }

        // GenerationMarkerStart()処理中(ソース生成中)
        else
        {
            if (iFunctionDecl->getName() != "GenerationMarkerStart")
    return true;

            if (iFunctionDecl->param_size() != 2)
            {
                gCustomDiag.ErrorReport(iFunctionDecl->getLocation(),
                    "GenerationMarkerStart() has 2 parameters. Second is inc-file path.");
    return true;
            }
        }

//      ---<<< 第2パラメータのデフォルト値取り出し >>>---

        std::string aIncFileName;
        if (mModifing)
        {
            // 第2パラメータがchar const*であることをチェック
            bool aIsCharConstPointer = false;
            clang::ParmVarDecl* aParam=iFunctionDecl->getParamDecl(1);
            QualType qt = aParam->getType();

            clang::PointerType const* pt = qt->getAs<clang::PointerType>();
            if (pt)
            {
                QualType pointee = pt->getPointeeType();
                if (pointee.isConstQualified() && pointee->isCharType())
                {
                    aIsCharConstPointer = true;
                }
            }
            if (!aIsCharConstPointer)
            {
                gCustomDiag.ErrorReport(aParam->getLocation(),
                    "GenerationMarkerStart() second parameter is %0.(modify to 'char const*')")
                    << qt.getAsString();
    return true;
            }

            // 第2パラメータのデフォルト値を取り出す
            clang::Expr* aDefault = aParam->getDefaultArg();
            if (aDefault == nullptr)
            {
                gCustomDiag.ErrorReport(aParam->getLocation(),
                    "GenerationMarkerStart() second parameter has default.(inc-file path)");
    return true;
            }

            aIncFileName = pullupString(aDefault);
llvm::outs() << "    aIncFileName=" << aIncFileName << "\n";
llvm::outs().flush();
            if (aIncFileName.empty())
            {
                gCustomDiag.ErrorReport(aParam->getLocation(),
                    "GenerationMarkerStart() unknown error.");
    return true;
            }
        }

//      ---<<< 第1パラメータの取り出し >>>---

        // 先頭パラメータの型取得
        clang::ParmVarDecl* aParam=iFunctionDecl->getParamDecl(0);
        QualType qt = aParam->getType();

//      ---<<< Startは重複チェックとコード生成／Endは位置登録 >>>---

        switch (qt->getTypeClass())
        {
        case Type::Enum:
            {
llvm::outs() << "enum : " << qt.getAsString() << "\n";
llvm::outs().flush();

                // Decl*取り出し
                EnumType const* et = qt->getAs<EnumType>();
                ERROR(!et, iFunctionDecl, true);
                EnumDecl* aTargetEnum = et->getDecl();
                ERROR(!aTargetEnum, iFunctionDecl, true);

                // 処理
                if (!mModifing)
                {
                    if (!mAstInterface.addEndMarker(aTargetEnum, iFunctionDecl->getLocation()))
                    {
                        gCustomDiag.ErrorReport(aParam->getLocation(),
                            "GenerationMarkerEnd() multiple definition(%0).") << qt.getAsString();
    return true;
                    }
                }
                else
                {
                    if (!aTargetEnum->isThisDeclarationADefinition())
                    {
                        gCustomDiag.ErrorReport(aParam->getLocation(),
                            "GenerationMarkerStart() needs enum definition.");
    return true;
                    }

                    if (!mAstInterface.addStartMarker(aTargetEnum, iFunctionDecl->getLocation()))
                    {
                        gCustomDiag.ErrorReport(aParam->getLocation(),
                            "GenerationMarkerStart() multiple definition(%0).") <<qt.getAsString();
    return true;
                    }

                    // ソース生成
                    mModifySource->createEnum(aTargetEnum, aIncFileName, iFunctionDecl);
                }
            }
            break;

        case Type::Record:
            {
llvm::outs() << "class : " << qt.getAsString() << "\n";
llvm::outs().flush();

                // Decl*取り出し
                CXXRecordDecl* aTargetClass=qt->getAsCXXRecordDecl();
                ERROR(!aTargetClass, iFunctionDecl, true);

                // 処理
                if (!mModifing)
                {
                    if (!mAstInterface.addEndMarker(aTargetClass, iFunctionDecl->getLocation()))
                    {
                        gCustomDiag.ErrorReport(aParam->getLocation(),
                            "GenerationMarkerEnd() multiple definition(%0).") << qt.getAsString();
    return true;
                    }
                }
                else
                {
                    if (!aTargetClass->isThisDeclarationADefinition())
                    {
                        gCustomDiag.ErrorReport(aParam->getLocation(),
                            "GenerationMarkerStart() needs class/struct definition.");
    return true;
                    }

                    if (!mAstInterface.addStartMarker(aTargetClass,iFunctionDecl->getLocation()))
                    {
                        gCustomDiag.ErrorReport(aParam->getLocation(),
                            "GenerationMarkerStart() multiple definition(%0).") <<qt.getAsString();
    return true;
                    }

                    // ソース生成
                    mModifySource->createClass(aTargetClass, aIncFileName, iFunctionDecl);
                }
            }
            break;
        }
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
            enumerateDecl(iNamespaceDecl);
    return true;
        }

        enumerateDecl(iNamespaceDecl);
        return true;
    }

//      ---<<< extern "C"/"C++" >>>---

    virtual bool VisitLinkageSpecDecl(LinkageSpecDecl *iLinkageSpecDecl)
    {
        switch (iLinkageSpecDecl->getLanguage())
        {
        case LinkageSpecDecl::lang_c:           // "C" : NOP
            enumerateDecl(iLinkageSpecDecl);
            break;

        case LinkageSpecDecl::lang_cxx:         // "C++"
            enumerateDecl(iLinkageSpecDecl);
            break;
        }
        return true;
    }

//----------------------------------------------------------------------------
//      Declリスト1レベルの枚挙処理
//----------------------------------------------------------------------------

    bool mModifing;
public:
    void enumerateDecl(DeclContext *iDeclContext)
    {
        for (auto decl : iDeclContext->decls())
        {
            Visit(decl);
        }
    }

//----------------------------------------------------------------------------
//      ソース生成開始
//----------------------------------------------------------------------------

    void startModifySource()
    {
        mModifing = true;
        mModifySource.reset(new ModifySource(mAstInterface));
    }

//----------------------------------------------------------------------------
//      基本
//----------------------------------------------------------------------------

//      ---<<< 出力 >>>---

private:
    AstInterface&                   mAstInterface;  // ソース修正処理とのI/F
    std::unique_ptr<ModifySource>   mModifySource;  // ソース修正用クラス

//      ---<<< 内部処理 >>>---

    bool    mIsStdSpace;                            // std名前空間処理中

public:
    ASTVisitor(AstInterface& iAstInterface, ASTContext* iASTContext) :
        mModifing(false),
        mAstInterface(iAstInterface),
        mIsStdSpace(false)
    { }
};

// ***************************************************************************
//      プリプロセッサからのコールバック処理
//          AST解析を実行するかどうか判定する
// ***************************************************************************

class TheorideASTConsumer : public ASTConsumer
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

    virtual void HandleTranslationUnit(ASTContext &iContext)
    {
        // GenerationMarkerEnd()を見つけておく
        mASTVisitor.enumerateDecl(iContext.getTranslationUnitDecl());

        // ソース生成開始
        mASTVisitor.startModifySource();

        // GenerationMarkerStart()を処理する
        mASTVisitor.enumerateDecl(iContext.getTranslationUnitDecl());
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
    explicit TheorideASTConsumer(CompilerInstance *iCompilerInstance) :
        mPreprocessor(iCompilerInstance->getPreprocessor()),
        mASTVisitor(mAstInterface, &(iCompilerInstance->getASTContext())),
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
    ~TheorideASTConsumer()
    {
        gCustomDiag.resetDiagnosticsEngine();
    }
};

//----------------------------------------------------------------------------
//      メイン処理からの中継
//----------------------------------------------------------------------------

class TheorideFrontendAction : public SyntaxOnlyAction /*ASTFrontendAction*/
{
public:
    virtual std::unique_ptr<ASTConsumer> CreateASTConsumer(
                        CompilerInstance &iCompilerInstance, StringRef file)
    {
        return llvm::make_unique<TheorideASTConsumer>(&iCompilerInstance); 
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

static llvm::cl::OptionCategory MyToolCategory("Theoride Driver");
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
    TheorideDiagnosticConsumer    DiagClient(llvm::errs(), &*diag_opts);
    Tool.setDiagnosticConsumer(&DiagClient);
    int ret=Tool.run(clang::tooling::newFrontendActionFactory<TheorideFrontendAction>().get());

    return ret;
}

#endif
