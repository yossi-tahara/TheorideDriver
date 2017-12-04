#define THEORIDE_STRINGIZE(...)       THEORIDE_STRINGIZE_I(__VA_ARGS__)
#define THEORIDE_STRINGIZE_I(...)     #__VA_ARGS__ ""

inline std::string getString(THEORIDE_CLASS const& iClass, unsigned iIndent=0)
{
    std::stringstream   ss;
    std::string         aIndent0(iIndent*4, ' ');
    std::string         aIndent1((iIndent+1)*4, ' ');

    ss << "\n" << aIndent0 << "{";

    // クラス名
    ss << "\n" << aIndent1 << "\"<Class Name>\":\"" THEORIDE_STRINGIZE(THEORIDE_CLASS) << "\"";

    #define THEORIDE_BASE_PUBLIC(dBase)\
        ss << ",\n" << aIndent1 << "\"(" #dBase ")\":"\
           << getString(static_cast<dBase>(iClass), iIndent+1);

    #define THEORIDE_ELEMENT_PUBLIC(dElement)\
        ss << ",\n" << aIndent1 << "\"" #dElement "\":" << getString(iClass.dElement, iIndent+1);

    // 非publicにはアクセスできない
    #define THEORIDE_BASE_PROTECTED(dBase)
    #define THEORIDE_BASE_PRIVATE(dBase)
    #define THEORIDE_ELEMENT_PROTECTED(dElement)
    #define THEORIDE_ELEMENT_PRIVATE(dElement)

    THEORIDE_BASE_LIST()
    THEORIDE_ELEMENT_LIST()

    #undef THEORIDE_ELEMENT_PRIVATE
    #undef THEORIDE_ELEMENT_PROTECTED
    #undef THEORIDE_ELEMENT_PUBLIC
    #undef THEORIDE_BASE_PRIVATE
    #undef THEORIDE_BASE_PROTECTED
    #undef THEORIDE_BASE_PUBLIC

    ss << "\n" << aIndent0 << "}";
    return ss.str();
}

inline std::ostream& operator<<(std::ostream& os, THEORIDE_CLASS iClass)
{
    os << getString(iClass);
    return os;
}

#undef THEORIDE_STRINGIZE
#undef THEORIDE_STRINGIZE_I
