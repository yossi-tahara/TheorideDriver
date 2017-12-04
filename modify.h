//############################################################################
//      ソース修正
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

#if !defined(THEOLIZER_MODIFY_H)
#define THEOLIZER_MODIFY_H

//############################################################################
//      ソース修正用クラス
//############################################################################

class ModifySource
{
// ***************************************************************************
//      ユーティリティ
// ***************************************************************************

private:

//----------------------------------------------------------------------------
//      文字列の改行を正規化する
//----------------------------------------------------------------------------

    std::string normalizeLF(std::string&& iString)
    {
        if (kLineEnding.equals("\n"))
    return iString;

        std::string ret;
        StringRef aString(iString);
        bool aEndIsLF=aString.endswith(kLineEnding);
        for (std::pair<StringRef, StringRef> aParsing=aString.split(kLineEnding);
             true;
             aParsing=aParsing.second.split(kLineEnding))
        {
            ret.append(aParsing.first);
            if (aParsing.second.empty())
        break;
            ret.append("\n");
        }
        if (aEndIsLF) ret.append("\n");
        return ret;
    }

//----------------------------------------------------------------------------
//      文字列の改行をOSに合わせる
//----------------------------------------------------------------------------

    std::string reverseLF(std::string&& iString)
    {
        if (kLineEnding.equals("\n"))
    return iString;

        std::string ret;
        StringRef aString(iString);
        bool aEndIsLF=aString.endswith("\n");
        for (std::pair<StringRef, StringRef> aParsing=aString.split('\n');
             true;
             aParsing=aParsing.second.split('\n'))
        {
            ret.append(aParsing.first);
            if (aParsing.second.empty())
        break;
            ret.append(kLineEnding);
        }
        if (aEndIsLF) ret.append(kLineEnding);
        return ret;
    }

//----------------------------------------------------------------------------
//      指定位置の現ソース獲得
//----------------------------------------------------------------------------

    std::string getNowSource(SourceLocation iBegin, SourceLocation iEnd)
    {
         return normalizeLF(
            std::move(mRewriter.getRewrittenText(clang::SourceRange(iBegin, iEnd))));
    }

//----------------------------------------------------------------------------
//      指定Declの現ソース獲得
//----------------------------------------------------------------------------

    std::string getDeclSource(Decl const* iDecl)
    {
        std::string ret;
        if (iDecl)
        {
            ret = normalizeLF(std::move(mRewriter.getRewrittenText(iDecl->getSourceRange())));
        }
        return ret;
    }

//----------------------------------------------------------------------------
//      FileID記録
//----------------------------------------------------------------------------

    void addFileID(const SourceLocation iLocation)
    {
        FileID aFileId = gASTContext->getFullLoc(iLocation).getFileID();
        auto found = std::lower_bound(mModifiedFiles.begin(),
                                      mModifiedFiles.end(),
                                      aFileId);
        if ((found == mModifiedFiles.end()) || (*found != aFileId))
        {
            mModifiedFiles.insert(found, aFileId);
        }
    }

//----------------------------------------------------------------------------
//      文字列置き換え
//----------------------------------------------------------------------------

    void replaceString(SourceLocation iBegin, SourceLocation iEnd, std::string&& iString)
    {
        if (mRewriter.ReplaceText(clang::SourceRange(iBegin, iEnd), reverseLF(std::move(iString))))
        {
            gCustomDiag.FatalReport(iBegin,
                "Fatal error. Can not replace source.");
    return;
        }
        addFileID(iBegin);
    }

//----------------------------------------------------------------------------
//      文字列挿入
//----------------------------------------------------------------------------

    void instertString(SourceLocation iLocation, std::string&& iString)
    {
        if (mRewriter.InsertText(iLocation, reverseLF(std::move(iString))))
        {
            gCustomDiag.FatalReport(iLocation,
                "Fatal error. Can not replace source.");
    return;
        }
        addFileID(iLocation);
    }

//----------------------------------------------------------------------------
//      名前空間取り出し
//----------------------------------------------------------------------------

class NameSpaces
{
    struct NamespaceInfo
    {
        bool        mIsInline;
        StringRef   mName;
        NamespaceInfo(bool iIsInline, StringRef iName) :
            mIsInline(iIsInline), mName(iName)
        { }
    };
    typedef SmallVector<NamespaceInfo, 8>           Vector;
    typedef typename Vector::const_reverse_iterator Iterator;

    Vector  mVector;

public:
    NameSpaces() { }
    NameSpaces(NamedDecl const* iNamedDecl, bool iSuppressUnwrittenScope=true)
    {
        clang::DeclContext const* aContext = iNamedDecl->getDeclContext();
        while (aContext && clang::isa<NamedDecl>(aContext))
        {
            clang::NamespaceDecl const* nsd = dyn_cast<clang::NamespaceDecl>(aContext);
            if (nsd)
            {
                if (iSuppressUnwrittenScope)
                {
                    if (!(nsd->isAnonymousNamespace()) && (!(nsd->isInline())))
                        mVector.push_back(NamespaceInfo(false, nsd->getName()));
                }
                else
                {
                    if (nsd->isAnonymousNamespace())
                    {
                        mVector.push_back(NamespaceInfo(false, StringRef("")));
                    }
                    else
                    {
                        mVector.push_back(NamespaceInfo(nsd->isInline(), nsd->getName()));
                    }
                }
            }
            aContext = aContext->getParent();
        }
    }

    Iterator begin() {return mVector.rbegin();}
    Iterator end()   {return mVector.rend();}
};

//----------------------------------------------------------------------------
//      名前空間付きの名前獲得
//----------------------------------------------------------------------------

std::string getQualifiedName(NamedDecl const* iNamedDecl, bool iSuppressUnwrittenScope=true)
{
    std::string ret;
    for (auto aNameSpace : NameSpaces(iNamedDecl, iSuppressUnwrittenScope))
    {
        ret.append(aNameSpace.mName.str());
        ret.append("::");
    }
    ret.append(iNamedDecl->getName().str());

    return ret;
}

//----------------------------------------------------------------------------
//      マーカ用FunctionDeclの終わり(;の次の位置)を探す
//          ;までの間にスペースとタブ以外の文字があるとエラーとする。
//          改行も不許可とする。
//----------------------------------------------------------------------------

    SourceLocation getMarkerEnd(SourceLocation iStartLoc)
    {
        FullSourceLoc aLoc =
            gASTContext->getFullLoc(iStartLoc).getSpellingLoc();

        // ;まで探す
        FileID aFileId=aLoc.getFileID();
        unsigned aLine=aLoc.getSpellingLineNumber();
        unsigned aCol =aLoc.getSpellingColumnNumber()+1;
        for (char const* p = gSourceManager->getCharacterData(aLoc)+1;
             *p != ';';
             ++p, ++aCol)
        {
            if ((*p != ' ') && (*p != '\t'))
            {
                SourceLocation aErrLoc=gSourceManager->translateLineCol(aFileId, aLine, aCol);
                gCustomDiag.ErrorReport(aErrLoc, "No semicolon.");
    return FullSourceLoc();
            }
        }
        ++aCol;

        return gSourceManager->translateLineCol(aFileId, aLine, aCol);
    }

// ***************************************************************************
//      ソース修正
// ***************************************************************************

    void modifySource
    (
        clang::FunctionDecl* iStartMarker,
        clang::TagDecl* iTarget,
        std::string&& iString
    )
    {
        SourceLocation aStartLoc = getMarkerEnd(iStartMarker->getLocEnd());

        auto aEnd = mAstInterface.mEndMarkerList.find(iTarget);
        // 終了マーカなし
        if (aEnd == mAstInterface.mEndMarkerList.end())
        {
            instertString(aStartLoc, std::move(iString));
        }

        // 終了マーカあり
        else
        {
            SourceLocation aEndLoc = getMarkerEnd(aEnd->second);
            // 変更があれば修正する
            if (getNowSource(aStartLoc, aEndLoc) != iString)
            {
                replaceString(aStartLoc, aEndLoc, std::move(iString));
            }
        }
    }

// ***************************************************************************
//      enum用ソース生成
//          #define THEORIDE_ENUM       enum名(名前空間付き)
//          #define THEORIDE_ENUM_LIST()\
//              THEORIDE_ELEMENT(シンボル名)\
//              THEORIDE_ELEMENT(シンボル名)\
//              THEORIDE_ELEMENT(シンボル名)\
//                  :
//          #include "指定ファイル名"
//          #undef THEORIDE_ENUM_LIST
//          #undef THEORIDE_ENUM
//          void GenerationMarkerEnd(enum名);
// ***************************************************************************

public:
    void createEnum
    (
        clang::EnumDecl*        iTargetEnum,
        std::string const&      iIncFileName,
        clang::FunctionDecl*    iStartMarker
    )
    {
        std::string aEnumName = getQualifiedName(iTargetEnum);

        std::stringstream   aSource;
        aSource << "\n#define THEORIDE_ENUM " << aEnumName << "\n";

        // シンボル・リスト生成
        aSource << "#define THEORIDE_ENUM_LIST()";
        for (auto symbol : iTargetEnum->enumerators())
        {
            aSource << "\\\n"
                << "    THEORIDE_ELEMENT(" << symbol->getName().str() << ")";
        }
        aSource << "\n";

        aSource << "#include \"" << iIncFileName << "\"\n";
        aSource << "#undef THEORIDE_ENUM_LIST\n";
        aSource << "#undef THEORIDE_ENUM\n";
        aSource << "void GenerationMarkerEnd(" << aEnumName << ");";

        modifySource(iStartMarker, iTargetEnum, aSource.str());
    }

// ***************************************************************************
//      class用ソース生成
//          #define THEORIDE_CLASS      class名(名前空間付き)
//          #define THEORIDE_BASE_LIST()\
//              THEORIDE_BASE_PUBLIC(基底クラス名)\
//              THEORIDE_BASE_PROTECTED(基底クラス名)\
//              THEORIDE_BASE_PRIVATE(基底クラス名)\
//                  :
//          #define THEORIDE_ELEMENT_LIST()\
//              THEORIDE_ELEMENT_PUBLIC(変数名)\
//              THEORIDE_ELEMENT_PROTECTED(変数名)\
//              THEORIDE_ELEMENT_PRIVATE(変数名)\
//                  :
//          #include "指定ファイル名"
//          #undef THEORIDE_ELEMENT_LIST
//          #undef THEORIDE_BASE_LIST
//          #undef THEORIDE_CLASS
//          void GenerationMarkerEnd(class名);
// ***************************************************************************

    void createClass
    (
        clang::CXXRecordDecl*   iTargetClass,
        std::string const&      iIncFileName,
        clang::FunctionDecl*    iStartMarker
    )
    {

        SourceLocation aBegin = iStartMarker->getSourceRange().getBegin();
        FullSourceLoc aLoc =
            gASTContext->getFullLoc(aBegin).getSpellingLoc();
        unsigned indent =aLoc.getSpellingColumnNumber()-1;
        std::string aIndent(indent, ' ');

        std::string aClassName = getQualifiedName(iTargetClass);

        std::stringstream   aSource;
        aSource << "\n" << aIndent << "#define THEORIDE_CLASS " << aClassName;

        // ベース・リスト生成
        aSource << "\n" << aIndent << "#define THEORIDE_BASE_LIST()";
        for (auto& base : iTargetClass->bases())
        {
            QualType qt = base.getType().getCanonicalType();
            CXXRecordDecl const* aBase = qt->getAsCXXRecordDecl();
            if (!aBase)
        continue;

            std::string aBaseName = qt.getCanonicalType().getAsString(mPrintingPolicy);
            switch(base.getAccessSpecifier())
            {
            case clang::AS_public:
                aSource << "\\\n" << aIndent << "    THEORIDE_BASE_PUBLIC(" << aBaseName << ")";
                break;

            case clang::AS_protected:
                aSource << "\\\n" << aIndent << "    THEORIDE_BASE_PROTECTED(" << aBaseName << ")";
                break;

            case clang::AS_private:
                aSource << "\\\n" << aIndent << "    THEORIDE_BASE_PRIVATE(" << aBaseName << ")";
                break;
            }
        }

        // メンバ変数リスト生成
        aSource << "\n" << aIndent << "#define THEORIDE_ELEMENT_LIST()";
        for (auto field : iTargetClass->fields())
        {
            std::string aElementName = field->getName();
            switch(field->getAccess())
            {
            case clang::AS_public:
                aSource<<"\\\n"<<aIndent<<"    THEORIDE_ELEMENT_PUBLIC("<< aElementName<<")";
                break;

            case clang::AS_protected:
                aSource<<"\\\n"<<aIndent<<"    THEORIDE_ELEMENT_PROTECTED("<<aElementName<<")";
                break;

            case clang::AS_private:
                aSource<<"\\\n"<<aIndent<<"    THEORIDE_ELEMENT_PRIVATE("<<aElementName<<")";
                break;
            }
        }

        aSource << "\n" << aIndent << "#include \"" << iIncFileName << "\"";
        aSource << "\n" << aIndent << "#undef THEORIDE_ELEMENT_LIST";
        aSource << "\n" << aIndent << "#undef THEORIDE_BASE_LIST";
        aSource << "\n" << aIndent << "#undef THEORIDE_CLASS";
        aSource << "\n" << aIndent << "void GenerationMarkerEnd(" << aClassName << ");";

        modifySource(iStartMarker, iTargetClass, aSource.str());
    }

// ***************************************************************************
//      ソース保存
// ***************************************************************************

    void saveSources()
    {
        // 致命的エラーが発生していたら保存しない
        if (gCustomDiag.getDiags().hasFatalErrorOccurred())
    return;
        // Theolizerがエラー検出していたら保存しない
        if (gCustomDiag.hasErrorOccurred())
    return;

        // 修正ファイルがないなら保存しない
        if (!mModifiedFiles.size())
    return;

        // 修正開始してみる。他が修正していたら、再処理する
        boostI::upgradable_lock<ExclusiveControl> lock_try(*gExclusiveControl,boostI::try_to_lock);
        if (!lock_try)
    return;

        // 他のプロセスのファイル開放待ち
        boostI::scoped_lock<ExclusiveControl> lock(*gExclusiveControl);

        for (auto aFileId2 : mModifiedFiles)
        {
            std::string file_data;
            llvm::raw_string_ostream ss(file_data);
            mRewriter.getEditBuffer(aFileId2).write(ss);
            ss.flush();
        }

        // 保存処理
        mRewriter.overwriteChangedFiles();

        // 保存処理を実行したら、mRetryAST(最新版変更によるリトライ)をgRetryASTへ反映
        gRetryAST = mRetryAST;
    }

// ***************************************************************************
//      生成と削除
// ***************************************************************************

private:
    // 入力メンバ
    AstInterface&           mAstInterface;          // AST解析処理とのI/F
    clang::PrintingPolicy const& mPrintingPolicy;

    // 処理用メンバ
    Rewriter                mRewriter;              // ソース修正用
    std::vector<FileID>     mModifiedFiles;         // 修正したソース

    std::stringstream       mLastVersion;           // 最新版マクロ定義ソース

    int                     mNamespaceNestCount;    // 名前空間のネスト数
    NamedDecl const*        mTarget;                // 処理中のターゲット(エラー表示用)

    bool                    mRetryAST;              // AST処理再実行要求(最新版を変更した時)

public:
    ModifySource(AstInterface& iAstInterface) :
        mAstInterface(iAstInterface),
        mPrintingPolicy(gASTContext->getPrintingPolicy()),
        mRetryAST(false)

    {
        // リライター準備
        mRewriter.setSourceMgr(*gSourceManager, gASTContext->getLangOpts());
        mModifiedFiles.clear();
    }

    ~ModifySource()
    {
    }
};

#endif
