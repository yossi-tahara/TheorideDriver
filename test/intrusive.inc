﻿#define THEORIDE_STRINGIZE(...)       THEORIDE_STRINGIZE_I(__VA_ARGS__)
#define THEORIDE_STRINGIZE_I(...)     #__VA_ARGS__ ""

friend std::string to_string(THEORIDE_CLASS const& iClass, unsigned iIndent=0)
{
    std::stringstream   ss;
    std::string         aIndent0(iIndent*4, ' ');
    std::string         aIndent1((iIndent+1)*4, ' ');

    ss << "\n" << aIndent0 << "{";

    // クラス名
    ss << "\n" << aIndent1 << "\"<Class Name>\":\"" THEORIDE_STRINGIZE(THEORIDE_CLASS) << "\"";

    #define THEORIDE_BASE_PUBLIC(dBase)\
        ss << ",\n" << aIndent1 << "\"(" #dBase ")\":"\
           << to_string(static_cast<dBase>(iClass), iIndent+1);

    #define THEORIDE_BASE_PROTECTED(dBase)\
        ss << ",\n" << aIndent1 << "\"(" #dBase ")\":"\
           << to_string(static_cast<dBase>(iClass), iIndent+1);

    #define THEORIDE_BASE_PRIVATE(dBase)\
        ss << ",\n" << aIndent1 << "\"(" #dBase ")\":"\
           << to_string(static_cast<dBase>(iClass), iIndent+1);

    #define THEORIDE_ELEMENT_PUBLIC(dElement)\
        ss << ",\n" << aIndent1 << "\"" #dElement "\":" << to_string(iClass.dElement, iIndent+1);

    #define THEORIDE_ELEMENT_PROTECTED(dElement)\
        ss << ",\n" << aIndent1 << "\"" #dElement "\":" << to_string(iClass.dElement, iIndent+1);

    #define THEORIDE_ELEMENT_PRIVATE(dElement)\
        ss << ",\n" << aIndent1 << "\"" #dElement "\":" << to_string(iClass.dElement, iIndent+1);

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

friend std::ostream& operator<<(std::ostream& os, THEORIDE_CLASS iClass)
{
    os << to_string(iClass);
    return os;
}

#undef THEORIDE_STRINGIZE
#undef THEORIDE_STRINGIZE_I
